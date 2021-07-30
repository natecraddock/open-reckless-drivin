#include <stdbool.h>
#include <stdlib.h>

#include "defines.h"
#include "gamesounds.h"
#include "input.h"
#include "objects.h"
#include "packs.h"
#include "particlefx.h"
#include "random.h"
#include "screen.h"
#include "sprites.h"
#include "textfx.h"
#include "trig.h"
#include "vec2d.h"
#include <math.h>

#define kSeperatio 1.01
#define kMaxCollDist 120.0
#define kPlayerBounceMass 4000.0
#define kTireVelo 30.0
#define kDebrisProbility 5
#define kFlameProbility 10
#define kDamageVelo 15.0
#define kDamagePerMSsq 20
#define kMinDamageVelo 5
#define kMinJumpVelo 20.0

enum {
  kFrontSide,
  kFrontLeftSide,
  kFrontRightSide,
  kLeftSide,
  kRightSide,
  kRearLeftSide,
  kRearRightSide,
  kRearSide
};

int CalcObjPoints(tObject *theObj, t2DPoint *points) {
  int i, hi = 0;
  float cosDir = cos(theObj->dir);
  float sinDir = sin(theObj->dir);
  tObjectTypePtr objType = theObj->type;
  points[0] = P2D(sinDir * (*objType).length - cosDir * (*objType).width,
                  cosDir * (*objType).length + sinDir * (*objType).width);
  points[1] = P2D(sinDir * (*objType).length + cosDir * (*objType).width,
                  cosDir * (*objType).length - sinDir * (*objType).width);
  points[2] = P2D(-sinDir * (*objType).length + cosDir * (*objType).width,
                  -cosDir * (*objType).length - sinDir * (*objType).width);
  points[3] = P2D(-sinDir * (*objType).length - cosDir * (*objType).width,
                  -cosDir * (*objType).length + sinDir * (*objType).width);
  for (i = 0; i < 4; i++) {
    points[i] = VEC2D_Sum(VEC2D_Scale(points[i], kScale), theObj->pos);
    if (points[i].y > points[hi].y)
      hi = i;
  }
  return hi;
}

int SectLines(t2DPoint *l1p1, t2DPoint *l1p2, t2DPoint *l2p1, t2DPoint *l2p2) {
  float dydxl1 = (l1p2->y - l1p1->y) / (l1p2->x - l1p1->x);
  float l2p1offs = l2p1->y - (l1p1->y + (l2p1->x - l1p1->x) * dydxl1);
  float l2p2offs = l2p2->y - (l1p1->y + (l2p2->x - l1p1->x) * dydxl1);
  if (l2p1offs * l2p2offs > 0)
    return false;
  else {
    float sectX =
        l2p1->x + (l2p2->x - l2p1->x) * l2p1offs / (l2p1offs - l2p2offs);
    if ((sectX > l1p1->x) && (sectX < l1p2->x))
      return true;
    if ((sectX > l1p2->x) && (sectX < l1p1->x))
      return true;
    return false;
  }
}

t2DPoint SectPoint(t2DPoint *l1p1, t2DPoint *l1p2, t2DPoint *l2p1,
                   t2DPoint *l2p2) {
  float m1 = (l1p2->y - l1p1->y) / (l1p2->x - l1p1->x);
  float m2 = (l2p2->y - l2p1->y) / (l2p2->x - l2p1->x);
  float b1 = l1p1->y - (m1 * l1p1->x);
  float b2 = l2p1->y - (m2 * l2p1->x);
  float x = (b2 - b1) / (m1 - m2);
  float y = m1 * x + b1;
  return P2D(x, y);
}

inline float sqr(float x) {
  return x * x;
}

int TestCollision(tObject *obj1, tObject *obj2, float sqDist) {
  t2DPoint obj1Pts[4], obj2Pts[4];
  int i, j;
  int hi1 = CalcObjPoints(obj1, obj1Pts);
  int hi2 = CalcObjPoints(obj2, obj2Pts);
  if (obj2->jumpHeight != obj1->jumpHeight &&
      !(((*obj1->type).flags2 | (*obj2->type).flags2) & kObjectMissile))
    return false;
  if (sqDist <
      sqr(kScale * (obj1->type->width < obj1->type->length
                        ? obj1->type->width
                        : obj1->type->length) +
          (obj2->type->width < obj2->type->length ? obj2->type->width
                                                  : obj2->type->length)))
    return true;
  if (obj1Pts[hi1].y < obj2Pts[(hi2 + 2) & 3].y)
    return false;
  if (obj2Pts[hi2].y < obj1Pts[(hi1 + 2) & 3].y)
    return false;
  if (obj1Pts[(hi1 + 1) & 3].x < obj2Pts[(hi2 - 1) & 3].x)
    return false;
  if (obj2Pts[(hi2 + 1) & 3].x < obj1Pts[(hi1 - 1) & 3].x)
    return false;
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++)
      if (SectLines(&obj1Pts[i], &obj1Pts[(i + 1) & 3], &obj2Pts[j],
                    &obj2Pts[(j + 1) & 3]))
        return true;
  if (SectLines(&obj1Pts[0], &obj1Pts[2], &obj2Pts[0], &obj2Pts[2]))
    return true;
  if (SectLines(&obj1Pts[1], &obj1Pts[3], &obj2Pts[0], &obj2Pts[2]))
    return true;
  if (SectLines(&obj1Pts[0], &obj1Pts[2], &obj2Pts[1], &obj2Pts[3]))
    return true;
  if (SectLines(&obj1Pts[1], &obj1Pts[3], &obj2Pts[1], &obj2Pts[3]))
    return true;
  return false;
}

void MakeDebris(tObject *theObj, int damagePos, float damage, float maxDamage) {
  tObjectTypePtr objType = theObj->type;
  float xSize = (*objType).width * kScale, ySize = (*objType).length * kScale;
  if (damagePos != kMotor && RanProb(kDebrisProbility * damage / maxDamage)) {
    tObject *debrisObj;
    switch (damagePos) {
      case kFrontBumper:
        debrisObj = NewObject(theObj, 1014);
        debrisObj->pos = VEC2D_Sum(theObj->pos, P2D(sin(theObj->dir) * ySize,
                                                    cos(theObj->dir) * ySize));
        debrisObj->dir = theObj->dir;
        break;
      case kBackBumper:
        debrisObj = NewObject(theObj, 1014);
        debrisObj->pos = VEC2D_Sum(theObj->pos, P2D(-sin(theObj->dir) * ySize,
                                                    -cos(theObj->dir) * ySize));
        debrisObj->dir = theObj->dir + PI;
        break;
      case kFrontLeftTire:
        debrisObj = NewObject(theObj, 1012);
        debrisObj->pos = VEC2D_Sum(theObj->pos, P2D(sin(theObj->dir) * ySize,
                                                    cos(theObj->dir) * ySize));
        debrisObj->pos =
            VEC2D_Sum(debrisObj->pos, P2D(-cos(theObj->dir) * xSize,
                                          -sin(theObj->dir) * xSize));
        debrisObj->dir = RanFl(0, 2 * PI);
        break;
      case kFrontRightTire:
        debrisObj = NewObject(theObj, 1012);
        debrisObj->pos = VEC2D_Sum(theObj->pos, P2D(sin(theObj->dir) * ySize,
                                                    cos(theObj->dir) * ySize));
        debrisObj->pos =
            VEC2D_Sum(debrisObj->pos,
                      P2D(cos(theObj->dir) * xSize, sin(theObj->dir) * xSize));
        debrisObj->dir = RanFl(0, 2 * PI);
        break;
      case kBackLeftTire:
        debrisObj = NewObject(theObj, 1012);
        debrisObj->pos = VEC2D_Sum(
            theObj->pos, P2D(-sin(theObj->dir) * (*objType).length * kScale,
                             -cos(theObj->dir) * (*objType).length * kScale));
        ;
        debrisObj->pos = VEC2D_Sum(
            debrisObj->pos, P2D(-cos(theObj->dir) * (*objType).width * kScale,
                                -sin(theObj->dir) * (*objType).width * kScale));
        ;
        debrisObj->dir = RanFl(0, 2 * PI);
        break;
      case kBackRightTire:
        debrisObj = NewObject(theObj, 1012);
        debrisObj->pos = VEC2D_Sum(
            theObj->pos, P2D(-sin(theObj->dir) * (*objType).length * kScale,
                             -cos(theObj->dir) * (*objType).length * kScale));
        ;
        debrisObj->pos = VEC2D_Sum(
            debrisObj->pos, P2D(cos(theObj->dir) * (*objType).width * kScale,
                                sin(theObj->dir) * (*objType).width * kScale));
        ;
        debrisObj->dir = RanFl(0, 2 * PI);
        break;
      case kLeftDoor:
        debrisObj = NewObject(theObj, 1015);
        debrisObj->pos = VEC2D_Sum(
            theObj->pos, P2D(-cos(theObj->dir) * (*objType).width * kScale,
                             -sin(theObj->dir) * (*objType).width * kScale));
        ;
        debrisObj->dir = theObj->dir;
        debrisObj->rotVelo = -2 * PI;
        break;
      case kRightDoor:
        debrisObj = NewObject(theObj, 1016);
        debrisObj->pos = VEC2D_Sum(
            theObj->pos, P2D(cos(theObj->dir) * (*objType).width * kScale,
                             sin(theObj->dir) * (*objType).width * kScale));
        ;
        debrisObj->dir = theObj->dir;
        debrisObj->rotVelo = 2 * PI;
        break;
    }
    debrisObj->velo = VEC2D_Sum(
        theObj->velo, P2D(sin(debrisObj->dir) * 10, cos(debrisObj->dir) * 10));
  }
}

void Explosion(t2DPoint pos, t2DPoint velo, int offs, float mass, int sound);

void DamageObj(tObject *theObj, float damage, t2DPoint diff) {
  tObjectTypePtr objType = theObj->type;
  float xSize = (*objType).width * kScale, ySize = (*objType).length * kScale;
  float maxDamage = (*objType).maxDamage;
  float damDir =
      VEC2D_DotProduct(diff, P2D(sin(theObj->dir), cos(theObj->dir)));
  t2DPoint damPoint =
      VEC2D_Sum(theObj->pos, P2D(sin(theObj->dir + damDir) * xSize,
                                 cos(theObj->dir + damDir) * ySize));
  int damagePos;
  NewParticleFX(damPoint, theObj->velo, damage * 4, 0xf6, 1, 50);
  theObj->damage += damage;
  if (damDir > 0.83)
    damagePos = theObj->damageFlags & kFrontBumper ? kMotor : kFrontBumper;
  else if (damDir > 0.35)
    damagePos =
        VEC2D_CrossProduct(diff, P2D(sin(theObj->dir), cos(theObj->dir))) > 0
            ? kFrontRightTire
            : kFrontLeftTire;
  else if (damDir > -0.35)
    damagePos =
        VEC2D_CrossProduct(diff, P2D(sin(theObj->dir), cos(theObj->dir))) > 0
            ? kRightDoor
            : kLeftDoor;
  else if (damDir > -0.83)
    damagePos =
        VEC2D_CrossProduct(diff, P2D(sin(theObj->dir), cos(theObj->dir))) > 0
            ? kBackRightTire
            : kBackLeftTire;
  else
    damagePos = kBackBumper;
  Explosion(damPoint, theObj->velo, 0, damage / maxDamage * 2000, false);
  if (!(theObj->damageFlags & damagePos)) {
    if (damagePos == kMotor) {
      if (theObj->damage > maxDamage * 0.75)
        theObj->damageFlags |= damagePos;
      else
        return;
    }
    else
      theObj->damageFlags |= damagePos;
    switch (damagePos) {
      case kFrontBumper:
      case kMotor:
        theObj->frame = YDistortSprite(theObj->frame, -xSize, xSize, -ySize - 1,
                                       -ySize + 10, 0, damage / maxDamage * 20);
        break;
      case kBackBumper:
        theObj->frame = YDistortSprite(theObj->frame, -xSize, xSize, ySize - 10,
                                       ySize + 1, 1, damage / maxDamage * 20);
        break;
      case kFrontLeftTire:
        theObj->frame =
            XDistortSprite(theObj->frame, -ySize, -ySize * 0.3, -xSize - 1,
                           -xSize + 10, 0, damage / maxDamage * 20);
        break;
      case kFrontRightTire:
        theObj->frame =
            XDistortSprite(theObj->frame, -ySize, -ySize * 0.3, xSize - 10,
                           xSize + 1, 1, damage / maxDamage * 20);
        break;
      case kBackLeftTire:
        theObj->frame =
            XDistortSprite(theObj->frame, ySize * 0.3, ySize, -xSize - 1,
                           -xSize + 10, 0, damage / maxDamage * 20);
        break;
      case kBackRightTire:
        theObj->frame =
            XDistortSprite(theObj->frame, ySize * 0.3, ySize, xSize - 10,
                           xSize + 1, 1, damage / maxDamage * 20);
        break;
      case kLeftDoor:
        theObj->frame =
            XDistortSprite(theObj->frame, -ySize * 0.4, ySize * 0.4, -xSize - 1,
                           -xSize + 10, 0, damage / maxDamage * 20);
        break;
      case kRightDoor:
        theObj->frame =
            XDistortSprite(theObj->frame, -ySize * 0.4, ySize * 0.4, xSize - 10,
                           xSize + 1, 1, damage / maxDamage * 20);
        break;
    }
    MakeDebris(theObj, damagePos, damage, maxDamage);
  }
}

void DoBounce(tObject *obj1, tObject *obj2, t2DPoint diff) {
  t2DPoint new1Velo, new2Velo, new12Velo;
  t2DPoint impulse1, impulse2;
  float damage1 = 0, damage2 = 0;
  float momentum1, momentum2;
  float mass1 = obj1 == gPlayerObj ? kPlayerBounceMass : (*obj1->type).mass;
  float mass2 = obj2 == gPlayerObj ? kPlayerBounceMass : (*obj2->type).mass;
  new12Velo =
      VEC2D_Sum(VEC2D_Scale(obj1->velo, mass1), VEC2D_Scale(obj2->velo, mass2));
  new12Velo = VEC2D_Scale(new12Velo, 2 / (mass1 + mass2));
  new1Velo = VEC2D_Difference(VEC2D_Sum(VEC2D_Scale(obj1->velo, mass1),
                                        VEC2D_Scale(obj2->velo, mass2 * 2)),
                              VEC2D_Scale(obj1->velo, mass2));
  new2Velo = VEC2D_Difference(VEC2D_Sum(VEC2D_Scale(obj2->velo, mass2),
                                        VEC2D_Scale(obj1->velo, mass1 * 2)),
                              VEC2D_Scale(obj2->velo, mass1));
  new1Velo = VEC2D_Scale(new1Velo, 1 / (mass1 + mass2));
  new2Velo = VEC2D_Scale(new2Velo, 1 / (mass1 + mass2));
  new1Velo = VEC2D_Scale(VEC2D_Sum(new1Velo, new12Velo), 0.33);
  new2Velo = VEC2D_Scale(VEC2D_Sum(new2Velo, new12Velo), 0.33);
  impulse1 = VEC2D_Difference(obj1->velo, new1Velo);
  impulse2 = VEC2D_Difference(obj2->velo, new2Velo);
  obj1->velo = new1Velo;
  obj2->velo = new2Velo;
  if (obj1 == gPlayerObj || obj2 == gPlayerObj)
    if (gPlayerAddOns & kAddOnSpikes) {
      if (fabs(VEC2D_DotProduct(
              diff, P2D(sin(gPlayerObj->dir), cos(gPlayerObj->dir)))) < 0.3)
        if (obj1 == gPlayerObj)
          damage2 += VEC2D_Value(gPlayerObj->velo) * kFrameDuration * 40;
        else
          damage1 += VEC2D_Value(gPlayerObj->velo) * kFrameDuration * 40;
    }
  if ((*obj1->type).flags2 & kObjectDamageble)
    if ((damage1 += VEC2D_Value(impulse1)) > kMinDamageVelo)
      DamageObj(obj1, damage1, VEC2D_Scale(diff, -1));
  if ((*obj2->type).flags2 & kObjectDamageble)
    if ((damage2 += VEC2D_Value(impulse2)) > kMinDamageVelo)
      DamageObj(obj2, damage2, diff);
  momentum1 = VEC2D_CrossProduct(VEC2D_Scale(impulse1, kCalcFPS), diff);
  momentum2 = VEC2D_CrossProduct(VEC2D_Scale(impulse2, kCalcFPS), diff);
  obj1->rotVelo += momentum1 / mass1;
  obj2->rotVelo -= momentum2 / mass2;
  if (obj1 == gPlayerObj || obj2 == gPlayerObj) {
    float joltMagnitude =
        (obj1 == gPlayerObj) ? VEC2D_Value(impulse1) : VEC2D_Value(impulse2);
    joltMagnitude *= 0.05;
    if (joltMagnitude > 1)
      joltMagnitude = 1;
    FFBJolt(joltMagnitude, joltMagnitude, 0.4);
  }
}
/*void DoBounceUnelastic(tObject *obj1,tObject *obj2,t2DPoint diff)
{
        t2DPoint newVelo;
        t2DPoint impulse1,impulse2;
        float momentum1,momentum2;
        float mass1=obj1==gPlayerObj?kPlayerBounceMass:(**obj1->type).mass;
        float mass2=obj2==gPlayerObj?kPlayerBounceMass:(**obj2->type).mass;
        newVelo=VEC2D_Sum(VEC2D_Scale(obj1->velo,mass1),VEC2D_Scale(obj2->velo,mass2));
        newVelo=VEC2D_Scale(newVelo,1/(mass1+mass2));
        impulse1=VEC2D_Difference(obj1->velo,newVelo);
        impulse2=VEC2D_Difference(obj2->velo,newVelo);
//
obj1->throttle*=VEC2D_DotProduct(impulse1,VEC2D_Norm(new1Velo))/VEC2D_Value(obj1->velo);
//
obj2->throttle*=VEC2D_DotProduct(impulse2,VEC2D_Norm(new2Velo))/VEC2D_Value(obj2->velo);
        obj1->velo=newVelo;
        obj2->velo=newVelo;
        if((**obj1->type).flags2&kObjectDamageble)
                DamageObj(obj1,VEC2D_Value(impulse1),VEC2D_Scale(diff,-1));
        if((**obj2->type).flags2&kObjectDamageble)
                DamageObj(obj2,VEC2D_Value(impulse2),diff);
        momentum1=VEC2D_CrossProduct(VEC2D_Scale(impulse1,kCalcFPS),diff);
        momentum2=VEC2D_CrossProduct(VEC2D_Scale(impulse2,kCalcFPS),diff);
        obj1->rotVelo+=momentum1/mass1;
        obj2->rotVelo-=momentum2/mass2;
}

void DoBounceElastic(tObject *obj1,tObject *obj2,t2DPoint diff)
{
        t2DPoint new1Velo,new2Velo;
        t2DPoint impulse1,impulse2;
        float momentum1,momentum2;
        float mass1=obj1==gPlayerObj?kPlayerBounceMass:(**obj1->type).mass;
        float mass2=obj2==gPlayerObj?kPlayerBounceMass:(**obj2->type).mass;
        new1Velo=VEC2D_Difference(VEC2D_Sum(VEC2D_Scale(obj1->velo,mass1),VEC2D_Scale(obj2->velo,mass2*2)),VEC2D_Scale(obj1->velo,mass2));
        new2Velo=VEC2D_Difference(VEC2D_Sum(VEC2D_Scale(obj2->velo,mass2),VEC2D_Scale(obj1->velo,mass1*2)),VEC2D_Scale(obj2->velo,mass1));
        new1Velo=VEC2D_Scale(new1Velo,1/(mass1+mass2));
        new2Velo=VEC2D_Scale(new2Velo,1/(mass1+mass2));
        impulse1=VEC2D_Difference(obj1->velo,new1Velo);
        impulse2=VEC2D_Difference(obj2->velo,new2Velo);
//
obj1->throttle*=VEC2D_DotProduct(impulse1,VEC2D_Norm(new1Velo))/VEC2D_Value(obj1->velo);
//
obj2->throttle*=VEC2D_DotProduct(impulse2,VEC2D_Norm(new2Velo))/VEC2D_Value(obj2->velo);
        obj1->velo=new1Velo;
        obj2->velo=new2Velo;
        if((**obj1->type).flags2&kObjectDamageble)
                DamageObj(obj1,VEC2D_Value(impulse1),VEC2D_Scale(diff,-1));
        if((**obj2->type).flags2&kObjectDamageble)
                DamageObj(obj2,VEC2D_Value(impulse2),diff);
        momentum1=VEC2D_CrossProduct(VEC2D_Scale(impulse1,kCalcFPS),diff);
        momentum2=VEC2D_CrossProduct(VEC2D_Scale(impulse2,kCalcFPS),diff);
        obj1->rotVelo+=momentum1/mass1;
        obj2->rotVelo-=momentum2/mass2;
}
*/
void BounceObjects(tObject *obj1, tObject *obj2) {
  t2DPoint diff;
  float boom =
      (VEC2D_Value(VEC2D_Difference(obj1->velo, obj2->velo)) - kMinDamageVelo) *
      0.05;
  boom = boom < 0 ? 0 : boom;
  if (boom)
    PlaySound(obj1->pos, P2D(0, 0), 1, boom > 1 ? 1 : boom, 128);
  do {
    diff = VEC2D_Difference(obj1->pos, obj2->pos);
    if (diff.x * kSeperatio == diff.x)
      diff.x = 1;
    if (diff.y * kSeperatio == diff.y)
      diff.y = 1;
    obj2->pos = VEC2D_Sum(obj1->pos, VEC2D_Scale(diff, -kSeperatio));
    obj1->pos =
        VEC2D_Sum(obj2->pos, VEC2D_Scale(diff, kSeperatio * kSeperatio));
  } while (TestCollision(obj1, obj2, INFINITY));
  diff = VEC2D_Norm(diff);
  DoBounce(obj1, obj2, diff);
}

void BonusObject(tObject *theObj) {
  tObjectTypePtr objType = theObj->type;
  if ((*objType).flags2 & kObjectAddOnFlag) {
    int ok = false;
    do {
      switch (RanInt(0, 8)) {
        case 0:
          if (!(gPlayerAddOns & kAddOnLock)) {
            tTextEffect fx = {320, 240, kEffectSinLines + kEffectMoveLeft, 0,
                              "\pADDONShLOCKEDf"};
            NewTextEffect(&fx);
            gPlayerAddOns |= kAddOnLock;
            ok = true;
          }
          break;
        case 1: {
          tTextEffect fx = {320, 240, kEffectSinLines + kEffectMoveDown, 0,
                            "\pMINESee"};
          NewTextEffect(&fx);
          gNumMines += 5;
          ok = true;
        } break;
        case 2: {
          tTextEffect fx = {320, 240, kEffectExplode, 0, "\pMISSILESe"};
          NewTextEffect(&fx);
          gNumMissiles += 5;
          ok = true;
        } break;
        case 3:
          if (!(gPlayerAddOns & kAddOnSpikes)) {
            tTextEffect fx = {320, 240, kEffectExplode, 0, "\pSPIKESe"};
            NewTextEffect(&fx);
            gPlayerAddOns |= kAddOnSpikes;
            ok = true;
          }
          break;
        case 4:
          if (!(gPlayerAddOns & kAddOnCop)) {
            tTextEffect fx = {320, 240, kEffectSinLines, 0, "\pPOLICEhJAMMER"};
            NewTextEffect(&fx);
            gPlayerAddOns |= kAddOnCop;
            ok = true;
          }
          break;
        case 5:
          if (!(gPlayerAddOns & kAddOnTurbo)) {
            tTextEffect fx = {320, 240, kEffectExplode, 0, "\pTURBOhENGINEeee"};
            NewTextEffect(&fx);
            gPlayerAddOns |= kAddOnTurbo;
            ok = true;
          }
          break;
        case 6: {
          tTextEffect fx = {320, 240, kEffectExplode, 0, "\p][[[hAWARDEDf"};
          NewTextEffect(&fx);
          gPlayerScore += 2000;
          ok = true;
        } break;
        case 7: {
          tTextEffect fx = {320, 240, kEffectSinLines + kEffectMoveUp, 0,
                            "\pEXTRAhLIFEee"};
          NewTextEffect(&fx);
          gPlayerLives++;
          SimplePlaySound(154);
          ok = true;
        } break;
      }
    } while (!ok);
  }
  else
    gPlayerBonus = theObj->frame - (*objType).frame + 2;
  KillObject(theObj);
}

void HandleCollision(tObject *posObj) {
  tObject *theObj = gFirstVisObj;
  while (theObj != gLastVisObj) {
    if (theObj != posObj && theObj != posObj->shooter &&
        posObj != theObj->shooter) {
      float xdist = theObj->pos.x - posObj->pos.x;
      float ydist = theObj->pos.y - posObj->pos.y;
      float sqdist = xdist * xdist + ydist * ydist;
      if (sqdist < kMaxCollDist * kMaxCollDist)
        if (TestCollision(posObj, theObj, sqdist)) {
          if ((*theObj->type).flags & kObjectBounce) {
            if ((*posObj->type).flags2 & kObjectMissile) {
              posObj->jumpHeight = 0;
              posObj->jumpVelo = 0;
              KillObject(theObj);
            }
            else
              BounceObjects(theObj, posObj);
          }
          if (VEC2D_Value(posObj->velo) > kMinJumpVelo) {
            int jumpScore = 0;
            if (theObj->type->flags2 & kObjectRamp) {
              posObj->jumpVelo = VEC2D_Value(posObj->velo);
              jumpScore = theObj->type->score;
            }
            if (theObj->type->flags2 & kObjectBump) {
              posObj->jumpVelo = VEC2D_Value(posObj->velo) * 0.45;
              jumpScore = theObj->type->score;
            }
            if (posObj == gPlayerObj && jumpScore) {
              tTextEffect fx;
              Str31 str;
              gPlayerScore += jumpScore;
              NumToString(jumpScore, str);
              fx.x = theObj->pos.x;
              fx.y = theObj->pos.y;
              fx.effectFlags = kEffectExplode + kEffectTiny + kEffectAbsPos;
              MakeFXStringFromNumStr(str, fx.text);
              NewTextEffect(&fx);
            }
          }
          if (theObj->type->flags & kObjectKillsCars)
            KillObject(posObj);
          if (theObj->type->flags & kObjectKilledByCars)
            KillObject(theObj);
          if ((*theObj->type).flags2 & kObjectOil)
            posObj->rotVelo = (fabs(posObj->rotVelo) > 0.2 ? 1 : 0) *
                              (posObj->rotVelo > 0 ? 1 : -1) * PI * 0.05 *
                              VEC2D_Value(posObj->velo);
          if (posObj == gPlayerObj) {
            if ((*theObj->type).flags & kObjectBonusFlag)
              BonusObject(theObj);
          }
          else if (theObj == gSpikeObj)
            if (posObj->type->flags2 & kObjectDamageble) {
              DamageObj(posObj,
                        VEC2D_Value(gPlayerObj->velo) * kLowFrameDuration * 9,
                        VEC2D_Norm(VEC2D_Difference(theObj->pos, posObj->pos)));
              PlaySound(posObj->pos, gPlayerObj->velo, 1,
                        VEC2D_Value(gPlayerObj->velo) / 70, 143);
            }
        }
    }
    theObj = (tObject *)theObj->next;
  }
}

void ShotHitObject(tObject *theObj, t2DPoint *srcPoint, t2DPoint *shotPoint,
                   t2DPoint *impactPoint) {
  t2DPoint objPts[4];
  float closeSect = INFINITY;
  t2DPoint closeSectPoint, localImpactPoint;
  int i;
  CalcObjPoints(theObj, objPts);
  for (i = 0; i < 4; i++)
    if (SectLines(&objPts[i], &objPts[(i + 1) & 3], srcPoint, shotPoint)) {
      t2DPoint sect =
          SectPoint(&objPts[i], &objPts[(i + 1) & 3], srcPoint, shotPoint);
      t2DPoint distVec = VEC2D_Difference(*srcPoint, sect);
      float dist = sect.x * sect.x + sect.y * sect.y;
      if (dist < closeSect) {
        closeSectPoint = sect;
        closeSect = dist;
      }
    }
  *impactPoint = closeSectPoint;
  localImpactPoint = VEC2D_Difference(closeSectPoint, theObj->pos);
  localImpactPoint = P2D(sin(-theObj->dir) * localImpactPoint.y +
                             cos(-theObj->dir) * localImpactPoint.x,
                         -sin(-theObj->dir) * localImpactPoint.x +
                             cos(-theObj->dir) * localImpactPoint.y);
  localImpactPoint = VEC2D_Scale(localImpactPoint, RanFl(0.5, 1));
  theObj->frame =
      BulletHitSprite(theObj->frame, localImpactPoint.x, localImpactPoint.y);
  if ((*theObj->type).flags2 & kObjectDamageble)
    theObj->damage += 18;
}

/*#define kNumShots 2

void HandleShot(float length,tObject* shooter,t2DPoint offset)
{
        int i;
        for(i=0;i<kNumShots;i++)
        {
                float ranDir=shooter->dir+RanFl(-0.015,0.015);
                float ranLength=length*(1+RanFl(-0.05,0.05));
                tObject	*theObj=gFirstVisObj;
                t2DPoint normDir=P2D(cos(ranDir),sin(ranDir));
                t2DPoint
srcPoint=VEC2D_Sum(shooter->pos,P2D(sin(ranDir)*offset.y+cos(ranDir)*offset.x,-sin(ranDir)*offset.x+cos(ranDir)*offset.y));
                t2DPoint
shotPoint=VEC2D_Sum(srcPoint,P2D(sin(ranDir)*ranLength,cos(ranDir)*ranLength));
                float closeDist =INFINITY;
                tObject *closeObj=NULL;
                if(i==0){
                        tObject *nozzleObj=NewObject(shooter,197);
                        nozzleObj->dir=shooter->dir;
                        nozzleObj->pos=srcPoint;
                        nozzleObj->velo=shooter->velo;
                }
                while(theObj!=gLastVisObj)
                {
                        if(theObj!=shooter&&theObj->layer==shooter->layer)
                        {
                                t2DPoint
relObjPos=VEC2D_Difference(theObj->pos,shooter->pos); float
dist=fabs(VEC2D_DotProduct(normDir,relObjPos)); if(dist<kMaxCollDist*2)
                                {
                                        t2DPoint objPts[4];
                                        int i;
                                        CalcObjPoints(theObj,objPts);
                                        for(i=0;i<4;i++)
                                                if(SectLines(&objPts[i],&objPts[(i+1)&3],&srcPoint,&shotPoint))
                                                {
                                                        t2DPoint
objDist=VEC2D_Difference(theObj->pos,srcPoint); float
sqDist=objDist.x*objDist.x+objDist.y*objDist.y; if(sqDist<closeDist)
                                                        {
                                                                closeDist=sqDist;
                                                                closeObj=theObj;
                                                        }
                                                }
                                }
                        }
                        theObj=(tObject*)theObj->next;
                }
                if(closeObj)
                {
                        tObject *gunObj=NewObject(closeObj,196);
                        ShotHitObject(closeObj,&srcPoint,&shotPoint,&gunObj->pos);
                        NewParticleFX(gunObj->pos,closeObj->velo,10,0x03,1,25);
                }
                else
                {
                        tObject
*gunObj=NewObject(shooter,(CalcBackCollision(shotPoint)==2?(*gRoadInfo).deathOffs:0)+196);
                        gunObj->pos=shotPoint;
                }
        }
}*/

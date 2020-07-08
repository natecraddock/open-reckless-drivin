#include "gamesounds.h"
#include "input.h"
#include "objectcontrol.h"
#include "objects.h"
#include "particlefx.h"
#include "random.h"
#include "roads.h"
#include "screen.h"
#include "trig.h"
#include "vec2d.h"
#include <math.h>

#define kMechResistance 100
#define kMinSteerTime                                                          \
  0.8 // minimum time (in seconds) for the steering wheel to go to maximum
      // steering.
#define kVeloSteerTime                                                         \
  0.075 // multiplied by cars velocity and added to kMinSteerTime.
#define kJustTime                                                              \
  0.2 // minimum time (in seconds) for the steering wheel to recenter.
#define kSlideVelo 15
#define kHeliRotAccel 10.0
#define kHeliResistance 5.0
#define kMaxHeliRotVelo PI
#define kMinSmokeDamage 0.6
#define kMinSlide 0.75
#define kSlideFactor                                                           \
  0.5 // determines how much control one has over the car when it is sliding
      // (0=no difference/1= 'no control')
#define kMotorRunTime                                                          \
  0.3 // time in seconds for motor to reach maximum acceleration.
#define kTrackIntensity 0.075 // Intensity of rubber Tracks left by cars.

void HandleCollision(tObject *);

t2DPoint CalcWheelForce(float dir, float wheelPower, float fRoadFriction,
                        float mass, float velo, t2DPoint vVelo, tObject *theObj,
                        float *slide) {
  t2DPoint fWheel = P2D(0, 0), fResistance;
  float val, slideOut;
  if (theObj->input.brake && !(*gRoadInfo).water && velo) {
    if (theObj->input.brake < 0.8)
      fWheel = VEC2D_Scale(vVelo, -1500 * mass * theObj->input.brake *
                                      theObj->input.brake / velo);
    else
      fWheel = VEC2D_Scale(vVelo, -fRoadFriction);
    if (VEC2D_Value(VEC2D_Scale(fWheel, 1 / mass * kFrameDuration)) > velo)
      fWheel = VEC2D_Scale(vVelo, -mass * kLowCalcFPS);
  }
  if (theObj->input.brake < 0.8 || (*gRoadInfo).water && velo) {
    t2DPoint vDir = P2D(sin(dir), cos(dir));
    t2DPoint ortoDir = VEC2D_CP(vDir);
    float ortoVelo = VEC2D_DotProduct(vVelo, ortoDir);
    t2DPoint fSidewall = VEC2D_Scale(ortoDir, -ortoVelo * mass * kLowCalcFPS);
    t2DPoint fEngine = VEC2D_Scale(vDir, wheelPower);
    fWheel = VEC2D_Sum(fWheel, VEC2D_Sum(fEngine, fSidewall));
  }
  val = VEC2D_Value(fWheel);
  slideOut = val / fRoadFriction;
  *slide += slideOut / 4;
  if (slideOut > 1)
    fWheel = VEC2D_Scale(fWheel, 1 / slideOut);
  fResistance =
      VEC2D_Sum(VEC2D_Scale(theObj->velo, -velo * (*gRoadInfo).airResistance),
                VEC2D_Scale(vVelo, -kMechResistance));
  fWheel = VEC2D_Sum(fWheel, fResistance);
  return fWheel;
}

t2DPoint CalcWheelForcePlayer(float dir, float wheelPower, float fRoadFriction,
                              float mass, float velo, t2DPoint vVelo,
                              tObject *theObj, float *slide, float brake) {
  t2DPoint fWheel = P2D(0, 0);
  t2DPoint fResistance;
  float val;
  if (brake > 1)
    brake = 1;
  if (brake && !(*gRoadInfo).water && velo) {
    if (brake < 0.8)
      fWheel = VEC2D_Scale(vVelo, -1500 * mass * brake * brake / velo);
    else
      fWheel = VEC2D_Scale(vVelo, -fRoadFriction);
    if (VEC2D_Value(VEC2D_Scale(fWheel, 1 / mass * kFrameDuration)) > velo)
      fWheel = VEC2D_Scale(vVelo, -mass * kCalcFPS);
  }
  if (brake < 0.8 || (*gRoadInfo).water && velo) {
    t2DPoint vDir = P2D(sin(dir), cos(dir));
    t2DPoint ortoDir = VEC2D_CP(vDir);
    float ortoVelo = VEC2D_DotProduct(vVelo, ortoDir);
    t2DPoint fSidewall = VEC2D_Scale(ortoDir, -ortoVelo * mass * kCalcFPS);
    t2DPoint fEngine = VEC2D_Scale(vDir, wheelPower);
    fWheel = VEC2D_Sum(fWheel, VEC2D_Sum(fEngine, fSidewall));
  }
  val = VEC2D_Value(fWheel);
  *slide = val / fRoadFriction;
  if (*slide > 1)
    fWheel = VEC2D_Scale(fWheel, 1 / (*slide));
  fResistance =
      VEC2D_Sum(VEC2D_Scale(theObj->velo, -velo * (*gRoadInfo).airResistance),
                VEC2D_Scale(vVelo, -kMechResistance));
  fWheel = VEC2D_Sum(fWheel, fResistance);
  return fWheel;
}

void AddTrack(t2DPoint p1, t2DPoint p2, float intensity) {
  gTrackCount++;
  if (gTrackCount >= kMaxTracks) {
    BlockMoveData(gTracks + kMaxTracks / 2, gTracks,
                  kMaxTracks / 2 * sizeof(tTrackSeg));
    gTrackCount -= kMaxTracks / 2;
  }
  if (p1.y > p2.y) {
    gTracks[gTrackCount].p1 = p1;
    gTracks[gTrackCount].p2 = p2;
  } else {
    gTracks[gTrackCount].p1 = p2;
    gTracks[gTrackCount].p2 = p1;
  }
  gTracks[gTrackCount].intensity = intensity > 1 ? 1 : intensity;
  gTracks[gTrackCount].time = gFrameCount;
}

void CalcSteeringPlayer(tObject *theObj, float velo) {
  float steerTime = kMinSteerTime + velo * kVeloSteerTime;
  float maxSteering = (*theObj->type).steering;
  if (theObj->input.steering * maxSteering > theObj->steering) {
    theObj->steering +=
        kFrameDuration / (theObj->steering > 0 ? steerTime : kJustTime);
    if (theObj->input.steering * maxSteering < theObj->steering)
      theObj->steering = theObj->input.steering * maxSteering;
  } else if (theObj->input.steering * maxSteering < theObj->steering) {
    theObj->steering -=
        kFrameDuration / (theObj->steering < 0 ? steerTime : kJustTime);
    if (theObj->input.steering * maxSteering > theObj->steering)
      theObj->steering = theObj->input.steering * maxSteering;
  }
}

float CalcPower(tObject *theObj, float velo) {
  tObjectTypePtr objType = theObj->type;
  float signedVelo =
      VEC2D_DotProduct(theObj->velo, P2D(sin(theObj->dir), cos(theObj->dir)));
  float power;
  int accel = ((theObj->input.kickdown && !gRoadInfo->water)
                   ? (velo < 25 || signedVelo < 0 ? 50 : 3)
                   : 2);
  float throttle = theObj->throttle *
                   (velo < 50 ? accel / (velo * 0.1 + 1) + 1 - accel / 6 : 1) *
                   15;
  if (gRoadInfo->water && theObj->input.brake)
    throttle = -theObj->input.brake * (signedVelo > 0 ? 30 : -30);
  power = signedVelo > -30 ? throttle * (*objType).maxEngineForce : 0;
  if (theObj == gPlayerObj)
    power *=
        (gPlayerAddOns & kAddOnTurbo && gRoadInfo->friction >= 100 ? 1.7 : 1);
  if (theObj->input.throttle > theObj->throttle) {
    theObj->throttle += kFrameDuration / kMotorRunTime;
    if (theObj->throttle > theObj->input.throttle)
      theObj->throttle = theObj->input.throttle;
  } else if (theObj->input.throttle < theObj->throttle) {
    theObj->throttle -= kFrameDuration / kMotorRunTime;
    if (theObj->throttle < theObj->input.throttle)
      theObj->throttle = theObj->input.throttle;
  }
  return power;
}

inline float SlideFriction(float slide) {
  if (slide > kMinSlide) {
    slide -= kMinSlide;
    slide *= kSlideFactor;
    if (slide > 1)
      slide = 1;
    return 1 - slide * (*gRoadInfo).slideFriction;
  } else
    return 1;
}

void ControlWheelObjectPlayer(tObject *theObj) {
  tObjectTypePtr objType = theObj->type;
  float cosDir = cos(theObj->dir);
  float sinDir = sin(theObj->dir);
  t2DPoint objDir = P2D(sinDir, cosDir);
  float velo = VEC2D_Value(theObj->velo);
  float fRoadFriction =
      (*objType).mass * (*objType).friction * (*gRoadInfo).friction;
  float power = CalcPower(theObj, velo);
  float rotMass = (*objType).length * (*objType).length * (*objType).mass;
  float slide[4] = {0, 0, 0, 0};

  t2DPoint pFrontLeft =
      P2D(sinDir * (*objType).wheelLength - cosDir * (*objType).wheelWidth,
          cosDir * (*objType).wheelLength + sinDir * (*objType).wheelWidth);
  t2DPoint vFrontLeft = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pFrontLeft), theObj->rotVelo));
  t2DPoint pTotalFrontLeft =
      VEC2D_Sum(VEC2D_Scale(pFrontLeft, kScale), theObj->pos);
  t2DPoint fFrontLeft = CalcWheelForcePlayer(
      theObj->dir +
          ((*objType).flags2 & kObjectRearSteer ? 0 : theObj->steering),
      ((*objType).flags2 & kObjectRearDrive ? 0 : power),
      fRoadFriction * SlideFriction(gPlayerSlide[0]), (*objType).mass, velo,
      vFrontLeft, theObj, &slide[0], theObj->input.brake);
  float mFrontLeft = VEC2D_CrossProduct(fFrontLeft, pFrontLeft);

  t2DPoint pFrontRight =
      P2D(sinDir * (*objType).wheelLength + cosDir * (*objType).wheelWidth,
          cosDir * (*objType).wheelLength - sinDir * (*objType).wheelWidth);
  t2DPoint vFrontRight = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pFrontRight), theObj->rotVelo));
  t2DPoint pTotalFrontRight =
      VEC2D_Sum(VEC2D_Scale(pFrontRight, kScale), theObj->pos);
  t2DPoint fFrontRight = CalcWheelForcePlayer(
      theObj->dir +
          ((*objType).flags2 & kObjectRearSteer ? 0 : theObj->steering),
      ((*objType).flags2 & kObjectRearDrive ? 0 : power),
      fRoadFriction * SlideFriction(gPlayerSlide[1]), (*objType).mass, velo,
      vFrontRight, theObj, &slide[1], theObj->input.brake);
  float mFrontRight = VEC2D_CrossProduct(fFrontRight, pFrontRight);

  t2DPoint pRearLeft =
      P2D(-sinDir * (*objType).wheelLength - cosDir * (*objType).wheelWidth,
          -cosDir * (*objType).wheelLength + sinDir * (*objType).wheelWidth);
  t2DPoint vRearLeft = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pRearLeft), theObj->rotVelo));
  t2DPoint pTotalRearLeft =
      VEC2D_Sum(VEC2D_Scale(pRearLeft, kScale), theObj->pos);
  t2DPoint fRearLeft = CalcWheelForcePlayer(
      theObj->dir -
          ((*objType).flags2 & kObjectRearSteer ? theObj->steering : 0),
      ((*objType).flags2 & kObjectRearDrive ? power : 0),
      fRoadFriction * SlideFriction(gPlayerSlide[2]), (*objType).mass, velo,
      vRearLeft, theObj, &slide[2],
      theObj->input.handbrake + 0.8 * theObj->input.brake);
  float mRearLeft = VEC2D_CrossProduct(fRearLeft, pRearLeft);

  t2DPoint pRearRight =
      P2D(-sinDir * (*objType).wheelLength + cosDir * (*objType).wheelWidth,
          -cosDir * (*objType).wheelLength - sinDir * (*objType).wheelWidth);
  t2DPoint vRearRight = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pRearRight), theObj->rotVelo));
  t2DPoint pTotalRearRight =
      VEC2D_Sum(VEC2D_Scale(pRearRight, kScale), theObj->pos);
  t2DPoint fRearRight = CalcWheelForcePlayer(
      theObj->dir -
          ((*objType).flags2 & kObjectRearSteer ? theObj->steering : 0),
      ((*objType).flags2 & kObjectRearDrive ? power : 0),
      fRoadFriction * SlideFriction(gPlayerSlide[3]), (*objType).mass, velo,
      vRearRight, theObj, &slide[3],
      theObj->input.handbrake + 0.8 * theObj->input.brake);
  float mRearRight = VEC2D_CrossProduct(fRearRight, pRearRight);

  t2DPoint aTotal = VEC2D_Scale(
      VEC2D_Sum(VEC2D_Sum(VEC2D_Sum(fFrontLeft, fFrontRight), fRearLeft),
                fRearRight),
      0.25 / (*objType).mass * kFrameDuration);
  float aRotTotal = (mFrontLeft + mFrontRight + mRearLeft + mRearRight) /
                    (4 * rotMass) * kFrameDuration;

  if (slide[0] > (*gRoadInfo).trackSlide)
    if (!CalcBackCollision(pTotalFrontLeft))
      AddTrack(pTotalFrontLeft,
               VEC2D_Sum(VEC2D_Scale(vFrontLeft, kScale * kFrameDuration),
                         pTotalFrontLeft),
               (slide[0] - (*gRoadInfo).trackSlide) * kTrackIntensity);
  if (slide[0] > (*gRoadInfo).dustSlide)
    NewParticleFX(pTotalFrontLeft, vFrontLeft, 12, (*gRoadInfo).dustColor, 0,
                  20);
  if (slide[1] > (*gRoadInfo).trackSlide)
    if (!CalcBackCollision(pTotalFrontRight))
      AddTrack(pTotalFrontRight,
               VEC2D_Sum(VEC2D_Scale(vFrontRight, kScale * kFrameDuration),
                         pTotalFrontRight),
               (slide[1] - (*gRoadInfo).trackSlide) * kTrackIntensity);
  if (slide[1] > (*gRoadInfo).dustSlide)
    NewParticleFX(pTotalFrontRight, vFrontRight, 12, (*gRoadInfo).dustColor, 0,
                  20);
  if (slide[2] > (*gRoadInfo).trackSlide)
    if (!CalcBackCollision(pTotalRearLeft))
      AddTrack(pTotalRearLeft,
               VEC2D_Sum(VEC2D_Scale(vRearLeft, kScale * kFrameDuration),
                         pTotalRearLeft),
               (slide[2] - (*gRoadInfo).trackSlide) * kTrackIntensity);
  if (slide[2] > (*gRoadInfo).dustSlide)
    NewParticleFX(pTotalRearLeft, vRearLeft, 12, (*gRoadInfo).dustColor, 0, 20);
  if (slide[3] > (*gRoadInfo).trackSlide)
    if (!CalcBackCollision(pTotalRearRight))
      AddTrack(pTotalRearRight,
               VEC2D_Sum(VEC2D_Scale(vRearRight, kScale * kFrameDuration),
                         pTotalRearRight),
               (slide[3] - (*gRoadInfo).trackSlide) * kTrackIntensity);
  if (slide[3] > (*gRoadInfo).dustSlide)
    NewParticleFX(pTotalRearRight, vRearRight, 12, (*gRoadInfo).dustColor, 0,
                  20);
  theObj->velo = VEC2D_Sum(aTotal, theObj->velo);
  theObj->rotVelo += aRotTotal;
  theObj->slide = (slide[0] + slide[1] + slide[2] + slide[3]) / 4;
  gPlayerSlide[0] = slide[0];
  gPlayerSlide[1] = slide[1];
  gPlayerSlide[2] = slide[2];
  gPlayerSlide[3] = slide[3];
  CalcSteeringPlayer(theObj, velo);
}

void ControlWheelObject(tObject *theObj) {
  tObjectTypePtr objType = theObj->type;
  float cosDir = cos(theObj->dir);
  float sinDir = sin(theObj->dir);
  t2DPoint objDir = P2D(sinDir, cosDir);
  float velo = VEC2D_Value(theObj->velo);
  float fRoadFriction = (*objType).mass * (*objType).friction *
                        (*gRoadInfo).friction * SlideFriction(theObj->slide);
  float power = CalcPower(theObj, velo);
  float rotMass = (*objType).length * (*objType).length * (*objType).mass;
  float slide = 0;

  t2DPoint pFrontLeft =
      P2D(sinDir * (*objType).wheelLength - cosDir * (*objType).wheelWidth,
          cosDir * (*objType).wheelLength + sinDir * (*objType).wheelWidth);
  t2DPoint vFrontLeft = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pFrontLeft), theObj->rotVelo));
  t2DPoint fFrontLeft =
      CalcWheelForce(theObj->dir + theObj->steering, power, fRoadFriction,
                     (*objType).mass, velo, vFrontLeft, theObj, &slide);
  float mFrontLeft = VEC2D_CrossProduct(fFrontLeft, pFrontLeft);

  t2DPoint pFrontRight =
      P2D(sinDir * (*objType).wheelLength + cosDir * (*objType).wheelWidth,
          cosDir * (*objType).wheelLength - sinDir * (*objType).wheelWidth);
  t2DPoint vFrontRight = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pFrontRight), theObj->rotVelo));
  t2DPoint fFrontRight =
      CalcWheelForce(theObj->dir + theObj->steering, power, fRoadFriction,
                     (*objType).mass, velo, vFrontRight, theObj, &slide);
  float mFrontRight = VEC2D_CrossProduct(fFrontRight, pFrontRight);

  t2DPoint pRearLeft =
      P2D(-sinDir * (*objType).wheelLength - cosDir * (*objType).wheelWidth,
          -cosDir * (*objType).wheelLength + sinDir * (*objType).wheelWidth);
  t2DPoint vRearLeft = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pRearLeft), theObj->rotVelo));
  t2DPoint fRearLeft =
      CalcWheelForce(theObj->dir, 0, fRoadFriction, (*objType).mass, velo,
                     vRearLeft, theObj, &slide);
  float mRearLeft = VEC2D_CrossProduct(fRearLeft, pRearLeft);

  t2DPoint pRearRight =
      P2D(-sinDir * (*objType).wheelLength + cosDir * (*objType).wheelWidth,
          -cosDir * (*objType).wheelLength - sinDir * (*objType).wheelWidth);
  t2DPoint vRearRight = VEC2D_Sum(
      theObj->velo, VEC2D_Scale(VEC2D_CP(pRearRight), theObj->rotVelo));
  t2DPoint fRearRight =
      CalcWheelForce(theObj->dir, 0, fRoadFriction, (*objType).mass, velo,
                     vRearRight, theObj, &slide);
  float mRearRight = VEC2D_CrossProduct(fRearRight, pRearRight);

  t2DPoint aTotal = VEC2D_Scale(
      VEC2D_Sum(VEC2D_Sum(VEC2D_Sum(fFrontLeft, fFrontRight), fRearLeft),
                fRearRight),
      0.25 / (*objType).mass * kLowFrameDuration);
  float aRotTotal = (mFrontLeft + mFrontRight + mRearLeft + mRearRight) /
                    (4 * rotMass) * kLowFrameDuration;

  theObj->velo = VEC2D_Sum(aTotal, theObj->velo);
  theObj->rotVelo += aRotTotal;
  theObj->slide = slide;
  theObj->steering = theObj->input.steering;
}

void ControlWheelObjectFast(tObject *theObj) {
  t2DPoint objDir = P2D(sin(theObj->dir), cos(theObj->dir));
  theObj->dir += theObj->input.steering * PI * kLowFrameDuration;
  theObj->velo = VEC2D_Scale(objDir, theObj->throttle * 30);
  theObj->rotVelo = 0;
  if (theObj->throttle < theObj->input.throttle)
    theObj->throttle += kLowFrameDuration;
  else
    theObj->throttle -= kLowFrameDuration;
  if (theObj->input.brake)
    theObj->throttle = 0;
}

void ControlHeliObject(tObject *theObj) {
  tObjectTypePtr objType = theObj->type;
  t2DPoint objDir = P2D(sin(theObj->dir), cos(theObj->dir));
  float heliDist = VEC2D_Value(VEC2D_Difference(theObj->pos, gPlayerObj->pos));
  float velo = VEC2D_Value(theObj->velo);
  float fEngine = CalcPower(theObj, velo);
  float spanArea =
      fabs(VEC2D_DotProduct(objDir, VEC2D_Norm(theObj->velo))) * 3 +
      fabs(VEC2D_DotProduct(VEC2D_CP(objDir), VEC2D_Norm(theObj->velo)) * 10);
  t2DPoint vfEngine = VEC2D_Scale(objDir, fEngine);
  t2DPoint vfResistance =
      VEC2D_Scale(theObj->velo, -velo * (*gRoadInfo).airResistance * spanArea);
  t2DPoint aTotal = VEC2D_Scale(VEC2D_Sum(vfResistance, vfEngine),
                                kLowFrameDuration / (*objType).mass);
  theObj->velo = VEC2D_Sum(aTotal, theObj->velo);
  theObj->rotVelo += theObj->steering * kHeliRotAccel * kLowFrameDuration;
  theObj->rotVelo -= theObj->rotVelo * kHeliResistance * kLowFrameDuration;
  if (fabs(theObj->rotVelo) > kMaxHeliRotVelo)
    theObj->rotVelo =
        (theObj->rotVelo > 0 ? kMaxHeliRotVelo : -kMaxHeliRotVelo);
  theObj->steering = theObj->input.steering;
}

void SolidObject(tObject *theObj) {
  tObjectTypePtr objType = theObj->type;
  float aFriction =
      0.015 * (*objType).friction * (*gRoadInfo).friction * kLowFrameDuration;
  float velo = VEC2D_Value(theObj->velo);
  if (aFriction < velo) {
    t2DPoint aVFriction = VEC2D_Scale(theObj->velo, -aFriction / velo);
    theObj->velo = VEC2D_Sum(theObj->velo, aVFriction);
  } else
    theObj->velo = P2D(0, 0);
  aFriction *= 0.05;
  if (theObj->rotVelo) {
    theObj->rotVelo =
        theObj->rotVelo + (theObj->rotVelo > 0 ? -aFriction : aFriction);
    if ((theObj->rotVelo < 0) && (theObj->rotVelo + aFriction > 0))
      theObj->rotVelo = 0;
  }
}

int CheckObjectMotion(tObject *theObj) {
  if (theObj->velo.x || theObj->velo.y)
    return true;
  if (theObj->input.throttle)
    return true;
  if (!theObj->rotVelo)
    return false;
}

void MakeSmoke(tObject *theObj) {
  tObject *smokeObj = NewObject(theObj, 195);
  tObjectTypePtr objType = theObj->type;
  smokeObj->pos = VEC2D_Sum(theObj->pos,
                            P2D(sin(theObj->dir) * (*objType).length * kScale,
                                cos(theObj->dir) * (*objType).length * kScale));
  smokeObj->pos = VEC2D_Sum(smokeObj->pos, P2D(RanFl(-10, 10), RanFl(-10, 10)));
}

void BurnObj(tObject *theObj) {
  tObject *expObj;
  float l = (*theObj->type).length * kScale;
  expObj = NewObject(theObj, 1001);
  expObj->frameDuration = RanFl(0, 4);
  expObj->frame = 0;
  expObj->pos.x = theObj->pos.x + RanFl(0, l);
  expObj->pos.y = theObj->pos.y + RanFl(0, l);
  expObj->dir = RanFl(0, 2 * PI);
  expObj->velo = VEC2D_Scale(theObj->velo, 0.5);
}

void Explosion(t2DPoint pos, t2DPoint velo, int offs, float mass, int sound);

void ObjectPhysics(tObject *theObj) {
  if (!theObj->jumpHeight || theObj->type->flags & kObjectHeliFlag) {
    if (theObj->type->flags & kObjectWheelFlag)
      if (fabs(theObj->pos.y - gCameraObj->pos.y) < 10 * kVisDist) {
        if (theObj == gPlayerObj)
          ControlWheelObjectPlayer(theObj);
        else if (CheckObjectMotion(theObj))
          ControlWheelObject(theObj);
      } else
        ControlWheelObjectFast(theObj);
    else if (theObj->type->flags & kObjectSolidFrictionFlag)
      SolidObject(theObj);
    else if (theObj->type->flags & kObjectHeliFlag)
      ControlHeliObject(theObj);
    if (theObj->type->flags2 & kObjectMissile) {
      if (theObj->jumpHeight == 0 && theObj->jumpVelo == 0)
        KillObject(theObj);
    } else if (fabs(theObj->pos.y - gCameraObj->pos.y) < kVisDist) {
      if (theObj->type->flags & kObjectBackCollFlag ||
          theObj->type->flags2 & kObjectSink && (*gRoadInfo).deathOffs) {
        if (CalcBackCollision(theObj->pos) == 2)
          KillObject(theObj);
      } else if (theObj->type->flags2 & kObjectFrontCollFlag) {
        if (CalcBackCollision(theObj->pos) == 0)
          KillObject(theObj);
      }
      if (theObj->type->flags & kObjectBounce)
        HandleCollision(theObj);
      if (theObj->type->flags & kObjectCop)
        if (!(gPlayerAddOns & kAddOnCop) && !gFinishDelay &&
            !gPlayerDeathDelay) {
          t2DPoint dirVec = P2D(sin(theObj->dir), cos(theObj->dir));
          t2DPoint normDiffVec =
              VEC2D_Norm(VEC2D_Difference(gPlayerObj->pos, theObj->pos));
          if (theObj->control == kObjectDriveUp ||
              theObj->control == kObjectNoInput)
            theObj->control = kObjectCopControl;
          else if (theObj->control == kObjectDriveDown) {
            if (gPlayerObj->pos.y > theObj->pos.y + kStartChaseDist * kScale)
              ObjectStartChase(theObj, kStartChaseDist * kScale,
                               kStartChaseSpeed);
          } else if (theObj->control == kObjectCopControl)
            if (theObj->type->weaponObj)
              if (VEC2D_DotProduct(dirVec, normDiffVec) > 0.9)
                if (VEC2D_Value(
                        VEC2D_Difference(gPlayerObj->pos, theObj->pos)) > 70)
                  if (RanProb(1.0 / theObj->type->weaponInfo))
                    FireWeapon(theObj, theObj->type->weaponObj);
        }
      if (theObj->type->flags2 & kObjectDamageble) {
        if (theObj->damage * 2 > theObj->type->maxDamage &&
            fabs(theObj->pos.y - gCameraObj->pos.y) < kVisDist)
          if (RanProb(0.25))
            MakeSmoke(theObj);
        if (theObj->damage > theObj->type->maxDamage) {
          theObj->damage += kLowFrameDuration * 0.5 * theObj->type->maxDamage;
          if (RanProb((theObj->damage - theObj->type->maxDamage) /
                      (float)theObj->type->maxDamage * 0.25))
            Explosion(theObj->pos, theObj->velo, 0, 250, 0);
          if (theObj->damage > theObj->type->maxDamage * 2)
            KillObject(theObj);
        }
      }
    } else if (theObj->type->flags2 & kObjectDieWhenOutOfScreen)
      RemoveObject(theObj);
  } else if (theObj->type->flags2 & kObjectMissile)
    HandleCollision(theObj);
}

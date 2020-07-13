#include <stdlib.h>

#include "objectcontrol.h"
#include "gamesounds.h"
#include "input.h"
#include "objects.h"
#include "random.h"
#include "roads.h"
#include "screen.h"
#include "trig.h"
#include "vec2d.h"
#include <math.h>

#define kOvertakeDist                                                          \
  7.0 // Distance in meters cars move to the left to overtake.
#define kControlSteering 0.8 // Maximal steering input given to ai cars.
#define kPreSteerTime 4 * kFrameDuration
#define kTargetSwitchDist 50.0

#define kChaseSpeed 2.4
#define kChaseDist 40.0
#define kWaitSpeed 0.15
#define kWaitDist 60.0
#define kFinalChaseSpeed 20.0
#define kCallFriendProbility                                                   \
  0.1 // Probility that a cop will call Assistance within each second
#define kCopExtraBurstVelo                                                     \
  30.0 // when cops move slower than this, they will get extra acceleration
#define kCopExtraBurst                                                         \
  1.8 // amount of extra acceleration recieved (1= no extra acceleration,2=
      // double acceleration)

float GetCloseCar(t2DPoint pos) {
  tObject *theObj = gFirstVisObj;
  tObject *closeObj = NULL;
  int dist = 0x7fffffff;
  while (theObj != gLastVisObj) {
    if ((*theObj->type).flags & kObjectBackCollFlag + kObjectBounce ||
        (*theObj->type).flags2 & kObjectRoadKill) {
      float xdist = theObj->pos.x - pos.x;
      float ydist = theObj->pos.y - pos.y;
      float sqdist = xdist * xdist + ydist * ydist;
      if (sqdist < dist) {
        dist = sqdist;
        closeObj = theObj;
      }
    }
    theObj = theObj->next;
  }
  return dist;
}

void CheckTarget(tObject *theObj) {
  tTrackInfo *track = (theObj->control == kObjectDriveUp ||
                       theObj->control == kObjectCopControl)
                          ? gTrackUp
                          : gTrackDown;
  t2DPoint targDist = VEC2D_Difference(
      P2D(track->track[theObj->target].x, track->track[theObj->target].y),
      theObj->pos);
  float sqTargDist = targDist.x * targDist.x + targDist.y * targDist.y;
  int passed = (theObj->control == kObjectDriveUp ||
                theObj->control == kObjectCopControl)
                   ? track->track[theObj->target].y < theObj->pos.y
                   : track->track[theObj->target].y > theObj->pos.y;
  if (sqTargDist < kTargetSwitchDist * kTargetSwitchDist || passed)
    theObj->target = (theObj->target + 1) % track->num;
}

void TargetObject(tObject *theObj, tTrackInfoSeg *target) {
  target->x = theObj->pos.x;
  target->y = theObj->pos.y;
  if (!(theObj->type->flags & kObjectKilledByCars))
    target->flags = kTargetNoStop;
  target->velo = kFinalChaseSpeed + VEC2D_Value(gPlayerObj->velo);
}

void ObjectFollow(tObject *theObj, tTrackInfoSeg *target, tInputData *input) {
  t2DPoint objDir = P2D(sin(theObj->dir), cos(theObj->dir));
  tObjectTypePtr objType = theObj->type;
  float velo = VEC2D_DotProduct(objDir, theObj->velo) < 0
                   ? -VEC2D_Value(theObj->velo)
                   : VEC2D_Value(theObj->velo);
  int offs = (((*objType).flags & kObjectOvertake) &&
              (target->flags == kTargetOvertake))
                 ? (theObj->control == kObjectDriveUp ? -kOvertakeDist * kScale
                                                      : kOvertakeDist * kScale)
                 : 0;
  t2DPoint targetDiff =
      VEC2D_Difference(P2D(target->x + offs, target->y), theObj->pos);
  float targetDist = VEC2D_Value(targetDiff);
  input->throttle = (velo < (((*objType).flags & kObjectSlow) ? 0.6 : 1) *
                                (offs ? 2 : 1) * target->velo)
                        ? 1
                        : 0;
  input->brake = 0;
  if (fabs(theObj->pos.y - gCameraObj->pos.y) < kVisDist)
    if (!(target->flags & kTargetNoStop) &&
        !((*objType).flags & kObjectHeliFlag)) {
      int dist;
      if (dist = GetCloseCar(VEC2D_Sum(
                     theObj->pos,
                     VEC2D_Scale(objDir, kMinCarDist * 0.5 * kScale))) <
                 kScale * kScale * 5 * 5)
        input->brake = 0.1;
      else if (dist < kScale * kScale * kMinCarDist * kMinCarDist * 0.5 * 0.5) {
        if (GetCloseCar(VEC2D_Sum(theObj->pos,
                                  VEC2D_Scale(objDir, kMinCarDist * kScale))) <
            kScale * kScale * 8 * 8)
          input->throttle = 0.0;
        else if (GetCloseCar(VEC2D_Sum(
                     theObj->pos,
                     VEC2D_Scale(objDir, kMinCarDist * 0.2 * kScale))) <
                 kScale * kScale * 3 * 3) {
          input->brake = 0.4;
          if (!(theObj->velo.x > 0.5 || theObj->velo.y > 0.5))
            if (RanProb(0.012))
              PlaySound(theObj->pos, theObj->velo, 1.0, 1.0, 136);
        }
      }
    }
  objDir = P2D(sin(theObj->dir + theObj->rotVelo * kPreSteerTime),
               cos(theObj->dir + theObj->rotVelo * kPreSteerTime));
  input->steering = (-VEC2D_CrossProduct(objDir, targetDiff) /
                     (targetDist > 100 ? targetDist : 100)) *
                    kControlSteering;
  if (VEC2D_DotProduct(objDir, targetDiff) < 0)
    if (!(objType->flags2 & kObjectRoadKill))
      input->brake = 0.6;
  if (theObj->control == kObjectCopControl)
    if (VEC2D_Value(theObj->velo) < kCopExtraBurstVelo)
      input->throttle *= kCopExtraBurst;
  if (input->brake)
    input->throttle = 0;
}

void ObjectStartChase(tObject *theObj, int distance, float velo) {
  int target;
  for (target = 1; (target < gTrackUp->num) &&
                   (gTrackUp->track[target].y < gPlayerObj->pos.y - distance);
       target++)
    ;
  target++;
  theObj->target = target;
  theObj->control = kObjectCopControl;
  theObj->pos.y = gPlayerObj->pos.y - kStartChaseDist * kScale;
  theObj->pos.x =
      gTrackUp->track[target - 1].x +
      (gTrackUp->track[target].x - gTrackUp->track[target - 1].x) /
          (gTrackUp->track[target].y - gTrackUp->track[target - 1].y) *
          (theObj->pos.y - gTrackUp->track[target - 1].y);
  theObj->dir = atan((theObj->pos.x - gTrackUp->track[target].x) /
                     (theObj->pos.y - gTrackUp->track[target].y));
  theObj->velo =
      VEC2D_Scale(VEC2D_Norm(P2D((theObj->pos.x - gTrackUp->track[target].x),
                                 (theObj->pos.y - gTrackUp->track[target].y))),
                  -velo);
}

void CallFriend(tObject *copObj) {
  tObject *theObj = gFirstObj->next;
  tObject *farObj = NULL;
  int dist = 0;
  while (theObj != gFirstObj) {
    if ((*theObj->type).flags & kObjectCop) {
      float xdist = theObj->pos.x - copObj->pos.x;
      float ydist = theObj->pos.y - copObj->pos.y;
      float sqdist = xdist * xdist + ydist * ydist;
      if (sqdist > dist) {
        dist = sqdist;
        farObj = theObj;
      }
    }
    theObj = theObj->next;
  }
  if (farObj)
    if (fabs(farObj->pos.y - gPlayerObj->pos.y) > kVisDist * 2)
      ObjectStartChase(farObj, gPlayerObj->pos.y - copObj->pos.y + 50,
                       kStartChaseSpeed);
}

void HandleShot(float, tObject *, t2DPoint);

void CopFollow(tObject *theObj, tInputData *input) {
  tTrackInfoSeg target;
  if (fabs(theObj->pos.y - gCameraObj->pos.y) > kVisDist ||
      gPlayerAddOns & kAddOnCop || gFinishDelay || gPlayerDeathDelay)
    theObj->control = kObjectDriveUp;
  else {
    if ((gPlayerObj->pos.y > theObj->pos.y) &&
        (gPlayerObj->pos.y < theObj->pos.y + kChaseDist * kScale)) {
      tRoad objRoadData = gRoadData + (int)(theObj->pos.y / 2);
      tRoad plrRoadData = gRoadData + (int)(gPlayerObj->pos.y / 2);
      if (RanProb(kCallFriendProbility * kLowFrameDuration))
        CallFriend(theObj);
      if (((*objRoadData)[1] == (*objRoadData)[2]) ||
          (((*objRoadData)[1] > theObj->pos.x) ==
           ((*plrRoadData)[1] > gPlayerObj->pos.x))) {
        tInputData playerTarget, roadTarget;
        TargetObject(gPlayerObj, &target);
        ObjectFollow(theObj, &target, &playerTarget);
        target = gTrackUp->track[theObj->target];
        target.velo = kFinalChaseSpeed + VEC2D_Value(gPlayerObj->velo);
        if (!(theObj->type->flags & kObjectKilledByCars))
          target.flags |= kTargetNoStop;
        ObjectFollow(theObj, &target, &roadTarget);
        *input = (fabs(playerTarget.steering - roadTarget.steering) > 0.9)
                     ? roadTarget
                     : playerTarget;
        return;
      }
    }
    if (gPlayerObj->pos.y > theObj->pos.y) {
      target = gTrackUp->track[theObj->target];
      target.velo *= kChaseSpeed;
      if (gPlayerObj->pos.y < theObj->pos.y + 5 * kChaseDist * kScale) {
        if (!(theObj->type->flags & kObjectKilledByCars))
          target.flags |= kTargetNoStop;
        target.velo = kFinalChaseSpeed + VEC2D_Value(gPlayerObj->velo);
      }
      ObjectFollow(theObj, &target, input);
    }
    else {
      target = gTrackUp->track[theObj->target];
      target.velo = 0;
      ObjectFollow(theObj, &target, input);
      input->brake = 1;
      input->throttle = 0;
    }
  }
}

void ObjectCrossRoad(tObject *theObj, tInputData *input) {
  tTrackInfoSeg target;
  tRoad roadData = gRoadData + (int)(theObj->pos.y / 2);
  target.y = theObj->pos.y;
  target.flags = theObj->type->flags & kObjectOvertake ? kTargetNoStop : 0;
  target.velo = INFINITY;
  if ((*roadData)[3] + 200 < theObj->pos.x)
    target.x = (*roadData)[0];
  else if ((*roadData)[0] - 200 > theObj->pos.x)
    target.x = (*roadData)[3];
  else if (theObj->dir > PI)
    target.x = (*roadData)[0];
  else
    target.x = (*roadData)[3];
  ObjectFollow(theObj, &target, input);
}

void ObjectControl(tObject *theObj, tInputData *input) {
  CheckTarget(theObj);
  if (theObj == gPlayerObj) {
    theObj->input = *input;
    if (gFinishDelay) {
      theObj->input.handbrake = 1;
      theObj->input.throttle = 0;
    }
  }
  else if (!((*theObj->type).flags2 & kObjectDamageble) ||
           theObj->damage < (*theObj->type).maxDamage)
    switch (theObj->control) {
    case kObjectNoInput:
      theObj->input.brake = 0;
      theObj->input.throttle = 0;
      theObj->input.steering = 0;
      break;
    case kObjectDriveUp:
      ObjectFollow(theObj, &gTrackUp->track[theObj->target], &theObj->input);
      break;
    case kObjectDriveDown:
      ObjectFollow(theObj, &gTrackDown->track[theObj->target], &theObj->input);
      break;
    case kObjectCrossRoad:
      ObjectCrossRoad(theObj, &theObj->input);
      break;
    case kObjectCopControl:
      CopFollow(theObj, &theObj->input);
      break;
    }
  else {
    theObj->input.brake = 0;
    theObj->input.throttle = 0;
    theObj->input.steering = 0;
  }
}

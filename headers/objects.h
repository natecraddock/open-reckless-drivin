#ifndef __OBJECTS
#define __OBJECTS

#include "input.h"
#include "roads.h"

enum {
  kObjectNoInput,
  kObjectDriveUp,
  kObjectDriveDown,
  kObjectCrossRoad,
  kObjectCopControl
};

enum {
  kObjectWheelFlag = 1 << 0,
  kObjectSolidFrictionFlag = 1 << 1,
  kObjectBackCollFlag = 1 << 2,
  kObjectRandomFrameFlag = 1 << 3,
  kObjectDieWhenAnimEndsFlag = 1 << 4,
  kObjectDefaultDeath = 1 << 5,
  kObjectFollowMarks = 1 << 6,
  kObjectOvertake = 1 << 7,
  kObjectSlow = 1 << 8,
  kObjectLong = 1 << 9,
  kObjectKilledByCars = 1 << 10,
  kObjectKillsCars = 1 << 11,
  kObjectBounce = 1 << 12,
  kObjectCop = 1 << 13,
  kObjectHeliFlag = 1 << 14,
  kObjectBonusFlag = 1 << 15,
  kObjectAddOnFlag = 1 << 0,
  kObjectFrontCollFlag = 1 << 1,
  kObjectOil = 1 << 2,
  kObjectMissile = 1 << 3,
  kObjectRoadKill = 1 << 4,
  kObjectLayerFlag1 = 1 << 5,
  kObjectLayerFlag2 = 1 << 6,
  kObjectEngineSound = 1 << 7,
  kObjectRamp = 1 << 8,
  kObjectSink = 1 << 9,
  kObjectDamageble = 1 << 10,
  kObjectDieWhenOutOfScreen = 1 << 11,
  kObjectRearDrive = 1 << 12,
  kObjectRearSteer = 1 << 13,
  kObjectFloating = 1 << 14,
  kObjectBump = 1 << 15
};

enum {
  kAddOnLock = 1 << 3,
  kAddOnCop = 1 << 4,
  kAddOnTurbo = 1 << 5,
  kAddOnSpikes = 1 << 6
};

enum {
  kFrontBumper = 1 << 0,
  kBackBumper = 1 << 1,
  kFrontLeftTire = 1 << 2,
  kFrontRightTire = 1 << 3,
  kBackLeftTire = 1 << 4,
  kBackRightTire = 1 << 5,
  kLeftDoor = 1 << 6,
  kRightDoor = 1 << 7,
  kMotor = 1 << 8
};

typedef struct {
  float mass;
  float maxEngineForce, maxNegEngineForce;
  float friction;
  UInt16 flags;
  SInt16 deathObj;
  SInt16 frame;
  UInt16 numFrames;
  float frameDuration;
  float wheelWidth, wheelLength;
  float steering;
  float width, length;
  UInt16 score;
  UInt16 flags2;
  SInt16 creationSound, otherSound;
  float maxDamage;
  SInt16 weaponObj;
  SInt16 weaponInfo;
} tObjectType;
typedef tObjectType *tObjectTypePtr;

typedef struct {
  void *next, *prev;
  t2DPoint pos;
  t2DPoint velo;
  float dir;
  float rotVelo;
  float slide;
  float throttle, steering;
  float frameDuration;
  float jumpVelo, jumpHeight;
  float damage;
  int target;
  int control;
  int frame;
  int frameRepeation;
  int layer;
  int damageFlags;
  tObjectTypePtr type;
  void *shooter;
  tInputData input;
} tObject;

enum { kGroundLayer = 0, kCarLayer, kTreeLayer, kFlyLayer, kNumLayers };

#define kMaxTracks 4096
#define kCalcFPS 60.0
#define kFrameDuration (1 / kCalcFPS)
#define kLowCalcRatio 4
#define kLowFrameDuration (kLowCalcRatio / kCalcFPS)
#define kLowCalcFPS (kCalcFPS / kLowCalcRatio)
#define kScale 9.0 // pixels per meter
#define kMinCarDist                                                            \
  25 // minum distance cars try to keep from each other in meters
#define kVisDist                                                               \
  1500 // distance from player in which environment is accuratly simulated in
       // pixels

#define kNormalPlayerCarID 128
#define kNormalPlayerBoatID 201

extern tTrackSeg gTracks[kMaxTracks];
extern int gTrackCount;
extern tObject *gFirstObj, *gCameraObj, *gPlayerObj, *gSpikeObj, *gBrakeObj,
    *gFirstVisObj, *gLastVisObj;
extern int gPlayerLives, gExtraLives;
extern float gPlayerDeathDelay, gFinishDelay;
extern int gPlayerScore, gDisplayScore;
extern int gPlayerBonus;
extern UInt32 gPlayerAddOns;
extern float gGameTime;
extern tObject *gHeliObj;
extern unsigned long gFrameCount, gGraphFrameCount;
extern float gPlayerSlide[4];
extern int gNumMissiles, gNumMines;
extern float gXDriftPos, gYDriftPos, gXFrontDriftPos, gYFrontDriftPos,
    gZoomVelo;
extern float gSpikeFrame;

void MoveObjects();
tObject *NewObject(tObject *, SInt16);
void RemoveObject(tObject *);
void InsertObjectGroup(tObjectGroupReference);
void KillObject(tObject *);
int CalcBackCollision(t2DPoint);
void SortObjects();
void FireWeapon(tObject *shooter, int weaponID);

#endif

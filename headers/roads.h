#ifndef __ROADS
#define __ROADS

#include "vec2d.h"

enum { kTargetOvertake = 1, kTargetNoStop };

typedef SInt16 tRoadSeg[4];

typedef tRoadSeg *tRoad;

extern tRoad gRoadData;
extern UInt32 *gRoadLenght;

typedef struct {
  float friction, airResistance;
  float backResistance; // obsolete
  UInt16 tolerance;
  SInt16 marks, deathOffs;
  SInt16 backgroundTex, foregroundTex;
  SInt16 roadLeftBorder, roadRightBorder;
  SInt16 tracks;
  SInt16 skidSound;
  SInt16 filler;
  float xDrift, yDrift;
  float xFrontDrift, yFrontDrift;
  float trackSlide, dustSlide;
  UInt8 dustColor;
  UInt8 water;
  UInt16 filler2;
  float slideFriction;
} tRoadInfo;

extern tRoadInfo *gRoadInfo;

typedef struct {
  SInt16 typeRes, minOffs, maxOffs, probility;
  float dir;
} tObjectGroupEntry;

typedef struct {
  UInt32 numEntries;
  tObjectGroupEntry data[1];
} tObjectGroup;

typedef struct {
  SInt16 resID, numObjs;
} tObjectGroupReference;

typedef struct {
  SInt16 roadInfo;
  UInt16 time;
  tObjectGroupReference objGrps[10];
  SInt16 xStartPos;
  UInt16 levelEnd;
} tLevelData;

extern tLevelData *gLevelData;

typedef struct {
  UInt16 flags;
  SInt16 x;
  SInt32 y;
  float velo;
} tTrackInfoSeg;

typedef struct {
  UInt32 num;
  tTrackInfoSeg track[1];
} tTrackInfo;

extern tTrackInfo *gTrackUp, *gTrackDown;

typedef struct {
  SInt32 x, y;
  float dir;
  SInt16 typeRes;
  SInt16 filler;
} tObjectPos;

typedef struct {
  t2DPoint p1, p2;
  float intensity;
  UInt32 time;
} tTrackSeg;

typedef struct {
  t2DPoint p1, p2;
} tMarkSeg;

extern tMarkSeg *gMarks;
extern int gMarkSize;
#endif

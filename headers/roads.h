#ifndef __ROADS
#define __ROADS

#include <stdint.h>

#include "vec2d.h"

enum { kTargetOvertake = 1, kTargetNoStop };

typedef int16_t tRoadSeg[4];

typedef tRoadSeg *tRoad;

extern tRoad gRoadData;
extern uint32_t *gRoadLength;

typedef struct {
  float friction, airResistance;
  float backResistance; // obsolete
  uint16_t tolerance;
  int16_t marks, deathOffs;
  int16_t backgroundTex, foregroundTex;
  int16_t roadLeftBorder, roadRightBorder;
  int16_t tracks;
  int16_t skidSound;
  int16_t filler;
  float xDrift, yDrift;
  float xFrontDrift, yFrontDrift;
  float trackSlide, dustSlide;
  uint8_t dustColor;
  uint8_t water;
  uint16_t filler2;
  float slideFriction;
} tRoadInfo;

extern tRoadInfo *gRoadInfo;

typedef struct {
  int16_t typeRes, minOffs, maxOffs, probility;
  float dir;
} tObjectGroupEntry;

typedef struct {
  uint32_t numEntries;
  tObjectGroupEntry data[1];
} tObjectGroup;

typedef struct {
  int16_t resID, numObjs;
} tObjectGroupReference;

typedef struct {
  int16_t roadInfo;
  uint16_t time;
  tObjectGroupReference objGrps[10];
  int16_t xStartPos;
  uint16_t levelEnd;
} tLevelData;

extern tLevelData *gLevelData;

typedef struct {
  uint16_t flags;
  int16_t x;
  int32_t y;
  float velo;
} tTrackInfoSeg;

typedef struct {
  uint32_t num;
  tTrackInfoSeg track[1];
} tTrackInfo;

extern tTrackInfo *gTrackUp, *gTrackDown;

typedef struct {
  int32_t x, y;
  float dir;
  int16_t typeRes;
  int16_t filler;
} tObjectPos;

typedef struct {
  t2DPoint p1, p2;
  float intensity;
  uint32_t time;
} tTrackSeg;

typedef struct {
  t2DPoint p1, p2;
} tMarkSeg;

extern tMarkSeg *gMarks;
extern int gMarkSize;
#endif

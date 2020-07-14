#include <stdint.h>

#include "error.h"
#include "objects.h"
#include "packs.h"
#include "preferences.h"
#include "roads.h"
#include "screen.h"

/*
Ptr DrawLine(Ptr drawPos,int xDrawStart,int x1,int x2,int y,int xDrift,int
yDrift,Ptr data)
{
        int u,v;
        if(x1<xDrawStart)x1=xDrawStart;
        if(x2>xDrawStart+gXSize)x2=xDrawStart+gXSize;
        if(x2<x1) return drawPos;
        v=((-y+yDrift&0x007f)<<7);
        u=x1+xDrift&0x007f;
        x2-=x1;
        data+=v;
        while(u&0x0003)
                if(x2)
                {
                        *(drawPos++)=data[u++&0x007f];
                        x2--;
                }
                else return drawPos;
        while(x2>=sizeof(long))
        {
                *((long*)drawPos)=*(long*)(data+(u&0x007f));
                drawPos+=sizeof(long);
                u+=sizeof(long);
                x2-=sizeof(long);
        }
        if(x2>=sizeof(short))
        {
                *((short*)drawPos)=*(short*)(data+(u&0x007f));
                drawPos+=sizeof(short);
                u+=sizeof(short);
                x2-=sizeof(short);
        }
        if(x2)
                *(drawPos++)=data[(u&0x007f)];
        return drawPos;
}

inline Ptr DrawBorder(Ptr drawPos,int xDrawStart,int x1,int x2,int y,Ptr data)
{
        int v=(-y&0x007f)<<4;
        int u=0;
        if(x2<xDrawStart) return drawPos;
        if(x1<xDrawStart){
                u=xDrawStart-x1;
                x1=xDrawStart;
        }
        if(x2>xDrawStart+gXSize)x2=xDrawStart+gXSize;
        if(x2<x1) return drawPos;
        data+=v;
        x2-=x1;
        while(x2>=sizeof(long))
        {
                *((long*)drawPos)=*(long*)(data+u);
                drawPos+=sizeof(long);
                u+=sizeof(long);
                x2-=sizeof(long);
        }
        if(x2>=sizeof(short))
        {
                *((short*)drawPos)=*(short*)(data+u);
                drawPos+=sizeof(short);
                u+=sizeof(short);
                x2-=sizeof(short);
        }
        if(x2)
                *(drawPos++)=data[u];
        return drawPos;
}

Ptr DrawBorderLine(Ptr drawPos,int xDrawStart,int x1,int x2,int y,Ptr data,Ptr
leftBorder,Ptr rightBorder)
{
        int leftBordEnd=x1+16;
        int rightBordEnd=x2-16;
        if(leftBordEnd>rightBordEnd)
        {
                leftBordEnd=x1+((x2-x1)>>1);
                rightBordEnd=leftBordEnd;
        }
        drawPos=DrawBorder(drawPos,xDrawStart,x1,leftBordEnd,y,leftBorder);
        drawPos=DrawLine(drawPos,xDrawStart,leftBordEnd,rightBordEnd,y,gXDriftPos,gYDriftPos,data);
        return DrawBorder(drawPos,xDrawStart,rightBordEnd,x2,y,rightBorder);
}

void DrawRoad(int xDrawStart,int yDrawStart)
{
        int y;
        int rowBytesSkip=gRowBytes-gXSize;
        Ptr drawPos=gBaseAddr;
        Ptr
backgrTex=GetUnsortedPackEntry(kPackTxtR,(*gRoadInfo).backgroundTex); Ptr
roadTex=GetUnsortedPackEntry(kPackTxtR,(*gRoadInfo).foregroundTex); Ptr
leftBorder=GetUnsortedPackEntry(kPackTxtR,(*gRoadInfo).roadLeftBorder); Ptr
rightBorder=GetUnsortedPackEntry(kPackTxtR,(*gRoadInfo).roadRightBorder); tRoad
roadData=gRoadData+yDrawStart/2; if(gPrefs.lineSkip) if(!gOddLines)
for(y=yDrawStart;y>yDrawStart-gYSize+(gFinishDelay?0:kInvLines);y--)
                {
                        drawPos=DrawBorderLine(drawPos,xDrawStart,0x80000000,(*roadData)[0],y,backgrTex,leftBorder,rightBorder);
                        drawPos=DrawLine(drawPos,xDrawStart,(*roadData)[0],(*roadData)[1],y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                        drawPos=DrawBorderLine(drawPos,xDrawStart,(*roadData)[1],(*roadData)[2],y,backgrTex,leftBorder,rightBorder);
                        drawPos=DrawLine(drawPos,xDrawStart,(*roadData)[2],(*roadData)[3],y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                        drawPos=DrawBorderLine(drawPos,xDrawStart,(*roadData)[3],0x7fffffff,y,backgrTex,leftBorder,rightBorder);
                        y--;
                        drawPos+=gRowBytes+rowBytesSkip;
                        roadData--;
                }
                else
for(y=yDrawStart;y>yDrawStart-gYSize+(gFinishDelay?0:kInvLines);y--)
                {
                        drawPos+=gRowBytes+rowBytesSkip;
                        y--;
                        drawPos=DrawBorderLine(drawPos,xDrawStart,0x80000000,((*roadData)[0]+(*(roadData-1))[0])>>1,y,backgrTex,leftBorder,rightBorder);
                        drawPos=DrawLine(drawPos,xDrawStart,((*roadData)[0]+(*(roadData-1))[0])>>1,((*roadData)[1]+(*(roadData-1))[1])>>1,y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                        drawPos=DrawBorderLine(drawPos,xDrawStart,((*roadData)[1]+(*(roadData-1))[1])>>1,((*roadData)[2]+(*(roadData-1))[2])>>1,y,backgrTex,leftBorder,rightBorder);
                        drawPos=DrawLine(drawPos,xDrawStart,((*roadData)[2]+(*(roadData-1))[2])>>1,((*roadData)[3]+(*(roadData-1))[3])>>1,y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                        drawPos=DrawBorderLine(drawPos,xDrawStart,((*roadData)[3]+(*(roadData-1))[3])>>1,0x7fffffff,y,backgrTex,leftBorder,rightBorder);
                        roadData--;
                }
        else
for(y=yDrawStart;y>yDrawStart-gYSize+(gFinishDelay?0:kInvLines);y--)
        {
                drawPos=DrawBorderLine(drawPos,xDrawStart,0x80000000,(*roadData)[0],y,backgrTex,leftBorder,rightBorder);
                drawPos=DrawLine(drawPos,xDrawStart,(*roadData)[0],(*roadData)[1],y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                drawPos=DrawBorderLine(drawPos,xDrawStart,(*roadData)[1],(*roadData)[2],y,backgrTex,leftBorder,rightBorder);
                drawPos=DrawLine(drawPos,xDrawStart,(*roadData)[2],(*roadData)[3],y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                drawPos=DrawBorderLine(drawPos,xDrawStart,(*roadData)[3],0x7fffffff,y,backgrTex,leftBorder,rightBorder);
                drawPos+=rowBytesSkip;
                y--;
                drawPos=DrawBorderLine(drawPos,xDrawStart,0x80000000,((*roadData)[0]+(*(roadData-1))[0])>>1,y,backgrTex,leftBorder,rightBorder);
                drawPos=DrawLine(drawPos,xDrawStart,((*roadData)[0]+(*(roadData-1))[0])>>1,((*roadData)[1]+(*(roadData-1))[1])>>1,y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                drawPos=DrawBorderLine(drawPos,xDrawStart,((*roadData)[1]+(*(roadData-1))[1])>>1,((*roadData)[2]+(*(roadData-1))[2])>>1,y,backgrTex,leftBorder,rightBorder);
                drawPos=DrawLine(drawPos,xDrawStart,((*roadData)[2]+(*(roadData-1))[2])>>1,((*roadData)[3]+(*(roadData-1))[3])>>1,y,gXFrontDriftPos,gYFrontDriftPos,roadTex);
                drawPos=DrawBorderLine(drawPos,xDrawStart,((*roadData)[3]+(*(roadData-1))[3])>>1,0x7fffffff,y,backgrTex,leftBorder,rightBorder);
                drawPos+=rowBytesSkip;
                roadData--;
        }
}
*/

Ptr DrawLineZoomed(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                   int xDrift, int yDrift, Ptr data, float zoom) {
  uint32_t u, v;
  uint32_t dudx;
  if (x1 < 0)
    x1 = 0;
  if (x2 > gXSize)
    x2 = gXSize;
  if (x2 < x1)
    return drawPos;
  v = (-y + yDrift & 0x007f) << 7;
  u = ((int)(x1 * zoom + xDrawStart + xDrift) & 0x007f) << 8;
  x2 -= x1;
  data += v;
  dudx = (int)(zoom * 256);
  while (x2) {
    *(drawPos++) = data[((u >> 8) & 0x007f)];
    u += dudx;
    x2--;
  }
  return drawPos;
}

Ptr DrawBorderZoomed(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                     Ptr data, float zoom) {
  uint32_t dudx;
  uint32_t v = (-y & 0x007f) << 4;
  uint32_t u = 0;
  if (x2 < 0)
    return drawPos;
  if (x1 < 0) {
    u = -x1 << 8;
    x1 = 0;
  }
  if (x2 > gXSize)
    x2 = gXSize;
  if (x2 < x1)
    return drawPos;
  x2 -= x1;
  data += v;
  dudx = (int)(zoom * 256);
  while (x2) {
    *(drawPos++) = data[u >> 8];
    u += dudx;
    x2--;
  }
  return drawPos;
}

Ptr DrawBorderLineZoomed(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                         Ptr data, Ptr leftBorder, Ptr rightBorder,
                         float zoom) {
  int leftBordEnd = x1 + 16 / zoom;
  int rightBordEnd = x2 - 16 / zoom;
  if (leftBordEnd > rightBordEnd) {
    leftBordEnd = x1 + ((x2 - x1) >> 1);
    rightBordEnd = leftBordEnd;
  }
  drawPos = DrawBorderZoomed(drawPos, xDrawStart, x1, leftBordEnd, y,
                             leftBorder, zoom);
  drawPos = DrawLineZoomed(drawPos, xDrawStart, leftBordEnd, rightBordEnd, y,
                           gXDriftPos, gYDriftPos, data, zoom);
  return DrawBorderZoomed(drawPos, xDrawStart, rightBordEnd, x2, y, rightBorder,
                          zoom);
}

float InZeroMax(float x, float max) {
  if (x < 0)
    return 0;
  else if (x > max)
    return max;
  else
    return x;
}

void DrawRoadZoomed(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int screenY;
  int rowBytesSkip = gRowBytes - gXSize;
  Ptr drawPos = gBaseAddr;
  Ptr backgrTex =
      GetUnsortedPackEntry(kPackTxtR, (*gRoadInfo).backgroundTex, 0);
  Ptr roadTex = GetUnsortedPackEntry(kPackTxtR, (*gRoadInfo).foregroundTex, 0);
  Ptr leftBorder =
      GetUnsortedPackEntry(kPackTxtR, (*gRoadInfo).roadLeftBorder, 0);
  Ptr rightBorder =
      GetUnsortedPackEntry(kPackTxtR, (*gRoadInfo).roadRightBorder, 0);
  if (gPrefs.lineSkip)
    rowBytesSkip += gRowBytes;
  for (screenY = 0; screenY < gYSize; screenY += (gPrefs.lineSkip ? 2 : 1)) {
    float worldY = InZeroMax(yDrawStart - screenY * zoom, *gRoadLength * 2);
    float ceilRoadLine = ceil(worldY * 0.5);
    float floorRoadLine = floor(worldY * 0.5);
    float floorPerc = ceilRoadLine - worldY * 0.5;
    tRoadSeg roadData;
    tRoad ceilRoad = (gRoadData + (int)(ceilRoadLine));
    tRoad floorRoad = (gRoadData + (int)(floorRoadLine));
    int ceilSplit = (*ceilRoad)[1] != (*ceilRoad)[2];
    int floorSplit = (*floorRoad)[1] != (*floorRoad)[2];
    if (ceilSplit != floorSplit)
      if (ceilSplit)
        floorPerc = 1;
      else
        floorPerc = 0;
    else if (ceilSplit && floorSplit)
      if ((*ceilRoad)[3] < (*floorRoad)[2] || (*ceilRoad)[2] > (*floorRoad)[3])
        floorPerc = 1;
    roadData[0] =
        ((floorPerc * (*floorRoad)[0] + (1 - floorPerc) * (*ceilRoad)[0]) -
         xDrawStart) *
        invZoom;
    roadData[1] =
        ((floorPerc * (*floorRoad)[1] + (1 - floorPerc) * (*ceilRoad)[1]) -
         xDrawStart) *
        invZoom;
    roadData[2] =
        ((floorPerc * (*floorRoad)[2] + (1 - floorPerc) * (*ceilRoad)[2]) -
         xDrawStart) *
        invZoom;
    roadData[3] =
        ((floorPerc * (*floorRoad)[3] + (1 - floorPerc) * (*ceilRoad)[3]) -
         xDrawStart) *
        invZoom;
    drawPos =
        DrawBorderLineZoomed(drawPos, xDrawStart, 0x80000000, roadData[0],
                             worldY, backgrTex, leftBorder, rightBorder, zoom);
    drawPos =
        DrawLineZoomed(drawPos, xDrawStart, roadData[0], roadData[1], worldY,
                       gXFrontDriftPos, gYFrontDriftPos, roadTex, zoom);
    drawPos =
        DrawBorderLineZoomed(drawPos, xDrawStart, roadData[1], roadData[2],
                             worldY, backgrTex, leftBorder, rightBorder, zoom);
    drawPos =
        DrawLineZoomed(drawPos, xDrawStart, roadData[2], roadData[3], worldY,
                       gXFrontDriftPos, gYFrontDriftPos, roadTex, zoom);
    drawPos =
        DrawBorderLineZoomed(drawPos, xDrawStart, roadData[3], 0x7fffffff,
                             worldY, backgrTex, leftBorder, rightBorder, zoom);
    drawPos += rowBytesSkip;
  }
  if (drawPos != gBaseAddr + 640 * (gPrefs.lineSkip ? 240 : 480) +
                     (gPrefs.lineSkip ? 240 : 480) * rowBytesSkip)
    DoError(paramErr);
}

Ptr DrawLineZoomed16(uint16_t *drawPos, int xDrawStart, int x1, int x2, int y,
                     int xDrift, int yDrift, uint16_t *data, float zoom) {
  uint32_t u, v;
  uint32_t dudx;
  if (x1 < 0)
    x1 = 0;
  if (x2 > gXSize)
    x2 = gXSize;
  if (x2 < x1)
    return drawPos;
  v = (-y + yDrift & 0x007f) << 7;
  u = ((int)(x1 * zoom + xDrawStart + xDrift) & 0x007f) << 8;
  x2 -= x1;
  data += v;
  dudx = (int)(zoom * 256);
  while (x2) {
    *(drawPos++) = data[((u >> 8) & 0x007f)];
    u += dudx;
    x2--;
  }
  return drawPos;
}

Ptr DrawBorderZoomed16(uint16_t *drawPos, int xDrawStart, int x1, int x2, int y,
                       uint16_t *data, float zoom) {
  uint32_t dudx;
  uint32_t v = (-y & 0x007f) << 4;
  uint32_t u = 0;
  if (x2 < 0)
    return drawPos;
  if (x1 < 0) {
    u = -x1 * 256 * zoom;
    x1 = 0;
  }
  if (x2 > gXSize)
    x2 = gXSize;
  if (x2 < x1)
    return drawPos;
  x2 -= x1;
  data += v;
  dudx = (int)(zoom * 256);
  while (x2) {
    *(drawPos++) = data[u >> 8];
    u += dudx;
    x2--;
  }
  return drawPos;
}

Ptr DrawBorderLineZoomed16(Ptr drawPos, int xDrawStart, int x1, int x2, int y,
                           Ptr data, Ptr leftBorder, Ptr rightBorder,
                           float zoom) {
  int leftBordEnd = x1 + 16 / zoom;
  int rightBordEnd = x2 - 16 / zoom;
  if (leftBordEnd > rightBordEnd) {
    leftBordEnd = x1 + ((x2 - x1) >> 1);
    rightBordEnd = leftBordEnd;
  }
  drawPos = DrawBorderZoomed16(drawPos, xDrawStart, x1, leftBordEnd, y,
                               leftBorder, zoom);
  drawPos = DrawLineZoomed16(drawPos, xDrawStart, leftBordEnd, rightBordEnd, y,
                             gXDriftPos, gYDriftPos, data, zoom);
  return DrawBorderZoomed16(drawPos, xDrawStart, rightBordEnd, x2, y,
                            rightBorder, zoom);
}

void DrawRoadZoomed16(float xDrawStart, float yDrawStart, float zoom) {
  float invZoom = 1 / zoom;
  int screenY;
  int rowBytesSkip = gRowBytes - gXSize * 2;
  Ptr drawPos = gBaseAddr;
  Ptr backgrTex =
      GetUnsortedPackEntry(kPackTx16, (*gRoadInfo).backgroundTex, 0);
  Ptr roadTex = GetUnsortedPackEntry(kPackTx16, (*gRoadInfo).foregroundTex, 0);
  Ptr leftBorder =
      GetUnsortedPackEntry(kPackTx16, (*gRoadInfo).roadLeftBorder, 0);
  Ptr rightBorder =
      GetUnsortedPackEntry(kPackTx16, (*gRoadInfo).roadRightBorder, 0);
  if (gPrefs.lineSkip)
    rowBytesSkip += gRowBytes;
  for (screenY = 0; screenY < gYSize; screenY += (gPrefs.lineSkip ? 2 : 1)) {
    float worldY = InZeroMax(yDrawStart - screenY * zoom, *gRoadLength * 2);
    float ceilRoadLine = ceil(worldY * 0.5);
    float floorRoadLine = floor(worldY * 0.5);
    float floorPerc = ceilRoadLine - worldY * 0.5;
    tRoadSeg roadData;
    tRoad ceilRoad = (gRoadData + (int)(ceilRoadLine));
    tRoad floorRoad = (gRoadData + (int)(floorRoadLine));
    int ceilSplit = (*ceilRoad)[1] != (*ceilRoad)[2];
    int floorSplit = (*floorRoad)[1] != (*floorRoad)[2];
    if (ceilSplit != floorSplit)
      if (ceilSplit)
        floorPerc = 1;
      else
        floorPerc = 0;
    else if (ceilSplit && floorSplit)
      if ((*ceilRoad)[3] < (*floorRoad)[2] || (*ceilRoad)[2] > (*floorRoad)[3])
        floorPerc = 1;
    roadData[0] =
        ((floorPerc * (*floorRoad)[0] + (1 - floorPerc) * (*ceilRoad)[0]) -
         xDrawStart) *
        invZoom;
    roadData[1] =
        ((floorPerc * (*floorRoad)[1] + (1 - floorPerc) * (*ceilRoad)[1]) -
         xDrawStart) *
        invZoom;
    roadData[2] =
        ((floorPerc * (*floorRoad)[2] + (1 - floorPerc) * (*ceilRoad)[2]) -
         xDrawStart) *
        invZoom;
    roadData[3] =
        ((floorPerc * (*floorRoad)[3] + (1 - floorPerc) * (*ceilRoad)[3]) -
         xDrawStart) *
        invZoom;
    drawPos = DrawBorderLineZoomed16(drawPos, xDrawStart, 0x80000000,
                                     roadData[0], worldY, backgrTex, leftBorder,
                                     rightBorder, zoom);
    drawPos =
        DrawLineZoomed16(drawPos, xDrawStart, roadData[0], roadData[1], worldY,
                         gXFrontDriftPos, gYFrontDriftPos, roadTex, zoom);
    drawPos = DrawBorderLineZoomed16(drawPos, xDrawStart, roadData[1],
                                     roadData[2], worldY, backgrTex, leftBorder,
                                     rightBorder, zoom);
    drawPos =
        DrawLineZoomed16(drawPos, xDrawStart, roadData[2], roadData[3], worldY,
                         gXFrontDriftPos, gYFrontDriftPos, roadTex, zoom);
    drawPos = DrawBorderLineZoomed16(drawPos, xDrawStart, roadData[3],
                                     0x7fffffff, worldY, backgrTex, leftBorder,
                                     rightBorder, zoom);
    drawPos += rowBytesSkip;
  }
  if (drawPos != gBaseAddr + 640 * (gPrefs.lineSkip ? 240 : 480) * 2 +
                     (gPrefs.lineSkip ? 240 : 480) * rowBytesSkip)
    DoError(paramErr);
}

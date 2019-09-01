#ifndef __GAMESOUND
#define __GAMESOUND

#include "vec2d.h"

void LoadSounds();
void InitChannels();
void PlaySound(t2DPoint,t2DPoint,float,float,int);
void SimplePlaySound(int);
void SetCarSound(float,float,float,float);
void StartCarChannels();
void SetGameVolume(int);
void SetSystemVolume();
void BeQuiet();

#endif
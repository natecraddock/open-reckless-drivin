#ifndef __INPUT
#define __INPUT

typedef struct {
  float steering, throttle, brake, handbrake;
  int reverse, kickdown;
} tInputData;

enum { kInputSuspended, kInputRunning, kInputStopped };

enum {
  kForward,
  kBackward,
  kLeft,
  kRight,
  kKickdown,
  kBrake,
  kFire,
  kMissile,
  kAbort,
  kPause,
  kNumElements
};

extern int gFire, gMissile;

void InputMode(int);
void InitInput();
void Input(tInputData **);
void ConfigureInput();
UInt64 GetMSTime();
void FlushInput();
void FFBJolt(float lMag, float rMag, float duration);
void FFBDirect(float lMag, float rMag);
int ContinuePress();

#endif

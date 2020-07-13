#include <stdbool.h>
#include <stdint.h>

#include "error.h"
#include "objects.h"
#include "packs.h"
#include "preferences.h"
#include "random.h"
#include "roads.h"
#include "vec2d.h"
#include <math.h>
#include <sound.h>

#define kNumChannels 3   // number of sound channels
#define kNumHQChannels 6 // number of sound channels in high quality mode
#define kMaxPanDist                                                            \
  400.0 // distance for maximum stereo panning (complete left or complete right)
#define kMaxListenDist 1250.0 // maximum distance from whihc sounds can be heard
#define kDopplerFactor                                                         \
  0.004 // determinates strenght of doppler effect simulation: 0 => no doppler
        // effect
#define kMaxNoiseVelo 65.0 // velocity at which engine makes maximum noise
#define kMinSqueakSlide 0.5
#define kSqueakFactor 0.5
#define kNumGears 4       // number of gears
#define kHighestGear 55.0 // velocitiy at which highest gear is set (in m/s)
#define kHighestGearTurbo                                                      \
  70.0 // velocitiy at which highest gear is set with Turbo Engine(in m/s)
#define kShiftTolerance 2.5

enum { kSoundPriorityHigher = 1 << 0 };

typedef struct {
  uint32_t numSamples;
  uint32_t priority;
  uint32_t flags;
  uint32_t offsets[1];
} tSound;

SndChannelPtr gChannels[kNumHQChannels];
SndChannelPtr gEngineChannel, gSkidChannel;
int gSystemVolumeActive = true, gChannelsInited = 0;
float gVolume;
long gSystemRate = 0;
int gGear;
int gBuggySoundManager;
UniversalProcPtr gSoundCallBack = nil;

uint32_t U32Version(NumVersion v);

void SetGameVolume(int volume) {
  ComponentDescription theDesc = {kSoundOutputDeviceType, 0, 0, 0, 0};
  Component theDefaultSoundOutputDevice = FindNextComponent(nil, &theDesc);
  if (theDefaultSoundOutputDevice) {
    SoundInfoList rates;
    UnsignedFixed searchRate = gPrefs.hqSound ? rate44khz : rate22050hz;
    int i, found = false;
    if (gSystemVolumeActive) {
      gSystemVolumeActive = false;
      DoError(GetSoundOutputInfo(theDefaultSoundOutputDevice, siSampleRate,
                                 &gSystemRate));
    }
    DoError(GetSoundOutputInfo(theDefaultSoundOutputDevice,
                               siSampleRateAvailable, &rates));
    for (i = 0; i < rates.count; i++)
      if (((UnsignedFixed *)*rates.infoHandle)[i] == searchRate)
        found = true;
    if (found)
      DoError(SetSoundOutputInfo(theDefaultSoundOutputDevice, siSampleRate,
                                 (void *)searchRate));
    DisposeHandle(rates.infoHandle);
  }
  if (volume == -1)
    gVolume = gPrefs.volume / 256.0;
  else
    gVolume = volume / 256.0;
}

void SetSystemVolume() {
  ComponentDescription theDesc = {kSoundOutputDeviceType, 0, 0, 0, 0};
  Component theDefaultSoundOutputDevice = FindNextComponent(nil, &theDesc);
  if (theDefaultSoundOutputDevice && gSystemRate) {
    gSystemVolumeActive = true;
    DoError(SetSoundOutputInfo(theDefaultSoundOutputDevice, siSampleRate,
                               (void *)gSystemRate));
  }
}

pascal void SndCallBackProc(SndChannelPtr chan, SndCommand *inCmd) {
  if (inCmd->param1)
    chan->userInfo = 1;
  else
    chan->userInfo = 0;
}

void BeQuiet() {
  SndCommand cmd;
  int i;
  cmd.param1 = 0;
  cmd.param2 = 0;
  cmd.cmd = flushCmd;
  for (i = 0; i < (gPrefs.hqSound ? kNumHQChannels : kNumChannels); i++)
    DoError(SndDoImmediate(gChannels[i], &cmd));
  DoError(SndDoImmediate(gEngineChannel, &cmd));
  DoError(SndDoImmediate(gSkidChannel, &cmd));
  cmd.cmd = quietCmd;
  for (i = 0; i < (gPrefs.hqSound ? kNumHQChannels : kNumChannels); i++)
    DoError(SndDoImmediate(gChannels[i], &cmd));
  DoError(SndDoImmediate(gEngineChannel, &cmd));
  DoError(SndDoImmediate(gSkidChannel, &cmd));
  for (i = 0; i < (gPrefs.hqSound ? kNumHQChannels : kNumChannels); i++)
    gChannels[i]->userInfo = 0;
}

void InitChannels() {
  int i;
  long init = initMono | (gPrefs.hqSound ? initNoDrop : initNoInterp);
  if (!gSoundCallBack)
    gSoundCallBack = NewSndCallBackUPP(&SndCallBackProc);
  if (gChannelsInited) {
    for (i = 0; i < gChannelsInited; i++)
      DoError(SndDisposeChannel(gChannels[i], true));
    DoError(SndDisposeChannel(gEngineChannel, true));
    DoError(SndDisposeChannel(gSkidChannel, true));
    gChannelsInited = 0;
  }
  for (i = 0; i < (gPrefs.hqSound ? kNumHQChannels : kNumChannels); i++) {
    gChannels[i] = nil;
    DoError(SndNewChannel(&gChannels[i], sampledSynth, init, gSoundCallBack));
    gChannels[i]->userInfo = 0;
    gChannelsInited++;
  }
  gEngineChannel = nil;
  DoError(SndNewChannel(&gEngineChannel, sampledSynth, init, gSoundCallBack));
  gEngineChannel->userInfo = 0;
  gSkidChannel = nil;
  DoError(SndNewChannel(&gSkidChannel, sampledSynth, init, gSoundCallBack));
  gSkidChannel->userInfo = 0;
  gBuggySoundManager = U32Version(SndSoundManagerVersion()) < 0x03600000 &&
                       U32Version(SndSoundManagerVersion()) >= 0x03300000;
}

#define rateCmd 82
#define getRateCmd 85

void SetCarSound(float engine, float skidL, float skidR, float velo) {
  if (gPrefs.engineSound && gPrefs.sound) {
    SndCommand cmd;
    float engineVol, gearVelo;
    if (!(*gRoadInfo).water) {
      float highestGear =
          gPlayerAddOns & kAddOnTurbo ? kHighestGearTurbo : kHighestGear;
      // switch to correct gear
      while (velo > (gGear + 1) * highestGear / kNumGears + kShiftTolerance &&
             (gGear + 1) < kNumGears)
        gGear++;
      while (velo < gGear * highestGear / kNumGears - kShiftTolerance &&
             gGear > 0)
        gGear--;
      gearVelo = fabs(velo) / highestGear * kNumGears - gGear;
      velo /= kMaxNoiseVelo;
      if (gearVelo > 2)
        gearVelo = 2;
    }
    else {
      velo /= kMaxNoiseVelo;
      gearVelo = (engine + velo) / 2;
    }
    velo = fabs(velo);
    if (engine != -1)
      engineVol =
          -gVolume / ((0.6 * engine + 0.15 * velo + 0.25 * gearVelo) - 2);
    else
      engineVol = 0;
    if (gEngineChannel->userInfo) {
      tSound *sound = (tSound *)GetSortedPackEntry(kPackSnds, 132, nil);
      gEngineChannel->userInfo = 0;
      cmd.cmd = bufferCmd;
      cmd.param2 =
          (int32_t)sound + sound->offsets[RanInt(0, sound->numSamples)];
      DoError(SndDoCommand(gEngineChannel, &cmd, false));
      cmd.cmd = callBackCmd;
      cmd.param1 = 1;
      DoError(SndDoCommand(gEngineChannel, &cmd, false));
    }
    cmd.cmd = volumeCmd;
    cmd.param1 = 0;
    cmd.param2 = ((int)(0x0048 * engineVol * gVolume) & 0xffff) |
                 ((int)(0x0048 * engineVol * gVolume) << 16);
    DoError(SndDoImmediate(gEngineChannel, &cmd));
    if (gBuggySoundManager) {
      cmd.cmd = rateCmd;
      cmd.param2 =
          0x00016000 * (0.2 + 0.3 * engine + 0.2 * velo + 0.3 * gearVelo);
      DoError(SndDoImmediate(gEngineChannel, &cmd));
    }
    else {
      cmd.cmd = rateMultiplierCmd;
      cmd.param2 =
          0x0000b000 * (0.2 + 0.3 * engine + 0.2 * velo + 0.3 * gearVelo);
      DoError(SndDoImmediate(gEngineChannel, &cmd));
    }
    skidL -= kMinSqueakSlide;
    skidL /= kSqueakFactor;
    if (skidL < 0)
      skidL = 0;
    else if (skidL > 1)
      skidL = 1;
    skidR -= kMinSqueakSlide;
    skidR /= kSqueakFactor;
    if (skidR < 0)
      skidR = 0;
    else if (skidR > 1)
      skidR = 1;
    if (gSkidChannel->userInfo) {
      tSound *sound =
          (tSound *)GetSortedPackEntry(kPackSnds, (*gRoadInfo).skidSound, nil);
      gSkidChannel->userInfo = 0;
      cmd.cmd = bufferCmd;
      cmd.param2 =
          (int32_t)sound + sound->offsets[RanInt(0, sound->numSamples)];
      DoError(SndDoCommand(gSkidChannel, &cmd, false));
      cmd.cmd = callBackCmd;
      cmd.param1 = 1;
      DoError(SndDoCommand(gSkidChannel, &cmd, false));
    }
    cmd.cmd = volumeCmd;
    cmd.param1 = 0;
    cmd.param2 =
        ((int)(0x0100 * (skidL * gVolume * (velo * 0.5 + 0.5))) & 0xffff) |
        ((int)(0x0100 * (skidR * gVolume * (velo * 0.5 + 0.5))) << 16);
    DoError(SndDoImmediate(gSkidChannel, &cmd));
    if (gBuggySoundManager) {
      cmd.cmd = rateCmd;
      if (skidL + skidR)
        cmd.param2 =
            0x00010000 * (-1 / ((skidL + skidR) * 0.25 + velo * 0.5 - 2));
      else
        cmd.param2 = 0;
      DoError(SndDoImmediate(gSkidChannel, &cmd));
    }
    else {
      cmd.cmd = rateMultiplierCmd;
      if (skidL + skidR)
        cmd.param2 =
            0x00020000 * (-1 / ((skidL + skidR) * 0.25 + velo * 0.5 - 2));
      else
        cmd.param2 = 0;
      DoError(SndDoImmediate(gSkidChannel, &cmd));
    }
  }
}

void StartCarChannels() {
  int i;
  SndCommand cmd;
  cmd.param1 = 0;
  cmd.param2 = 0;
  cmd.cmd = flushCmd;
  DoError(SndDoImmediate(gEngineChannel, &cmd));
  DoError(SndDoImmediate(gSkidChannel, &cmd));
  cmd.cmd = quietCmd;
  DoError(SndDoImmediate(gEngineChannel, &cmd));
  DoError(SndDoImmediate(gSkidChannel, &cmd));
  cmd.cmd = volumeCmd;
  DoError(SndDoCommand(gEngineChannel, &cmd, false));
  if (gPrefs.engineSound && gPrefs.sound)
    for (i = 0; i < 2; i++) {
      tSound *sound = (tSound *)GetSortedPackEntry(kPackSnds, 132, nil);
      cmd.cmd = bufferCmd;
      cmd.param1 = 0;
      cmd.param2 =
          (int32_t)sound + sound->offsets[RanInt(0, sound->numSamples)];
      DoError(SndDoCommand(gEngineChannel, &cmd, false));
      cmd.cmd = callBackCmd;
      cmd.param1 = 1;
      cmd.param2 = 0;
      DoError(SndDoCommand(gEngineChannel, &cmd, false));
      sound =
          (tSound *)GetSortedPackEntry(kPackSnds, (*gRoadInfo).skidSound, nil);
      cmd.cmd = bufferCmd;
      cmd.param1 = 0;
      cmd.param2 =
          (int32_t)sound + sound->offsets[RanInt(0, sound->numSamples)];
      DoError(SndDoCommand(gSkidChannel, &cmd, false));
      cmd.cmd = callBackCmd;
      cmd.param1 = 1;
      cmd.param2 = 0;
      DoError(SndDoCommand(gSkidChannel, &cmd, false));
    }
  SetCarSound(0, 0, 0, 0);
}

void PlaySound(t2DPoint pos, t2DPoint velo, float freq, float vol, int id) {
  if (gPrefs.sound) {
    tSound *sound = (tSound *)GetSortedPackEntry(kPackSnds, id, nil);
    SndChannelPtr chan;
    SndCommand cmd;
    int i;
    float pan, dist;
    uint32_t priority = 0xffffffff;
    for (i = 0; i < (gPrefs.hqSound ? kNumHQChannels : kNumChannels); i++)
      if (priority > gChannels[i]->userInfo) {
        priority = gChannels[i]->userInfo;
        chan = gChannels[i];
      }
    if (sound->flags & kSoundPriorityHigher) {
      if (priority >= sound->priority)
        return;
    }
    else if (priority > sound->priority)
      return;
    dist = 1 - VEC2D_Value(VEC2D_Difference(pos, gCameraObj->pos)) *
                   (1 / kMaxListenDist);
    if (dist <= 0)
      return;
    chan->userInfo = sound->priority;
    pan = (pos.x - gCameraObj->pos.x) * (1 / kMaxPanDist);
    if (fabs(pan) > 1)
      if (pan > 0)
        pan = 1;
      else
        pan = -1;
    if (gPrefs.hqSound) {
      t2DPoint veloDiff = VEC2D_Difference(velo, gCameraObj->velo);
      t2DPoint spaceDiff = VEC2D_Norm(VEC2D_Difference(gCameraObj->pos, pos));
      freq *= pow(2, VEC2D_DotProduct(veloDiff, spaceDiff) * kDopplerFactor);
    }
    vol *= gVolume;
    cmd.param1 = 0;
    cmd.param2 = 0;
    cmd.cmd = flushCmd;
    DoError(SndDoImmediate(chan, &cmd));
    cmd.cmd = quietCmd;
    DoError(SndDoImmediate(chan, &cmd));
    cmd.cmd = bufferCmd;
    cmd.param2 = (int32_t)sound + sound->offsets[RanInt(0, sound->numSamples)];
    DoError(SndDoImmediate(chan, &cmd));
    cmd.cmd = volumeCmd;
    cmd.param2 = ((int)(0x0100 * vol * dist * (1 - pan)) & 0xffff) |
                 ((int)(0x0100 * vol * dist * (1 + pan)) << 16);
    DoError(SndDoImmediate(chan, &cmd));
    if (gBuggySoundManager) {
      uint32_t rate;
      cmd.cmd = getRateCmd;
      cmd.param2 = (int32_t)&rate;
      DoError(SndDoImmediate(chan, &cmd));
      cmd.cmd = rateCmd;
      cmd.param2 = rate * freq;
      DoError(SndDoImmediate(chan, &cmd));
    }
    else {
      cmd.cmd = rateMultiplierCmd;
      cmd.param2 = 0x00010000 * freq;
      DoError(SndDoImmediate(gEngineChannel, &cmd));
    }
    cmd.cmd = callBackCmd;
    cmd.param1 = 0;
    DoError(SndDoCommand(chan, &cmd, false));
  }
}

void SimplePlaySound(int id) {
  if (gPrefs.sound) {
    tSound *sound = (tSound *)GetSortedPackEntry(kPackSnds, id, nil);
    SndChannelPtr chan;
    SndCommand cmd;
    int i;
    float pan, dist;
    uint32_t priority = 0xffffffff;
    for (i = 0; i < (gPrefs.hqSound ? kNumHQChannels : kNumChannels); i++)
      if (priority > gChannels[i]->userInfo) {
        priority = gChannels[i]->userInfo;
        chan = gChannels[i];
      }
    if (sound->flags & kSoundPriorityHigher) {
      if (priority >= sound->priority)
        return;
    }
    else if (priority > sound->priority)
      return;
    cmd.param1 = 0;
    cmd.param2 = 0;
    cmd.cmd = flushCmd;
    DoError(SndDoImmediate(chan, &cmd));
    cmd.cmd = quietCmd;
    DoError(SndDoImmediate(chan, &cmd));
    cmd.cmd = bufferCmd;
    cmd.param2 = (int32_t)sound + sound->offsets[RanInt(0, sound->numSamples)];
    DoError(SndDoImmediate(chan, &cmd));
    cmd.cmd = volumeCmd;
    cmd.param2 =
        ((int)(0x0100 * gVolume) & 0xffff) | ((int)(0x0100 * gVolume) << 16);
    DoError(SndDoImmediate(chan, &cmd));
    if (!gBuggySoundManager) {
      cmd.cmd = rateMultiplierCmd;
      cmd.param2 = 0x00010000;
      DoError(SndDoImmediate(gEngineChannel, &cmd));
    }
    cmd.cmd = callBackCmd;
    cmd.param1 = 0;
    DoError(SndDoCommand(chan, &cmd, false));
  }
}

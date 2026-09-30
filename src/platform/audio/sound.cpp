#include "platform/audio/sound.h"

/*
 * First Windows CE backend: silent audio.
 *
 * Gameplay and world logic are being ported independently from the PSP
 * audio mixer.  Keeping the public sound API alive lets the game compile
 * and run while the S730 audio backend is implemented separately.
 */

void soundInit(void) {}
void soundShutdown(void) {}
void soundPlay(const char* name, float volume, float pitch, int catOverride)
{
    (void)name;
    (void)volume;
    (void)pitch;
    (void)catOverride;
}
void soundStopAll(void) {}
void soundStopWorld(void) {}
void soundSetVolume(float volume) { (void)volume; }
void soundMusicUpdate(void) {}
void soundMusicStop(void) {}
void soundPowerResume(void) {}
const char* soundMusicCurrentTrack(void) { return 0; }
void soundSetCategoryVolume(int cat, float volume)
{
    (void)cat;
    (void)volume;
}
float soundAttenuate(float distSq, float volume)
{
    (void)distSq;
    return volume;
}

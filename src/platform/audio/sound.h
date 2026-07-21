
#ifndef MCPSP_PLATFORM_AUDIO_SOUND_H
#define MCPSP_PLATFORM_AUDIO_SOUND_H

void soundInit(void);

void soundPlay(const char* name, float volume, float pitch);

void soundSetVolume(float volume);

float soundAttenuate(float distSq, float volume);

#endif

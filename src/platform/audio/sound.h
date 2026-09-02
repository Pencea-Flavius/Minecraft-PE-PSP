
#ifndef MCPSP_PLATFORM_AUDIO_SOUND_H
#define MCPSP_PLATFORM_AUDIO_SOUND_H

void soundInit(void);

void soundShutdown(void);

void soundPlay(const char* name, float volume, float pitch, int catOverride = -1);

void soundStopAll(void);

void soundStopWorld(void);

void soundSetVolume(float volume);

enum {
    SND_CAT_MUSIC,
    SND_CAT_BLOCK,
    SND_CAT_HOSTILE,
    SND_CAT_FRIENDLY,
    SND_CAT_PLAYER,
    SND_CAT_AMBIENT,
    SND_CAT_UI,
    SND_CAT_COUNT
};

void soundMusicUpdate(void);

void soundMusicStop(void);

void soundPowerResume(void);

const char* soundMusicCurrentTrack(void);

void soundSetCategoryVolume(int cat, float volume);

float soundAttenuate(float distSq, float volume);

#endif

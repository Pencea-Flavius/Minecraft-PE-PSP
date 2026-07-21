#include "platform/audio/sound.h"

#include <pspkernel.h>
#include <pspaudio.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

#include "platform/path.h"
#include "client/player/player.h"

#define SAMPLE_RATE   44100
#define SOURCE_RATE   22050
#define SAMPLE_COUNT   1024
#define MAX_VOICES       16

#define NAME_LEN         24
#define PACK_MAGIC 0x4653434DU

struct Entry {
    char         name[NAME_LEN];
    unsigned int offset;
    unsigned int frames;
};

struct Voice {
    const short* frames;
    unsigned int frameCount;
    unsigned int pos;
    unsigned int step;
    int          vol;
    volatile int playing;
};

static Entry*       g_index;
static int          g_count;
static const short* g_pcm;
static Voice        g_voices[MAX_VOICES];
static int          g_channel = -1;
static float        g_master  = 1.0f;

static bool loadPack(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return false;

    unsigned int header[3];
    if (fread(header, sizeof(header), 1, f) != 1 || header[0] != PACK_MAGIC) {
        fclose(f);
        return false;
    }

    int count = (int)header[1];
    unsigned int pcmBytes = header[2];
    Entry* index = (Entry*)malloc(sizeof(Entry) * count);
    short* pcm   = (short*)malloc(pcmBytes);
    if (!index || !pcm ||
        fread(index, sizeof(Entry), count, f) != (size_t)count ||
        fread(pcm, 1, pcmBytes, f) != pcmBytes) {
        free(index);
        free(pcm);
        fclose(f);
        return false;
    }
    fclose(f);

    g_index = index;
    g_count = count;
    g_pcm   = pcm;
    return true;
}

static int findFirst(const char* name) {
    int lo = 0, hi = g_count - 1, hit = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int cmp = strcmp(g_index[mid].name, name);
        if (cmp < 0)      lo = mid + 1;
        else if (cmp > 0) hi = mid - 1;
        else { hit = mid; hi = mid - 1; }
    }
    return hit;
}

void soundMixBlock(short* out) {
    bool any = false;
    for (int v = 0; v < MAX_VOICES; v++) {
        if (g_voices[v].playing) { any = true; break; }
    }
    if (!any) {
        memset(out, 0, SAMPLE_COUNT * 2 * sizeof(short));
        return;
    }

    static int mix[SAMPLE_COUNT];

    memset(mix, 0, sizeof(mix));

    for (int v = 0; v < MAX_VOICES; v++) {
        Voice* s = &g_voices[v];
        if (!s->playing) continue;

        unsigned int pos = s->pos;
        for (int i = 0; i < SAMPLE_COUNT; i++) {
            unsigned int frame = pos >> 16;
            if (frame >= s->frameCount) { s->playing = 0; break; }

            int sample1 = s->frames[frame];
            int sample2 = (frame + 1 < s->frameCount) ? s->frames[frame + 1] : sample1;
            int frac = pos & 0xFFFF;
            int interp = sample1 + (((sample2 - sample1) * frac) >> 16);

            mix[i] += (interp * s->vol) >> 12;
            pos += s->step;
        }
        s->pos = pos;
    }

    for (int i = 0; i < SAMPLE_COUNT; i++) {
        int sample = mix[i];
        if (sample >  32767) sample =  32767;
        if (sample < -32768) sample = -32768;
        out[i * 2] = out[i * 2 + 1] = (short)sample;
    }
}

static int mixerThread(SceSize , void* ) {

    static short out[2][SAMPLE_COUNT * 2];
    int buf = 0;
    for (;;) {
        soundMixBlock(out[buf]);
        int vol = g_muted ? 0 : (int)(g_master * PSP_AUDIO_VOLUME_MAX);
        sceAudioOutputPannedBlocking(g_channel, vol, vol, out[buf]);
        buf ^= 1;
    }
    return 0;
}

void soundInit(void) {
    if (!loadPack(assetPath("data/sound/sounds.bin")) &&
        !loadPack("data/sound/sounds.bin")) {
        return;
    }

    g_channel = sceAudioChReserve(0, SAMPLE_COUNT, PSP_AUDIO_FORMAT_STEREO);
    if (g_channel < 0) return;

    int thid = sceKernelCreateThread("sound_thread", mixerThread, 0x12, 0x10000,
                                     PSP_THREAD_ATTR_USER, 0);
    if (thid < 0) return;
    sceKernelStartThread(thid, 0, 0);
}

void soundSetVolume(float volume) {
    g_master = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
}

float soundAttenuate(float distSq, float volume) {

    float dd = 16.0f;
    if (volume > 1.0f) dd *= volume;
    if (distSq >= dd * dd) return 0.0f;

    float mult = 1.1f - sqrtf(distSq) / 20.0f;
    if (mult < -1.0f) mult = -1.0f; else if (mult > 1.0f) mult = 1.0f;
    float v = volume * mult;
    if (v <= 0.0f) return 0.0f;
    return v > 1.0f ? 1.0f : v;
}

void soundPlay(const char* name, float volume, float pitch) {
    if (g_channel < 0 || !name || !name[0] || g_muted || g_master <= 0.0f) return;
    if (volume <= 0.0f) return;

    int first = findFirst(name);
    if (first < 0) return;

    int variants = 1;
    while (first + variants < g_count &&
           strcmp(g_index[first + variants].name, name) == 0)
        variants++;
    const Entry* e = &g_index[first + (variants > 1 ? rand() % variants : 0)];

    Voice* s = 0;
    for (int v = 0; v < MAX_VOICES; v++) {
        if (!g_voices[v].playing) { s = &g_voices[v]; break; }
    }
    if (!s) return;

    if (volume > 1.0f) volume = 1.0f;
    if (pitch < 0.1f) pitch = 0.1f;
    if (pitch > 4.0f) pitch = 4.0f;

    s->frames     = g_pcm + e->offset / 2;
    s->frameCount = e->frames;
    s->pos        = 0;
    s->step       = (unsigned int)(((float)SOURCE_RATE / SAMPLE_RATE) * pitch * 65536.0f);
    s->vol        = (int)(volume * 4096.0f);
    s->playing    = 1;
}

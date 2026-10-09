#include "dioom.h"

static void wrap_audio_phase(double *phase);
static double fm_osc(double *carrier_phase, double *mod_phase, double carrier_hz, double mod_hz, double index);
static double midi_note_frequency(int note);
static const FmInstrument *fm_instrument(int instrument);
static int active_music_track_index(void);
static double active_music_drone_root(void);
static double fm_midi_voice_envelope(const FmMidiVoice *voice);
static void fm_midi_note_off(uint8_t note, uint8_t channel);
static double fm_midi_music_sample(void);
static double fm_music_sample(void);
static void reset_midi_track(MidiMusic *music);
static void audio_callback(void *userdata, uint8_t *stream, int len);
static const char *sfx_path(int sfx);
static void free_sfx(void);
static int load_sfx_samples(const SDL_AudioSpec *device_spec);
static uint16_t read_be16(const uint8_t *data);
static uint32_t read_be32(const uint8_t *data);

double music_volume = DEFAULT_MUSIC_VOLUME_STEP / (double)AUDIO_VOLUME_STEPS;

double sfx_volume = DEFAULT_SFX_VOLUME_STEP / (double)AUDIO_VOLUME_STEPS;

static int music_volume_step = DEFAULT_MUSIC_VOLUME_STEP;

static int sfx_volume_step = DEFAULT_SFX_VOLUME_STEP;

static double audio_rate = 44100.0;

static SDL_AudioDeviceID audio_device = 0;

int active_music_track = MUSIC_TRACK_DIES_IRAE;

FmMidiVoice midi_voices[MAX_MIDI_VOICES];

MidiMusic midi_tracks[MUSIC_TRACK_COUNT];

static FmMusicState fm_music;

static SampleVoice sample_voices[MAX_SAMPLE_VOICES];

static SfxSample sfx_samples[SFX_COUNT];

static void wrap_audio_phase(double *phase)
{
    double cycle = M_PI * 2.0;
    if (*phase >= cycle) {
        *phase -= cycle;
    } else if (*phase < 0.0) {
        *phase += cycle;
    }
}

static double fm_osc(double *carrier_phase, double *mod_phase, double carrier_hz, double mod_hz, double index)
{
    double sample = sin(*carrier_phase + sin(*mod_phase) * index);
    double phase_step = (M_PI * 2.0) / audio_rate;
    *carrier_phase += carrier_hz * phase_step;
    *mod_phase += mod_hz * phase_step;
    wrap_audio_phase(carrier_phase);
    wrap_audio_phase(mod_phase);
    return sample;
}

static double midi_note_frequency(int note)
{
    static const int shifts[MUSIC_TRACK_COUNT] = {-12, 0, -24, -12};
    int shift = active_music_track >= 0 && active_music_track < MUSIC_TRACK_COUNT ? shifts[active_music_track] : -12;
    note += shift;
    if (note < 12) {
        note = 12;
    }
    if (note > 108) {
        note = 108;
    }
    return 440.0 * pow(2.0, ((double)note - 69.0) / 12.0);
}

static const FmInstrument *fm_instrument(int instrument)
{
    static const FmInstrument instruments[FM_INST_COUNT] = {
        {0.503, 1.0, 1.4, 0.30, 0.006, 8.4, 0.18, 0.22, 18.0, 0.82},
        {1.997, 2.1, 2.2, 0.12, 0.014, 2.0, 0.72, 0.30, 8.2, 0.70},
        {2.002, 1.5, 1.8, 0.18, 0.010, 4.6, 0.34, 0.42, 7.0, 0.78},
        {3.018, 3.7, 2.8, 0.04, 0.004, 7.8, 0.10, 0.32, 5.8, 0.54},
        {1.251, 0.9, 1.0, 0.22, 0.050, 1.2, 0.68, 0.58, 3.8, 0.48},
        {2.414, 3.1, 3.0, 0.08, 0.008, 5.8, 0.22, 0.24, 10.0, 0.62},
    };
    if (instrument < 0 || instrument >= FM_INST_COUNT) {
        instrument = FM_INST_EPIANO;
    }
    return &instruments[instrument];
}

static int active_music_track_index(void)
{
    return active_music_track >= 0 && active_music_track < MUSIC_TRACK_COUNT ? active_music_track : MUSIC_TRACK_DIES_IRAE;
}

int fm_instrument_for_note(int track, int channel, int note)
{
    if (note < 43) {
        return FM_INST_BASS;
    }
    if (track == MUSIC_TRACK_MASONIC_FUNERAL) {
        if (note >= 72) return FM_INST_BELL;
        return (channel & 1) ? FM_INST_PAD : FM_INST_EPIANO;
    }
    if (track == MUSIC_TRACK_PATHETIQUE) {
        if (note >= 76) return FM_INST_BELL;
        if (note < 55) return FM_INST_EPIANO;
        return (channel & 1) ? FM_INST_PAD : FM_INST_EPIANO;
    }
    if (track == MUSIC_TRACK_TOCCATA) {
        if (note >= 74) return FM_INST_LEAD;
        return (channel % 3) == 0 ? FM_INST_ORGAN : FM_INST_EPIANO;
    }
    if (note >= 76) {
        return FM_INST_BELL;
    }
    return (channel & 1) ? FM_INST_ORGAN : FM_INST_EPIANO;
}

static double active_music_drone_root(void)
{
    static const double roots[MUSIC_TRACK_COUNT] = {36.71, 27.50, 29.14, 24.50};
    if (active_music_track == MUSIC_TRACK_FOREST) {
        return 34.65;
    }
    int track = active_music_track >= 0 && active_music_track < MUSIC_TRACK_COUNT ? active_music_track : MUSIC_TRACK_DIES_IRAE;
    return roots[track];
}

static double fm_midi_voice_envelope(const FmMidiVoice *voice)
{
    const FmInstrument *instrument = fm_instrument(voice->instrument);
    double attack = instrument->attack > 0.001 ? instrument->attack : FM_NOTE_ATTACK;
    double sustain = instrument->sustain;
    double hold = clamp01(voice->age / attack) *
        (sustain + (1.0 - sustain) * exp(-voice->age * instrument->decay));
    if (voice->release_time >= 0.0) {
        return voice->release_level * exp(-voice->release_time * instrument->release);
    }
    return hold;
}

void fm_midi_note_on(uint8_t note, uint8_t velocity, uint8_t channel)
{
    int slot = -1;
    double oldest = -1.0;
    for (int i = 0; i < ACTIVE_MIDI_VOICE_LIMIT; ++i) {
        if (!midi_voices[i].active) {
            slot = i;
            break;
        }
        double age_score = midi_voices[i].release_time >= 0.0 ? midi_voices[i].age + 1000.0 : midi_voices[i].age;
        if (age_score > oldest) {
            oldest = age_score;
            slot = i;
        }
    }

    if (slot < 0) {
        return;
    }
    int instrument = fm_instrument_for_note(active_music_track_index(), channel, note);
    double freq = midi_note_frequency(note);
    midi_voices[slot] = (FmMidiVoice){
        .active = 1,
        .note = note,
        .channel = channel,
        .instrument = instrument,
        .velocity = clamp01(velocity / 127.0),
        .freq = freq,
        .mod_ratio = fm_instrument(instrument)->mod_ratio * (note & 1 ? 1.003 : 0.997),
        .phase = 0.0,
        .mod_phase = 0.0,
        .age = 0.0,
        .release_time = -1.0,
        .release_level = 0.0,
    };
}

static void fm_midi_note_off(uint8_t note, uint8_t channel)
{
    for (int i = 0; i < ACTIVE_MIDI_VOICE_LIMIT; ++i) {
        FmMidiVoice *voice = &midi_voices[i];
        if (voice->active && voice->note == note && voice->channel == channel && voice->release_time < 0.0) {
            voice->release_level = fm_midi_voice_envelope(voice);
            voice->release_time = 0.0;
        }
    }
}

double fm_midi_voices_sample(void)
{
    double sample = 0.0;
    double dt = 1.0 / audio_rate;
    double phase_step = M_PI * 2.0 / audio_rate;

    for (int i = 0; i < ACTIVE_MIDI_VOICE_LIMIT; ++i) {
        FmMidiVoice *voice = &midi_voices[i];
        if (!voice->active) {
            continue;
        }

        const FmInstrument *instrument = fm_instrument(voice->instrument);
        if (voice->release_time < 0.0 && voice->age >= instrument->hold) {
            voice->release_level = fm_midi_voice_envelope(voice);
            voice->release_time = 0.0;
        }

        double env = fm_midi_voice_envelope(voice);
        if (voice->release_time >= 0.0 && env < 0.001) {
            voice->active = 0;
            continue;
        }

        double index = instrument->index_base + voice->velocity * instrument->velocity_index;
        double tone = sin(voice->phase + sin(voice->mod_phase) * index);
        tone += sin(voice->phase * 0.5) * instrument->sub_level;
        sample += tone * env * voice->velocity * instrument->level * 0.090;

        voice->phase += voice->freq * phase_step;
        voice->mod_phase += voice->freq * voice->mod_ratio * phase_step;
        wrap_audio_phase(&voice->phase);
        wrap_audio_phase(&voice->mod_phase);
        voice->age += dt;
        if (voice->release_time >= 0.0) {
            voice->release_time += dt;
        }
    }
    return sample;
}

static double fm_midi_music_sample(void)
{
    if (active_music_track < 0 || active_music_track >= MUSIC_TRACK_COUNT) {
        return 0.0;
    }
    MidiMusic *music = &midi_tracks[active_music_track];
    if (!music->events || music->event_count <= 0 || music->length <= 0.0) {
        return 0.0;
    }

    while (music->next_event < music->event_count &&
           music->events[music->next_event].time <= music->playhead) {
        const MidiMusicEvent *event = &music->events[music->next_event++];
        if (event->on) {
            fm_midi_note_on(event->note, event->velocity, event->channel);
        } else {
            fm_midi_note_off(event->note, event->channel);
        }
    }

    double sample = fm_midi_voices_sample();
    music->playhead += 1.0 / audio_rate;
    if (music->playhead >= music->length) {
        music->playhead = 0.0;
        music->next_event = 0;
        memset(midi_voices, 0, sizeof(midi_voices));
    }
    return sample;
}

static double fm_music_sample(void)
{
    double root = active_music_drone_root();
    double fade = clamp01(fm_music.time / 3.0);
    double pulse = 0.78 + 0.22 * sin(fm_music.time * M_PI * 0.11);
    double drone = fm_osc(&fm_music.drone_phase, &fm_music.drone_mod_phase, root, root * 1.503, 3.2);
    double midi = fm_midi_music_sample();
    fm_music.time += 1.0 / audio_rate;

    double sample = (drone * 0.08 * pulse + midi * 1.18) * fade * FM_MUSIC_VOLUME * music_volume;
    return sample / (1.0 + fabs(sample) * 0.45);
}

int clamp_volume_step(int step)
{
    if (step < 0) {
        return 0;
    }
    if (step > AUDIO_VOLUME_STEPS) {
        return AUDIO_VOLUME_STEPS;
    }
    return step;
}

void set_audio_volume_steps(int sfx_step, int music_step)
{
    int next_sfx = clamp_volume_step(sfx_step);
    int next_music = clamp_volume_step(music_step);
    if (audio_device) {
        SDL_LockAudioDevice(audio_device);
    }
    sfx_volume_step = next_sfx;
    music_volume_step = next_music;
    sfx_volume = sfx_volume_step / (double)AUDIO_VOLUME_STEPS;
    music_volume = music_volume_step / (double)AUDIO_VOLUME_STEPS;
    if (audio_device) {
        SDL_UnlockAudioDevice(audio_device);
    }
}

static void reset_midi_track(MidiMusic *music)
{
    music->playhead = 0.0;
    music->next_event = 0;
}

void set_active_music_track(int track)
{
    if (track < 0 || track > MUSIC_TRACK_FOREST) {
        track = MUSIC_TRACK_FOREST;
    }
    if (audio_device) {
        SDL_LockAudioDevice(audio_device);
    }
    active_music_track = track;
    if (active_music_track < MUSIC_TRACK_COUNT) {
        reset_midi_track(&midi_tracks[active_music_track]);
    }
    memset(midi_voices, 0, sizeof(midi_voices));
    fm_music.time = 0.0;
    fm_music.drone_phase = 0.0;
    fm_music.drone_mod_phase = 0.0;
    if (audio_device) {
        SDL_UnlockAudioDevice(audio_device);
    }
}

int music_track_for_relic(int relic_index)
{
    static const int tracks[RELIC_COUNT] = {
        MUSIC_TRACK_DIES_IRAE,
        MUSIC_TRACK_MASONIC_FUNERAL,
        MUSIC_TRACK_PATHETIQUE,
        MUSIC_TRACK_TOCCATA,
    };
    if (relic_index < 0 || relic_index >= RELIC_COUNT) {
        return MUSIC_TRACK_TOCCATA;
    }
    return tracks[relic_index];
}

static void audio_callback(void *userdata, uint8_t *stream, int len)
{
    (void)userdata;
    int16_t *out = (int16_t *)stream;
    int samples = len / (int)sizeof(int16_t);

    for (int i = 0; i < samples; ++i) {
        double sample = fm_music_sample();
        for (int v = 0; v < MAX_SAMPLE_VOICES; ++v) {
            SampleVoice *voice = &sample_voices[v];
            if (!voice->active) {
                continue;
            }
            const SfxSample *sfx = &sfx_samples[voice->sfx];
            if (voice->cursor + sizeof(int16_t) > sfx->length) {
                voice->active = 0;
                continue;
            }
            int16_t src;
            memcpy(&src, sfx->data + voice->cursor, sizeof(src));
            double env = 1.0;
            Uint32 samples_left = (sfx->length - voice->cursor) / (Uint32)sizeof(int16_t);
            if (samples_left < 512) {
                env = samples_left / 512.0;
            }
            sample += (src / 32768.0) * voice->volume * env * sfx_volume;
            voice->cursor += (Uint32)sizeof(int16_t);
        }
        if (sample > 1.0) sample = 1.0;
        if (sample < -1.0) sample = -1.0;
        out[i] = (int16_t)(sample * 32767.0);
    }
}

static const char *sfx_path(int sfx)
{
    static const char *paths[SFX_COUNT] = {
        "assets/sfx/pistol.wav",
        "assets/sfx/fireball.wav",
        "assets/sfx/explosion.wav",
        "assets/sfx/pickup.wav",
        "assets/sfx/hurt.wav",
        "assets/sfx/death.wav",
        "assets/sfx/melee.wav",
        "assets/sfx/portal.wav",
        "assets/sfx/door.wav",
        "assets/sfx/locked.wav",
        "assets/sfx/relic.wav",
        "assets/sfx/shrine.wav",
    };
    return paths[sfx];
}

static void free_sfx(void)
{
    for (int i = 0; i < SFX_COUNT; ++i) {
        if (sfx_samples[i].data) {
            SDL_FreeWAV(sfx_samples[i].data);
            sfx_samples[i].data = NULL;
            sfx_samples[i].length = 0;
        }
    }
}

static int load_sfx_samples(const SDL_AudioSpec *device_spec)
{
    for (int i = 0; i < SFX_COUNT; ++i) {
        SDL_AudioSpec spec;
        Uint8 *data = NULL;
        Uint32 length = 0;
        const char *path = sfx_path(i);
        if (!SDL_LoadWAV(path, &spec, &data, &length)) {
            fprintf(stderr, "error: cannot load required sound %s: %s\n", path, SDL_GetError());
            free_sfx();
            return 0;
        }
        if (spec.freq != device_spec->freq ||
            spec.format != device_spec->format ||
            spec.channels != device_spec->channels) {
            fprintf(stderr,
                    "error: sound %s must be %d Hz mono signed 16-bit PCM WAV\n",
                    path,
                    device_spec->freq);
            SDL_FreeWAV(data);
            free_sfx();
            return 0;
        }
        sfx_samples[i].data = data;
        sfx_samples[i].length = length;
    }
    return 1;
}

int verify_sfx_assets(void)
{
    for (int i = 0; i < SFX_COUNT; ++i) {
        const char *path = sfx_path(i);
        FILE *f = fopen(path, "rb");
        if (!f) {
            fprintf(stderr, "error: required sound asset is missing: %s\n", path);
            return 0;
        }

        uint8_t header[44];
        size_t read = fread(header, 1, sizeof(header), f);
        fclose(f);
        if (read != sizeof(header) ||
            memcmp(header, "RIFF", 4) != 0 ||
            memcmp(header + 8, "WAVE", 4) != 0 ||
            memcmp(header + 12, "fmt ", 4) != 0) {
            fprintf(stderr, "error: required sound asset is not a PCM WAV: %s\n", path);
            return 0;
        }

        int channels = header[22] | (header[23] << 8);
        int rate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
        int bits = header[34] | (header[35] << 8);
        if (channels != 1 || rate != 44100 || bits != 16) {
            fprintf(stderr, "error: required sound asset must be 44.1 kHz mono 16-bit WAV: %s\n", path);
            return 0;
        }
    }
    return 1;
}

int init_audio(void)
{
    SDL_AudioSpec want;
    SDL_AudioSpec have;
    memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 1;
    want.samples = AUDIO_BUFFER_SAMPLES;
    want.callback = audio_callback;

    audio_device = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (!audio_device) {
        fprintf(stderr, "SDL_OpenAudioDevice failed: %s\n", SDL_GetError());
        return 0;
    }
    audio_rate = (double)have.freq;
    memset(&fm_music, 0, sizeof(fm_music));
    memset(midi_voices, 0, sizeof(midi_voices));
    if (!load_sfx_samples(&have)) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
        return 0;
    }
    SDL_PauseAudioDevice(audio_device, 0);
    return 1;
}

void shutdown_audio(void)
{
    if (audio_device) {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }
    free_sfx();
}

void play_sfx(int sfx, double volume)
{
    if (!audio_device) {
        return;
    }
    SDL_LockAudioDevice(audio_device);
    for (int i = 0; i < MAX_SAMPLE_VOICES; ++i) {
        SampleVoice *voice = &sample_voices[i];
        if (!voice->active) {
            voice->active = 1;
            voice->sfx = sfx;
            voice->cursor = 0;
            voice->volume = volume;
            break;
        }
    }
    SDL_UnlockAudioDevice(audio_device);
}

static uint16_t read_be16(const uint8_t *data)
{
    return (uint16_t)((data[0] << 8) | data[1]);
}

static uint32_t read_be32(const uint8_t *data)
{
    return ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
}

void free_midi_tracks(void)
{
    for (int i = 0; i < MUSIC_TRACK_COUNT; ++i) {
        free(midi_tracks[i].events);
    }
    memset(midi_tracks, 0, sizeof(midi_tracks));
    memset(midi_voices, 0, sizeof(midi_voices));
    active_music_track = MUSIC_TRACK_FOREST;
}

int load_midi_music(const char *path, MidiMusic *music)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open required MIDI music asset %s: %s\n", path, strerror(errno));
        return 0;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long file_size = ftell(f);
    if (file_size < 22 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }

    uint8_t *data = malloc((size_t)file_size);
    if (!data) {
        fclose(f);
        return 0;
    }
    int ok = fread(data, 1, (size_t)file_size, f) == (size_t)file_size;
    fclose(f);
    if (!ok) {
        free(data);
        return 0;
    }

    MidiRawEvent *raw_events = NULL;
    MidiTempoEvent *tempo_events = NULL;
    int raw_count = 0;
    int raw_capacity = 0;
    int tempo_count = 0;
    int tempo_capacity = 0;

    size_t pos = 0;
    if (memcmp(data, "MThd", 4) != 0 || read_be32(data + 4) < 6) {
        ok = 0;
    } else {
        uint16_t format = read_be16(data + 8);
        uint16_t track_count = read_be16(data + 10);
        uint16_t division = read_be16(data + 12);
        uint32_t header_size = read_be32(data + 4);
        pos = 8u + header_size;
        if ((format != 0 && format != 1) || track_count == 0 || (division & 0x8000u) != 0 || pos > (size_t)file_size) {
            ok = 0;
        } else {
            int ticks_per_quarter = (int)division;
            for (uint16_t track = 0; ok && track < track_count; ++track) {
                if (pos + 8 > (size_t)file_size || memcmp(data + pos, "MTrk", 4) != 0) {
                    ok = 0;
                    break;
                }
                uint32_t track_size = read_be32(data + pos + 4);
                pos += 8;
                if (pos + track_size > (size_t)file_size) {
                    ok = 0;
                    break;
                }
                ok = parse_midi_track(data, pos, pos + track_size,
                                      &raw_events, &raw_count, &raw_capacity,
                                      &tempo_events, &tempo_count, &tempo_capacity);
                pos += track_size;
            }

            if (ok) {
                int has_start_tempo = 0;
                for (int i = 0; i < tempo_count; ++i) {
                    if (tempo_events[i].tick == 0) {
                        has_start_tempo = 1;
                        break;
                    }
                }
                if (!has_start_tempo &&
                    !append_midi_tempo_event(&tempo_events, &tempo_count, &tempo_capacity,
                                             (MidiTempoEvent){0, 500000})) {
                    ok = 0;
                }
            }
            if (ok && raw_count > 0) {
                qsort(raw_events, (size_t)raw_count, sizeof(*raw_events), compare_midi_raw_events);
                qsort(tempo_events, (size_t)tempo_count, sizeof(*tempo_events), compare_midi_tempo_events);

                MidiMusicEvent *events = malloc((size_t)raw_count * sizeof(*events));
                if (!events) {
                    ok = 0;
                } else {
                    for (int i = 0; i < raw_count; ++i) {
                        events[i] = (MidiMusicEvent){
                            midi_tick_to_seconds(raw_events[i].tick, tempo_events, tempo_count, ticks_per_quarter),
                            raw_events[i].note,
                            raw_events[i].velocity,
                            raw_events[i].channel,
                            raw_events[i].on,
                        };
                    }
                    free(music->events);
                    music->events = events;
                    music->event_count = raw_count;
                    music->length = events[raw_count - 1].time + 3.0;
                    music->next_event = 0;
                    music->playhead = 0.0;
                }
            }
        }
    }

    free(raw_events);
    free(tempo_events);
    free(data);

    if (!ok || music->event_count == 0) {
        fprintf(stderr, "error: cannot parse required MIDI music asset %s\n", path);
        free(music->events);
        memset(music, 0, sizeof(*music));
        return 0;
    }
    return 1;
}

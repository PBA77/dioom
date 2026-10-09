#include "dioom.h"

int main(int argc, char **argv)
{
    if (argc == 5 && strcmp(argv[1], "--dump-resolution") == 0) {
        int resolution, mode;
        if (!parse_resolution(argv[2], &resolution) || !parse_generator_mode_name(argv[3], &mode)) {
            fprintf(stderr, "error: --dump-resolution expects 640x480 or 1280x960, level mode, output.ppm\n");
            return 1;
        }
        SCREEN_W = resolution ? 1280 : 640;
        SCREEN_H = resolution ? 960 : 480;
        return dump_frame_mode(argv[4], mode);
    }
    if (argc == 3 && strcmp(argv[1], "--dump") == 0) {
        return dump_frame(argv[2]);
    }
    if (argc == 4 && strcmp(argv[1], "--dump-quality") == 0) {
        int quality = RENDER_QUALITY_FAST;
        if (!parse_render_quality(argv[2], &quality)) {
            fprintf(stderr, "error: --dump-quality expects fast (legacy pbr is migrated)\n");
            return 1;
        }
        return dump_frame_quality(argv[3], quality);
    }
    if (argc == 5 && strcmp(argv[1], "--profile-dump") == 0) {
        int quality = RENDER_QUALITY_FAST;
        int mode = GENERATOR_ROOMS;
        if (!parse_render_quality(argv[2], &quality)) {
            fprintf(stderr, "error: --profile-dump expects fast (legacy pbr is migrated) quality\n");
            return 1;
        }
        if (!parse_generator_mode_name(argv[3], &mode)) {
            fprintf(stderr, "error: --profile-dump expects rooms, forest, tight, boss, or house mode\n");
            return 1;
        }
        return profile_dump_frame(argv[4], quality, mode);
    }
    if ((argc == 2 || argc == 3) && strcmp(argv[1], "--bench") == 0) {
        int frames = argc == 3 ? atoi(argv[2]) : 300;
        if (frames < 1) {
            fprintf(stderr, "error: --bench expects a positive frame count\n");
            return 1;
        }
        return bench_render(frames);
    }
    if (argc == 3 && strcmp(argv[1], "--dump-house") == 0) {
        return dump_frame_mode(argv[2], GENERATOR_HOUSE);
    }
    if (argc == 3 && strcmp(argv[1], "--dump-forest") == 0) {
        return dump_frame_mode(argv[2], GENERATOR_FOREST);
    }
    if (argc == 3 && strcmp(argv[1], "--dump-forest-forward") == 0) {
        return dump_forest_forward_frames(argv[2]);
    }

    RuntimeConfig config;
#ifdef __EMSCRIPTEN__
    web_restore_persistent_files(SAVEGAME_SLOT_COUNT);
#endif
    if (!parse_runtime_config(argc, argv, &config)) {
        return 1;
    }

    static Runtime rt;
    if (!init_runtime(&rt, &config)) {
        return 1;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_arg(runtime_frame, &rt, 0, 1);
#else
    while (rt.running) {
        runtime_frame(&rt);
    }
#endif

    return 0;
}

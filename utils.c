#include "utils.h"
#include "glib.h"
#include <stdio.h>
#include <getopt.h>

bool file_exists(const char* filepath) {
    bool result;
    FILE* f = fopen(filepath, "r");
    if (f) {
        result = true;
        fclose(f);
    } else {
        result = false;
    }
    return result;
}

gboolean parse_is_audio_only(int *argc, char*** argv, int *error) {
    static const struct option long_options[] = {
        {"audio", no_argument, 0, 0},
        {"video", no_argument, 0, 0},
        {0, 0, 0, 0}
    };
    int option_index;

    int r = getopt_long(*argc, *argv, "av", (const struct option*)&long_options, &option_index);
    if (r == -1) {
        *error = -1;
        return FALSE;
    }

    int is_audio_only;
    switch (r) {
        case 0: {
            const char* name = long_options[option_index].name;
            if (!strcmp(name, "audio")) {
                is_audio_only = 1;
            } else if (!strcmp(name, "video")) {
                is_audio_only = 0;
            }
            break;
        }
        case 'a': {
            is_audio_only = 1;
            break;
        } 
        case 'v': {
            is_audio_only = 0;
            break;
        }
        default: {
            is_audio_only = 0;
            break;
        }
    }
    *error = 0;
    return is_audio_only;
}

gboolean is_audio_only(const char* filepath) {
    const char *audio_exts[] = {
        ".mp3", ".aac", ".flac", ".wav", ".ogg", ".oga", ".m4a", ".wma", ".opus"
    };
    const char *video_exts[] = { // That also includes pictures
        ".mp4", ".mkv", ".mov", ".avi", ".flv", 
        ".webm", ".wmv", ".mpeg", ".mpg", ".m2ts", ".png", ".jpeg",
        ".jpg"
    };

    gboolean is_audio = FALSE;
    gboolean is_video = FALSE;

    for (int i = 0; i < ARRAY_SIZE(audio_exts); ++i) {
        if (g_str_has_suffix(filepath, audio_exts[i])) {
            is_audio = TRUE;
            break;
        }
    }   
    
    for (int i = 0; i < ARRAY_SIZE(audio_exts); ++i) {
        if (g_str_has_suffix(filepath, video_exts[i])) {
            is_video = TRUE;
        }
    }

    if (is_video) return FALSE;
    return TRUE;
}
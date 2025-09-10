#include "utils.h"
#include "glib.h"
#include <bits/getopt_core.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

gboolean file_exists(const char* filepath) {
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

static void parse_long_option(const char* option_name, Settings* settings) {
    if (!strcmp(option_name, "path")) {
        settings->filepath = strdup(optarg);
    } else if (!strcmp(option_name, "audio")) {
        settings->is_audio_only = TRUE;
    } else if (!strcmp(option_name, "video")) {
        settings->is_audio_only = FALSE;
    } else if (!strcmp(option_name, "volume")) {
        const char* double_volume_str = optarg;
        char *result = NULL;
        double volume = strtod(double_volume_str, &result);

        if (result && *double_volume_str == *result) {
            g_printerr("Error when converting to double\n");
            return;
        } else if (volume > 1. || volume < .0) {
            g_print("Volume can range from 0.0 to 1.0 only\n");
        }

        settings->volume = volume;
    } else if (!strcmp(option_name, "balance")) {
        const char* float_balance_str = optarg;
        char* result = NULL;
        float balance = strtof(float_balance_str, &result);

        if (result && *float_balance_str == *result) {
            g_printerr("Erro when converting to float\n");
            return;
        } else if (balance < -1. || balance > 1.) {
            g_print("Balance can range from -1.0 to 1.0 only\n");
            return;
        }

        settings->balance = balance;
    } else if (!strcmp(option_name, "lowpass")) {
        settings->pass_type = PassLow;
    } else if (!strcmp(option_name, "highpass")) {
        settings->pass_type = PassHigh;
    } else if (!strcmp(option_name, "cutoff")) {
        const char* float_cutoff_str = optarg;
        char* result = NULL;
        float cutoff = strtof(float_cutoff_str, &result);

        if (result && *float_cutoff_str == *result) {
            g_printerr("Erro when converting to float\n");
            return;
        } else if (cutoff < .0f || cutoff > 100000.f) {
            g_print("Cutoff can range from 0.0 to 100000.0 only\n");
            return;
        }

        settings->pass_cutoff = cutoff;
    }
}

void settings_set_default(Settings* settings) {
    settings->filepath = NULL;

    settings->is_audio_only = FALSE;
    settings->volume = 1.0;
    settings->balance = .0f;

    settings->pass_type = PassNone;
    settings->pass_cutoff = .0f;
}

char* settings_get_file_uri(Settings* settings) { // TODO: https url or file uri
    if (settings->filepath == NULL) {
        g_printerr("File was not set, aborting...\n");
        return NULL;
    }

    // TODO: Maybe more flexible deduction
    if (strstr(settings->filepath, "https://") || strstr(settings->filepath, "http://")) {
        return strdup(settings->filepath); 
        // To unify the behaviour between webpath and filepath, we dup this string, so the user has to take care of it, just like with filepath (there is malloc)
    } else {
        if (!file_exists(settings->filepath)) { // only if its a file
            g_printerr("Such file does not exist\n");
            return NULL;
        }
        // consturct a file://{path} format string
        char* full_path = malloc(strlen(FILE_PREFIX) + strlen(settings->filepath) + 1); // +1 for \0
        sprintf(full_path, "%s%s", FILE_PREFIX, settings->filepath);
        return full_path;
    }
}

void settings_parse_cli(Settings *settings, int *argc, char ***argv, int *error) {
    settings_set_default(settings);

    static const struct option long_options[] = {
    {"path", required_argument, 0, 0},
    {"audio", no_argument, 0, 0},
    {"video", no_argument, 0, 0},
    {"volume", required_argument, 0, 0},
    {"balance", required_argument, 0, 0},
    {"lowpass", no_argument, 0, 0},
    {"highpass", no_argument, 0, 0},
    {"cutoff", required_argument, 0, 0},
    {0, 0, 0, 0}
    };

    while (TRUE) {
        int option_index;

        int r = getopt_long(*argc, *argv, "p:av", (const struct option*)&long_options, &option_index);
        if ((char)r == '?') {
            *error = -1;
            return;
        }
        if (r == -1) {
            break;
        }
        switch (r) {
            case 0: {
                parse_long_option(long_options[option_index].name, settings);
                break;
            }
            case 'p': {
                settings->filepath = strdup(optarg);
                break;
            }
            case 'a': {
                settings->is_audio_only = TRUE;
                break;
            } 
            case 'v': {
                settings->is_audio_only = FALSE;
                break;
            }
            default: {
                settings->is_audio_only = FALSE;
                break;
            }
        }
    }
    
    *error = 0;
}
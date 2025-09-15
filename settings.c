#include "settings.h"
#include "glib.h"
#include <bits/getopt_core.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

static gboolean file_exists(const char* filepath) {
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


static gboolean is_path_web(const char* path) {
    return (strstr(path, "https://") && strstr(path, "http://"));
}

static gboolean is_audio_only_by_filepath(const char* filepath) {
    const char *video_exts[] = { // That also includes pictures
        ".mp4", ".mkv", ".mov", ".avi", ".flv",
        ".webm", ".wmv", ".mpeg", ".mpg", ".m2ts", ".png", ".jpeg",
        ".jpg"
    };

    gboolean is_video = FALSE;

    for (int i = 0; i < ARRAY_SIZE(video_exts); ++i) {
        if (g_str_has_suffix(filepath, video_exts[i])) {
            is_video = TRUE;
        }
    }

    return (is_video == FALSE);
}

static void parse_long_option(const char* option_name, Settings* settings) {
    // TODO: FACTOR OUT ARITHMETIC CONVERSIONS FROM STR INTO SEPARATE FUNCTIONS
    if (!strcmp(option_name, "path")) {
        settings->filepath = strdup(optarg);
        gboolean is_web = is_path_web(settings->filepath);
        if (!is_web) {
            settings->is_audio_only = is_audio_only_by_filepath(settings->filepath);
        }
    } else if (!strcmp(option_name, "audio")) {
        settings->is_audio_only = TRUE;
    } else if (!strcmp(option_name, "video")) {
        settings->is_audio_only = FALSE;
    } else if (!strcmp(option_name, "volume")) {
        const char* double_volume_str = optarg;
        char* endptr = NULL;
        errno = 0;

        double volume = strtod(double_volume_str, &endptr);

        if (endptr == double_volume_str) {
            g_printerr("no digits found when converting to double: %s\n", double_volume_str);
            return;
        }
        if (*endptr != '\0') {
            g_printerr("invalid characters after number: %s\n", endptr);
            return;
        }
        if (errno == ERANGE) {
            g_printerr("volume value out of range: %s\n", double_volume_str);
            return;
        }

        if (volume < 0.0 || volume > 1.0) {
            g_printerr("volume can range from 0.0 to 1.0 only (got %f)\n", volume);
            return;
        }

        settings->has_volume = TRUE;
        settings->volume = volume;
    } else if (!strcmp(option_name, "balance")) {
        const char* float_balance_str = optarg;
        char* endptr = NULL;
        errno = 0;

        float balance = strtof(float_balance_str, &endptr);

        // Check conversion errors
        if (endptr == float_balance_str) {
            g_printerr("no digits found when converting to float: %s\n", float_balance_str);
            return;
        }
        if (*endptr != '\0') {
            g_printerr("invalid characters after number: %s\n", endptr);
            return;
        }
        if (errno == ERANGE) {
            g_printerr("balance value out of range: %s\n", float_balance_str);
            return;
        }

        // Check bounds
        if (balance < -1.0f || balance > 1.0f) {
            g_printerr("Balance can range from -1.0 to 1.0 only (got %f)\n", balance);
            return;
        }

        settings->has_panorama = TRUE;
        settings->balance = balance;
    } else if (!strcmp(option_name, "lowpass")) {
        settings->pass_type = PassLow;
    } else if (!strcmp(option_name, "highpass")) {
        settings->pass_type = PassHigh;
    } else if (!strcmp(option_name, "cutoff")) {
        const char* float_cutoff_str = optarg;
        char* endptr = NULL;
        errno = 0;

        float cutoff = strtof(float_cutoff_str, &endptr);

        // check conversion errors
        if (endptr == float_cutoff_str) {
            g_printerr("no digits found when converting to float: %s\n", float_cutoff_str);
            return;
        }
        if (*endptr != '\0') {
            g_printerr("invalid characters after number: %s\n", endptr);
            return;
        }
        if (errno == ERANGE) {
            g_printerr("value out of range: %s\n", float_cutoff_str);
            return;
        }

        // check bounds
        if (cutoff < 0.0f || cutoff > 100000.0f) {
            g_printerr("Cutoff can range from 0.0 to 100000.0 only (got %f)\n", cutoff);
            return;
        }

        settings->pass_cutoff = cutoff;
    } else if (!strcmp(option_name, "delay")) {
        char *endptr = NULL;
        errno = 0;

        unsigned long tmp = strtoul(optarg, &endptr, 10);

        if (errno == ERANGE) {
            g_printerr("echo delay out of range: %s\n", optarg);
            return;
        }
        if (endptr == optarg) {
            g_printerr("invalid echo delay: %s\n", optarg);
            return;;
        }
        if (*endptr != '\0') {
            g_printerr("invalid characters in echo delay: %s\n", optarg);
            return;
        }

        settings->has_echo = TRUE;
        settings->echo_delay = (uint64_t)tmp;
    } else if (!strcmp(option_name, "feedback")) {
        const char* float_feedback_str = optarg;
        char* endptr = NULL;
        errno = 0;

        float feedback = strtof(float_feedback_str, &endptr);

        // check conversion errors
        if (endptr == float_feedback_str) {
            g_printerr("no digits found when converting to float: %s\n", float_feedback_str);
            return;
        }
        if (*endptr != '\0') {
            g_printerr("invalid characters after number: %s\n", endptr);
            return;
        }
        if (errno == ERANGE) {
            g_printerr("value out of range: %s\n", float_feedback_str);
            return;
        }

        settings->has_echo = TRUE;
        settings->echo_feedback = feedback;
    } else if (!strcmp(option_name, "intensity")) {
        const char* float_intensity_str = optarg;
        char* endptr = NULL;
        errno = 0;

        float intensity = strtof(float_intensity_str, &endptr);

        // check conversion errors
        if (endptr == float_intensity_str) {
            g_printerr("no digits found when converting to float: %s\n", float_intensity_str);
            return;
        }
        if (*endptr != '\0') {
            g_printerr("invalid characters after number: %s\n", endptr);
            return;
        }
        if (errno == ERANGE) {
            g_printerr("value out of range: %s\n", float_intensity_str);
            return;
        }

        settings->has_echo = TRUE;
        settings->echo_intensity = intensity;
    }
}

void settings_set_default(Settings* settings) {
    settings->filepath = NULL;

    settings->is_audio_only = FALSE;
    settings->volume = 1.0;
    settings->balance = .0f;

    settings->pass_type = PassNone;
    settings->pass_cutoff = .0f;

    settings->echo_delay = 1;
    settings->echo_feedback = .0f;
    settings->echo_intensity = .0f;

    settings->has_echo = FALSE;
    settings->has_panorama = FALSE;
    settings->has_volume = FALSE;
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
    {"delay", required_argument, 0, 0},
    {"feedback", required_argument, 0, 0},
    {"intensity", required_argument, 0, 0},
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
                gboolean is_web = is_path_web(settings->filepath);
                if (!is_web) {
                    settings->is_audio_only = is_audio_only_by_filepath(settings->filepath);
                }
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

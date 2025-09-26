#include "settings.h"
#include "glib.h"
#include "glibconfig.h"
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

static gboolean parse_double(const char* double_str, double* min, double* max, double* result) {
    char* endptr = NULL;
    errno = 0;

    double value = strtod(double_str, &endptr);

    if (endptr == double_str) {
        g_printerr("no digits found when converting to double: %s\n", double_str);
        return FALSE;
    }
    if (*endptr != '\0') {
        g_printerr("invalid characters after number: %s\n", endptr);
        return FALSE;
    }
    if (errno == ERANGE) {
        g_printerr("volume value out of range: %s\n", double_str);
        return FALSE;
    }

    if ((min && max) && (value < *min || value > *max)) {
        g_printerr("volume can range from 0.0 to 1.0 only (got %f)\n", value);
        return FALSE;
    }

    *result = value;
    return TRUE;
}

static gboolean parse_float(const char* float_str, float* min, float* max, float* result) {
    char* endptr = NULL;
    errno = 0;

    float value = strtof(float_str, &endptr);

    if (endptr == float_str) {
        g_printerr("no digits found when converting to floats: %s\n", float_str);
        return FALSE;
    }
    if (*endptr != '\0') {
        g_printerr("invalid characters after number: %s\n", endptr);
        return FALSE;
    }
    if (errno == ERANGE) {
        g_printerr("volume value out of range: %s\n", float_str);
        return FALSE;
    }

    if ((min && max) && (value < *min || value > *max)) {
        g_printerr("volume can range from 0.0 to 1.0 only (got %f)\n", value);
        return FALSE;
    }

    *result = value;
    return TRUE;
}

static gboolean parse_ul(const char* ul_str, guint64* min, guint64* max, guint64* result) {
    char* endptr = NULL;
    errno = 0;

    guint64 value = strtoul(ul_str, &endptr, 10);

    if (endptr == ul_str) {
        g_printerr("no digits found when converting to int: %s\n", ul_str);
        return FALSE;
    }
    if (*endptr != '\0') {
        g_printerr("invalid characters after number: %s\n", endptr);
        return FALSE;
    }
    if (errno == ERANGE) {
        g_printerr("volume value out of range: %s\n", ul_str);
        return FALSE;
    }

    if ((min && max) && (value < *min || value > *max)) {
        g_printerr("volume can range from 0.0 to 1.0 only (got %ld)\n", value);
        return FALSE;
    }

    *result = value;
    return TRUE;
}

static void parse_long_option(const char* option_name, Settings* settings) {
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
        double min = 0.0; double max = 1.0;
        double result;
        if (parse_double(optarg, &min, &max, &result)) {
            settings->has_volume = TRUE;
            settings->volume = result;
        }
    } else if (!strcmp(option_name, "balance")) {
        float min = -1.0f; float max = 1.0f;
        float result;
        if (parse_float(optarg, &min, &max, &result)) {
            settings->has_panorama = TRUE;
            settings->balance = result;
        }
    } else if (!strcmp(option_name, "lowpass")) {
        settings->pass_type = PassLow;
    } else if (!strcmp(option_name, "highpass")) {
        settings->pass_type = PassHigh;
    } else if (!strcmp(option_name, "cutoff")) {
        float min = 0.0f; float max = 100000.0f;
        float result;
        if (parse_float(optarg, &min, &max, &result)) {
            settings->pass_cutoff = result;
        }
    } else if (!strcmp(option_name, "delay")) {

        guint64 result;
        if (parse_ul(optarg, NULL, NULL, &result)) {
            settings->has_echo = TRUE;
            settings->echo_delay = result;
        }
    } else if (!strcmp(option_name, "feedback")) {
        float result;
        if (parse_float(optarg, NULL, NULL, &result)) {
            settings->has_echo = TRUE;
            settings->echo_feedback = result;
        }
    } else if (!strcmp(option_name, "intensity")) {
        float result;
        if (parse_float(optarg, NULL, NULL, &result)) {
            settings->has_echo = TRUE;
            settings->echo_intensity = result;
        }
    } else if (!strcmp(option_name, "speed")) {
        float result;
        if (parse_float(optarg, NULL, NULL, &result)) {
            settings->has_speed = TRUE;
            settings->speed = result;
        }
    } else if (!strcmp(option_name, "pitch")) {
        float result;
        if (parse_float(optarg, NULL, NULL, &result)) {
            settings->has_pitch = TRUE;
            settings->pitch_pitch = result;
        }
    } else if (!strcmp(option_name, "grayscale")) {
        double result;
        if (parse_double(optarg, NULL, NULL, &result)) {
            settings->has_videobalance = TRUE;
            settings->video_saturation = result;
        }
    } else if (!strcmp(option_name, "colorinvert")) {
        settings->has_colorinvert = TRUE;
    } else if (!strcmp(option_name, "noisethreshold")) {
        float result;
        if (parse_float(optarg, NULL, NULL, &result)) {
            settings->has_noise_reduction = TRUE;
            settings->noise_reduction = result;
        }
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

    settings->pitch_pitch = 1.0f;
    settings->speed = 1.0f;
    settings->video_saturation = 1.0;
    settings->has_colorinvert = FALSE;

    settings->noise_reduction = 0.0f;

    settings->has_echo = FALSE;
    settings->has_panorama = FALSE;
    settings->has_volume = FALSE;
    settings->has_pitch = FALSE;
    settings->has_videobalance = FALSE;
    settings->has_speed = FALSE;
    settings->has_noise_reduction = FALSE;
}

char* settings_get_file_uri(Settings* settings) {
    if (settings->filepath == NULL) {
        g_printerr("File was not set, aborting...\n");
        return NULL;
    }

    if (strstr(settings->filepath, "https://") || strstr(settings->filepath, "http://")) {
        return strdup(settings->filepath);
        // To unify the behaviour between webpath and filepath, we dup this string, so the user has to take care of it, just like with filepath (there is malloc)
    } else {
        if (!file_exists(settings->filepath)) { // only if its a file
            g_printerr("Such file does not exist\n");
            return NULL;
        }
        // consturct a file://{path} format string
        // char* full_path = malloc(strlen(FILE_PREFIX) + strlen(settings->filepath) + 1); // +1 for \0
        // sprintf(full_path, "%s%s", FILE_PREFIX, settings->filepath);

        char* abs_path = g_canonicalize_filename(settings->filepath, NULL);

        GError* err = NULL;
        char* full_path = g_filename_to_uri(abs_path, NULL, &err);
        g_free(abs_path);
        if (err) {
            g_printerr("Could not get file absolute path\n");
            g_error_free(err);
            return NULL;
        }
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
    {"speed", required_argument, 0, 0}, // audio speed
    {"pitch", required_argument, 0, 0},
    {"grayscale", required_argument, 0, 0},
    {"colorinvert", required_argument, 0, 0},
    {"noisethreshold", required_argument, 0, 0},
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

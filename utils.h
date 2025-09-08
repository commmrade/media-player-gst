#ifndef __UTILS__H
#define __UTILS__H
#include "glib.h"
#include <stdbool.h>

#define FILE_PREFIX "file://"
#define MEDIA_AUDIO 0b00000001
#define MEDIA_VIDEO 0b00000010
#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(*arr)


typedef enum PassType {
    PassLow,
    PassHigh,
    PassNone
} PassType;

typedef struct Settings {
    char* path; // default null
    gboolean is_audio_only; // default false
    double volume; // 1.0 for default (0.0 to 1.0)
    float balance; // 0.0f for default

    PassType pass_type; // Default none
    float pass_cutoff; // Defalt 0
} Settings;

void settings_parse_cli(Settings *settings, int *argc, char ***argv, int *error);
char* settings_get_filepath(Settings* settings);


gboolean file_exists(const char* filepath); 
gboolean is_audio_only(const char* filepath);
// gboolean parse_is_audio_only(int *argc, char*** argv, int *error);


#endif
#ifndef __UTILS__H
#define __UTILS__H
#include "glib.h"
#include <stdbool.h>

#define MEDIA_AUDIO 0b00000001
#define MEDIA_VIDEO 0b00000010
#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(*arr)


bool file_exists(const char* filepath); 


gboolean is_audio_only(const char* filepath);
gboolean parse_is_audio_only(int *argc, char*** argv, int *error);


#endif
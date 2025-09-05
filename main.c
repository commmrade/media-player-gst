#include "glib-object.h"
#include "glib.h"
#include "gst/gstbus.h"
#include "gst/gstcaps.h"
#include "gst/gstclock.h"
#include "gst/gstelement.h"
#include "gst/gstelementfactory.h"
#include "gst/gstmessage.h"
#include "gst/gstpad.h"
#include "gst/gstpipeline.h"
#include "gst/gststructure.h"
#include "gst/gstutils.h"
#include <stdio.h>
#include <gst/gst.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

typedef struct State {
    GstElement* pipeline;
    GstElement* source;
    
    GstElement* audio_converter;
    GstElement* video_converter;
    
    GstElement* audio_resampler;

    GstElement* audio_sink;
    GstElement* video_sink;

    gboolean is_only_audio;
} State;


#define MEDIA_AUDIO 0b00000001
#define MEDIA_VIDEO 0b00000010
#define ARRAY_SIZE(arr) sizeof(arr) / sizeof(*arr)

static void pad_added_signal (GstElement *self, GstPad *new_pad, State* state);

static gboolean is_audio_only(const char* filepath);

static gboolean is_audio_only_parse(int *argc, char*** argv, int *error) {
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


int main(int argc, char** argv) {
    if (argc < 2) {
        g_print("Usage: ./exec video.file (full path) [-v (video)| -a (audio)]\n");
        return 1;
    }

    State state;


    gboolean is_only_audio = is_audio_only(argv[1]);
    state.is_only_audio = is_only_audio;

    // Cmd arguments must override automatic media type detection
    if (argc >= 3) {
        int error;
        char** argvv = argv + 1;
        gboolean is_only_audio_result = is_audio_only_parse(&argc, &argvv, &error);
        if (error == 0) { // no err
            state.is_only_audio = is_only_audio_result;
        }
    }
    
    GstBus* bus = NULL;
    GstMessage* message = NULL;
    
    gst_init(&argc, &argv);

    state.source = gst_element_factory_make("uridecodebin", "source");
    state.audio_converter = gst_element_factory_make("audioconvert", "audio-converter");
    state.audio_resampler = gst_element_factory_make("audioresample", "audio-resampler");
    state.audio_sink = gst_element_factory_make("autoaudiosink", "audio-sink");

    
    if (!state.is_only_audio) {
        state.video_converter = gst_element_factory_make("videoconvert", "video-converter");
        state.video_sink = gst_element_factory_make("autovideosink", "video-sink");
    }

    
    if (!state.is_only_audio) {
        if (!state.source || !state.audio_converter || !state.video_converter || !state.audio_resampler || !state.audio_sink || !state.video_sink) {
            g_printerr("Could not create all elements\n");
            return -1;
        }
    } else {
        if (!state.source || !state.audio_converter || !state.audio_resampler || !state.audio_sink) {
            g_printerr("Could not create all elements\n");
            return -1;
        }
    }
   
    
    state.pipeline = gst_pipeline_new("tiktok-pipeline");
    if (!state.pipeline) {
        g_printerr("Could not create a pipeline\n");
        return -1;
    }

    const char* prefix_file = "file://";
    char* full_path = malloc(strlen(prefix_file) + strlen(argv[1]) + 1); // +1 for \0
    sprintf(full_path, "%s%s", prefix_file, argv[1]);

    // Set absolute path
    g_object_set(state.source, "uri", full_path, NULL);

    gst_bin_add_many(GST_BIN(state.pipeline), state.source, state.audio_converter, state.audio_resampler, state.audio_sink, NULL);
    if (!state.is_only_audio) {
        gst_bin_add_many(GST_BIN(state.pipeline), state.video_converter, state.video_sink, NULL);
    }

    // Link audio stuff
    if (!gst_element_link_many(state.audio_converter, state.audio_resampler, state.audio_sink, NULL)) {
        g_printerr("Unable to link audio elements\n");
        g_object_unref(state.pipeline);
        return -1;
    }
    // Link video stuff
    if (!state.is_only_audio) {
        if (!gst_element_link_many(state.video_converter, state.video_sink, NULL)) {
            g_printerr("Unable to link audio elements\n");
            g_object_unref(state.pipeline);
            return -1;
        }
    }

    // link source to pad added handler
    g_signal_connect(state.source, "pad-added", G_CALLBACK(pad_added_signal), &state);
    
    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(state.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Was unable to change state\n");
        return -1;
    }
    
    gboolean is_running = TRUE;
    bus = gst_element_get_bus(state.pipeline);
    do {
        message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        if (!message) continue;
        
        switch (GST_MESSAGE_TYPE(message)) {
            case GST_MESSAGE_ERROR: {
                GError* err;
                gchar* debug_info;

                gst_message_parse_error(message, &err, &debug_info);
                g_printerr("Error from %s: Message: %s\n", GST_OBJECT_NAME(message->src), err->message);

                g_clear_error(&err);
                g_free(debug_info);
                is_running = FALSE;
                break;
            }
            case GST_MESSAGE_EOS: {
                g_print("EOS Reached\n");

                is_running = FALSE;
                break;
            }
            default: {
                g_printerr("Should not end up here\n");
                break;
            }
        }

        gst_message_unref(message);
    } while (is_running);

exit:
    g_object_unref(bus);
    gst_element_set_state(state.pipeline, GST_STATE_NULL);
    g_object_unref(state.pipeline);
    return 0;
}

static void pad_added_signal (GstElement *self, GstPad *new_pad, State* state) {
    GstPad* converter_sink = NULL;

    GstCaps* new_pad_caps = gst_pad_get_current_caps(new_pad);
    GstStructure* new_pad_caps_structure = gst_caps_get_structure(new_pad_caps, 0);
    const char* new_pad_type = gst_structure_get_name(new_pad_caps_structure);

    if (g_str_has_prefix(new_pad_type, "audio/x-raw")) {
        converter_sink = gst_element_get_static_pad(state->audio_converter, "sink");
        
        // do nothing if already linked
        if (gst_pad_is_linked(converter_sink)) {
            g_print("Audio pad is already linked\n");
            goto exit;
        }

        // link new_pad output to audio converter sink
        GstPadLinkReturn ret = gst_pad_link(new_pad, converter_sink);
        if (GST_PAD_LINK_FAILED(ret)) {
            g_printerr("Could not link audio pad\n");
        }
    } else if (!state->is_only_audio && g_str_has_prefix(new_pad_type, "video/x-raw")) {
        converter_sink = gst_element_get_static_pad(state->video_converter, "sink");

        if (gst_pad_is_linked(converter_sink)) {
            g_print("Video pad is already linked\n");
            goto exit;
        }

        // link new_pad output to video converter sink
        GstPadLinkReturn ret = gst_pad_link(new_pad, converter_sink);
        if (GST_PAD_LINK_FAILED(ret)) {
            g_printerr("Could not link video pad\n");
        }
    }

exit:
    if (converter_sink) {
        gst_object_unref(converter_sink);
    }
    if (new_pad_caps) {
        gst_caps_unref(new_pad_caps);
    }
}   


static gboolean is_audio_only(const char* filepath) {
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
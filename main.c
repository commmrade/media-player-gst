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
#include <getopt.h>
#include "utils.h"

typedef struct State {
    GstElement* pipeline;
    GstElement* source;
    
    GstElement* audio_converter;

    GstElement* video_converter;
    
    GstElement* audio_resampler;

    GstElement* audio_sink;
    GstElement* video_sink;

    // Filters, effects
    GstElement* volume;
    GstElement* panorama; // balance left-ear right-ear effect
    GstElement* pass_filter; // low pass, high pass

    gboolean is_audio_only;

    gboolean is_running;
} State;


static void handle_message(GstMessage *message, State *state);
static void pad_added_signal (GstElement *self, GstPad *new_pad, State* state);


static gboolean state_add_and_link_elements(State* state, Settings* settings) {
     gst_bin_add_many(GST_BIN(state->pipeline), state->source, state->audio_converter, state->audio_resampler, state->audio_sink,/* Filters */ state->volume, state->panorama, NULL);
    if (settings->pass_type != PassNone) {
        gst_bin_add_many(GST_BIN(state->pipeline), state->pass_filter, NULL);
    }

    if (!state->is_audio_only) {
        gst_bin_add_many(GST_BIN(state->pipeline), state->video_converter, state->video_sink, NULL);
    }

    // Link audio stuff
    if (settings->pass_type != PassNone && !gst_element_link_many(state->audio_converter, state->audio_resampler, /* audio filters, video effects -> */ state->volume, state->panorama, state->pass_filter, /* <- filters end*/ state->audio_sink, NULL)) {
        g_printerr("Unable to link audio elements\n");
        g_object_unref(state->pipeline);
        return FALSE;
    } else if (settings->pass_type == PassNone && !gst_element_link_many(state->audio_converter, state->audio_resampler, /* audio filters, video effects -> */ state->volume, state->panorama, /* <- filters end*/ state->audio_sink, NULL)) {
        g_printerr("Unable to link audio elements\n");
        g_object_unref(state->pipeline);
        return FALSE;
    }
    

    // Link video stuff
    if (!state->is_audio_only) {
        if (!gst_element_link_many(state->video_converter, state->video_sink, NULL)) {
            g_printerr("Unable to link audio elements\n");
            g_object_unref(state->pipeline);
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean state_create_all_elements(State* state, Settings* settings) {
    state->source = gst_element_factory_make("uridecodebin", "source");
    state->audio_converter = gst_element_factory_make("audioconvert", "audio-converter");
    state->audio_resampler = gst_element_factory_make("audioresample", "audio-resampler");
    state->audio_sink = gst_element_factory_make("autoaudiosink", "audio-sink");

    // Create filters, effects
    // ...
    state->volume = gst_element_factory_make("volume", "volume-controller");
    state->panorama = gst_element_factory_make("audiopanorama", "panorama");

    if (settings->pass_type != PassNone) {
        state->pass_filter = gst_element_factory_make("audiocheblimit", "passfilter");
        if (!state->pass_filter) {
            g_printerr("Could not create pass filter");
            return FALSE;
        }
    }
    
    if (!state->is_audio_only) {
        state->video_converter = gst_element_factory_make("videoconvert", "video-converter");
        state->video_sink = gst_element_factory_make("autovideosink", "video-sink");
    }
    
    if (!state->source || !state->audio_converter || !state->audio_resampler || !state->audio_sink || /* Filters */ !state->volume || !state->panorama) {
        g_printerr("Could not create all elements\n");
        return FALSE;
    }
    if (!state->is_audio_only) {
        if (!state->video_converter || !state->video_sink) {
            g_printerr("Could not create all video elements\n");
            return FALSE;
        }
    }
    return TRUE;
}

static void state_setup_filter_values_from_settings(State* state, Settings* settings) {
    g_object_set(state->volume, "volume", settings->volume, NULL);
    g_object_set(state->panorama, "panorama", settings->balance, NULL);
    if (settings->pass_type != PassNone) {
        g_object_set(state->pass_filter, "mode", settings->pass_type, NULL);
        g_object_set(state->pass_filter, "cutoff", settings->pass_cutoff, NULL);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        g_print("Usage: ./exec [arguments]\n");
        return -1;
    }

    gst_init(&argc, &argv);

    State state;
    Settings settings;

    // TODO: get back auto detection by file extension
    // gboolean is_only_audio = is_audio_only(argv[1]);
    // settings.is_audio_only = is_only_audio;


    // Cmd arguments must override automatic media type detection
    int error;
    settings_parse_cli(&settings, &argc, &argv, &error);
    if (error == -1) {
        g_printerr("Error when parsing arguments encountered\n");
        return -1;
    }
    state.is_audio_only = settings.is_audio_only;
    
    if (!state_create_all_elements(&state, &settings)) {
        return -1;
    }
    
    state.pipeline = gst_pipeline_new("tiktok-pipeline");
    if (!state.pipeline) {
        g_printerr("Could not create a pipeline\n");
        return -1;
    }

    // Load a file, use abslute path
    // TODO: Support files from web
    char* full_path = settings_get_filepath(&settings);
    if (!file_exists(settings.path)) { // only if its a file
        g_printerr("Such file does not exist\n");
        gst_element_set_state(state.pipeline, GST_STATE_NULL);
        g_object_unref(state.pipeline);
        return -1;
    }

    g_object_set(state.source, "uri", full_path, NULL);

    // Setup filters
    state_setup_filter_values_from_settings(&state, &settings);

    // Add elemnts to pipeline and link
    if (!state_add_and_link_elements(&state, &settings)) {
        gst_element_set_state(state.pipeline, GST_STATE_NULL);
        g_object_unref(state.pipeline);
        return -1;
    }

    // link source to pad added handler
    g_signal_connect(state.source, "pad-added", G_CALLBACK(pad_added_signal), &state);
    
    // Start playing
    GstStateChangeReturn ret = gst_element_set_state(state.pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Was unable to change state\n");
        gst_element_set_state(state.pipeline, GST_STATE_NULL);
        g_object_unref(state.pipeline);
        return -1;
    }
    GstBus* bus = NULL;
    GstMessage* message = NULL;
    bus = gst_element_get_bus(state.pipeline);
    do {
        message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);
        if (!message) continue;
        handle_message(message, &state);
        gst_message_unref(message);
    } while (state.is_running);
exit:
    g_object_unref(bus);
    gst_element_set_state(state.pipeline, GST_STATE_NULL);
    g_object_unref(state.pipeline);
    free(full_path);
    free(settings.path);
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
    } else if (!state->is_audio_only && g_str_has_prefix(new_pad_type, "video/x-raw")) {
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

static void handle_message(GstMessage *message, State *state) {
    switch (GST_MESSAGE_TYPE(message)) {
        case GST_MESSAGE_ERROR: {
            GError* err;
            gchar* debug_info;

            gst_message_parse_error(message, &err, &debug_info);
            g_printerr("Error from %s: Message: %s\n", GST_OBJECT_NAME(message->src), err->message);

            g_clear_error(&err);
            g_free(debug_info);
            state->is_running = FALSE;
            break;
        }
        case GST_MESSAGE_EOS: {
            g_print("EOS Reached\n");

            state->is_running = FALSE;
            break;
        }
        default: {
            g_printerr("Should not end up here\n");
            break;
        }
    }
}
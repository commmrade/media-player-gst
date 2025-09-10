#include "state.h"
#include "gst/gstutils.h"

void state_add_elements(State* state, Settings* settings) {
    gst_bin_add_many(GST_BIN(state->pipeline), state->source, state->audio_converter, state->audio_resampler, state->audio_sink,/* Filters */ state->volume, state->panorama, NULL);
    if (settings->pass_type != PassNone) {
        gst_bin_add_many(GST_BIN(state->pipeline), state->pass_filter, NULL);
    }

    if (!state->is_audio_only) {
        gst_bin_add_many(GST_BIN(state->pipeline), state->video_converter, state->video_sink, NULL);
    }
}
gboolean state_link_elements(State* state, Settings* settings) {
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

gboolean state_create_all_elements(State* state, Settings* settings) {
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

void state_setup_filter_values_from_settings(State* state, Settings* settings) {
    g_object_set(state->volume, "volume", settings->volume, NULL);
    g_object_set(state->panorama, "panorama", settings->balance, NULL);
    if (settings->pass_type != PassNone) {
        g_object_set(state->pass_filter, "mode", settings->pass_type, NULL);
        g_object_set(state->pass_filter, "cutoff", settings->pass_cutoff, NULL);
    }
}
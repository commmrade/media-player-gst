#include "state.h"
#include "glib.h"
#include "gst/gstbin.h"
#include "gst/gstelement.h"
#include "gst/gstelementfactory.h"
#include "gst/gstutils.h"
#include <stdio.h>

void state_add_elements(State* state, Settings* settings) {
    gst_bin_add_many(GST_BIN(state->pipeline), state->source, state->audio_converter, state->audio_resampler, state->audio_sink, NULL);
    if (settings->has_volume) {
        gst_bin_add(GST_BIN(state->pipeline), state->volume);
    }
    if (settings->has_panorama) {
        gst_bin_add(GST_BIN(state->pipeline), state->panorama);
    }
    if (settings->has_echo) {
        gst_bin_add(GST_BIN(state->pipeline), state->audio_echo);
    }
    if (settings->pass_type != PassNone) {
        gst_bin_add(GST_BIN(state->pipeline), state->pass_filter);
    }
    if (settings->has_pitch) {
        gst_bin_add(GST_BIN(state->pipeline), state->pitch);
    }

    if (!state->is_audio_only) {
        gst_bin_add_many(GST_BIN(state->pipeline), state->video_converter, state->video_sink, NULL);
    }
}

gboolean state_link_elements(State* state, Settings* settings) {
    GPtrArray* audio_elements = g_ptr_array_new();

    // First add must-have elements for audio
    g_ptr_array_add(audio_elements, state->audio_converter);
    g_ptr_array_add(audio_elements, state->audio_resampler);

    if (settings->has_volume) {
        g_ptr_array_add(audio_elements, state->volume);
    }
    if (settings->has_panorama) {
        g_ptr_array_add(audio_elements, state->panorama);
    }
    if (settings->has_echo) {
        g_ptr_array_add(audio_elements, state->audio_echo);
    }
    if (settings->pass_type != PassNone) {
        g_ptr_array_add(audio_elements, state->pass_filter);
    }
    if (settings->has_pitch) {
        g_ptr_array_add(audio_elements, state->pitch);
    }

    g_ptr_array_add(audio_elements, state->audio_sink);

    for (int i = 0; i < audio_elements->len - 1; ++i) {
        GstElement* src = g_ptr_array_index(audio_elements, i);
        GstElement* dst = g_ptr_array_index(audio_elements, i + 1);

        if (!gst_element_link_many(src, dst, NULL)) {
            g_printerr("Was unable to link %s and %s\n", gst_element_get_name(src), gst_element_get_name(dst));
            g_object_unref(state->pipeline);
            g_ptr_array_free(audio_elements, FALSE);
            return FALSE;
        }
    }

    g_ptr_array_free(audio_elements, FALSE);

    // Video path
    if (!state->is_audio_only) {
        if (!gst_element_link_many(state->video_converter, state->video_sink, NULL)) {
            g_printerr("Unable to link video elements\n");
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

    // -------------------------------------------------------------
    // Adding new filter checklist:
    // - [ ] add new element to settings (has_<filter>, <filter>_<value>)
    // - [ ] parse cli arguments
    // - [ ] create a gstelement in create_all_elements
    // - [ ] add new element to the pipeline
    // - [ ] link new element
    // - [ ] g_object_set new element
    // -------------------------------------------------------------

    if (settings->has_volume) {
        state->volume = gst_element_factory_make("volume", "volume-controller-filter");
        if (!state->volume) {
            g_printerr("Could not create volume filter, skipping..\n");
        }
    }
    if (settings->has_panorama) {
        state->panorama = gst_element_factory_make("audiopanorama", "panorama-filter");
        if (!state->panorama) {
            g_printerr("Could not create panorama filter, skipping..\n");
        }
    }
    if (settings->pass_type != PassNone) {
        state->pass_filter = gst_element_factory_make("audiocheblimit", "passfilter");
        if (!state->pass_filter) {
            g_printerr("Could not create pass filter, skipping...\n");
        }
    }
    if (settings->has_echo) {
        state->audio_echo = gst_element_factory_make("audioecho", "reverb-filter");
        if (!state->audio_echo) {
            g_printerr("Could not create echo filter, skipping...\n");
        }
    }
    if (settings->has_pitch) {
        state->pitch = gst_element_factory_make("pitch", "pitch-filter");
        if (!state->pitch) {
            g_printerr("Could not create pitch filter, skipping...\nn");
        }
    }
    

    // Video stuff
    if (!state->is_audio_only) {
        state->video_converter = gst_element_factory_make("videoconvert", "video-converter");
        state->video_sink = gst_element_factory_make("autovideosink", "video-sink");
    }
    
    if (!state->source || !state->audio_converter || !state->audio_resampler || !state->audio_sink) {
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
    if (settings->has_volume) {
        g_object_set(state->volume, "volume", settings->volume, NULL);
    }

    if (settings->has_panorama) {
        g_object_set(state->panorama, "panorama", settings->balance, NULL);
    }

    if (settings->pass_type != PassNone) {
        g_object_set(state->pass_filter, "mode", settings->pass_type, "cutoff", settings->pass_cutoff, NULL);
    }

    if (settings->has_echo) {
        g_object_set(state->audio_echo, "delay", settings->echo_delay, "feedback", settings->echo_feedback, "intensity", settings->echo_intensity, NULL);
    }
    if (settings->has_pitch) {
        g_object_set(state->pitch, "rate", settings->pitch_rate, "pitch", settings->pitch_pitch, NULL);
    }
}

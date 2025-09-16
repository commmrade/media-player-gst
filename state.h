#ifndef __STATE_H
#define __STATE_H


#include "gst/gstelement.h"
#include "settings.h"

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
    GstElement* audio_echo; // for echo | reverb
    GstElement* pitch; // for audio speed (aspeed) && pitch
    //

    gboolean is_audio_only;

    gboolean is_running;
} State;


gboolean state_link_elements(State* state, Settings* settings);
void state_add_elements(State* state, Settings* Settings);

gboolean state_create_all_elements(State* state, Settings* settings);
void state_setup_filter_values_from_settings(State* state, Settings* settings);

#endif
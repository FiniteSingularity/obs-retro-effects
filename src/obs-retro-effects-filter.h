#pragma once

#include <obs-module.h>

#include "version.h"
#include "obs-retro-effects.h"
#include "obs-utils.h"

static const char *retro_effects_filter_name(void *unused);
static void *retro_effects_filter_create(obs_data_t *settings,
					 obs_source_t *source);
static void retro_effects_filter_destroy(void *data);
static uint32_t retro_effects_filter_width(void *data);
static uint32_t retro_effects_filter_height(void *data);
static void retro_effects_filter_update(void *data, obs_data_t *settings);
static void retro_effects_filter_video_render(void *data, gs_effect_t *effect);
static obs_properties_t *retro_effects_filter_properties(void *data);
static void retro_effects_filter_video_tick(void *data, float seconds);
static void retro_effects_filter_defaults(obs_data_t *settings);
static void get_input_source(retro_effects_filter_data_t *filter);
static void draw_output(retro_effects_filter_data_t *filter);
static void retro_effects_render_filter(retro_effects_filter_data_t *filter);
static void load_output_effect(retro_effects_filter_data_t *filter);
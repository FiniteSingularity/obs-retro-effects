#include <obs-module.h>
#include "../obs-retro-effects.h"

#define SCANLINES_PROFILE_SIN 0
#define SCANLINES_PROFILE_SIN_LABEL "RetroEffects.Scanlines.Profile.Sin"
#define SCANLINES_PROFILE_SQUARE 1
#define SCANLINES_PROFILE_SQUARE_LABEL "RetroEffects.Scanlines.Profile.Square"
#define SCANLINES_PROFILE_SAWTOOTH 2
#define SCANLINES_PROFILE_SAWTOOTH_LABEL "RetroEffects.Scanlines.Profile.SawTooth"
#define SCANLINES_PROFILE_SMOOTHSTEP 3
#define SCANLINES_PROFILE_SMOOTHSTEP_LABEL "RetroEffects.Scanlines.Profile.SmoothStep"
#define SCANLINES_PROFILE_TRIANGULAR 4
#define SCANLINES_PROFILE_TRIANGULAR_LABEL "RetroEffects.Scanlines.Profile.Triangular"

struct scan_lines_filter_data;
typedef struct scan_lines_filter_data scan_lines_filter_data_t;

struct scan_lines_filter_data {
	gs_effect_t *effect_scan_lines;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_period;
	gs_eparam_t *param_offset;
	gs_eparam_t *param_intensity;

	bool loading_effect;
	bool reload_effect;
	float speed;
	float period;
	float offset;
	float intensity;
	uint8_t profile;
};

extern void scan_lines_create(retro_effects_filter_data_t *filter);
extern void scan_lines_destroy(retro_effects_filter_data_t *filter);
extern void scan_lines_unset_settings(retro_effects_filter_data_t* filter);

extern void scan_lines_filter_video_render(retro_effects_filter_data_t *data);
extern void scan_lines_filter_properties(retro_effects_filter_data_t *data,
					 obs_properties_t *props);
extern void scan_lines_filter_defaults(obs_data_t *settings);
extern void scan_lines_filter_update(retro_effects_filter_data_t *data,
				     obs_data_t *settings);
extern void scan_lines_filter_video_tick(retro_effects_filter_data_t *data,
				  float seconds);
static void scan_lines_set_functions(retro_effects_filter_data_t *filter);
static void scan_lines_load_effect(scan_lines_filter_data_t *filter);

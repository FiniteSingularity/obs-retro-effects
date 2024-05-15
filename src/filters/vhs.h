#include <obs-module.h>
#include "../obs-retro-effects.h"

struct vhs_filter_data;
typedef struct vhs_filter_data vhs_filter_data_t;

struct vhs_filter_data {
	gs_effect_t *effect_vhs;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_wrinkle_size;
	gs_eparam_t *param_wrinkle_position;
	gs_eparam_t *param_pop_line_prob;
	gs_eparam_t *param_time;
	gs_eparam_t *param_hs_primary_offset;
	gs_eparam_t *param_hs_primary_thickness;
	gs_eparam_t *param_hs_secondary_vert_offset;
	gs_eparam_t *param_hs_secondary_horiz_offset;
	gs_eparam_t *param_hs_secondary_thickness;
	gs_eparam_t *param_horizontal_offset;

	float tape_wrinkle_occurrence;
	float tape_wrinkle_size;
	float tape_wrinkle_duration;
	float wrinkle_position;
	float time;
	float pop_line_prob;
	float hs_primary_offset;
	float hs_primary_thickness;
	float hs_secondary_horiz_offset;
	float hs_secondary_vert_offset;
	float hs_secondary_thickness;
	float frame_jitter_min_size;
	float frame_jitter_max_size;
	float frame_jitter_min_period;
	float frame_jitter_max_period;
	float frame_jitter_min_interval;
	float frame_jitter_max_interval;

	float jitter_size;
	float jitter_period;
	float jitter_start_time;
	float target_jitter;
	float current_jitter;
	float jitter_step;
	float local_time;

	bool jitter;
	bool active_wrinkle;
	bool loading_effect;
	bool reload_effect;
};

extern void vhs_create(retro_effects_filter_data_t *filter);
extern void vhs_destroy(retro_effects_filter_data_t *filter);
extern void vhs_filter_video_render(retro_effects_filter_data_t *data);
extern void vhs_filter_properties(retro_effects_filter_data_t *data,
				  obs_properties_t *props);
extern void vhs_filter_video_tick(retro_effects_filter_data_t *data, float seconds);

extern void vhs_filter_defaults(obs_data_t *settings);
extern void vhs_filter_update(retro_effects_filter_data_t *data,
			      obs_data_t *settings);
static void vhs_set_functions(retro_effects_filter_data_t *filter);
static void vhs_load_effect(vhs_filter_data_t *filter);

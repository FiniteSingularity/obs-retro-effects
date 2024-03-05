#include <obs-module.h>
#include <graphics/image-file.h>

#include "../obs-retro-effects.h"

struct matrix_rain_filter_data;
typedef struct matrix_rain_filter_data matrix_rain_filter_data_t;

struct matrix_rain_filter_data {
	gs_effect_t *effect_matrix_rain;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_font_image;
	gs_eparam_t *param_font_texture_size;
	gs_eparam_t *param_font_texture_num_chars;
	gs_eparam_t *param_scale;
	gs_eparam_t *param_noise_shift;
	gs_eparam_t *param_local_time;
	gs_eparam_t *param_colorize;
	gs_eparam_t *param_text_color;
	gs_eparam_t *param_background_color;

	gs_image_file_t *font_image;

	struct vec2 font_texture_size;
	float font_num_chars;
	float scale;
	float noise_shift;
	float local_time;
	bool colorize;
	struct vec4 text_color;
	struct vec4 background_color;

	bool loading_effect;
	bool reload_effect;
};

extern void matrix_rain_create(retro_effects_filter_data_t *filter);
extern void matrix_rain_destroy(retro_effects_filter_data_t *filter);
extern void matrix_rain_filter_video_tick(retro_effects_filter_data_t *data,
				   float seconds);
extern void matrix_rain_filter_video_render(retro_effects_filter_data_t *data);
extern void matrix_rain_filter_properties(retro_effects_filter_data_t *data,
					  obs_properties_t *props);
extern void matrix_rain_filter_defaults(obs_data_t *settings);
extern void matrix_rain_filter_update(retro_effects_filter_data_t *data,
				      obs_data_t *settings);
static void matrix_rain_set_functions(retro_effects_filter_data_t *filter);
static void matrix_rain_load_effect(matrix_rain_filter_data_t *filter);


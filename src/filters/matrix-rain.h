#include <obs-module.h>
#include <graphics/image-file.h>

#include "../obs-retro-effects.h"
#include "../blur/bloom.h"

#define MATRIX_APP_INFO                                                                        \
	"<a href=\"https://finitesingularity.github.io/matrix-rain-tex-gen/\">Click Here</a> " \
	"to use the matrix-rain texture web app to generate a texture file. Set your font, size " \
        "and other parameters, then click the Generate Texture button, save the texture to your " \
        "computer, and select it in the 'Texture File' field above. Then set the 'Character Count' " \
        "field to the number of characters in your texture."

struct matrix_rain_filter_data;
typedef struct matrix_rain_filter_data matrix_rain_filter_data_t;

struct matrix_rain_filter_data {
	gs_effect_t *effect_matrix_rain;

	gs_texrender_t *matrix_rain_texrender;

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
	gs_eparam_t *param_min_brightness;
	gs_eparam_t *param_max_brightness;
	gs_eparam_t *param_min_fade_value;
	gs_eparam_t *param_active_rain_brightness;
	gs_eparam_t *param_fade_distance;

	gs_image_file_t *font_image;

	obs_data_t *textures_data;

	struct vec2 font_texture_size;
	uint32_t char_set;
	float font_num_chars;
	float scale;
	float noise_shift;
	float local_time;
	float min_brightness;
	float max_brightness;
	float min_fade_value;
	float active_rain_brightness;
	float fade_distance;
	float speed_factor;
	float bloom_radius;
	float bloom_threshold;
	float bloom_intensity;
	bool colorize;
	struct vec4 text_color;
	struct vec4 background_color;
	struct dstr custom_texture_file;

	bool loading_effect;
	bool reload_effect;
};

extern void matrix_rain_create(retro_effects_filter_data_t *filter);
extern void matrix_rain_destroy(retro_effects_filter_data_t *filter);
extern void matrix_rain_unset_settings(retro_effects_filter_data_t* filter);


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

static bool setting_char_set_modified(obs_properties_t *props,
				      obs_property_t *property,
				      obs_data_t *settings);
void set_character_texture(matrix_rain_filter_data_t *filter,
			   const char *filename, float num_chars);

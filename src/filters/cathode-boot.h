#include <obs-module.h>
#include "../obs-retro-effects.h"

struct cathode_boot_filter_data;
typedef struct cathode_boot_filter_data cathode_boot_filter_data_t;

struct cathode_boot_filter_data {
	gs_effect_t *effect_cathode_boot;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;

	bool loading_effect;
	bool reload_effect;
};

extern void cathode_boot_create(retro_effects_filter_data_t *filter);
extern void cathode_boot_destroy(retro_effects_filter_data_t *filter);
extern void cathode_boot_filter_video_render(retro_effects_filter_data_t *data);
extern void cathode_boot_filter_properties(retro_effects_filter_data_t *data,
					   obs_properties_t *props);
extern void cathode_boot_filter_defaults(obs_data_t *settings);
extern void cathode_boot_filter_update(retro_effects_filter_data_t *data,
				       obs_data_t *settings);
static void cathode_boot_set_functions(retro_effects_filter_data_t *filter);
static void cathode_boot_load_effect(cathode_boot_filter_data_t *filter);
static bool cathode_boot_type_modified(obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings);
static bool cathode_boot_bayer_size_modified(void *data,
					     obs_properties_t *props,
					     obs_property_t *p,
					     obs_data_t *settings);

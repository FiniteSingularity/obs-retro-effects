#include <obs-module.h>

#include "version.h"

extern struct obs_source_info obs_retro_effects_filter;

OBS_DECLARE_MODULE();

OBS_MODULE_USE_DEFAULT_LOCALE("obs-retro-effects", "en-US");

OBS_MODULE_AUTHOR("FiniteSingularity");

bool obs_module_load(void)
{
	blog(LOG_INFO, "[Retro Effects] loaded version %s", PROJECT_VERSION);
	obs_register_source(&obs_retro_effects_filter);
	return true;
}

void obs_module_unload(void) {}

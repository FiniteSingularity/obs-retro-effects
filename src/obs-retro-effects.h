#pragma once

#include <obs-module.h>
#include "base-filter.h"

#define PLUGIN_INFO                                                                                               \
	"<a href=\"https://github.com/finitesingularity/obs-retro-effects/\">Retro Effects</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

#define RETRO_FILTER_FRAME_SKIP 1
#define RETRO_FILTER_FRAME_SKIP_LABEL "RetroEffects.FrameSkip"
#define RETRO_FILTER_INTERLACE 2
#define RETRO_FILTER_INTERLACE_LABEL "RetroEffects.Interlace"
#define RETRO_FILTER_CA 3
#define RETRO_FILTER_CA_LABEL "RetroEffects.ChromaticAberration"
#define RETRO_FILTER_POSTERIZE 4
#define RETRO_FILTER_POSTERIZE_LABEL "RetroEffects.Posterize"
#define RETRO_FILTER_DITHER 5
#define RETRO_FILTER_DITHER_LABEL "RetroEffects.Dither"
#define RETRO_FILTER_CRT 6
#define RETRO_FILTER_CRT_LABEL "RetroEffects.CRT"
#define RETRO_FILTER_NTSC 7
#define RETRO_FILTER_NTSC_LABEL "RetroEffects.NTSC"
#define RETRO_FILTER_CATHODE_BOOT 8
#define RETRO_FILTER_CATHODE_BOOT_LABEL "RetroEffects.CathodeBoot"
#define RETRO_FILTER_MATRIX_RAIN 9
#define RETRO_FILTER_MATRIX_RAIN_LABEL "RetroEffects.MatrixRain"
#define RETRO_FILTER_CODEC 10
#define RETRO_FILTER_CODEC_LABEL "RetroEffects.Codec"
#define RETRO_FILTER_VHS 11
#define RETRO_FILTER_VHS_LABEL "RetroEffects.VHS"
#define RETRO_FILTER_BLOOM 12
#define RETRO_FILTER_BLOOM_LABEL "RetroEffects.Bloom"
#define RETRO_FILTER_SCANLINES 13
#define RETRO_FILTER_SCANLINES_LABEL "RetroEffects.Scanlines"
#define RETRO_FILTER_DIGITAL_GLITCH 14
#define RETRO_FILTER_DIGITAL_GLITCH_LABEL "RetroEffects.DigitalGlitch"
#define RETRO_FILTER_ANALOG_GLITCH 15
#define RETRO_FILTER_ANALOG_GLITCH_LABEL "RetroEffects.AnalogGlitch"

struct retro_effects_filter_data;
typedef struct retro_effects_filter_data retro_effects_filter_data_t;

typedef struct blur_data blur_data_t;
typedef struct bloom_data bloom_data_t;

struct retro_effects_filter_data {
	base_filter_data_t *base;
	void *active_filter_data;
	blur_data_t *blur_data;
	bloom_data_t *bloom_data;

	int active_filter;
	bool initial_load;

	// Filter Function Pointers
	void (*filter_properties)(retro_effects_filter_data_t *,
				  obs_properties_t *); // Properties UI setup.
	void (*filter_defaults)(obs_data_t *);         // defaults
	void (*filter_update)(retro_effects_filter_data_t *,
			      obs_data_t *);                        // update
	void (*filter_video_render)(retro_effects_filter_data_t *); // render
	void (*filter_destroy)(retro_effects_filter_data_t *);      // destroy
	void (*filter_video_tick)(retro_effects_filter_data_t *,
				  float); // Video Tick

	// Parameters go here
	int frames_to_skip;
	int frames_skipped;
};

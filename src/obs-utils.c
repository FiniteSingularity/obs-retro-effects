#include "obs-utils.h"

gs_texrender_t *create_or_reset_texrender(gs_texrender_t *render)
{
	if (!render) {
		render = gs_texrender_create(GS_RGBA, GS_ZS_NONE);
	} else {
		gs_texrender_reset(render);
	}
	return render;
}

void set_blending_parameters()
{
	gs_blend_state_push();
	gs_reset_blend_state();
	gs_enable_blending(false);
	gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
}

void set_render_parameters()
{
	// Culling
	gs_set_cull_mode(GS_NEITHER);

	// Enable all colors.
	gs_enable_color(true, true, true, true);

	// Depth- No depth test.
	gs_enable_depth_test(false);
	gs_depth_function(GS_ALWAYS);

	// Setup Stencil- No test, no write.
	gs_enable_stencil_test(false);
	gs_enable_stencil_write(false);
	gs_stencil_function(GS_STENCIL_BOTH, GS_ALWAYS);
	gs_stencil_op(GS_STENCIL_BOTH, GS_ZERO, GS_ZERO, GS_ZERO);
}

bool add_source_to_list(void *data, obs_source_t *source)
{
	obs_property_t *p = data;
	const char *name = obs_source_get_name(source);
	size_t count = obs_property_list_item_count(p);
	size_t idx = 0;
	while (idx < count &&
	       strcmp(name, obs_property_list_item_string(p, idx)) > 0)
		idx++;
	obs_property_list_insert_string(p, idx, name, name);
	return true;
}

// Loads the shader file at `effect_file_path` into *effect
gs_effect_t *load_shader_effect(gs_effect_t *effect,
				const char *effect_file_path)
{
	if (effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(effect);
		effect = NULL;
		obs_leave_graphics();
	}
	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, effect_file_path);
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;

	obs_enter_graphics();
	effect = gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);

	if (effect == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load .effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	}

	dstr_free(&filename);

	return effect;
}

// Performs loading of shader from file.  Properly includes #include directives.
char *load_shader_from_file(const char *file_name)
{
	char *file = os_quick_read_utf8_file(file_name);
	if (file == NULL)
		return NULL;
	char **lines = strlist_split(file, '\n', true);
	struct dstr shader_file;
	dstr_init(&shader_file);

	size_t line_i = 0;
	while (lines[line_i] != NULL) {
		char *line = lines[line_i];
		line_i++;
		if (strncmp(line, "#include", 8) == 0) {
			// Open the included file, place contents here.
			char *pos = strrchr(file_name, '/');
			const size_t length = pos - file_name + 1;
			struct dstr include_path = {0};
			dstr_ncopy(&include_path, file_name, length);
			char *start = strchr(line, '"') + 1;
			char *end = strrchr(line, '"');

			dstr_ncat(&include_path, start, end - start);
			char *abs_include_path =
				os_get_abs_path_ptr(include_path.array);
			char *file_contents =
				load_shader_from_file(abs_include_path);
			dstr_cat(&shader_file, file_contents);
			dstr_cat(&shader_file, "\n");
			bfree(abs_include_path);
			bfree(file_contents);
			dstr_free(&include_path);
		} else {
			// else place current line here.
			dstr_cat(&shader_file, line);
			dstr_cat(&shader_file, "\n");
		}
	}

	bfree(file);
	strlist_free(lines);
	return shader_file.array;
}

void setting_visibility(const char *prop_name, bool visible,
			obs_properties_t *props)
{
	obs_property_t *p = obs_properties_get(props, prop_name);
	obs_property_set_enabled(p, visible);
	obs_property_set_visible(p, visible);
}

void texrender_set_texture(gs_texture_t *source, gs_texrender_t *dest)
{
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	uint32_t w = gs_texture_get_width(source);
	uint32_t h = gs_texture_get_height(source);

	gs_eparam_t *image = gs_effect_get_param_by_name(pass_through, "image");
	gs_effect_set_texture(image, source);
	set_blending_parameters();
	if (gs_texrender_begin(dest, w, h)) {
		gs_ortho(0.0f, (float)w, 0.0f, (float)h, -100.0f, 100.0f);
		while (gs_effect_loop(pass_through, "Draw"))
			gs_draw_sprite(source, 0, w, h);
		gs_texrender_end(dest);
	}
	gs_blend_state_pop();
}

void get_input_source(base_filter_data_t *base)
{
	// Use the OBS default effect file as our effect.
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	// Set up our color space info.
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(base->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	// Set up our input_texrender to catch the output texture.
	base->input_texrender =
		create_or_reset_texrender(base->input_texrender);

	// Start the rendering process with our correct color space params,
	// And set up your texrender to recieve the created texture.
	if (obs_source_process_filter_begin_with_color_space(
		    base->context, format, source_space,
		    OBS_NO_DIRECT_RENDERING) &&
	    gs_texrender_begin(base->input_texrender,
			       base->width, base->height)) {

		set_blending_parameters();
		gs_ortho(0.0f, (float)base->width, 0.0f,
			 (float)base->height, -100.0f, 100.0f);
		// The incoming source is pre-multiplied alpha, so use the
		// OBS default effect "DrawAlphaDivide" technique to convert
		// the colors back into non-pre-multiplied space.
		obs_source_process_filter_tech_end(base->context,
						   pass_through,
						   base->width,
						   base->height,
						   "DrawAlphaDivide");
		gs_texrender_end(base->input_texrender);
		gs_blend_state_pop();
		base->input_texture_generated = true;
	}
}

void draw_output(retro_effects_filter_data_t *filter)
{
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->base->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	if (!obs_source_process_filter_begin_with_color_space(
		    filter->base->context, format, source_space,
		    OBS_NO_DIRECT_RENDERING)) {
		return;
	}

	gs_texture_t *texture =
		gs_texrender_get_texture(filter->base->output_texrender);
	gs_effect_t *pass_through = filter->base->output_effect;

	if (filter->base->param_output_image) {
		gs_effect_set_texture(filter->base->param_output_image,
				      texture);
	}

	obs_source_process_filter_end(filter->base->context, pass_through,
				      filter->base->width,
				      filter->base->height);
}

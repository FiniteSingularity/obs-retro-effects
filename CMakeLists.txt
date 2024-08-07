# --- Detect if the plugin is build out of tree or not ---
if(CMAKE_PROJECT_NAME STREQUAL "obs-studio")
  set(BUILD_OUT_OF_TREE OFF)
  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    legacy_check()
  endif()
else()
  set(BUILD_OUT_OF_TREE ON)
  cmake_minimum_required(VERSION 3.18)
endif()

project(obs-retro-effects VERSION 1.0.0)
set(PROJECT_FULL_NAME "Retro Effects")

# Set new UUIDs when you start to create a new plugin.
set(MACOS_PACKAGE_UUID "3a15f7de-bc97-4652-b32a-15f1ce79417c")
set(MACOS_INSTALLER_UUID "598e7d68-00e7-4b30-8617-a3ad2e98985d")

add_library(${PROJECT_NAME} MODULE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/version.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/version.h)

target_sources(${PROJECT_NAME} PRIVATE
    src/base-filter.h
	src/obs-retro-effects-filter.c
	src/obs-retro-effects-plugin.c
	src/obs-retro-effects-filter.h
	src/obs-retro-effects.h
	src/obs-utils.c
	src/obs-utils.h
	src/blur/bloom.c
	src/blur/bloom.h
	src/blur/blur.c
	src/blur/blur.h
	src/blur/gaussian-kernel.c
	src/blur/gaussian-kernel.h
	src/filters/analog-glitch.c
	src/filters/analog-glitch.h
	src/filters/bloom-f.c
	src/filters/bloom-f.h
	src/filters/cathode-boot.c
	src/filters/cathode-boot.h
	src/filters/chromatic-aberration.c
	src/filters/chromatic-aberration.h
	src/filters/crt.c
	src/filters/crt.h
    src/filters/codec.c
	src/filters/codec.h
	src/filters/digital-glitch.c
	src/filters/digital-glitch.h
	src/filters/dither.c
	src/filters/dither.h
	src/filters/frame-skip.c
	src/filters/frame-skip.h
	src/filters/interlace.c
	src/filters/interlace.h
	src/filters/matrix-rain.c
	src/filters/matrix-rain.h
	src/filters/ntsc.c
	src/filters/ntsc.h
	src/filters/posterize.c
	src/filters/posterize.h
	src/filters/vhs.c
	src/filters/vhs.h
	src/filters/scan-lines.c
	src/filters/scan-lines.h
	src/version.h)

if(BUILD_OUT_OF_TREE)
	find_package(libobs REQUIRED)
	include(cmake/ObsPluginHelpers.cmake)
endif()
  
if(OS_WINDOWS)
	get_filename_component(ISS_FILES_DIR "${CMAKE_BINARY_DIR}\\..\\package" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_FILES_DIR}" ISS_FILES_DIR)
	get_filename_component(ISS_PACKAGE_DIR "${CMAKE_PACKAGE_PREFIX}\\.." ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_PACKAGE_DIR}" ISS_PACKAGE_DIR)
	get_filename_component(ISS_SOURCE_DIR "${PROJECT_SOURCE_DIR}" ABSOLUTE)
	file(TO_NATIVE_PATH "${ISS_SOURCE_DIR}" ISS_SOURCE_DIR)
	configure_file("installer.iss.in"
			"${PROJECT_BINARY_DIR}/installer.iss"
		)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/resource.rc.in ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
	target_sources(${PROJECT_NAME} PRIVATE 
		${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.rc)
elseif(OS_MACOS)
		set(MACOS_BUNDLEID "com.finitesingularity.${PROJECT_NAME}")
		set(MACOSX_PLUGIN_GUI_IDENTIFIER "${MACOS_BUNDLEID}")
		set(MACOSX_PLUGIN_BUNDLE_VERSION "${PROJECT_VERSION}")
		set(MACOSX_PLUGIN_SHORT_VERSION_STRING "1")
		configure_file(cmake/bundle/macos/installer-macos.pkgproj.in ${CMAKE_BINARY_DIR}/installer-macos.generated.pkgproj)
		target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
endif()

target_link_libraries(${PROJECT_NAME}
	OBS::libobs)

if(BUILD_OUT_OF_TREE)
    if(NOT LIB_OUT_DIR)
        set(LIB_OUT_DIR "/lib/obs-plugins")
    endif()
    if(NOT DATA_OUT_DIR)
        set(DATA_OUT_DIR "/share/obs/obs-plugins/${PROJECT_NAME}")
    endif()
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
    install(TARGETS ${PROJECT_NAME}
        LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/${LIB_OUT_DIR})
    install(DIRECTORY data/locale
        DESTINATION ${CMAKE_INSTALL_PREFIX}/${DATA_OUT_DIR})
    setup_plugin_target(${PROJECT_NAME})
else()
    if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
        set_target_properties_obs(${PROJECT_NAME} PROPERTIES FOLDER "plugins/finitesingularity" PREFIX "")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "plugins/finitesingularity")
        setup_plugin_target(${PROJECT_NAME})
    endif()
endif()


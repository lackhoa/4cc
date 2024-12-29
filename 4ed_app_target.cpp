/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 13.11.2015
 *
 * Application layer build target
 *
 */

// TOP

#define REMOVE_OLD_STRING
#define ED_CORRUPTION_CHECK 1

#include "4coder_version.h"
#include "4coder_table.h"
#include "4coder_events.h"
#include "4coder_doc_content_types.h"
#include "4coder_default_colors.h"
#define STATIC_LINK_API
#include "custom_api.gen.h"

#include "4coder_string_match.h"
#include "4coder_token.h"

#include "4coder_system_types.h"
#include "4ed_font_interface.h"

#include "4coder_profile.h"
#include "4coder_command_map.h"

#include "4ed_render_target.h"
#include "4ed.h"
#include "4ed_buffer_model.h"
#include "4ed_coroutine.h"

#include "4ed_dynamic_variables.h"

#include "4ed_buffer_model.h"
#include "4ed_translation.h"
#include "4ed_buffer.h"
#include "4ed_history.h"
#include "4ed_file.h"

#include "4ed_working_set.h"
#include "4ed_hot_directory.h"
#include "4ed_cli.h"
#include "4ed_layout.h"
#include "4ed_view.h"
#include "4ed_edit.h"
#include "4ed_text_layout.h"
#include "4ed_font_set.h"
#include "4coder_log_core.h"
#include "4ed_app_models.h"

#include "lexer_cpp.gen.h"
#include "4ed_api_definition.h"
#include "docs/4ed_doc_helper.h"

////////////////////////////////


#include "4coder_layout.cpp"
#include "4coder_string_match.cpp"
#include "4coder_stringf.cpp"
#include "4coder_system_helpers.cpp"
#include "4coder_profile.cpp"
#include "4coder_profile_static_enable.cpp"
#include "4coder_hash_functions.cpp"
#include "4coder_log.cpp"
#include "4coder_command_map.cpp"
#include "4coder_codepoint_map.cpp"

#include "4coder_token.cpp"
#include "4coder_token2.cpp"
#include "lexer_cpp.gen.cpp"

#include "4coder_log_core.cpp"
#include "4ed_coroutine.cpp"
#include "4ed_dynamic_variables.cpp"
#include "4ed_font_set.cpp"
#include "4ed_translation.cpp"
#include "4ed_render_target.cpp"
#include "4ed_app_models.cpp"
#include "4ed_buffer.cpp"
#include "4ed_string_matching.cpp"
#include "4ed_history.cpp"
#include "4ed_file.cpp"
#include "4ed_working_set.cpp"
#include "4ed_hot_directory.cpp"
#include "4ed_cli.cpp"
#include "4ed_layout.cpp"
#include "4ed_view.cpp"
#include "4ed_edit.cpp"
#include "4ed_text_layout.cpp"

#include "4coder_game.h"
#include "4ed.cpp"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#include "stb_image.h"

#define STATIC_LINK_API
#include "ed_api.gen.cpp"
#include "4coder_kv.cpp"
#include "4ed_api_implementation.cpp"

// BOTTOM

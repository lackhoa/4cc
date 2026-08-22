/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 07.02.2019
 *
 * Types used for edit operations
 *
 */

// TOP

#pragma once

struct Edit_Behaviors{
 b32 no_post_to_history;
 i64 pos_before_edit;
};
struct Edit_Behaviors2{
 b32 no_post_to_history;
 i64 pos_before_edit;
 b32 automated;
};

global b32 human_has_edited_after_macro;

// BOTTOM

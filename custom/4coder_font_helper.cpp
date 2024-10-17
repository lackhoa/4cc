/*
4coder_font_helper.cpp - Procedures for font setting operations
*/

// TOP

function Face_Description
get_buffer_face_description(App *app, Buffer_ID buffer){
    Face_ID current_id = get_face_id(app, buffer);
    Face_Description description = {};
    if (current_id != 0){
        description = get_face_description(app, current_id);
    }
    return(description);
}

function Face_Description
get_global_face_description(App *app){
    return(get_buffer_face_description(app, 0));
}

function b32
font_load_location_match(Font_Load_Location *a, Font_Load_Location *b){
    return(string_match(a->filename, b->filename));
}

function b32
face_load_parameters_match(Face_Load_Parameters *a, Face_Load_Parameters *b){
    return(block_compare(a, b, sizeof(*a)) == 0);
}

function b32
face_description_match(Face_Description *a, Face_Description *b){
    b32 result = false;
    if (font_load_location_match(&a->font, &b->font) &&
        face_load_parameters_match(&a->parameters, &b->parameters)){
        result = true;
    }
    return(result);
}

function Face_ID
face_id_from_font_load_target(App *app, Font_Load_Location *font){
    Face_ID largest_id = get_largest_face_id(app);
    Face_ID result = 0;
    for (Face_ID id = 1; id <= largest_id; ++id){
        Face_Description compare = get_face_description(app, id);
        if (font_load_location_match(&compare.font, font)){
            result = id;
            break;
        }
    }
    return(result);
}

function Face_ID
face_id_from_face_load_parameters(App *app, Face_Load_Parameters *parameters){
    Face_ID largest_id = get_largest_face_id(app);
    Face_ID result = 0;
    for (Face_ID id = 1; id <= largest_id; ++id){
        Face_Description compare = get_face_description(app, id);
        if (face_load_parameters_match(&compare.parameters, parameters)){
            result = id;
            break;
        }
    }
    return(result);
}

function Face_ID
face_id_from_description(App *app, Face_Description *description){
    Face_ID largest_id = get_largest_face_id(app);
    Face_ID result = 0;
    for (Face_ID id = 1; id <= largest_id; ++id){
        Face_Description compare = get_face_description(app, id);
        if (face_description_match(&compare, description)){
            result = id;
            break;
        }
    }
    return(result);
}

function b32
modify_global_face_by_description(App *app, Face_Description description){
    Face_ID face_id = get_face_id(app, 0);
    return(try_modify_face(app, face_id, &description));
}

function void
set_buffer_face_by_description(App *app, Buffer_ID buffer, Face_Description *description){
    Face_ID id = face_id_from_description(app, description);
    if (id == 0){
        id = try_create_new_face(app, description);
    }
    if (id != 0){
        buffer_set_face(app, buffer, id);
    }
}

function void
set_buffer_face_by_font_load_location(App *app, Buffer_ID buffer, Font_Load_Location *font){
    Face_Description description = get_buffer_face_description(app, buffer);
    description.font = *font;
    set_buffer_face_by_description(app, buffer, &description);
}

function void
set_buffer_face_by_face_load_parameters(App *app, Buffer_ID buffer, Face_Load_Parameters *parameters){
    Face_Description description = get_buffer_face_description(app, buffer);
    description.parameters = *parameters;
    set_buffer_face_by_description(app, buffer, &description);
}

// BOTTOM


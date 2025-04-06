//-#processed
enum Text_Object_Kind
{// NOTE See @Text_Object and @M_Text_Object
 Text_Object_None,
 Text_Object_Drawn,
 Text_Object_Image,
 Text_Object_Preset,
 
 Text_Object_Kind_Count,
};
global String text_object_kind_names[] = {
 strlit(""),
 strlit("Drawn"),
 strlit("Image"),
 strlit("Preset"),
};
static_assert(alen(text_object_kind_names) == Text_Object_Kind_Count);
//-
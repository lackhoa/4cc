/* 
 * Buffer seek descriptor constructors.
 */

// TOP

internal Buffer_Seek
seek_pos(i64 pos){
 Buffer_Seek result;
 result.type = buffer_seek_pos;
 result.pos = pos;
 return(result);
}

internal seek_line_col_return
seek_line_col(seek_line_col_params)
{
    Buffer_Seek result;
    result.type = buffer_seek_line_col;
    result.line = line;
    result.col = col;
    return(result);
}

// BOTTOM


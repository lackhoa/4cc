/* 
 * Buffer seek descriptor constructors.
 */

// TOP

function Buffer_Seek
seek_pos(i64 pos){
 Buffer_Seek result;
 result.type = buffer_seek_pos;
 result.pos = pos;
 return(result);
}

// BOTTOM


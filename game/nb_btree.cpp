//
global const i32 b_tree_max_keys = 3;

struct B_Tree
{
 i32 height;
 
 i32 key_count;
 i32 keys[b_tree_max_keys];
 B_Tree *children[b_tree_max_keys+1];
};

// NOTE(kv) Our weird assumption that there is no duplicate keys!
// ;b_tree_no_duplicate_keys
function void
btree_insert(B_Tree *root, i32 key_ins)
{
 // NOTE(kv) Find "node" and "key_index"
 B_Tree *node = root;
 i32 key_index;
 while(1)
 {
  // NOTE(kv) Figure out the key index
  key_index = alen(node->keys);
  for_i32(ki, 0, alen(node->keys))
  {
   // @b_tree_no_duplicate_keys
   kv_assert(key_ins != node->keys[ki]);
   
   if(key_ins < node->keys[ki])
   {
    key_index = ki;
    break;
   }
  }
  
  if(key_index > 0)
  {
   kv_assert(key_ins > node->keys[key_index-1]);
  }
  
  if(key_index < alen(node->keys))
  {
   // @b_tree_no_duplicate_keys
   kv_assert(key_ins < node->keys[key_index]);
  }
  
  if(node->height > 0)
  {
   node = node->children[key_index];
  }
  else
  {
   break;
  }
 }
 
 kv_assert(node->height == 0);
 
 // NOTE(kv) Shift things over
 for(i32 i = alen(node->keys)-1;
     i > key_index;
     i--)
 {
  node->keys[i] = node->keys[i-1];
 }
 node->keys ;
 
 for(i32 i = alen(node->children)-1;
     i > key_index;
     i--)
 {
  node->children[i] = node->children[i-1];
 }
}
//
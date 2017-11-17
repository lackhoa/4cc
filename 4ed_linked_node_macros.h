/*
 * Mr. 4th Dimention - Allen Webster
 *
 * 01.03.2016
 *
 * generic dynamically linked list 
 *
 */

// TOP

// NOTE(allen): These macros are setup to work on structs
// with a next and prev pointer where the type of the struct
// is the same as the type of the next/prev pointers.

#define dll_init_sentinel(s) (s)->next=(s),(s)->prev=(s)
#define dll_insert(p,v)      (v)->next=(p)->next,(v)->prev=(p),(p)->next=(v),(v)->next->prev=(v)
#define dll_back_insert(p,v) (v)->prev=(p)->prev,(v)->next=(p),(p)->prev=(v),(v)->prev->next=(v)
#define dll_remove(v)        (v)->next->prev=(v)->prev,(v)->prev->next=(v)->next

// HACK(allen): I don't like this anymore, get rid of it.
// for(dll_items(iterator, sentinel_ptr)){...}
#define dll_items(it, st) ((it) = (st)->next); ((it) != (st)); ((it) = (it)->next)

// NOTE(allen): These macros work on structs with a next
// pointer to the saem type as the containing struct.

#define sll_push(f,l,n) if((f)==0&&(l)==0){(f)=(l)=(n);}else{(l)->next=(n);(l)=(n);}(l)->next=0
#define sll_pop(f,l) if((f)!=(l)){(f)=(f)->next;}else{(f)=(l)=0;}

#define sll_init_sentinel(s) do{ (s)->next=(s); }while(0)
#define sll_insert(p,v) do{ (v)->next=(p)->next; (p)->next = (v); }while(0)
#define sll_remove(p,v) do{ Assert((p)->next == (v)); (p)->next = (v)->next; }while(0)

// HACK(allen): I don't like this anymore, get rid of it.
// for(sll_items(iterator, sentinel_ptr)){...}
#define sll_items(it, st) ((it) = (st)->next); ((it) != (st)); ((it) = (it)->next)

// BOTTOM

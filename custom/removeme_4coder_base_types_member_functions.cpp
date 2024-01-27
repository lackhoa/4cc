nono removeme
// NOTE(kv): from 4coder_base_types.cpp
// NOTE(kv): thank you C++ for not allowing member functions to be static

////////////////////////////////

Scratch_Block::Scratch_Block(Thread_Context *t){
    this->tctx = t;
    this->arena = tctx_reserve(t);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1){
    this->tctx = t;
    this->arena = tctx_reserve(t, a1);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1, Arena *a2){
    this->tctx = t;
    this->arena = tctx_reserve(t, a1, a2);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::Scratch_Block(Thread_Context *t, Arena *a1, Arena *a2, Arena *a3){
    this->tctx = t;
    this->arena = tctx_reserve(t, a1, a2, a3);
    this->temp = begin_temp(this->arena);
}

Scratch_Block::~Scratch_Block(){
    end_temp(this->temp);
    tctx_release(this->tctx, this->arena);
}

Scratch_Block::operator Arena*(){
    return(this->arena);
}

void
Scratch_Block::restore(void){
    end_temp(this->temp);
}

Temp_Memory_Block::Temp_Memory_Block(Temp_Memory t){
    this->temp = t;
}

Temp_Memory_Block::Temp_Memory_Block(Arena *arena){
    this->temp = begin_temp(arena);
}

Temp_Memory_Block::~Temp_Memory_Block(){
    end_temp(this->temp);
}

void
Temp_Memory_Block::restore(void){
    end_temp(this->temp);
}

////////////////////////////////

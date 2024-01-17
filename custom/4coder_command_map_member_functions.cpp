Command_Binding::Command_Binding(){
    block_zero_struct(this);
}
Command_Binding::Command_Binding(Custom_Command_Function *c){
    this->custom = c;
}
Command_Binding::Command_Binding(char *n){
    this->name = n;
}
Command_Binding::operator Custom_Command_Function*(){
    return(this->custom);
}
Command_Binding::operator char*(){
    return(this->name);
}

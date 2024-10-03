#pragma once

struct Ed_Parser;
function Token *ep_get_token(Ed_Parser *p);
function String ep_print_token(Ed_Parser *p, Arena *arena=0);
function String ep__print_given_token(Ed_Parser *p, Arena *arena, Token *token);
function Token *ep__get_token_please(Ed_Parser *p);



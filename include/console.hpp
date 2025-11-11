#pragma once

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <vector>

#include "emu.hpp"
#include "cpu.hpp"

#define no_arg 1
#define one_arg 2
#define few_arg 3

typedef struct{
	string shrt;
	string lng;
	int argc;
}console_args;

extern int console_show_help;

string console_get_input(struct termios *);
void help_find();
void console_cooked_mode(struct termios *);
void console_raw_mode(struct termios *);
void console_handle_token(string, console_args *, int *, emu_args *, TD4m_cpu *);
console_args console_find_token(string, console_args *, console_args *, int, int *);
string console_parce_input(string, int *);
void console_handle_input(string, console_args *, int, emu_args *, TD4m_cpu *);

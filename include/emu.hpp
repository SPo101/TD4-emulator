#pragma once

#include <iostream>
#include <getopt.h>
#include <vector>
#include <algorithm>
#include "cpu.hpp"

using namespace std;
#define PRINT_LINES 10

typedef struct {
	int mode; //auto/hand
	char *path;
	int frequency;
} settings;

typedef struct {
	vector<unsigned char> bp;
	unsigned char rom;
	unsigned char ram;
	unsigned char step;
	unsigned char restart;
} cons_args;

extern int stopped;

void emu_stop_exec_target(int);
void emu_console_input(TD4m_cpu *, cons_args *);
void emu_breakpoint_handler(TD4m_cpu *, cons_args *);
void emu_step_handler(TD4m_cpu *, cons_args *);
void emu_restart_handler(TD4m_cpu *, cons_args *);

void cpu_print_state(TD4m_cpu *, cons_args *);
void cpu_cycle(TD4m_cpu *, unsigned char *);
void cpu_data_input(TD4m_cpu *, unsigned char *);
void cpu_man_mode_next_step();

void usage();
void man();

void read_args(int, char **, settings *);
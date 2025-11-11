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
	unordered_map<unsigned char, string> mnemo;
	vector<unsigned char> bp;
	vector<unsigned char> rbp;
	unsigned char rom;
	unsigned char ram;
	unsigned char step;
	unsigned char restart;
} emu_args;

extern int stopped;

void emu_stop_exec_target(int);
void emu_breakpoint_handler(TD4m_cpu *, emu_args *);
void emu_step_handler(TD4m_cpu *, emu_args *);
void emu_restart_handler(TD4m_cpu *, emu_args *);

void cpu_print_set_mnemo(unordered_map<unsigned char, string>&);
void cpu_print_state(TD4m_cpu *, emu_args *);
void cpu_cycle(TD4m_cpu *, unsigned char *);
void cpu_data_input(TD4m_cpu *, unsigned char *);
void cpu_man_mode_next_step();

void usage();
void man();

void read_args(int, char **, settings *);
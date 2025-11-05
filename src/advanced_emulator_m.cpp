#include <iostream>
#include <time.h>
#include "cpu.hpp"
#include "logic.hpp"
#include "emu.hpp"

using namespace std;

int stopped = 0;

/*
to add
|-	console input
	|-	?target
	|-	new target
	|-	?run
	|-  after bp ability to step back
		|- need state dumps for that
|-  console handler input
	|-  all implied setting should influence on emu



after ctrl-z input commands mode; commands for showing special mem, 
		adding bp, sb and so on.
*/

int main(int argc, char *argv[]){

	signal(SIGTSTP, emu_stop_exec_target);

	TD4m_cpu td4m;
	unsigned char cpu_input = 0x00;
	settings start_set;
	cons_args cargs;
	cargs.rom = 0x00;	
	cargs.ram = 0x00;
	cargs.step = 0x00;
	cargs.restart = 0x00;
	struct timespec start, stop;

	start_set.mode = 1;
	start_set.path = nullptr;
	start_set.frequency = 1;
	read_args(argc, argv, &start_set);

	if(start_set.path != nullptr)
		td4m.write_rom(start_set.path);

	int cycles_done = 0;
	for(;;){

		emu_restart_handler(&td4m, &cargs);
		emu_step_handler(&td4m, &cargs);

		if(stopped){
			printf("\033[%dB\r", PRINT_LINES+1);
			printf("\x1b[2K");
			fflush(stdout);
			emu_console_input(&td4m, &cargs);
			continue;
		}

		timespec_get(&start, TIME_UTC);

	loop:
		cpu_print_state(&td4m, &cargs);
		cpu_data_input(&td4m, &cpu_input);
		cpu_cycle(&td4m, &cpu_input);

		emu_breakpoint_handler(&td4m, &cargs);

		if(!start_set.frequency){//in manual mode, processor doesnt need to sleep.
			cpu_man_mode_next_step();
			continue;
		}

		cycles_done++;
		if(cycles_done < start_set.frequency)
			goto loop;
		cycles_done = 0;

		timespec_get(&stop, TIME_UTC);
		stop.tv_sec -= start.tv_sec;
		if(stop.tv_sec)//if work of processor takes more than second, it doesnt need to sleep.
			continue;
		stop.tv_nsec = 1000000000 - stop.tv_nsec + start.tv_nsec;//remaining time to sleep.

		int res = nanosleep(&stop, NULL);

		if(res == -1)
			;//there should be handler if nanosleep was interrupted by signal

	}
}

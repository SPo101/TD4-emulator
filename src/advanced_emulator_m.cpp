#include <iostream>
#include <time.h>
#include "cpu.hpp"
#include "logic.hpp"
#include "emu.hpp"
#include "console.hpp"

using namespace std;

int stopped = 0;
int console_show_help = 0;

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

?rewrite emu_console_input
*/

int main(int argc, char *argv[]){

	signal(SIGTSTP, emu_stop_exec_target);

    static struct termios console;
    tcgetattr(0, &console);

	console_args cargs[] = {
		{"bp",	"breakpoint", 			few_arg},
		{"rbp",	"removebreakpoint",		few_arg},
		{"", 	"ram", 					one_arg},
		{"", 	"rom", 					one_arg},
		{"ss",	"steps", 				one_arg},

		{"sbp",	"showbreakpoint",		no_arg},
		{"pcs", "printcpustate", 		no_arg},
		{"c",	"continue", 			no_arg},
		{"s",	"step", 				no_arg},
		{"", 	"exit", 				no_arg},
		{"r", 	"restart", 				no_arg},
	};	
	int ln = sizeof(cargs)/sizeof(console_args);

	string input;

	TD4m_cpu td4m;
	unsigned char cpu_input = 0x00;
	settings start_set;
	emu_args eargs;
	eargs.rom = 0x00;	
	eargs.ram = 0x00;
	eargs.step = 0x00;
	eargs.restart = 0x00;
	cpu_print_set_mnemo(eargs.mnemo);
	struct timespec start, stop;

	start_set.mode = 1;
	start_set.path = nullptr;
	start_set.frequency = 1;
	read_args(argc, argv, &start_set);

	if(start_set.path != nullptr)
		td4m.write_rom(start_set.path);

	int cycles_done = 0;
	for(;;){

		emu_restart_handler(&td4m, &eargs);
		emu_step_handler(&td4m, &eargs);

		if(stopped){
			printf("\033[%dB\r", PRINT_LINES+1);
			printf("\x1b[2K");
			fflush(stdout);

			input = console_get_input(&console, &cargs[0], ln);
			console_handle_input(input, &cargs[0], ln, &eargs, &td4m);
			continue;
		}

		timespec_get(&start, TIME_UTC);

	loop:
		cpu_print_state(&td4m, &eargs);
		cpu_data_input(&td4m, &cpu_input);
		cpu_cycle(&td4m, &cpu_input);

		emu_breakpoint_handler(&td4m, &eargs);

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

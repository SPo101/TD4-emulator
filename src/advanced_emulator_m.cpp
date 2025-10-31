#include <iostream>
#include <time.h>
#include <getopt.h>
#include "cpu.hpp"
#include "logic.hpp"

using namespace std;
#define PRINT_LINES 10

typedef struct {
	int mode; //auto/hand
	char *path;
	int frequency;
} settings;

void usage(){
	exit(-1);
}

void print_state(TD4m_cpu *td4m){
	printf("ROM\tRAM\n");
	printf("\t\t\t[  A]=%2hhx [  B]=%2hhx\n", td4m->A, td4m->B);
	printf("\t\t\t[ PC]=%2hhx [ XY]=%2hhx\n", td4m->PC, td4m->XY);
	printf("\t\t\t[ CF]=%2hhx [ ZF]=%2hhx\n\n", td4m->CF, td4m->ZF);
	printf("\t\t\t[out]=%2hhx\n", td4m->output);
	printf("\033[%dA\r", 5);


	for(int i=0; i<PRINT_LINES; i++)
		printf("%2hhx\t%2hhx\n", *(td4m->ROM+td4m->PC+i), *(td4m->RAM+td4m->XY+i));
	printf("\033[%dA\r", PRINT_LINES+1);
}

void read_args(int cnt_args, char *args[], settings *start_set){

	if(cnt_args == 1)
		usage();	

	int Option = 0; 
	int Option_index = 0;

	//add break points, pieses of mem to show
	static struct option Long_options[] = {
		{"auto",	no_argument,		0,	'a'},
		{"manual",	no_argument,		0,	'm'},
		{"file",	required_argument,	0,	'f'},
		{"freq",	required_argument,	0, 	0 },
		{"help",	no_argument,		0, 	0 },
	};
	static const char *Short_options = "amf:";

	while(1){
		Option = getopt_long(cnt_args, args, Short_options, Long_options, &Option_index);
		if(Option == -1)
			break;
		
		switch(Option){
			case 0:
				if(Option_index == 4)//help
					usage();
				if(Option_index == 3)//freq
					start_set->frequency = atoi(optarg);
				break;
			case 'a':
				start_set->mode = 1; //auto
				break;
			case 'm':
				start_set->mode = 0; //manual
				break;
			case 'n':
				start_set->path = NULL;
				break;
			case 'f':
				start_set->path = optarg;
				break;
		}
	}
	if(!start_set->mode)//in manual mode the processor doesnt require frequency.
		start_set->frequency = 0;
}

int stopped = 0;

void sig_handler(int sig){
		stopped++;
		stopped %= 2;
}

int main(int argc, char *argv[]){

	signal(SIGTSTP, sig_handler);

	TD4m_cpu td4m;
	settings start_set;
	unsigned char inst = 0x00;
	struct timespec start, stop;

	start_set.mode = 1;
	start_set.path = NULL;
	start_set.frequency = 1;
	read_args(argc, argv, &start_set);

	td4m.write_rom(start_set.path);

	int cycles_done = 0;
	for(;;){

		if(stopped)
			continue;

		timespec_get(&start, TIME_UTC);

	loop:

		print_state(&td4m);
		inst = td4m.get_instruction();
		td4m.opcode_decode(&inst);
		td4m.alu(&inst);
		td4m.flags_handler();
		td4m.next_step();


		cycles_done++;
		if(cycles_done < start_set.frequency)
			goto loop;
		cycles_done = 0;


		if(!start_set.frequency)//in manual mode, processor doesnt need to sleep.
			continue;


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


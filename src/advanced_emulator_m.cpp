#include <iostream>
#include <time.h>
#include <getopt.h>
#include <vector>
#include "cpu.hpp"
#include "logic.hpp"

using namespace std;
#define PRINT_LINES 10

typedef struct {
	int mode; //auto/hand
	char *path;
	int frequency;
} settings;

typedef struct {
	vector<unsigned char> bp;
	vector<unsigned char> ram;
	vector<unsigned char> rom;
	unsigned char cont;
} cons_args;

void usage();
void print_state(TD4m_cpu *);
void read_args(int, char **, settings *);
void sig_handler(int);
void cycle(TD4m_cpu *);
void console_input(cons_args *);
/*
to add
|-	console input
	|-  after bp ability to step back
		|- need state dumps for that
|-  console handler input
	|-  all implied setting should influence on emu
		|- bp
		|- continue
		|- mem



after ctrl-z input commands mode; commands for showing special mem, 
		adding bp, sb and so on.
*/
int stopped = 0;

int main(int argc, char *argv[]){

	signal(SIGTSTP, sig_handler);

	TD4m_cpu td4m;
	settings start_set;
	cons_args cargs;
	cargs.cont = 0x00;
	struct timespec start, stop;

	start_set.mode = 1;
	start_set.path = NULL;
	start_set.frequency = 1;
	read_args(argc, argv, &start_set);

	td4m.write_rom(start_set.path);

	int cycles_done = 0;
	for(;;){

		if(stopped){
			printf("\033[%dB\r", PRINT_LINES+1);
			printf("\x1b[2K");
			fflush(stdout);
			console_input(&cargs);
			printf("\033[%dA\r", PRINT_LINES+2);
			continue;
		}

		//console_arg_handler();

		timespec_get(&start, TIME_UTC);

	loop:
		cycle(&td4m);



		if(!start_set.frequency){//in manual mode, processor doesnt need to sleep.
			printf("\033[%dB\r", PRINT_LINES+1);
			printf("\033[%dmNext step:\033[%dm ", 33, 0);
			char c = getchar();
			printf("\033[%dA\r", PRINT_LINES+2);
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
void cycle(TD4m_cpu *td4m){
	print_state(td4m);
	unsigned char inst = td4m->get_instruction();

	if(((inst&0xf0) == 0x20) | ((inst&0xf0) == 0x60)){
		printf("\033[%dB\r", PRINT_LINES+1);
		td4m->data_input();
		printf("\033[%dA\r", PRINT_LINES+1);
	}

	td4m->opcode_decode(&inst);
	td4m->alu(&inst);
	td4m->flags_handler();
	td4m->next_step();
}

void console_input(cons_args *cargs){
	string input;
	string token;
	getline(cin, input);
	int state = 0;

	int ln = input.length();
	for(int i=0; i<ln; i++){
		if(input[i] != ' ')
			token.push_back(input[i]);
		if((input[i] == ' ') | (i == ln-1)){
			if((!token.compare("bp")) | (!token.compare("breakpoint"))){
				state = 1;
				token.erase();
				continue;
			}
			if(!token.compare("ram")){
				state = 2;
				token.erase();
				continue;
			}
			if(!token.compare("rom")){
				state = 3;
				token.erase();
				continue;
			}
			if((!token.compare("c")) | (!token.compare("continue"))){
				cargs->cont += 0x01;
				token.erase();
				continue;
			}

			unsigned int data = 0;
			try{
				data = stoul(token, nullptr, 16);
			}
			catch (invalid_argument& e){
				token.erase();
				continue;
			}

			switch(state){
			case 1:
				cargs->bp.push_back(static_cast<unsigned char>(data));
				break;
			case 2:
				cargs->ram.push_back(static_cast<unsigned char>(data));
				break;
			case 3:
				cargs->rom.push_back(static_cast<unsigned char>(data));
				break;
			case 0:
				break;
			}

			token.erase();
		}
	}
}

void usage(){

	printf(" __       __  __ __  \n"  \
			"/\\ \\__   /\\ \\/\\ \\\\ \\  \n" \
			"\\ \\ ,_\\  \\_\\ \\ \\ \\\\ \\      ___ ___    \n" \
			" \\ \\ \\/  /'_` \\ \\ \\\\ \\_  /' __` __`\\   \n" \
			"  \\ \\ \\_/\\ \\L\\ \\ \\__ ,__\\/\\ \\/\\ \\/\\ \\ \n" \
			"   \\ \\__\\ \\___,_\\/_/\\_\\_/\\ \\_\\ \\_\\ \\_\\    \n" \
			"    \\/__/\\/__,_ /  \\/_/   \\/_/\\/_/\\/_/by spo101 \n\n");


	printf("Usage:\n");
	printf("-a --auto\t\tAuto executed mode.\n");
	printf("-m --manual\t\tManual executed mode.\n");
	printf("-f --file\t<path>\tInput opcode from bin file.\n");
	printf("-- --freq\t<cnt>\tEnter frequency of processor.\n");
	printf("-- --help\n\n");

	printf("<Ctrl-z> to stop/continue executing or enter/exit command mode.\n\n");

	//printf("Use breakpoint [bp], continue [c], stepback [sb], ram, rom \n");
	exit(0);
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

void sig_handler(int sig){
		stopped++;
		stopped %= 2;
}


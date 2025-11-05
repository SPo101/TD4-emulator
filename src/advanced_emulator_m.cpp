#include <iostream>
#include <time.h>
#include <getopt.h>
#include <vector>
#include <algorithm>
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


int stopped = 0;

void usage();
void man();
void read_args(int, char **, settings *);
void stop_exec_target(int);
void cpu_print_state(TD4m_cpu *);
void cpu_cycle(TD4m_cpu *, unsigned char *);
void cpu_data_input(TD4m_cpu *, unsigned char *);
void console_input(TD4m_cpu *, cons_args *);
void console_arg_handler(TD4m_cpu *, cons_args *);
void dbg_breakpoint_handler(TD4m_cpu *, cons_args *);
/*
to add
|-	console input
	|-  after bp ability to step back
		|- need state dumps for that
|-  console handler input
	|-	print cpu state
	|-	?target
	|-	?run
	|-  all implied setting should influence on emu
		|- bp
		|- continue
		|- mem



after ctrl-z input commands mode; commands for showing special mem, 
		adding bp, sb and so on.
*/

int main(int argc, char *argv[]){

	signal(SIGTSTP, stop_exec_target);

	TD4m_cpu td4m;
	unsigned char cpu_input = 0x00;
	settings start_set;
	cons_args cargs;
	cargs.cont = 0x00;
	struct timespec start, stop;

	start_set.mode = 1;
	start_set.path = nullptr;
	start_set.frequency = 1;
	read_args(argc, argv, &start_set);

	if(start_set.path != nullptr)
		td4m.write_rom(start_set.path);


	int cycles_done = 0;
	for(;;){

		if(stopped){
			printf("\033[%dB\r", PRINT_LINES+1);
			printf("\x1b[2K");
			fflush(stdout);
			console_input(&td4m, &cargs);
			//printf("\033[%dA\r", PRINT_LINES+2);
			continue;
		}

		//console_arg_handler(&td4m, &cargs);

		timespec_get(&start, TIME_UTC);

	loop:
		cpu_print_state(&td4m);
		cpu_data_input(&td4m, &cpu_input);
		cpu_cycle(&td4m, &cpu_input);

		dbg_breakpoint_handler(&td4m, &cargs);


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


void console_arg_handler(TD4m_cpu *td4m, cons_args *cargs){
}

void dbg_breakpoint_handler(TD4m_cpu *td4m, cons_args *cargs){
	int in = 0;
	int ln = cargs->bp.size();
	if(!ln)
		return;

	for(int i = 0; i < ln; i++)
		if(td4m->PC == cargs->bp.at(i)){
			in = 1;
			break;
		}

	if(!in)
		return;


	stop_exec_target(0);
	printf("\033[%dB\r", PRINT_LINES+1);
	printf("Breakpoint at address - %hhx\n", td4m->PC);
	printf("\033[%dA\r", 1);
	fflush(stdout);
}

void cpu_data_input(TD4m_cpu *td4m, unsigned char *cpu_input){
	unsigned char inst = td4m->get_instruction();
	if(((inst&0xf0) != 0x20) & ((inst&0xf0) != 0x60)){
		*cpu_input = 0x00;
		return;
	}

	string str;
	printf("\033[%dB\r", PRINT_LINES+1);
	printf("\033[%dmInput data:\033[%dm ", 32, 0);
	getline(cin, str);
	try{
		unsigned int data = stoul(str, nullptr, 16);
		*cpu_input = static_cast<unsigned char>(data);
	}
	catch (invalid_argument& e){
		*cpu_input = 0x00;
	}
	printf("\033[1A\r\x1b[2K");
	printf("\033[%dA\r", PRINT_LINES+1);
	fflush(stdout);
}

void cpu_cycle(TD4m_cpu *td4m, unsigned char *cpu_input){
	unsigned char inst = td4m->get_instruction();

	if(((inst&0xf0) == 0x20) | ((inst&0xf0) == 0x60))
		td4m->input = *cpu_input;

	td4m->opcode_decode(&inst);
	td4m->alu(&inst);
	td4m->flags_handler();
	td4m->next_step();
}

void console_input(TD4m_cpu *td4m, cons_args *cargs){
	string input;
	string token;
	printf("> ");
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
			if((!token.compare("s")) | (!token.compare("step"))){
				cargs->cont += 0x01;
				token.erase();
				continue;
			}
			if((!token.compare("c")) | (!token.compare("continue"))){
				stop_exec_target(0);
				token.erase();
				return;
			}
			if(!token.compare("exit"))
				exit(0);
			if((!token.compare("pcs")) | (!token.compare("printcpustate"))){
				cpu_print_state(td4m);
				printf("\033[%dB\r", PRINT_LINES+2);
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
				state = 0;
				break;
			case 3:
				cargs->rom.push_back(static_cast<unsigned char>(data));
				state = 0;
				break;
			case 0:
				break;
			}

			token.erase();
		}
	}
	sort(cargs->bp.begin(), cargs->bp.end());
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
	printf("-- --man\t\tSee all built-in console args\n");
	printf("-- --help\n\n");

	printf("<Ctrl-z> to stop/continue executing or enter/exit command mode.\n\n");

	//printf("Use breakpoint [bp], continue [c], stepback [sb], ram, rom \n");
	exit(0);
}

void man(){
	printf(" __       __  __ __  \n"  \
			"/\\ \\__   /\\ \\/\\ \\\\ \\  \n" \
			"\\ \\ ,_\\  \\_\\ \\ \\ \\\\ \\      ___ ___    \n" \
			" \\ \\ \\/  /'_` \\ \\ \\\\ \\_  /' __` __`\\   \n" \
			"  \\ \\ \\_/\\ \\L\\ \\ \\__ ,__\\/\\ \\/\\ \\/\\ \\ \n" \
			"   \\ \\__\\ \\___,_\\/_/\\_\\_/\\ \\_\\ \\_\\ \\_\\    \n" \
			"    \\/__/\\/__,_ /  \\/_/   \\/_/\\/_/\\/_/by spo101 \n\n");
	printf("Man:\n");
	printf("bp\tbreakpoint\t<1st> ... <last> args\tMake a breakpoint(s).\n");
	printf("s\tstep\t--\t\t\t\tIn auto mode continue until next bp. In manual mode make one step.\n");
	printf("--\tram\t\t<arg>\t\t\tPrint RAM area from <arg> place.\n");
	printf("--\trom\t\t<arg>\t\t\tPrint ROM area from <arg> place.\n");
	printf("c \tcontinue\t\t--\t\tExit console mode(preffered). Equivalent of <Ctrl-z>.\n");
	printf("--\texit\t\t--\t\t\tExit emulator. Equivalent of <Ctrl-c>.\n");
	printf("\n");

	exit(0);
}

void cpu_print_state(TD4m_cpu *td4m){
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
		{"man",		no_argument,		0, 	0 },
	};
	static const char *Short_options = "amf:";

	while(1){
		Option = getopt_long(cnt_args, args, Short_options, Long_options, &Option_index);
		if(Option == -1)
			break;
		
		switch(Option){
			case 0:
				if(Option_index == 3)//freq
					start_set->frequency = atoi(optarg);
				if(Option_index == 4)//help
					usage();
				if(Option_index == 5)//man
					man();
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

void stop_exec_target(int sig){
		stopped++;
		stopped %= 2;
}


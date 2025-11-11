#include "emu.hpp"

void emu_restart_handler(TD4m_cpu *td4m, emu_args *eargs){
	if(!eargs->restart)
		return;

	td4m->reset();
	eargs->restart = 0x00;
}

void emu_step_handler(TD4m_cpu *td4m, emu_args *eargs){
	if(!eargs->step)
		return;

	unsigned char cpu_input = 0x00;
	while(eargs->step){
		cpu_data_input(td4m, &cpu_input);
		cpu_cycle(td4m, &cpu_input);

		emu_breakpoint_handler(td4m, eargs);
		eargs->step--;
	}
}

void emu_breakpoint_handler(TD4m_cpu *td4m, emu_args *eargs){
	int in = 0;
	int ln = 0;

	ln = eargs->bp.size();
	if(!ln)
		return;
	sort(eargs->bp.begin(), eargs->bp.end());

	ln = eargs->rbp.size();
	if(ln)
		for(int i=0; i<ln; i++)
			eargs->bp.erase(remove(eargs->bp.begin(), eargs->bp.end(), eargs->rbp.at(i)), eargs->bp.end());

	ln = eargs->bp.size();
	for(int i = 0; i < ln; i++)
		if(td4m->PC == eargs->bp.at(i)){
			in = 1;
			break;
		}

	if(!in)
		return;

	if(!stopped)
		emu_stop_exec_target(0);
	printf("\033[%dB\r", PRINT_LINES+1);
	printf("Breakpoint at address - %hhx\n", td4m->PC);
	printf("\033[%dA\r", 1);
	fflush(stdout);
}

void emu_console_input(TD4m_cpu *td4m, emu_args *eargs){
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
			if((!token.compare("sbp")) | (!token.compare("showbreakpoint"))){
				if(eargs->bp.empty()){
					printf("no breakpoints until\n");
					continue;
				}

				sort(eargs->bp.begin(), eargs->bp.end());
				for(unsigned char bp_ : eargs->bp)
					printf("%hhx ", bp_);
				printf("\n");
				continue;
			}
			if((!token.compare("rbp")) | (!token.compare("removebreakpoint"))){
				state = 5;
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
				eargs->step += 0x01;
				token.erase();
				continue;
			}
			if((!token.compare("ss")) | (!token.compare("steps"))){
				state = 4;
				token.erase();
				continue;
			}
			if((!token.compare("c")) | (!token.compare("continue"))){
				emu_stop_exec_target(0);
				token.erase();
				return;
			}
			if(!token.compare("exit"))
				exit(0);
			if((!token.compare("pcs")) | (!token.compare("printcpustate"))){
				cpu_print_state(td4m, eargs);
				printf("\033[%dB\r", PRINT_LINES+2);
				continue;
			}
			if((!token.compare("r")) | (!token.compare("restart"))){
				emu_stop_exec_target(0);
				eargs->restart = 0x01;
				token.erase();
				return;
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
				eargs->bp.push_back(static_cast<unsigned char>(data));
				break;
			case 2:
				eargs->ram = static_cast<unsigned char>(data);
				state = 0;
				break;
			case 3:
				eargs->rom = static_cast<unsigned char>(data);
				state = 0;
				break;
			case 4:
				eargs->step += static_cast<unsigned char>(data);
				state = 0;
				break;
			case 5:
				eargs->rbp.push_back(static_cast<unsigned char>(data));
				break;
			case 0:
				break;
			}
			token.erase();
		}
	}
}

void emu_stop_exec_target(int sig){
		stopped++;
		stopped %= 2;
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

void cpu_man_mode_next_step(){
	printf("\033[%dB\r", PRINT_LINES+1);
	printf("\033[%dmNext step:\033[%dm ", 33, 0);
	char c = getchar();
	printf("\033[%dA\r", PRINT_LINES+2);
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

void cpu_print_state(TD4m_cpu *td4m, emu_args *eargs){
	printf("\rROM\tRAM\n");
	printf("\r\t\t\t[  A]=%2hhx [  B]=%2hhx\n", td4m->A, td4m->B);
	printf("\r\t\t\t[ PC]=%2hhx [ XY]=%2hhx\n", td4m->PC, td4m->XY);
	printf("\r\t\t\t[ CF]=%2hhx [ ZF]=%2hhx\n\n", td4m->CF, td4m->ZF);
	printf("\r\t\t\t[out]=%2hhx\n", td4m->output);
	printf("\033[%dA\r", 5);


	for(int i=0; i<PRINT_LINES; i++)
		printf("\r\033[%dm%2hhx\033[0m\t%2hhx\n",(i+eargs->rom==td4m->PC) ? 31 : 0 ,*(td4m->ROM+i+eargs->rom), *(td4m->RAM+i+eargs->ram));
	printf("\033[%dA\r", PRINT_LINES+1);
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
	printf("-c --console\t\tStart in console mode.\n");
	printf("-a --auto\t\tAuto executed mode.\n");
	printf("-m --manual\t\tManual executed mode.\n");
	printf("-f --file\t<path>\tInput opcode from bin file.\n");
	printf("-- --freq\t<cnt>\tEnter frequency of processor.\n");
	printf("-- --man\t\tSee all built-in console args\n");
	printf("-- --help\n\n");

	printf("<Ctrl-z> to stop/continue executing or enter/exit command mode.\n\n");
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
	printf("sbp\tshowbreakpoint\t--\t\t\tShow all breakpoints.\n");
	printf("rbp\tromovebreakpoint<1st> ... <last> args\tDel n breakpoints.\n");
	printf("s\tstep\t\t--\t\t\tMake one step.\n");
	printf("ss\tsteps\t\t<arg>\t\t\tMake n steps.\n");
	printf("--\tram\t\t<arg>\t\t\tPrint RAM area from <arg> place.\n");
	printf("--\trom\t\t<arg>\t\t\tPrint ROM area from <arg> place.\n");

	printf("r\trestart\t\t--\t\t\tRestarts execution of target\n");
	printf("pcs\tprintcpustate\t--\t\t\tShow registers state\n");

	printf("c\tcontinue\t--\t\t\tExit console mode(preffered). Equivalent of <Ctrl-z>.\n");
	printf("--\texit\t\t--\t\t\tExit emulator. Equivalent of <Ctrl-c>.\n");
	printf("\n");

	exit(0);
}

void read_args(int cnt_args, char *args[], settings *start_set){

	if(cnt_args == 1)
		usage();	

	int Option = 0; 
	int Option_index = 0;
	int hints = 0;

	static struct option Long_options[] = {
		{"auto",	no_argument,		0,	'a'},
		{"manual",	no_argument,		0,	'm'},
		{"file",	required_argument,	0,	'f'},
		{"freq",	required_argument,	0, 	0 },
		{"help",	no_argument,		0, 	0 },
		{"man",		no_argument,		0, 	0 },
		{"console",	no_argument,		0, 	'c'},
	};
	static const char *Short_options = "amf:c";

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
			case 'c':
				TD4m_cpu p_td4m;
				emu_args p_eargs;
				p_eargs.rom = 0x00;	
				p_eargs.ram = 0x00;
				p_eargs.step = 0x00;
				cpu_print_state(&p_td4m, &p_eargs);
				emu_stop_exec_target(0);
				break;
		}
	}
	if(!start_set->mode)//in manual mode the processor doesnt require frequency.
		start_set->frequency = 0;
}


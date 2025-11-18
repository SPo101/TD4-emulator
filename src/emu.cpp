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

void cpu_print_set_mnemo(unordered_map<unsigned char, string>& mnemo){
	mnemo[0x00] = "ADD A";
	mnemo[0x10] = "MOV A,B";
	mnemo[0x20] = "IN A";
	mnemo[0x30] = "MOV A";
	mnemo[0x40] = "MOV B,A";
	mnemo[0x50] = "ADD B";
	mnemo[0x60] = "IN B";
	mnemo[0x70] = "MOV B";

	mnemo[0x80] = "ADD A,B";
	mnemo[0x81] = "NEG A";
	mnemo[0x82] = "NOT A";
	mnemo[0x83] = "OR A,B";
	mnemo[0x84] = "AND A,B";
	mnemo[0x85] = "XOR A,B";
	mnemo[0x86] = "SUB A,B";
	mnemo[0x87] = "OUT A";
	mnemo[0x88] = "LD A";
	mnemo[0x89] = "ST A";
	mnemo[0x8a] = "LD B";
	mnemo[0x8b] = "ST B";
	mnemo[0x8c] = "MOV X,A";
	mnemo[0x8d] = "MOV Y,A";
	mnemo[0x8e] = "INC X,Y";
	mnemo[0x8f] = "JMP X,Y";

	mnemo[0x90] = "OUT B";
	mnemo[0xa0] = "JZ";
	mnemo[0xb0] = "OUT";
	mnemo[0xc0] = "MOV Y";
	mnemo[0xd0] = "MOV X";
	mnemo[0xe0] = "JNC";
	mnemo[0xf0] = "JMP";	
}

void cpu_print_state(TD4m_cpu *td4m, emu_args *eargs){
	printf("\rDIS_ASM\t\t\tROM\tRAM\n");
	printf("\r\t\t\t\t\t\t[  A]=%2hhx [  B]=%2hhx\n", td4m->A, td4m->B);
	printf("\r\t\t\t\t\t\t[ PC]=%2hhx [ XY]=%2hhx\n", td4m->PC, td4m->XY);
	printf("\r\t\t\t\t\t\t[ CF]=%2hhx [ ZF]=%2hhx\n\n", td4m->CF, td4m->ZF);
	printf("\r\t\t\t\t\t\t[out]=%2hhx\n", td4m->output);
	printf("\033[%dA\r", 5);

	unsigned char inst;
	unsigned char data;

	for(int i=0; i<PRINT_LINES; i++){
		inst = *(td4m->ROM+i+eargs->rom);
		if((inst&0xf0) != 0x80){
			data = inst & 0x0f;
			inst &= 0xf0;
			if(data){
				printf("\r\033[%dm%s,%hhx\t\t\t%2hhx\033[0m\t%2hhx\n",(i+eargs->rom==td4m->PC) ? 31 : 0 , eargs->mnemo[inst].c_str(), data, *(td4m->ROM+i+eargs->rom), *(td4m->RAM+i+eargs->ram));
				continue;
			}
		}
		printf("\r\033[%dm%s\t\t\t%2hhx\033[0m\t%2hhx\n",(i+eargs->rom==td4m->PC) ? 31 : 0 , eargs->mnemo[inst].c_str(), *(td4m->ROM+i+eargs->rom), *(td4m->RAM+i+eargs->ram));
	}
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


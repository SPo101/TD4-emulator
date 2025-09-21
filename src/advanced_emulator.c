#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <dlfcn.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>

#define MEM_SIZE 16 // 16 bytes;

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  ((byte) & 0x80 ? '1' : '0'), \
  ((byte) & 0x40 ? '1' : '0'), \
  ((byte) & 0x20 ? '1' : '0'), \
  ((byte) & 0x10 ? '1' : '0'), \
  ((byte) & 0x08 ? '1' : '0'), \
  ((byte) & 0x04 ? '1' : '0'), \
  ((byte) & 0x02 ? '1' : '0'), \
  ((byte) & 0x01 ? '1' : '0') 


/*
ADD A,Im - 0000_xxxx - 0x00 - V
ADD B,Im - 0101_xxxx - 0x50 - V

MOV A,Im - 0011_xxxx - 0x30 - V
MOV B,Im - 0111_xxxx - 0x70 - V

MOV A,B  - 0001_0000 - 0x10 - V
MOV B,A  - 0100_0000 - 0x40 - V

JMP Im   - 1111_xxxx - 0xf0 - V
JNC Im   - 1110_xxxx - 0xe0 - V

IN A     - 0010_0000 - 0x20 - V
IN B     - 0110_0000 - 0x60 - V

OUT B     - 1001_0000 - 0x90 - V
OUT Im    - 1011_xxxx - 0xb0 - V
*/

typedef struct {
	unsigned char A;
	unsigned char B;
	unsigned char PC;
	unsigned char CF;
	unsigned char input;
	unsigned char output;
	unsigned char zero;

	unsigned char *work_register;
	unsigned char *load_register;

	unsigned char cnt_cf;
} registers;


int stopped = 0;

unsigned char togglebit(unsigned char, int);
void read_str_opcode(unsigned char *);

//void read_bin_opcode(unsigned char *);

void print_console(registers *, unsigned char *);
void opcode_decode(unsigned char *, registers *);
int read_args(int, char *[]);
void usage();
void input_data(unsigned char *, registers *, unsigned char *, int);
void sig_handler(int);



int main(int argc, char *argv[]){
	
	registers TD4;
	
	TD4.A = 0x00;
	TD4.B = 0x00;
	TD4.PC = 0x00;
	TD4.CF = 0x00;
	TD4.input = 0x00;
	TD4.output = 0x00;
	TD4.zero = 0x00;
	TD4.cnt_cf = 0x00;

	TD4.work_register = NULL;
	TD4.load_register = NULL;
	
	unsigned char *mem = calloc(MEM_SIZE, 1);
	unsigned char instruction;
	unsigned char data;


	signal(SIGTSTP, sig_handler);

	int auto_ = read_args(argc, argv);
	read_str_opcode(mem);


	printf("\033[38;5;199m<Ctrl> + <z> to stop/continue\033[0m\n");

	for(;;){

		if(stopped)
			continue;
			
		instruction = *(mem+TD4.PC) & 0xf0;
		data = *(mem+TD4.PC) & 0x0f;


		print_console(&TD4, mem);

		input_data(&instruction, &TD4, mem, auto_);

		printf("\033[%dA\r", MEM_SIZE);

		
		opcode_decode(&instruction, &TD4);

		*TD4.load_register = *TD4.work_register + data;

		if(*TD4.load_register >= 0x10){
			*TD4.load_register &= 0x0f;
			TD4.CF = 0x01;
			TD4.cnt_cf = 0x02;
		}

		sleep(1);
		if(TD4.load_register != &TD4.PC)
			TD4.PC += 0x01;

		TD4.PC %= MEM_SIZE;
	
		TD4.cnt_cf -= 0x01;
		if(TD4.cnt_cf <= 0x00){
			TD4.CF = 0x00;
			TD4.cnt_cf = 0x00;
		}	
	}
}

unsigned char togglebit(unsigned char num, int pos){
	return num ^ (1<<pos);
}

void read_str_opcode(unsigned char *mem){
	size_t size = 0;
	char *str = NULL;
	char str2[2];

	getline(&str, &size, stdin);
	
	for(int i=0; i<MEM_SIZE; i++){
		memcpy(&str2, str+i*2, 2);
		sscanf(str2, "%x", mem+i);
	}
	free(str);
}

void print_console(registers *TD4, unsigned char *mem){

	printf("\n");
	for(int i=0; i<MEM_SIZE; i++){
		switch(i){
			case 0:
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[ A] = "BYTE_TO_BINARY_PATTERN \
				"\t[ B] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->A), BYTE_TO_BINARY(TD4->B));
				break;

			case 1:
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[CF] = "BYTE_TO_BINARY_PATTERN \
				"\t[PC] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->CF), BYTE_TO_BINARY(TD4->PC));
				break;

			case 3:
			printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN \
				"\t\033[0m[out] = "BYTE_TO_BINARY_PATTERN \
				"\n", \
				(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)), \
				BYTE_TO_BINARY(TD4->output));
				break;

			case MEM_SIZE-1:
				printf("\033[%dm\033[2K\r%2d | "BYTE_TO_BINARY_PATTERN \
					"\t\033[0mEnter input: ", \
					(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)));
				break;

			default:
				printf("\033[%dm%2d | "BYTE_TO_BINARY_PATTERN"\n", \
					(i == TD4->PC) ? 31 : 0, i, BYTE_TO_BINARY(*(mem+i)));
				break;
		}
	}
	fflush(stdout);
}


void opcode_decode(unsigned char *instruction, registers *TD4){ 
	unsigned char D4 = *instruction & 0x10; 
	unsigned char D5 = *instruction & 0x20;
	unsigned char D6 = *instruction & 0x40;
	unsigned char D7 = *instruction & 0x80; 

	unsigned char BA = D5 | ((D7>>3) | D4);
	unsigned char LOAD = (D7 | D6<<1) | \
					D7>>1 | togglebit(D6, 6) | \
					togglebit(D7>>2 & togglebit(D6>>1, 5), 5) | \
					togglebit(D6>>2 & D7>>3 & (D4 | togglebit(TD4->CF, 0)<<4), 4);


	switch(BA){ //define 1st summand for ALU
		case 0x00: //A register
			TD4->work_register = &TD4->A;
			break;
		case 0x10: //B register
			TD4->work_register = &TD4->B;
			break;
		case 0x20: //IN
			TD4->work_register = &TD4->input;
			break;
		case 0x30: //ZERO
			TD4->work_register = &TD4->zero;
			break;
	}

	
	switch(LOAD){//define where to store a result from ALU
		case 0x70: //A register
			TD4->load_register = &TD4->A;
			break;
		case 0xb0: //B register
			TD4->load_register = &TD4->B;
			break;
		case 0xd0: //OUT
			TD4->load_register = &TD4->output;
			break;
		case 0xe0: //PC
			TD4->load_register = &TD4->PC;
			break;
	}
}

int read_args(int cnt_args, char *args[]){

	if(cnt_args == 1)
		usage();	

	int Option = 0; 
	int Option_index = 0;


	static struct option Long_options[] = {
		{"auto", 	no_argument, 	0, 'a'},
		{"hand", 	no_argument, 	0, 'h'},
	};
	static char *Short_options = "ah";

	while(1){
		Option = getopt_long(cnt_args, args, Short_options, Long_options, &Option_index);
		if(Option == -1)
			break;
		
		switch(Option){
			case 0:
				//for long args;
				break;
			case 'a':
				return 1;
				break;
			case 'h':
				return 0;
				break;
		}
	}
	return 1;
}

void input_data(unsigned char *instruction, registers *TD4, unsigned char *mem, int mode){

	char *input = NULL;
	char buf[2];
	buf[0] = 0x00;
	buf[1] = '\0';
	size_t len = 0;
	ssize_t read = 0;
	int color = 32;
	int exec = 1;


	if((*instruction == 0x20) || (*instruction == 0x60)){
		get_input:
			printf("\033[%dm\033[2K\r%2d | "BYTE_TO_BINARY_PATTERN \
					"\t\033[%dmEnter input: ", \
					(MEM_SIZE-1 == TD4->PC) ? 31 : 0, \
					MEM_SIZE-1, BYTE_TO_BINARY(*(mem+MEM_SIZE-1)), color);

			read = getline(&input, &len, stdin);
			if(input[read-1] == '\n'){
				input[read-1] = '\0';
				read--;
			}
			printf("\033[%dA\r", 1);
	
			memcpy(&buf, input, 1);
			sscanf(buf, "%x", &TD4->input);
			exec = 0;
	}
	//mode=1 for auto; mode=0 for hand
	if((!mode) & exec){
		color = 33;
		goto get_input;
	}

	free(input);
}


void usage(){
	printf("USaGE\n");
	exit(1);
}

void sig_handler(int sig){
		stopped++;
		stopped %= 2;
}

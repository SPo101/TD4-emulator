#include <iostream>
#include "cpu.hpp"
#include "logic.hpp"

using namespace std;
#define PRINT_LINES 10

void print_state(TD4m_cpu *td4m){
	printf("ROM\tRAM\n");
	printf("\t\t\t[  A]=%2hhx [  B]=%2hhx\n", td4m->A, td4m->B);
	printf("\t\t\t[ PC]=%2hhx [ XY]=%2hhx\n", td4m->PC, td4m->XY);
	printf("\t\t\t[ CF]=%2hhx [ ZF]=%2hhx\n\n", td4m->CF, td4m->ZF);
	printf("\t\t\t[out]=%2hhx\n", td4m->output);
	printf("\033[%dA\r", 5);


	for(int i=0; i<PRINT_LINES; i++)
		printf("%2hhx\t%2hhx\n", *(td4m->ROM+td4m->PC+i), *(td4m->RAM+td4m->XY+i));
	//printf("\033[%dA\r", PRINT_LINES+1);
}

int main(int argc, char *argv[]){

	TD4m_cpu td4m;
	unsigned char inst = 0x00;

	td4m.write_rom("opcode.bin");


	//for(int i=0; i<16; i++){
	for(;;){
		print_state(&td4m);
		inst = td4m.get_instruction();
		td4m.opcode_decode(&inst);
		td4m.alu(&inst);
		td4m.flags_handler();
		td4m.next_step();
	}

}


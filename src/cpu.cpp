#include "cpu.hpp"
#include "logic.hpp"

TD4m_cpu::TD4m_cpu() {
	//4bit reg
	this->A = 0x00;
	this->B = 0x00;
	this->input = 0x00;
	this->output = 0x00;

	//8bit reg
	this->PC = 0x00;
	this->XY = 0x00;

	//flags
	this->CF = 0x00;
	this->ZF = 0x00;

	//consts
	this->zero = 0x00;
	this->one = 0x01;

	//memory
	this->RAM = (unsigned char*) malloc(128 * sizeof(unsigned char));
	this->ROM = (unsigned char*) malloc(256 * sizeof(unsigned char));

	//accessory
	this->work_register1 = NULL;
	this->work_register2 = NULL;
	this->load_register = NULL;

	//accessory
	this->cnt_cf = 0x00;
	this->cnt_zf = 0x00;
	this->move_bits = 0x00;
}

TD4m_cpu::~TD4m_cpu() {
	free(this->RAM);
	free(this->ROM);
}

void TD4m_cpu::opcode_decode(unsigned char *instruction) { 
	unsigned char D0 = (*instruction & 0x01)>>0; 
	unsigned char D1 = (*instruction & 0x02)>>1;
	unsigned char D2 = (*instruction & 0x04)>>2;
	unsigned char D3 = (*instruction & 0x08)>>3; 

	unsigned char D4 = (*instruction & 0x10)>>4; 
	unsigned char D5 = (*instruction & 0x20)>>5;
	unsigned char D6 = (*instruction & 0x40)>>6;
	unsigned char D7 = (*instruction & 0x80)>>7; 

	unsigned char SUM1 = 0x00;
	unsigned char SUM2 = 0x00;
	unsigned char LOAD = 0x00;

	/*is data command?
	in cases 8,a,c,d
	*/
	unsigned char is_command = (D7 & not_(D6) & not_(D4)) | \
								(D7 & D6 & not_(D5));


	this->move_bits = 0x00;

	if(is_command)
		goto data_is_command;


	//block when last 4 bits present data
	SUM1 |= (tobool(D5) << 5);
	SUM1 |= (tobool(D7|D4) << 4);

	LOAD |= (tobool(D7|D6									) << 7);
	LOAD |= (tobool(D7|not_(D6)								) << 6);
	LOAD |= (tobool(not_(D7 & not_(D6))						) << 5);
	LOAD |= (tobool(not_(D6 & D7 & (D4 | not_(this->CF)))	) << 4);

	switch(SUM1){ //define 1st summand for ALU 
		case 0x00: //A register
			this->work_register1 = &this->A;
			break;
		case 0x10: //B register
			this->work_register1 = &this->B;
			break;
		case 0x20: //IN
			this->work_register1 = &this->input;
			break;
		case 0x30: //ZERO
			this->work_register1 = &this->zero;
			break;
	}

	this->work_register2 = NULL;

	switch(LOAD){//define where to store a result from ALU
		case 0x70: //A register
			this->load_register = &this->A;
			break;
		case 0xb0: //B register
			this->load_register = &this->B;
			break;
		case 0xd0: //OUT
			this->load_register = &this->output;
			break;
		case 0xe0: //PCL
			this->load_register = &this->PC;
			this->move_bits = 0x02;
			break;
		case 0xf0: //for JNC when C
			this->load_register = NULL;
			break;
	}
	return;

	//block when last 4 bits present command
	//A-0000 B-0001 RAM-0010 XY-0011 X-0100 Y-0101 PC-0110 zero-0111 one-1000 out-1001
data_is_command:
	SUM1 |= (tobool(D3 & ((not_(D2) & not_(D0)) | (D2 & D1))) << 5); //S1
	SUM1 |= (tobool(D3 & D1 & ( (not_(D2) * D0) | D2 )		) << 4); //S0

	SUM2 |= (tobool((not_(D3) & not_(D2) & not_(D1) & D0) | (D3 & D2 & D1 & not_(D0))							) << 7); //S3
	SUM2 |= (tobool((not_(D3)&D1&((not_(D2)&not_(D0))|(D2&D0))) | (D3&not_(D2)) | (D3&D2&(not_(D1)|(D1&D0)))	) << 6); //S2
	SUM2 |= (tobool(SUM2>>6																						) << 5); //S1
	SUM2 |= (tobool(not_(SUM2>>7)																				) << 4); //S0
	
	LOAD |= (tobool(not_(D3) & D2 & D1 & D0	) << 7); //L3
	LOAD |= (tobool(D3 & D2 & (not_(D1)	| D0)) << 6); //L2
	LOAD |= (tobool(D3 & ((not_(D2) & D0) | (D2 & D1))) << 5); //L1
	LOAD |= (tobool((D2 & D0 & ((D3 & not_(D1)) | (D1 & not_(D3)))) | \
									(D3 & D1 & not_(D0))) << 4); //L0


	//define 1st summand for ALU
	switch(SUM1){
		case 0x00: //A register
			this->work_register1 = &this->A;
			break;	
		case 0x10: //B register
			this->work_register1 = &this->B;
			break;	
		case 0x20: //RAM
			this->work_register1 = (this->RAM+this->XY);
			break;	
		case 0x30: //XY
			this->work_register1 = &this->XY;
			break;	
	}


	//define 2nd summand for ALU
	switch(SUM2){
		case 0x10: //B register
			this->work_register2 = &this->B;
			break;	
		case 0x70: //ZERO
			this->work_register2 = &this->zero;
			break;	
		case 0x80: //ONE
			this->work_register2 = &this->one;
			break;	
	}


		//define where to store a result from ALU	
	switch(LOAD){
		case 0x00: //A register
			this->load_register = &this->A;
			break;	
		case 0x10: //B register
			this->load_register = &this->B;
			break;	
		case 0x20: //RAM
			this->load_register = (this->RAM+this->XY);
			break;	
		case 0x90: //OUT
			this->load_register = &this->output;
			break;	
		case 0x30: //XY
			this->load_register = &this->XY;
			break;	
		case 0x40: //X
			this->load_register = &this->XY;
			this->move_bits = 0x01;
			break;	
		case 0x50: //Y
			this->load_register = &this->XY;
			this->move_bits = 0x02;
			break;	
		case 0x60: //PC
			this->load_register = &this->PC;
			break;	
	}


	if( ((*instruction)&0xf0) == 0xa0){
		*this->work_register1 = *instruction&0x0f;
		this->work_register2 = &this->zero;
		this->load_register = NULL;

		if(this->ZF){
			this->load_register = &this->PC;
			this->move_bits = 0x02;
		}

	}
	if( ((*instruction)&0xf0) == 0xc0){
		*this->work_register1 = *instruction&0x0f;
		this->work_register2 = &this->zero;
		this->load_register = &this->XY;
		this->move_bits = 0x02;
		
	}
	if( ((*instruction)&0xf0) == 0xd0){
		*this->work_register1 = *instruction&0x0f;
		this->work_register2 = &this->zero;
		this->load_register = &this->XY;
		this->move_bits = 0x01;
		
	}
	return;
}

void TD4m_cpu::alu(unsigned char *instruction) {
	if(((0x01 <= (*instruction&0x0f)) & ((*instruction&0x0f) <= 0x05)) & ((*instruction&0xf0) == 0x80))
		this->logic_unit(instruction);
	else
		this->arithmetic_unit(instruction);	
}

void TD4m_cpu::flags_handler() {
	c_flag_handler();
	z_flag_handler();
}

void TD4m_cpu::next_step(){
	if(this->load_register != &this->PC)
			this->PC += 0x01;

	this->PC %= 0xff;
}

void TD4m_cpu::write_rom(string path){
	ifstream file(path, ios::binary);
  if (!file) {
      cerr << "Failed to open file.\n";
      return;
  }

  file.seekg(0, ios::end);
  streamsize size = file.tellg();
  file.seekg(0, ios::beg);

  if (!file.read(reinterpret_cast<char*>(this->ROM), size)) {
      std::cerr << "Error reading file.\n";
      return;
  }
  file.close();
}

unsigned char TD4m_cpu::get_instruction(){
	return *(this->ROM+this->PC);
}

void TD4m_cpu::logic_unit(unsigned char *instruction){
	if(this->work_register2 == NULL)
		return;
	if(this->load_register == NULL)
		return;

	switch(*instruction){
		case 0x81: //A = -A ( A = neg(A) + 1)
		case 0x82: //A = !A ( A = neg(A) + 0)
			*this->load_register = (~(*this->work_register1)&0x0f) + *this->work_register2;
			break;
		case 0x83: //A or B
			*this->load_register = *this->work_register1 | *this->work_register2;
			break;
		case 0x84: //A and B
			*this->load_register = *this->work_register1 & *this->work_register2;
			break;
		case 0x85: //A xor B
			*this->load_register = *this->work_register1 ^ *this->work_register2;
			break;
	}
}

void TD4m_cpu::arithmetic_unit(unsigned char *instruction){
	if(this->load_register == NULL)
		return;

	if(this->work_register2 == NULL){
		*this->load_register = *this->work_register1 + (*instruction & 0x0f);
		return;
	}

	switch(*instruction){
		case 0x86: // A = A - B
			*this->load_register = *this->work_register1 - *this->work_register2;
			break;
		default:
			switch(this->move_bits){
				case 0x01: //X
					*this->load_register = *this->work_register1 << 4;
					break;
				case 0x02: //Y or PCL
					*this->load_register = *this->work_register1 & 0x0f;
					break;
				default: //all cases exept A-B, X, Y, PCL
					*this->load_register = *this->work_register1 + *this->work_register2;
					break;
			}
			break;
	}
}	

void TD4m_cpu::c_flag_handler(){
	if((*this->load_register >= 0x10) & (this->load_register != &this->XY) & (this->load_register != &this->PC)){
		*this->load_register &= 0x0f; 
		this->CF = 0x01;
		this->cnt_cf = 0x02;
	}

	this->cnt_cf -= 0x01;
	if(this->cnt_cf <= 0x00){
		this->CF = 0x00;
		this->cnt_cf = 0x00;
	}	
}

void TD4m_cpu::z_flag_handler(){
	if(this->load_register == 0x00){
		this->ZF = 0x01;
		this->cnt_zf = 0x02;
	}

	this->cnt_zf -= 0x01;
	if(this->cnt_zf <= 0x00){
		this->ZF = 0x00;
		this->cnt_zf = 0x00;
	}
}

void TD4m_cpu::reset(){
	//4bit reg
	this->A = 0x00;
	this->B = 0x00;
	this->input = 0x00;
	this->output = 0x00;

	//8bit reg
	this->PC = 0x00;
	this->XY = 0x00;

	//flags
	this->CF = 0x00;
	this->ZF = 0x00;

	//consts
	this->zero = 0x00;
	this->one = 0x01;

	//accessory
	this->work_register1 = NULL;
	this->work_register2 = NULL;
	this->load_register = NULL;

	//accessory
	this->cnt_cf = 0x00;
	this->cnt_zf = 0x00;
	this->move_bits = 0x00;	
}




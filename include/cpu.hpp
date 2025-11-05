#pragma once
#include <iostream>
#include <fstream>
#include <string>
using namespace std;

class cpu{
public:
	virtual void opcode_decode(unsigned char *) = 0;
	virtual void alu(unsigned char *) = 0;
	virtual void flags_handler() = 0;
};

class TD4m_cpu : public cpu{
public:
	TD4m_cpu();
	~TD4m_cpu();

	unsigned char A;
	unsigned char B;
	unsigned char PC;
	unsigned char XY;
	unsigned char CF;
	unsigned char ZF;
	unsigned char input;
	unsigned char output;
	unsigned char zero;
	unsigned char one;

	unsigned char *RAM;
	unsigned char *ROM;
	unsigned char *work_register1;
	unsigned char *work_register2;
	unsigned char *load_register;

	unsigned char cnt_cf;
	unsigned char cnt_zf;
	unsigned char move_bits;//0 for mostly all reg, 1 for x, 2 for y/pcl 

	void opcode_decode(unsigned char *instruction) override;
	void alu(unsigned char *instruction) override;
	void flags_handler() override;
	void next_step();
	void write_rom(string);
	unsigned char get_instruction();

private:
	void logic_unit(unsigned char *instruction);
	void arithmetic_unit(unsigned char *instruction);
	void c_flag_handler();
	void z_flag_handler();
};

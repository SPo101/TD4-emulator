#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>

#define MEM_SIZE 16

using namespace std;
/*

labels support

*/
int main(int argc, char *argv[]){

	unordered_map<string, unsigned char> mnemo;
	vector<string> inst;


	mnemo["ADDA"] = 0x00;
	mnemo["MOVAB"] = 0x10;
	mnemo["INA"] = 0x20;
	mnemo["MOVA"] = 0x30;
	mnemo["MOVBA"] = 0x40;
	mnemo["ADDB"] = 0x50;
	mnemo["INB"] = 0x60;
	mnemo["MOVB"] = 0x70;

	mnemo["ADDAB"] = 0x80;
	mnemo["NEGA"] = 0x81;
	mnemo["NOTA"] = 0x82;
	mnemo["ORAB"] = 0x83;
	mnemo["ANDAB"] = 0x84;
	mnemo["XORAB"] = 0x85;
	mnemo["SUBAB"] = 0x86;
	mnemo["OUTA"] = 0x87;
	mnemo["LDA"] = 0x88;
	mnemo["STA"] = 0x89;
	mnemo["LDB"] = 0x8a;
	mnemo["STB"] = 0x8b;
	mnemo["MOVXA"] = 0x8c;
	mnemo["MOVYA"] = 0x8d;
	mnemo["INCXY"] = 0x8e;
	mnemo["JMPXY"] = 0x8f;

	mnemo["OUTB"] = 0x90;
	mnemo["JZ"] = 0xa0;
	mnemo["OUT"] = 0xb0;
	mnemo["MOVY"] = 0xc0;
	mnemo["MOVX"] = 0xd0;
	mnemo["JNC"] = 0xe0;
	mnemo["JMP"] = 0xf0;


	fstream infile (argv[1], ios::in);
	fstream outfile (argv[2], ios::out | ios::binary | ios::trunc);

	if(!infile.is_open())
		exit(0);

	int ln;
	int data;
	unsigned char out_value;
	string token;

	for(string input; getline(infile, input);){
		ln = input.length();

		if(!ln){
			out_value = 0x00;
			outfile.write(reinterpret_cast<const char*>(&out_value), sizeof(out_value));
			continue;
		}

		for(int i=0; i<=ln; i++){
			if((input[i] == ' ') | (input[i] == ',') | (input[i] == '\0')){
				inst.push_back(token);
				token.erase();
			}
			else
				token.push_back(input[i]);
		}


		ln = inst.size();
		data = 0;
		if(("A" > inst.at(ln-1)) | ("Z" < inst.at(ln-1))){
			try{
				data = stoul(inst.at(ln-1), nullptr, 16);
			}
			catch (invalid_argument& e){
				data = 0;
			}

			for(int i=0; i<ln-1; i++)
				token+=inst.at(i);
		}
		else
			for(int i=0; i<ln; i++)
				token+=inst.at(i);

		printf("%s %x\n", token.c_str(), static_cast<unsigned char>(data) + mnemo[token]);

		out_value = static_cast<unsigned char>(data) + mnemo[token];
		outfile.write(reinterpret_cast<const char*>(&out_value), sizeof(out_value));

		token.erase();
		inst.clear();
	}
	infile.close();
	outfile.close();
}
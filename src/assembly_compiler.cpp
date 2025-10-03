#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#define MEM_SIZE 16

using namespace std;

struct item{
	string key;
	unsigned value;
};

class hash_table{
public:
	vector <item*> table;
	int size;
	int count;

	hash_table(int size_, int count_ = 0){
		size = size_;
		count = count_;
		table.resize(size, nullptr);
	}

	int put_item(item item_){
		int place = hash_function(item_.key);
		int was_put = 0;
		item *new_item = (item*) malloc(sizeof(item));
		new_item->key = item_.key;
		new_item->value = item_.value;

		for(int i=1; i<=size; i++){
			if( table[place] && table[place]->key.compare(new_item->key) != 0){
				place++;
				place%=size;
			}	
			else{
				was_put = 1;
				break;
			}
		}
		if(!was_put)
			return 0;

		table[place] = new_item;
		count++;
		return 1;
	}

	int put_item(string const key_, unsigned char const value_){
		int place = hash_function(key_);
		int was_put = 0;
		item *new_item = (item*) malloc(sizeof(item));
		new_item->key = key_;
		new_item->value = value_;

		for(int i=1; i<=size; i++){
			if( table[place] && table[place]->key.compare(new_item->key) != 0){
				place++;
				place%=size;
			}	
			else{
				was_put = 1;
				break;
			}
		}
		if(!was_put)
			return 0;

		table[place] = new_item;
		count++;
		return 1;
	}

	int get_item(string const& key){
		int place = hash_function(key);
		int was_get = 0;

		for(int i=1; i<=size; i++){
			if(table[place]->key.compare(key) != 0){
				place++;
				place%=size;
			}	
			else{
				was_get= 1;
				break;
			}
		}
		if(!was_get)
			return -1;

		return place;
	}

	void print_table(){
		/*
		for(item *item_ : table)
			if(item_)
				cout << item_->key << " " << item_->value << endl;
		*/
		for(int i=0; i<size; i++)
			if(table[i])
				printf("%2d %s \t %x\n", i, table[i]->key.c_str(), table[i]->value);
		printf("\nsize=%d count=%d\n", size, count);
	}

	unsigned operator[](int position){
		return table[position]->value;
	}

	unsigned operator[](string const &key){
		return table[this->get_item(key)]->value;
	}

private:
	//djb2 hash.
    int hash_function(string const& key){
        unsigned long hash = 5381;

        for (int c : key)
            hash = ((hash << 5) + hash) + c;

        return hash%size;
    }
};

int main(int argc, char *argv[]){
	hash_table mnemo(16);

	mnemo.put_item("ADD A,", 0x00);
	mnemo.put_item("ADD B,", 0x50);
	mnemo.put_item("MOV A,", 0x30);
	mnemo.put_item("MOV B,", 0x70);
	mnemo.put_item("MOV A,B",  0x10);
	mnemo.put_item("MOV B,A",  0x40);
	mnemo.put_item("JMP ",   0xf0);
	mnemo.put_item("JNC ",   0xe0);
	mnemo.put_item("In A",     0x20);
	mnemo.put_item("In B",     0x60);
	mnemo.put_item("OUT B",    0x90);
	mnemo.put_item("OUT ",   0xb0);

	//mnemo.print_table();

	string data_arr = "0123456789abcdef";
	stringstream str_data;
	unsigned data;
	unsigned char out_value;
	string num;

	fstream inputdata (argv[1], ios::in);
	fstream outputdata (argv[2], ios::out | ios::binary | ios::trunc);

	if(!inputdata.is_open())
		; //handler

	int pos;
	for(string line; getline(inputdata, line);){
		if(!line.size())
			continue;

		num = "0";
		if(data_arr.find(line.back()) != string::npos){
			num = line.back();
			line.pop_back();
			pos = mnemo.get_item(line);

		}
		else
			pos = mnemo.get_item(line);
		
		str_data.clear();
		str_data << hex << num;
		str_data >> data;

		out_value = static_cast<unsigned char>(mnemo[line] + data);
		outputdata.write(reinterpret_cast<const char*>(&out_value), sizeof(out_value));
	}
	inputdata.close();
	outputdata.close();
}








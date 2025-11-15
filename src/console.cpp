#include "console.hpp"

string console_get_input(struct termios *orig_termios, console_args *cargs, int cnt){
    console_raw_mode(orig_termios);

    string str;
    for(;;){

        char c = getchar();

        if((c == 8) | (c == 127)){
            str.pop_back();

            printf("\r\x1b[2K");
        }
        else if((c == '\n') | (c == '\r')){
            printf("\n");
            break;
        }
        else if(c == '\t'){
            help_find(str, cargs, cnt);
        }
        else{
            str += c;
            //handler for tab help func
	        if(console_show_help){
			    console_show_help++;
			    console_show_help %= 2;
		    	printf("\033[%dB\r", 1);
	        	printf("\r\x1b[2K");
		    	printf("\033[%dA\r", 1);
	        }
        }



        printf("\r> %s", str.c_str());
    }
    console_cooked_mode(orig_termios);
    return str;
}

void help_find(string input, console_args *cargs, int cnt){
	console_show_help++;
    console_show_help %= 2;
    int ln = input.length();

    string res;
    for(int i=0; i<cnt; i++)
    	if(cargs[i].lng.find(input) != -1){
    		res += " ";
    		res += cargs[i].lng;
    	}

    printf("\n\rhelp:%s", res.c_str());
    printf("\033[%dA\r", 1);
}

void console_cooked_mode(struct termios *orig_termios) {
    tcgetattr(STDIN_FILENO, orig_termios);

    struct termios raw = *orig_termios;
    raw.c_iflag |= (BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag |= (OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag |= (ECHO | ICANON | IEXTEN | ISIG);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void console_raw_mode(struct termios *orig_termios) {
    tcgetattr(STDIN_FILENO, orig_termios);

    struct termios raw = *orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void console_handle_token(string token, console_args *state, int *arg_pos, emu_args *eargs, TD4m_cpu *td4m){
	unsigned int data = 0;
	//commands with args processing
	if(*arg_pos == -1){
		try{
			data = stoul(token, nullptr, 16);
		}
		catch (invalid_argument& e){
			return;
		}

		if(!state->lng.compare("breakpoint")){
			eargs->bp.push_back(static_cast<unsigned char>(data));
		}
		if(!state->lng.compare("removebreakpoint"))
			eargs->rbp.push_back(static_cast<unsigned char>(data));
		if(!(state->lng.compare("ram")))
			eargs->ram = static_cast<unsigned char>(data);
		if(!state->lng.compare("rom"))
			eargs->rom = static_cast<unsigned char>(data);
		if(!state->lng.compare("steps"))
			eargs->step += static_cast<unsigned char>(data);
	}

	//commands without args processing
	if(!state->lng.compare("showbreakpoint")){
		if(eargs->bp.empty()){
			printf("no breakpoints until\n");
			return;
		}

		sort(eargs->bp.begin(), eargs->bp.end());
		for(unsigned char bp_ : eargs->bp)
			printf("%hhx ", bp_);
		printf("\n");
	}
	if(!state->lng.compare("printcpustate")){
		cpu_print_state(td4m, eargs);
		printf("\033[%dB\r", PRINT_LINES+2);
	}
	if(!state->lng.compare("continue"))
		emu_stop_exec_target(0);
	if(!state->lng.compare("step"))
		eargs->step += 0x01;
	if(!state->lng.compare("exit"))
		exit(0);
	if(!state->lng.compare("restart")){
		emu_stop_exec_target(0);
		eargs->restart = 0x01;
	}
}

console_args console_find_token(string token, console_args *prev_state, console_args *cargs, int cnt, int *arg_pos){
	int argc = 0;
	console_args res;

	res.shrt = "";
	res.lng = "null";
	*arg_pos = -1;
	for(int j=0; j<cnt; j++){
		if(cargs[j].shrt.empty()){
			if(token == cargs[j].lng){
				res.argc = cargs[j].argc;
				res.lng = cargs[j].lng;
				*arg_pos = j;
				break;
			}
			continue;
		}

		if((token == cargs[j].shrt) | (token == cargs[j].lng)){
			res.argc = cargs[j].argc;
			res.lng = cargs[j].lng;
			*arg_pos = j;
			break;
		}
	}

	if(res.lng == "null"){
		switch(prev_state->argc){
		case no_arg:
			res.argc = 0;
			res.lng = "";
			res.shrt = "";
			return res;
		case one_arg:
			prev_state->argc--;
			break;
		case few_arg:
			break;
		}


		res.argc = prev_state->argc;
		res.lng = prev_state->lng;
	}
	return res;
}

string console_parce_input(string input, int *where){
	string token;
	int ln = input.length();

	for(int i = *where; i<ln; i++){
		if(input[i] != ' ')
			token.push_back(input[i]);
		if((input[i] == ' ') | (i == ln-1)){
			*where = i+1;
			return token;
		}
	}
	return token;
}

void console_handle_input(string input, console_args *cargs, int cnt, emu_args *eargs, TD4m_cpu *td4m){
	if(input.empty())
		return;

	string token;
	int arg_pos = 0;
	int where = 0;
	console_args prev_state;
	while(where <= input.length() - 1 ){
		token = console_parce_input(input, &where);
		if(token == "")
			continue;
		prev_state = console_find_token(token, &prev_state, cargs, cnt, &arg_pos);

		if(prev_state.argc)
			console_handle_token(token, &prev_state, &arg_pos, eargs, td4m);

	}
}

#include "console.hpp"

string console_get_input(struct termios *term, console_args *cargs, int cnt){
    console_raw_mode(term);

	printf("\r> ");
	int no = 0;
    string str;
	int pos = -1;
    string old_str;
    string token;
    string new_token;
    for(;;){
        char c = getchar();

        switch(c){
        case 8:
        case 127:
            str.pop_back();
            printf("\r\x1b[2K");
            console_show_help = 0;
	        old_str = str;
	        no = 0;
        	break;

        case '\r':
        case '\n':
            printf("\n\r");
		    console_cooked_mode(term);
		    return str;

        case '\t':
            console_show_help += 1;
        	break;

        default:
            str += c;
            console_show_help = 0;
	        old_str = str;
		    no = 0;
        	break;
        }


        //pass last token of string not full one
        pos = str.rfind(" ");
        if(pos == -1){
        	token = old_str;
        	help_find(&token, cargs, cnt, &no);
        	str = token;
        }
        else{
        	token = old_str.substr(pos+1);
        	help_find(&token, cargs, cnt, &no);
        	str.replace(pos+1, str.length() - pos - 1, token);
        }
        printf("\r\x1b[2K> %s", str.c_str());
    }
    return str;
}

void help_find(string *input, console_args *cargs, int cnt, int *no){
	string str;
	switch(console_show_help){
	case 0:
    	printf("\n\r\x1b[2K");
    	printf("\033[%dA", 1);
		return;
	case 1:
		break;

	case 2:
	    for(int i = *no; i<cnt; i++){
	    	if(cargs[i].lng.find(*input) != -1){
				*input = cargs[i].lng;
			    *no = i+1;
			    *no %= cnt;
				break;
	    	}
	    	*no = 0;
	    }

        console_show_help --;
		break;

	}


    int ln = input->length();

    string res;
    for(int i=0; i<cnt; i++)
    	if(cargs[i].lng.find(*input) != -1){
    		res += " ";
    		res += cargs[i].lng;
    	}

    printf("\n\x1b[2K\rhelp:%s", res.c_str());
    printf("\033[%dA\r", 1);
}

void console_cooked_mode(struct termios *term) {
    tcgetattr(STDIN_FILENO, term);

    struct termios cooked = *term;
    cooked.c_iflag |= (BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    cooked.c_oflag |= (OPOST);
    cooked.c_cflag |= (CS8);
    cooked.c_lflag |= (ECHO | ICANON | IEXTEN | ISIG);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

void console_raw_mode(struct termios *term) {
    tcgetattr(STDIN_FILENO, term);

    struct termios raw = *term;
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

		if(!state->lng.compare("breakpoint"))
			eargs->bp.push_back(static_cast<unsigned char>(data));
		if(!state->lng.compare("removebreakpoint"))
			eargs->bp.erase(remove(eargs->bp.begin(), eargs->bp.end(), static_cast<unsigned char>(data)), eargs->bp.end());
		if(!(state->lng.compare("ram")))
			eargs->ram = static_cast<unsigned char>(data);
		if(!state->lng.compare("rom"))
			eargs->rom = static_cast<unsigned char>(data);
		if(!state->lng.compare("steps"))
			eargs->step += static_cast<unsigned char>(data);

		return;
	}

	//commands without args processing
	if(!state->lng.compare("showbreakpoint")){
		if(eargs->bp.empty()){
			printf("\r\x1b[2Kno breakpoints until\n");
			return;
		}

		sort(eargs->bp.begin(), eargs->bp.end());
		printf("\r\x1b[2K");
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
	if(!state->lng.compare("exit")){
		printf("\r");
		exit(0);
	}
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

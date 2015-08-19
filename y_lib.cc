#include "y_lib.h"

t_int v_argc;
t_list<t_string> v_argv;

void f_0_print(std::string s) {
	std::cout << s;
}

t_string f_0_input(void) {
	std::string s;
	std::getline(std::cin,s);
	return s;
}

t_string f_0_to_string( t_int x ) {
	return x.get_str();
}
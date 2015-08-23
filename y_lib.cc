#include "y_lib.h"

t_int v_argc;
t_list<t_string> v_argv;
std::default_random_engine random_generator;

void f_0_print(std::string s) {
	std::cout << s;
}

t_string f_0_input(void) {
	std::string s;
	std::getline(std::cin,s);
	return s;
}
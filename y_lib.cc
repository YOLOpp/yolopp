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

t_string f_0_to_string( t_float x ) {
	mp_exp_t e;
	t_string s = x.get_str( e );
	return s + "*10^" + std::to_string(e);
}

t_string f_0_to_string( t_rat x ) {
	return x.get_str();
}


t_float f_0_to_float(t_rat x) {
	return t_float( x );
}

t_rat f_0_to_rat( t_int x ) {
	return t_rat( x );
}
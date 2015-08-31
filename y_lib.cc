#include "y_lib.h"

t_int v_argc;
t_list<t_string> v_argv;
std::default_random_engine random_generator;

void f_0_print( t_string s) {
	std::cout << s;
}

t_string f_0_input(void) {
	std::string s;
	std::getline(std::cin,s);
	return s;
}

t_tuple<t_bool,t_int> f_0_parseInt( const t_string& s, t_int base ) {
	try {
		t_int x( s, cast<size_t>( base ) );
		return std::make_pair( true, x );
	} catch( std::invalid_argument e ) {
		return std::make_pair( false, t_int(0) );
	}
}

t_tuple<t_bool,t_real> f_0_parseReal( const t_string& s, t_int base ) {
	try {
		t_real x( s, mpf_get_default_prec(), cast<size_t>( base ) );
		return std::make_pair( true, x );
	} catch( std::invalid_argument e ) {
		return std::make_pair( false, t_int(0) );
	}
}
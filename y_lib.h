#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include <vector>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>

// typedef mpz_class t_int;

#define t_list std::vector
#define t_set std::set

typedef std::string t_string;
typedef mpz_class t_int;
typedef mpq_class t_rat;
typedef mpf_class t_float;

extern t_int v_argc;
extern t_list<t_string> v_argv;
extern std::default_random_engine random_generator;

void f_0_print( t_string s );
t_string f_0_input(void);
t_string f_0_to_string(t_int x);
t_string f_0_to_string(t_float x);
t_string f_0_to_string(t_rat x);
t_float f_0_to_float(t_rat x);
t_rat f_0_to_rat( t_int x );

template<typename T> t_list<T> shuffle( t_list<T>& m ) {
	t_list<T> l( m );
	shuffle( l.begin(), l.end(), random_generator );
	return l;
}

template<typename T> t_list<T> shuffle( t_list<T>&& m ) {
	t_list<T> l( std::move( m ) );
	shuffle( l.begin(), l.end(), random_generator );
	return l;
}

template<typename T,typename S> T cast( const S& o ) {
	return static_cast<T>( o );
}

template<> t_string cast( const t_int& o ) {
	return o.get_str();
}

template<> t_string cast( const t_rat& o ) {
	return o.get_str();
}

template<> t_string cast( const t_float& o ) {
	mp_exp_t e;
	t_string s = o.get_str( e );
	return "0." + s + "*10^" + std::to_string(e);
}


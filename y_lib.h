#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include <vector>

// typedef mpz_class t_int;

#define t_list std::vector

typedef std::string t_string;
typedef mpz_class t_int;
typedef mpq_class t_rat;
typedef mpf_class t_float;


/*class t_string : public std::string {

};

class t_int : public mpz_class {

};

class t_rat : public mpq_class {

};*/

/*class t_frac {
	mpz_class n, d;
	void simplify();
public:
	t_frac& operator+=( const t_frac& );
	t_frac& operator-=( const t_frac& );
	t_frac& operator*=( const t_frac& );
	t_frac& operator/=( const t_frac& );
	bool operator==( const t_frac& );
	bool operator<( const t_frac& );
	bool operator>( const t_frac& );
	bool operator<=( const t_frac& );
	bool operator>=( const t_frac& );
	t_frac operator+( t_frac );
	t_frac operator-( t_frac );
	t_frac operator*( t_frac );
	t_frac operator/( t_frac );

};

void t_frac::simplify() {
	mpz_t g = mpz_gcd( n, d );
	n /= g;
	d /= g;
}

t_frac& t_frac::operator+=( const t_frac& ) {
	t_frac t = d;
	d *= other.d;
	n = n * other.d + other.n * t;
	return *this;
}

t_frac& t_frac::operator-=( const t_frac& ) {
	t_frac t = d;
	d *= other.d;
	n = n * other.d + other.n * t;
	return *this;
}

t_frac& t_frac::operator*=( const t_frac& ) {

	return *this;
}

t_frac& t_frac::operator/=( const t_frac& ) {

	return *this;
}

bool t_frac::operator==( const t_frac& ) {

	return *this;
}

bool t_frac::operator<( const t_frac& ) {

}

bool t_frac::operator>( const t_frac& );
bool t_frac::operator<=( const t_frac& );
bool t_frac::operator>=( const t_frac& );
t_frac t_frac::operator+( t_frac );
t_frac t_frac::operator-( t_frac );
t_frac t_frac::operator*( t_frac );
t_frac t_frac::operator/( t_frac );
*/

void f_0_print( t_string s );
t_string f_0_input(void);
t_string f_0_to_string(t_int x);
t_string f_0_to_string(t_float x);
t_string f_0_to_string(t_rat x);
t_float f_0_to_float(t_rat x);
t_rat f_0_to_rat( t_int x );

extern t_int v_argc;
extern t_list<t_string> v_argv;
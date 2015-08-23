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
typedef bool t_bool;

extern t_int v_argc;
extern t_list<t_string> v_argv;
extern std::default_random_engine random_generator;

void f_0_print( t_string s );
t_string f_0_input(void);

template<typename T> t_list<T> shuffle( t_list<T>& m ) {
	t_list<T> l( m );
	std::shuffle( l.begin(), l.end(), random_generator );
	return l;
}

template<typename T> t_list<T> shuffle( t_list<T>&& m ) {
	t_list<T> l( std::move( m ) );
	std::shuffle( l.begin(), l.end(), random_generator );
	return l;
}

template<typename T> t_list<T> operator!( t_list<T>& m ) {
	t_list<T> l( m );
	std::sort( l.begin(), l.end() );
	return l;
}

template<typename T> t_list<T> operator!( t_list<T>&& m ) {
	t_list<T> l( std::move( m ) );
	std::sort( l.begin(), l.end() );
	return l;
}

template<typename T> t_int contains( const t_list<T>& x, const T& y ) {
	return std::count( x.begin(), x.end(), y );
}

template<typename T> t_bool contains( const t_set<T>& x, const T& y ) {
	return t_bool( x.count(y) );
}

template<typename T> t_int size_of( const t_list<T>& x ) {
	return x.size();
}

template<typename T> t_int size_of( const t_set<T>& x ) {
	return x.size();
}

template<typename T> struct cast_helper {
	template<typename U> static T cast( const U& o ) {
		return static_cast<T>( o );
	}
	// Replace with the following if we want to catch cast errors in the C++ compiler instead of the YOLO++ compiler
	/*static T cast( const t_int& o ) {
		return static_cast<T>( o );
	}
	static T cast( const t_rat& o ) {
		return static_cast<T>( o );
	}
	static T cast( const t_float& o ) {
		return static_cast<T>( o );
	}*/
};

template<> struct cast_helper<t_string> {
	static t_string cast( const t_int& o ) {
		return o.get_str();
	}
	static t_string cast( const t_rat& o ) {
		return o.get_str();
	}
	static t_string cast( const t_float& o ) {
		mp_exp_t e;
		t_string s = o.get_str( e );
		return "0." + s + "*10^" + std::to_string(e);
	}
	static t_string cast( const t_string& o ) {
		return o;
	}
	template<typename U> static t_string cast( const t_list<U>& o ) {
		t_string x = "[";
		bool comma = false;
		for( const U& p: o ) {
			if( comma )
				x += ",";
			comma = true;
			x += cast_helper<t_string>::cast( p );
		}
		return x + "]";
	}
	template<typename U> static t_string cast( const t_set<U>& o ) {
		t_string x = "{";
		bool comma = false;
		for( const U& p: o ) {
			if( comma )
				x += ",";
			comma = true;
			x += cast_helper<t_string>::cast( p );
		}
		return x + "}";
	}
};

template<typename S, typename T> S cast( const T& x ) {
	return cast_helper<S>::cast( x );
}

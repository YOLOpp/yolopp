#include <iostream>
#include <string>
#include <gmp.h>
#include <gmpxx.h>
#include <vector>
#include <set>
#include <algorithm>
#include <random>
#include <chrono>
#include <cstring>

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


template<typename T> struct cast_helper;
template<typename S, typename T> S cast( const T& x ) {
	return cast_helper<S>::cast_f( x );
}

template<typename X> struct t_range {
	typedef X			value_type;
	typedef X*			pointer;
	typedef const X*	const_pointer;
	typedef X&			reference;
	typedef const X&	const_reference;
	typedef size_t 		size_type;
	typedef X			difference_type;

	value_type a, b;
	difference_type i;

	struct iterator {
		typedef std::bidirectional_iterator_tag	iterator_category;
		typedef X			value_type;
		typedef X*			pointer;
		typedef const X*	const_pointer;
		typedef X&			reference;
		typedef const X&	const_reference;
		typedef size_t 		size_type;
		typedef X			difference_type;
		typedef t_range<X>::iterator self_type;

		const t_range<X>* p;
		value_type c;

		iterator( const value_type& q, const t_range<X>* r ) :  p(r), c(q) {};
		iterator() : c(-1), p(nullptr) {};

		reference operator*() { return c; }  // assigning to this will break everything
		pointer operator->() { return &(operator*()); }

		self_type &operator+=(difference_type n) { c += n; if( c >= p->b ) c = p->b; else if( c < p->a ) c = p->a; return *this; }
		self_type &operator-=(difference_type n) { return operator+=( -n ); }
		self_type operator+( difference_type n ) const { return std::copy(*this) += n; }
		self_type operator-(difference_type n) const { return std::copy(*this) -= n; }
		
		bool operator==(const self_type &other) const { return c == other.c; }
		bool operator!=(const self_type &other) const { return c != other.c; }

		self_type &operator++() { return operator+=(p->i); }
		self_type operator++(int) { self_type r = std::copy(*this); operator+=(p->i); return r; }
		self_type &operator--() { return operator-=(p->i); }
		self_type operator--(int) { self_type r = std::copy(*this); operator-=(p->i); return r; }
	};

	value_type front() { return a; }
	value_type back() { return b; }
	iterator begin() const { return iterator(a,this); }
	iterator end() const { return iterator(b,this); }
	size_type size() const { return cast<size_t,X>((b - a) / i); }

	t_range( value_type x, value_type y, difference_type z = 1 ) : a(x), b(y), i(z) {};

	operator t_list<X>() const { t_list<X> r; r.reserve( size() ); for( const X& x : *this ) r.push_back( x ); return r; }
};

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
	template<typename U> static T cast_f( const U& o ) {
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

template<> struct cast_helper<size_t> {
	static size_t cast_f( const t_int& o ) {
		return o.get_ui();
	}
	static size_t cast_f( const t_float& o ) {
		return cast<size_t>( cast<t_int>( o ) );
	}
	static size_t cast_f( const t_rat& o ) {
		return cast<size_t>( cast<t_float>( o ) );
	}
};

template<> struct cast_helper<t_string> {
	static t_string cast_f( const t_int& o ) {
		return o.get_str();
	}
	static t_string cast_f( const t_rat& o ) {
		return o.get_str();
	}
	static t_string cast_f( const t_float& o ) {
		mp_exp_t e;
		t_string s = o.get_str( e );
		return "0." + s + "*10^" + std::to_string(e);
	}
	static t_string cast_f( const t_string& o ) {
		return o;
	}
	template<typename U> static t_string cast_f( const t_list<U>& o ) {
		t_string x = "[";
		bool comma = false;
		for( const U& p: o ) {
			if( comma )
				x += ",";
			comma = true;
			x += cast_helper<t_string>::cast_f( p );
		}
		return x + "]";
	}
	template<typename U> static t_string cast_f( const t_set<U>& o ) {
		t_string x = "{";
		bool comma = false;
		for( const U& p: o ) {
			if( comma )
				x += ",";
			comma = true;
			x += cast_helper<t_string>::cast_f( p );
		}
		return x + "}";
	}
};


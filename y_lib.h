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
#include <initializer_list>
#include <tuple>
#include <utility>

// typedef mpz_class t_int;

//#define t_list std::vector
#define t_set std::set
#define t_tuple std::tuple


typedef std::string t_string;
typedef mpz_class t_int;
typedef mpq_class t_rat;
typedef mpf_class t_float;
typedef bool t_bool;
struct t_null {};

#define v_null (t_null())

template<typename T> class t_list_item;
template<typename X> struct t_range;
template<typename T> struct t_frame;
template<typename T> class t_list {	
	friend class t_list_item<T>;
	friend struct t_range<T>;
	friend struct t_frame<T>;

	std::vector<T> v;

	T& _at( size_t i ) { return v.at(i); }
	T&& _mov( size_t i ) { return std::move( v.at(i) ); }
public:
	size_t size() const { return v.size(); }

	typename std::vector<T>::iterator begin() { return v.begin(); }
	typename std::vector<T>::iterator end() { return v.end(); }
	typename std::vector<T>::const_iterator begin() const { return v.begin(); }
	typename std::vector<T>::const_iterator end() const { return v.end(); }

	t_list& push_back( const T& x ) { v.push_back( x ); return *this; }
	t_list& push_back( T&& x ) { v.push_back( x ); return *this; }
	t_list& pop_back() { v.pop_back(); return *this; }

	void erase( size_t i ) { v.erase( v.begin()+i ); }
	void erase( size_t a, size_t b ) { v.erase( v.begin()+a, v.begin()+b ); }

	t_list() {}
	t_list( std::initializer_list<T>&& l ) : v( l ) {}
	t_list_item<T> at( size_t i ) { return t_list_item<T>( *this, i ); }
	t_list_item<T> front() { return at( 0 ); }
	t_list_item<T> back() { return at( size()-1 ); }
};

extern t_int v_argc;
extern t_list<t_string> v_argv;
extern std::default_random_engine random_generator;


template<typename T> struct cast_helper;
template<typename S, typename T> S cast( const T& x ) {
	return cast_helper<S>::cast_f( x );
}

template<typename T> t_list_item<T> at( t_list<T>& x, const t_int& y );
//template<typename T> T& at( t_list<T>&& x, const t_int& y );
template<typename T> t_list<T> at( t_list<T>& x, const t_list<t_int>& y );
template<typename T> t_list<T> at( t_list<T>&& x, const t_list<t_int>& y );

template<typename T> class t_list_item {
public:
	t_list<T>& p;
	size_t i;

	t_list_item( t_list<T>& parent, size_t index ) : p( parent ), i( index ) {}

	T& _ref() { return p._at(i); }
	T&& _mov() { return std::move( p._mov(i) ); }

	T& operator=( const T& other ) { return p.at(i) = other; }
	t_null operator=( t_null ) { p.erase( i ); return v_null; }
	operator T() const { return p._at(i); }
};

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

	operator t_list<X>() const { t_list<X> r; r.v.reserve( size() ); for( const X& x : *this ) r.push_back( x ); return r; }
};

namespace std {
template<typename T> void swap( t_list_item<T> a, t_list_item<T> b ) {
	T temp = std::move( a._mov() );
	a._ref() = std::move( b._mov() );
	b._ref() = std::move( temp );
}
}

template<typename T> struct t_frame {
	size_t _a, _b;
	t_list<T>& _f;

	t_frame( t_list<T>& f, size_t a, size_t b ) : _a(a), _b(b), _f(f) {};

	struct iterator {
		typedef std::bidirectional_iterator_tag	iterator_category;
		typedef T			value_type;
		typedef T*			pointer;
		typedef const T*	const_pointer;
		typedef T&			reference;
		typedef const T&	const_reference;
		typedef size_t 		size_type;
		typedef T			difference_type;
		typedef t_frame<T>::iterator self_type;

		const t_frame<T>* p;
		size_t c;

		iterator( const size_t& q, const t_frame<T>* r ) :  p(r), c(q) {};
		iterator() : c(-1), p(nullptr) {};

		reference operator*() { return p->_f._at(c); }  // assigning to this will break everything
		pointer operator->() { return &(operator*()); }

		self_type &operator+=(size_t n) { c += n; if( c >= p->_b ) c = p->_b; else if( c < p->_a ) c = p->_a; return *this; }
		self_type &operator-=(size_t n) { return operator+=( -n ); }
		self_type operator+( size_t n ) const { return std::copy(*this) += n; }
		self_type operator-(size_t n) const { return std::copy(*this) -= n; }
		
		bool operator==(const self_type &other) const { return c == other.c && p == other.p; }
		bool operator!=(const self_type &other) const { return c != other.c || p != other.p; }

		self_type &operator++() { return operator+=(1); }
		self_type operator++(int) { self_type r = std::copy(*this); operator+=(1); return r; }
		self_type &operator--() { return operator-=(1); }
		self_type operator--(int) { self_type r = std::copy(*this); operator-=(1); return r; }
	};

	T front() { return _f.at(_a); }
	T back() { return _f.at(_b-1); }
	iterator begin() const { return iterator(_a,this); }
	iterator end() const { return iterator(_b,this); }
	size_t size() const { return _b-_a; }

	// TODO: add all required operators
	template<typename S> t_frame& operator+=( const S& other ) { for( auto& x : t_range<t_int>( _a, _b ) ) at(_f,x).operator+=( other ); return *this; }
	template<typename S> t_frame& operator-=( const S& other ) { for( auto& x : t_range<t_int>( _a, _b ) ) at(_f,x).operator-=( other ); return *this; }
	template<typename S> t_frame& operator*=( const S& other ) { for( auto& x : t_range<t_int>( _a, _b ) ) at(_f,x).operator*=( other ); return *this; }
	template<typename S> t_frame& operator/=( const S& other ) { for( auto& x : t_range<t_int>( _a, _b ) ) at(_f,x).operator/=( other ); return *this; }
	t_frame& operator= ( const T& other ) { for( auto& x : t_range<size_t>( _a, _b ) ) _f._at(x) = other; return *this; }
	t_frame& operator= ( t_null ) { _f.erase( _a, _b ); _b = _a; return *this; }

	operator t_list<T>() const { t_list<T> r; r.v.reserve( _b-_a ); for( const T& x : *this ) r.push_back( x ); return r; }
};

void f_0_print( t_string s );
t_string f_0_input(void);

// TODO: add inplace shuffle (not return a copy)
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

// TODO: add inplace sort (not return a copy)
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

template<typename T> t_list_item<T> at( t_list<T>& x, const t_int& y ) {
	return x.at( cast<size_t>(y) );
}

template<typename T> t_list_item<T> at( t_list<T>&& x, const t_int& y ) {
	return x.at( cast<size_t>(y) );
}

template<typename T> t_frame<T> at( t_list<T>& x, t_range<t_int> r ) {
	return t_frame<T>( x, cast<size_t>(r.front()), cast<size_t>(r.back()) );
}

template<typename T> t_list<T> at( t_list<T>& x, const t_list<t_int>& y ) {
	t_list<T> r;
	for( const auto& z : y )
		r.push_back( x.at( cast<size_t>( z ) ) );
	return r;
}

template<typename T> t_list<T> at( t_list<T>&& x, const t_list<t_int>& y ) {
	t_list<T> r;
	for( const auto& z : y )
		r.push_back( x.at( cast<size_t>( z ) ) );
	return r;
}

template<typename T> t_list<T> at( t_list<T>&& x, t_range<t_int> y ) {
	t_list<T> r;
	for( const auto& z : y )
		r.push_back( x.at( cast<size_t>( z ) ) );
	return r;
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

template<typename T> void f_0_push( t_list<T>& x, const T& y ) {
	x.push_back( y );
}

template<typename T> T f_0_pop( t_list<T>& x ) {
	T r = x.back();
	x.pop_back();
	return r;
}

template<typename T> void f_0_insert( t_set<T>& x, const T& y ) {
	x.insert( y );
}

template<typename T> void f_0_remove( t_set<T>& x, const T& y ) {
	x.erase( y );
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


template<size_t x> struct tuple_cast_helper {
	template<typename... U> static void cast_f( const t_tuple<U...>& o, t_string& s ) {
		tuple_cast_helper<x-1>::cast_f( o, s );
		s += "," + cast<t_string>( std::get<x>( o ) );
	}
};

template<> struct tuple_cast_helper<0> {
	template<typename... U> static void cast_f( const t_tuple<U...>& o, t_string& s ) {
		s += cast<t_string>( std::get<0>( o ) );
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
	template<typename U> static t_string cast_f( const t_frame<U>& o ) {
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
	/*template<typename U> static t_string cast_f( const t_list_item<U>& o ) {
		std::cerr << "list_item cast" << std::endl;
		return cast_helper<t_string>::cast_f<U>( U(o) );
	}*/
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
	template<typename... U> static t_string cast_f( const t_tuple<U...>& o ) {
		t_string s = "(";
		tuple_cast_helper<sizeof...(U)-1>::cast_f( o, s );
		s += ")";
		return s;
	}
};

template<size_t x,typename U> t_list_item<U> special_at( t_list<U>& l ) {
	return at( l, x );
}

template<size_t x,typename U> t_list_item<U> special_at( t_list<U>&& l ) {
	return at( l, x );
}

template<size_t x,typename... U> typename std::tuple_element<x, std::tuple<U...>>::type& special_at( t_tuple<U...>& l ) {
	return std::get<x>( l );
}
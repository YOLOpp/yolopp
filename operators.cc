#include <iostream>
#include <ctime>
#include <future>
#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cassert>

namespace yolo {

template<typename T> inline T& swap( T& a, T& b ) {
	T temp = a;
	a = b;
	b = temp;
	return a;
}

template<typename T, typename... Args> inline T& swap( T& x, T& y, T& z, Args&... args ) {
	return yolo::swap( x, yolo::swap( y, z, args... ) );
}

/*template<typename T, int N> class vector {
	
public:
	T data[N];

	T operator*( const yolo::vector< T, N >& other ) const;
	yolo::vector< T, N >& operator+=( const yolo::vector< T, N >& other );
	yolo::vector< T, N >& operator+( const yolo::vector< T, N >& other );
	T mult_subroutine( const yolo::vector< T, N >* other, int off ) const;
};

#define chunk_size 200

template<typename T, int N> T vector<T,N>::operator*( const yolo::vector< T, N >& other ) const {
	constexpr int thread_count = N / chunk_size;
	std::atomic<T> sum; // must auto-init to 0
	array<std::thread, thread_count> threads;
	int offset = 0;
	for( j = 0; j < thread_count; j++ ) {
		threads[j] = std::thread( yolo::vector<T,N>::mult_subroutine, other, offset, sum );
		offset += chunk_size;
	}
	for( int i = offset; i < N; i++ )
		sum.fetch_add( operator[]( i ) * other.operator[]( i ) );
	for( int i = 0; i < thread_count; i++ )
		threads[i].join();
	return T(sum);
}

template<typename T, int N> T vector<T,N>::mult_subroutine( const yolo::vector< T, N >* other, int off ) const {
	T intermediate = 0;
	for( int i = off; i < off+chunk_size; i++ )
		intermediate += other->data[i] * data[i];
	return intermediate;
}

template<typename T, int N> T vector<T,N>::operator*( const yolo::vector< T, N >& other ) const {
	constexpr int thread_count = N / chunk_size;
	T sum = 0;
	std::future<T> handle[thread_count];
	for( int i = 0; i < thread_count; i++ )
		handle[i] = std::async( std::launch::async, &vector<T,N>::mult_subroutine, this, &other, i*chunk_size );
	for( int i = thread_count*chunk_size; i < N; i++ )
		sum += data[i] * other.data[i];
	for( int i = 0; i < thread_count; i++ )
		sum += handle[i].get();
	return sum;
}*/

};

template<typename T, int N> T regular_mult( const T a[N], const T b[N] ) {
	T r = 0;
	for( int i = 0; i < N; i++ )
		r += a[i] * b[i];
	return r;
}

template<typename T, int N> T split_mult_async( const T* a, const T* b ) {
	auto handle = std::async( std::launch::async, &regular_mult<T,N>, a, b );
	T x = regular_mult<T,N>( a + N, b + N );
	return x + handle.get();
}

template<typename T, int N> void split_mult_thread_subroutine( const T* a, const T* b, T* r ) {
	for( int i = 0; i < N; i++ )
		(*r) += a[i] * b[i];
}

template<typename T, int N> T split_mult_thread( const T* a, const T* b ) {
	T y = 0;
	std::thread t( split_mult_thread_subroutine<T,N>, a, b, &y );
	T x = regular_mult<T,N>( a + N, b + N );
	t.join();
	return x + y;
}

template<typename T, int N> struct parameters_t {
	const T* a; 
	const T* b; 
	T* const y;
};

template<typename T, int N> void* split_mult_C_subroutine( void* data ) {
	parameters_t<T,N>& parameters = *reinterpret_cast<parameters_t<T,N>*>( data );
	for( int i = 0; i < N; i++ )
		(*parameters.y) += parameters.a[i] * parameters.b[i];
	return nullptr;
}

template<typename T, int N> T split_mult_C( const T* a, const T* b ) {
	parameters_t<T,N> parameters = { a + N, b + N, new T(0) };
	pthread_attr_t attributes;
	pthread_t thread;
	pthread_attr_init( &attributes );
	pthread_attr_setdetachstate( &attributes, PTHREAD_CREATE_JOINABLE );
	pthread_create( &thread, &attributes, &split_mult_C_subroutine<T,N>, reinterpret_cast<void*>( &parameters ) );
	int x = regular_mult<T,N>( a, b );
	pthread_join( thread, NULL );
	x += *parameters.y;
	delete parameters.y;
	return x;
}


int main() {
	const int N = 1000000;
	int a[N], b[N];
	std::clock_t start, end;

	for( int i = 0; i < N; i++ ) {
		a[i] = rand() % 5;
		b[i] = rand() % 5;
	}

	start = std::clock();
	int w = regular_mult<int, N>( a, b );
	end = std::clock();
	std::cout << "regular: " << (end - start) << " " << w << std::endl;

	start = std::clock();
	int x = split_mult_async<int, N / 2>( a, b );
	end = std::clock();
	std::cout << "async C++: " << (end - start) << " " << x << std::endl;

	start = std::clock();
	int y = split_mult_thread<int, N / 2>( a, b );
	end = std::clock();
	std::cout << "thread C++: " << (end - start) << " " << y << std::endl;

	start = std::clock();
	int z = split_mult_C<int, N / 2>( a, b );
	end = std::clock();
	std::cout << "pthread C: " << (end - start) << " " << z << std::endl;

	return 0;
}
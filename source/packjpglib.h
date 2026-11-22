// packJPGlib.h - function declarations for the packJPG library
#ifndef PACKJPGLIB_H
#define PACKJPGLIB_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#define PJGLIB_MSG_SIZE 128

#if defined(BUILD_DLL)
	#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
		#define PJGLIB_API __declspec( dllexport )
	#elif defined(__GNUC__) && ( __GNUC__ >= 4 )
		#define PJGLIB_API __attribute__( ( visibility( "default" ) ) )
	#else
		#define PJGLIB_API extern
	#endif
#elif defined(USE_PACKJPG_DLL) && !defined(BUILD_LIB)
	#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
		#define PJGLIB_API __declspec( dllimport )
	#else
		#define PJGLIB_API extern
	#endif
#else
	#define PJGLIB_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------
	function declarations: library only functions
	-----------------------------------------------
	Note: `msg` may be NULL or a buffer of at least
	PJGLIB_MSG_SIZE (128) bytes. Buffers returned by
	pjglib_convert_stream2mem() should be released
	via pjglib_free(). */

PJGLIB_API bool pjglib_convert_stream2stream( char* msg );
PJGLIB_API bool pjglib_convert_file2file( const char* in, const char* out, char* msg );
PJGLIB_API bool pjglib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
PJGLIB_API void pjglib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
PJGLIB_API void pjglib_free( void* ptr );
PJGLIB_API const char* pjglib_version_info( void );
PJGLIB_API const char* pjglib_short_name( void );

#ifdef __cplusplus
}
#endif

/* a short reminder about input/output stream types
   for the pjglib_init_streams() function
	
	if input is file
	----------------
	in_src -> name of input file
	in_type -> 0
	in_size -> ignore
	
	if input is memory
	------------------
	in_src -> array containing data
	in_type -> 1
	in_size -> size of data array
	
	if input is *FILE (f.e. stdin)
	------------------------------
	in_src -> stream pointer
	in_type -> 2
	in_size -> ignore
	
	vice versa for output streams! */

#endif /* PACKJPGLIB_H */

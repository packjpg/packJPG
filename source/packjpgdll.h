// packJPGdll.h - function import declarations for the packJPG DLL
#ifndef PACKJPGDLL_H
#define PACKJPGDLL_H

#ifndef __cplusplus
#include <stdbool.h>
#endif

#if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__)
	#define PJGDLL_API __declspec( dllimport )
#else
	#define PJGDLL_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* -----------------------------------------------
	function declarations: library only functions
	----------------------------------------------- */

PJGDLL_API bool pjglib_convert_stream2stream( char* msg );
PJGDLL_API bool pjglib_convert_file2file( const char* in, const char* out, char* msg );
PJGDLL_API bool pjglib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg );
PJGDLL_API void pjglib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type );
PJGDLL_API const char* pjglib_version_info( void );
PJGDLL_API const char* pjglib_short_name( void );

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

#endif /* PACKJPGDLL_H */


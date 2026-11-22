// packJPGdll.h - function import declarations for the packJPG DLL
#ifndef PACKJPGDLL_H
#define PACKJPGDLL_H

#if !defined(BUILD_DLL) && !defined(BUILD_LIB) && !defined(USE_PACKJPG_DLL)
	#define USE_PACKJPG_DLL 1
#endif

#include "packjpglib.h"

#ifndef PJGDLL_API
	#define PJGDLL_API PJGLIB_API
#endif

#endif /* PACKJPGDLL_H */

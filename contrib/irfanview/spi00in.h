#ifndef PACKJPG_IRFANVIEW_SPI00IN_H
#define PACKJPG_IRFANVIEW_SPI00IN_H

#include <windows.h>
#include <cstddef>
#include <cstdint>

// Susie '00IN' Image Plug-in ABI (32-bit .spi / 64-bit .sph)
// Loaded natively by IrfanView, XnView, Susie, and Total Commander.

#pragma pack(push, 1)
struct SusiePictureInfo {
	LONG   left;
	LONG   top;
	LONG   width;
	LONG   height;
	WORD   x_density;
	WORD   y_density;
	short  colorDepth;
#ifdef _WIN64
	char   dummy[ 2 ];
#endif
	HLOCAL hInfo;
};
#pragma pack(pop)

#ifdef _WIN64
static_assert( offsetof( SusiePictureInfo, hInfo ) == 24, "64-bit SusiePictureInfo::hInfo must sit at offset 24" );
static_assert( sizeof( SusiePictureInfo ) == 32, "64-bit SusiePictureInfo must be 32 bytes" );
#else
static_assert( offsetof( SusiePictureInfo, hInfo ) == 22, "32-bit SusiePictureInfo::hInfo must sit at offset 22" );
static_assert( sizeof( SusiePictureInfo ) == 26, "32-bit SusiePictureInfo must be 26 bytes" );
#endif

enum SusieResult : int {
	SPI_NO_FUNCTION      = -1,
	SPI_ALL_RIGHT        =  0,
	SPI_ABORT            =  1,
	SPI_NOT_SUPPORT      =  2,
	SPI_OUT_OF_ORDER     =  3,
	SPI_NO_MEMORY        =  4,
	SPI_MEMORY_ERROR     =  5,
	SPI_FILE_READ_ERROR  =  6,
	SPI_WINDOW_ERROR     =  7,
	SPI_OTHER_ERROR      =  8,
	SPI_FILE_WRITE_ERROR =  9,
	SPI_END_OF_FILE      = 10,
};

constexpr unsigned int SPI_INPUT_MASK   = 0x07u;
constexpr unsigned int SPI_INPUT_FILE   = 0x00u;
constexpr unsigned int SPI_INPUT_MEMORY = 0x01u;

using SpiProgressCallback = int (CALLBACK *)( int num, int denom, LONG_PTR user_data );

#endif

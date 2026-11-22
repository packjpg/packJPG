#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "spi00in.h"
#include "codec_pipeline.h"
#include "packjpglib.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

#define CHECK_COND(expr)                                                                         \
	do {                                                                                         \
		if ( !( expr ) ) {                                                                       \
			std::fprintf( stderr, "FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #expr );             \
			std::exit( 1 );                                                                      \
		}                                                                                        \
	} while ( false )

template <typename Fn>
Fn load_proc( HMODULE mod, const char* name )
{
	FARPROC raw = GetProcAddress( mod, name );
	if ( raw == nullptr ) {
		std::fprintf( stderr, "Missing exported symbol: %s\n", name );
		std::exit( 1 );
	}
	static_assert( sizeof( Fn ) == sizeof( FARPROC ), "function pointer size mismatch" );
	Fn fn = nullptr;
	std::memcpy( &fn, &raw, sizeof( fn ) );
	return fn;
}

using GetPlugInInfoFn = int ( __cdecl * )( char*, char* );
using ReadPJGWFn      = HANDLE ( __cdecl * )( LPCWSTR, UINT, LPWSTR );
using ReadPJGFn       = HANDLE ( __cdecl * )( LPCSTR, UINT, LPSTR );
using SavePJGWFn      = int ( __cdecl * )( LPCWSTR, HANDLE, int, LPWSTR );
using SavePJGFn       = int ( __cdecl * )( LPCSTR, HANDLE, int, LPSTR );

using SpiGetPluginInfoFn   = int ( __stdcall * )( int, LPSTR, int );
using SpiGetPluginInfoWFn  = int ( __stdcall * )( int, LPWSTR, int );
using SpiIsSupportedWFn    = int ( __stdcall * )( LPCWSTR, DWORD_PTR );
using SpiGetPictureInfoWFn = int ( __stdcall * )( LPCWSTR, LONG_PTR, unsigned int, SusiePictureInfo* );
using SpiGetPictureWFn     = int ( __stdcall * )( LPCWSTR, LONG_PTR, unsigned int, HANDLE*, HANDLE*, SpiProgressCallback, LONG_PTR );

HGLOBAL make_test_rgb24_dib( int width, int height, bool top_down = false )
{
	const std::size_t stride = ( static_cast<std::size_t>( width ) * 3u + 3u ) & ~std::size_t( 3u );
	const std::size_t img_bytes = stride * static_cast<std::size_t>( height );
	HGLOBAL h = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( BITMAPINFOHEADER ) + img_bytes );
	CHECK_COND( h != nullptr );

	auto* base = static_cast<std::uint8_t*>( GlobalLock( h ) );
	CHECK_COND( base != nullptr );
	auto* bih = reinterpret_cast<BITMAPINFOHEADER*>( base );
	bih->biSize          = sizeof( BITMAPINFOHEADER );
	bih->biWidth         = width;
	bih->biHeight        = top_down ? -height : height;
	bih->biPlanes        = 1;
	bih->biBitCount      = 24;
	bih->biCompression   = BI_RGB;
	bih->biSizeImage     = static_cast<DWORD>( img_bytes );
	bih->biXPelsPerMeter = 11811; // ~300 DPI
	bih->biYPelsPerMeter = 11811;

	std::uint8_t* pixels = base + sizeof( BITMAPINFOHEADER );
	for ( int y = 0; y < height; ++y ) {
		std::uint8_t* row = pixels + static_cast<std::size_t>( y ) * stride;
		for ( int x = 0; x < width; ++x ) {
			row[ x * 3 + 0 ] = static_cast<std::uint8_t>( ( x * 13 + y * 7 ) & 0xFF );
			row[ x * 3 + 1 ] = static_cast<std::uint8_t>( ( x * 5 + y * 19 + 40 ) & 0xFF );
			row[ x * 3 + 2 ] = static_cast<std::uint8_t>( ( x * 23 + y * 3 + 80 ) & 0xFF );
		}
	}
	GlobalUnlock( h );
	return h;
}

HGLOBAL make_test_gray8_dib( int width, int height )
{
	const std::size_t stride = ( static_cast<std::size_t>( width ) + 3u ) & ~std::size_t( 3u );
	const std::size_t pal_bytes = 256u * sizeof( RGBQUAD );
	const std::size_t img_bytes = stride * static_cast<std::size_t>( height );
	HGLOBAL h = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( BITMAPINFOHEADER ) + pal_bytes + img_bytes );
	CHECK_COND( h != nullptr );

	auto* base = static_cast<std::uint8_t*>( GlobalLock( h ) );
	CHECK_COND( base != nullptr );
	auto* bih = reinterpret_cast<BITMAPINFOHEADER*>( base );
	bih->biSize        = sizeof( BITMAPINFOHEADER );
	bih->biWidth       = width;
	bih->biHeight      = height;
	bih->biPlanes      = 1;
	bih->biBitCount    = 8;
	bih->biCompression = BI_RGB;
	bih->biClrUsed     = 256;
	bih->biSizeImage   = static_cast<DWORD>( img_bytes );

	auto* pal = reinterpret_cast<RGBQUAD*>( base + sizeof( BITMAPINFOHEADER ) );
	for ( int i = 0; i < 256; ++i ) {
		pal[ i ].rgbBlue  = static_cast<BYTE>( i );
		pal[ i ].rgbGreen = static_cast<BYTE>( i );
		pal[ i ].rgbRed   = static_cast<BYTE>( i );
	}

	std::uint8_t* pixels = base + sizeof( BITMAPINFOHEADER ) + pal_bytes;
	for ( int y = 0; y < height; ++y ) {
		for ( int x = 0; x < width; ++x ) {
			pixels[ static_cast<std::size_t>( y ) * stride + static_cast<std::size_t>( x ) ] =
				static_cast<std::uint8_t>( ( x * 11 + y * 17 ) & 0xFF );
		}
	}
	GlobalUnlock( h );
	return h;
}

HGLOBAL make_test_indexed_dib( int width, int height, int bpp )
{
	const std::size_t n_colors = std::size_t( 1u ) << bpp;
	const std::size_t pal_bytes = n_colors * sizeof( RGBQUAD );
	const std::size_t stride = ( ( static_cast<std::size_t>( width ) * static_cast<std::size_t>( bpp ) + 31u ) / 32u ) * 4u;
	const std::size_t img_bytes = stride * static_cast<std::size_t>( height );
	HGLOBAL h = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( BITMAPINFOHEADER ) + pal_bytes + img_bytes );
	CHECK_COND( h != nullptr );

	auto* base = static_cast<std::uint8_t*>( GlobalLock( h ) );
	CHECK_COND( base != nullptr );
	auto* bih = reinterpret_cast<BITMAPINFOHEADER*>( base );
	bih->biSize        = sizeof( BITMAPINFOHEADER );
	bih->biWidth       = width;
	bih->biHeight      = height;
	bih->biPlanes      = 1;
	bih->biBitCount    = static_cast<WORD>( bpp );
	bih->biCompression = BI_RGB;
	bih->biClrUsed     = static_cast<DWORD>( n_colors );
	bih->biSizeImage   = static_cast<DWORD>( img_bytes );

	auto* pal = reinterpret_cast<RGBQUAD*>( base + sizeof( BITMAPINFOHEADER ) );
	for ( std::size_t i = 0; i < n_colors; ++i ) {
		pal[ i ].rgbRed   = static_cast<BYTE>( ( i * 73 ) & 0xFF );
		pal[ i ].rgbGreen = static_cast<BYTE>( ( i * 41 + 30 ) & 0xFF );
		pal[ i ].rgbBlue  = static_cast<BYTE>( ( i * 113 + 90 ) & 0xFF );
	}

	std::uint8_t* pixels = base + sizeof( BITMAPINFOHEADER ) + pal_bytes;
	for ( int y = 0; y < height; ++y ) {
		std::uint8_t* row = pixels + static_cast<std::size_t>( y ) * stride;
		for ( int x = 0; x < width; ++x ) {
			if ( bpp == 1 ) {
				if ( ( ( x + y ) & 1 ) != 0 ) {
					row[ x >> 3 ] |= static_cast<std::uint8_t>( 1u << ( 7 - ( x & 7 ) ) );
				}
			}
			else if ( bpp == 4 ) {
				const std::uint8_t nib = static_cast<std::uint8_t>( ( x + y ) & 0x0F );
				if ( ( x & 1 ) == 0 ) {
					row[ x >> 1 ] |= static_cast<std::uint8_t>( nib << 4 );
				}
				else {
					row[ x >> 1 ] |= nib;
				}
			}
		}
	}
	GlobalUnlock( h );
	return h;
}

HGLOBAL make_test_rgb555_16bpp_dib( int width, int height )
{
	const std::size_t stride = ( static_cast<std::size_t>( width ) * 2u + 3u ) & ~std::size_t( 3u );
	const std::size_t img_bytes = stride * static_cast<std::size_t>( height );
	HGLOBAL h = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( BITMAPINFOHEADER ) + img_bytes );
	CHECK_COND( h != nullptr );

	auto* base = static_cast<std::uint8_t*>( GlobalLock( h ) );
	CHECK_COND( base != nullptr );
	auto* bih = reinterpret_cast<BITMAPINFOHEADER*>( base );
	bih->biSize        = sizeof( BITMAPINFOHEADER );
	bih->biWidth       = width;
	bih->biHeight      = height;
	bih->biPlanes      = 1;
	bih->biBitCount    = 16;
	bih->biCompression = BI_RGB;
	bih->biSizeImage   = static_cast<DWORD>( img_bytes );

	std::uint8_t* pixels = base + sizeof( BITMAPINFOHEADER );
	for ( int y = 0; y < height; ++y ) {
		std::uint8_t* row = pixels + static_cast<std::size_t>( y ) * stride;
		for ( int x = 0; x < width; ++x ) {
			const std::uint16_t r5 = static_cast<std::uint16_t>( ( x * 2 ) & 31 );
			const std::uint16_t g5 = static_cast<std::uint16_t>( ( y * 2 ) & 31 );
			const std::uint16_t b5 = static_cast<std::uint16_t>( ( x + y ) & 31 );
			const std::uint16_t px = static_cast<std::uint16_t>( ( r5 << 10 ) | ( g5 << 5 ) | b5 );
			std::memcpy( row + static_cast<std::size_t>( x ) * 2u, &px, sizeof( px ) );
		}
	}
	GlobalUnlock( h );
	return h;
}

HGLOBAL make_test_bitfields_32bpp_dib( int width, int height )
{
	const std::size_t mask_bytes = 3u * sizeof( DWORD );
	const std::size_t stride = static_cast<std::size_t>( width ) * 4u;
	const std::size_t img_bytes = stride * static_cast<std::size_t>( height );
	HGLOBAL h = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, sizeof( BITMAPINFOHEADER ) + mask_bytes + img_bytes );
	CHECK_COND( h != nullptr );

	auto* base = static_cast<std::uint8_t*>( GlobalLock( h ) );
	CHECK_COND( base != nullptr );
	auto* bih = reinterpret_cast<BITMAPINFOHEADER*>( base );
	bih->biSize        = sizeof( BITMAPINFOHEADER );
	bih->biWidth       = width;
	bih->biHeight      = height;
	bih->biPlanes      = 1;
	bih->biBitCount    = 32;
	bih->biCompression = BI_BITFIELDS;
	bih->biSizeImage   = static_cast<DWORD>( img_bytes );

	const DWORD masks[ 3 ] = { 0x000000FFu, 0x0000FF00u, 0x00FF0000u }; // R=low byte, G=mid, B=high
	std::memcpy( base + sizeof( BITMAPINFOHEADER ), masks, sizeof( masks ) );

	std::uint8_t* pixels = base + sizeof( BITMAPINFOHEADER ) + mask_bytes;
	for ( int y = 0; y < height; ++y ) {
		std::uint8_t* row = pixels + static_cast<std::size_t>( y ) * stride;
		for ( int x = 0; x < width; ++x ) {
			const std::uint32_t px =
				static_cast<std::uint32_t>( ( x * 11 ) & 0xFF ) |
				( static_cast<std::uint32_t>( ( y * 13 ) & 0xFF ) << 8 ) |
				( static_cast<std::uint32_t>( ( ( x + y ) * 7 ) & 0xFF ) << 16 );
			std::memcpy( row + static_cast<std::size_t>( x ) * 4u, &px, sizeof( px ) );
		}
	}
	GlobalUnlock( h );
	return h;
}

std::vector<std::uint8_t> make_minimal_cmyk_jpeg_8x8()
{
	// Valid 8x8 4-component CMYK baseline JPEG with Adobe APP14 (transform=0 -> CMYK)
	std::vector<std::uint8_t> jpg = {
		0xFF, 0xD8,
		// APP14 Adobe (length 14 = 0x000E, "Adobe", version=100, flags=0,0, transform=0)
		0xFF, 0xEE, 0x00, 0x0E, 'A', 'd', 'o', 'b', 'e', 0x00, 0x64, 0x00, 0x00, 0x00, 0x00, 0x00,
		0xFF, 0xDB, 0x00, 0x43, 0x00,
	};
	jpg.insert( jpg.end(), 64, 16 );
	const std::uint8_t tail[] = {
		// SOF0: 8x8, 4 components ('C'=1, 'M'=2, 'Y'=3, 'K'=4), all 1x1 qt=0
		0xFF, 0xC0, 0x00, 0x14, 0x08, 0x00, 0x08, 0x00, 0x08, 0x04,
		0x01, 0x11, 0x00,
		0x02, 0x11, 0x00,
		0x03, 0x11, 0x00,
		0x04, 0x11, 0x00,
		// DC table 0: symbol 0 -> 1-bit '0'
		0xFF, 0xC4, 0x00, 0x14, 0x00,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		// AC table 0: symbol 0 (EOB) -> 1-bit '0'
		0xFF, 0xC4, 0x00, 0x14, 0x10,
		0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		// SOS: 4 components -> 4 blocks * 2 zero bits ('00') = 8 zero bits -> 1 byte 0x00!
		0xFF, 0xDA, 0x00, 0x0E, 0x04, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x04, 0x00, 0x00, 0x3F, 0x00,
		0x00,
		0xFF, 0xD9,
	};
	jpg.insert( jpg.end(), std::begin( tail ), std::end( tail ) );
	return jpg;
}

int CALLBACK abort_progress_cb( int num, int denom, LONG_PTR user_data )
{
	(void) denom;
	if ( user_data == 99 && num == 0 ) return 1;
	return ( num > 0 && user_data == 42 ) ? 1 : 0;
}

void verify_plugin_module( const wchar_t* dll_path )
{
	HMODULE mod = LoadLibraryW( dll_path );
	CHECK_COND( mod != nullptr );

	auto fn_info    = load_proc<GetPlugInInfoFn>( mod, "GetPlugInInfo" );
	auto fn_read_w  = load_proc<ReadPJGWFn>( mod, "ReadPJG_W" );
	auto fn_read_a  = load_proc<ReadPJGFn>( mod, "ReadPJG" );
	auto fn_save_w  = load_proc<SavePJGWFn>( mod, "SavePJG_W" );
	auto fn_save_a  = load_proc<SavePJGFn>( mod, "SavePJG" );

	auto spi_info_a  = load_proc<SpiGetPluginInfoFn>( mod, "GetPluginInfo" );
	auto spi_info_w  = load_proc<SpiGetPluginInfoWFn>( mod, "GetPluginInfoW" );
	auto spi_supp_w  = load_proc<SpiIsSupportedWFn>( mod, "IsSupportedW" );
	auto spi_pinfo_w = load_proc<SpiGetPictureInfoWFn>( mod, "GetPictureInfoW" );
	auto spi_pic_w   = load_proc<SpiGetPictureWFn>( mod, "GetPictureW" );
	(void) load_proc<SpiGetPictureWFn>( mod, "GetPreviewW" );

	// 1. Metadata queries
	char ver[ 16 ]{};
	char fmts[ 256 ]{};
	CHECK_COND( fn_info( ver, fmts ) == 0 );
	CHECK_COND( std::strncmp( ver, "2.5", 3 ) == 0 );
	CHECK_COND( std::strstr( fmts, "PJG" ) != nullptr );

	char spi_ver[ 64 ]{};
	wchar_t spi_ext[ 64 ]{};
	CHECK_COND( spi_info_a( 0, spi_ver, sizeof( spi_ver ) ) == 4 );
	CHECK_COND( std::strcmp( spi_ver, "00IN" ) == 0 );
	CHECK_COND( spi_info_w( 2, spi_ext, 64 ) > 0 );
	CHECK_COND( std::wcscmp( spi_ext, L"*.pjg" ) == 0 );

	const std::wstring dir_leaf = L"packjpg_iv_test_" + std::to_wstring( GetCurrentProcessId() );
	const std::filesystem::path tmp_dir = std::filesystem::temp_directory_path() / dir_leaf;
	std::filesystem::remove_all( tmp_dir );
	std::filesystem::create_directories( tmp_dir );

	// 2. Native SavePJG_W -> lossless PJG verification -> ReadPJG_W (24bpp, moveable HGLOBAL, & forced 8bpp)
	const std::filesystem::path rgb_pjg = tmp_dir / L"rgb_\u03C0_test.pjg";
	HGLOBAL src_rgb = make_test_rgb24_dib( 24, 16 );
	wchar_t werr[ 256 ]{};
	CHECK_COND( fn_save_w( rgb_pjg.c_str(), src_rgb, 92, werr ) == 0 );
	GlobalFree( src_rgb );
	CHECK_COND( std::filesystem::exists( rgb_pjg ) );

	std::vector<std::uint8_t> pjg_file_bytes;
	{
		std::ifstream ifs( rgb_pjg, std::ios::binary );
		pjg_file_bytes.assign( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
	}
	CHECK_COND( pjg_file_bytes.size() > 16 );
	CHECK_COND( pjg_file_bytes[ 0 ] == 'J' && pjg_file_bytes[ 1 ] == 'S' );

	std::vector<std::uint8_t> decoded_jpg;
	CHECK_COND( packjpg::convert_memory( pjg_file_bytes.data(), pjg_file_bytes.size(), decoded_jpg ) );
	CHECK_COND( decoded_jpg.size() > pjg_file_bytes.size() );
	CHECK_COND( decoded_jpg[ 0 ] == 0xFF && decoded_jpg[ 1 ] == 0xD8 );

	HANDLE loaded_rgb = fn_read_w( rgb_pjg.c_str(), 0, werr );
	CHECK_COND( loaded_rgb != nullptr );
	// Finding #8 regression: returned HGLOBAL must be valid moveable global memory
	CHECK_COND( GlobalFlags( loaded_rgb ) != GMEM_INVALID_HANDLE );
	{
		auto* bih = static_cast<const BITMAPINFOHEADER*>( GlobalLock( loaded_rgb ) );
		CHECK_COND( bih != nullptr );
		CHECK_COND( bih->biWidth == 24 );
		CHECK_COND( bih->biHeight == 16 );
		CHECK_COND( bih->biBitCount == 24 );
		CHECK_COND( bih->biXPelsPerMeter >= 11700 && bih->biXPelsPerMeter <= 11900 );
		GlobalUnlock( loaded_rgb );
	}
	GlobalFree( loaded_rgb );

	HANDLE loaded_forced_gray = fn_read_w( rgb_pjg.c_str(), 1u, werr );
	CHECK_COND( loaded_forced_gray != nullptr );
	{
		auto* bih = static_cast<const BITMAPINFOHEADER*>( GlobalLock( loaded_forced_gray ) );
		CHECK_COND( bih != nullptr );
		CHECK_COND( bih->biBitCount == 8 );
		CHECK_COND( bih->biClrUsed == 256 );
		GlobalUnlock( loaded_forced_gray );
	}
	GlobalFree( loaded_forced_gray );

	// 3. 8-bit grayscale DIB SavePJG -> ReadPJG (ANSI wrappers using ASCII-safe directory)
	char short_tmp[ MAX_PATH ]{};
	const DWORD tlen = GetTempPathA( MAX_PATH, short_tmp );
	if ( ( tlen > 0 ) && ( tlen < MAX_PATH - 32 ) ) {
		const std::string gray_pjg_a = std::string( short_tmp ) + "pjg_iv_gray8_" + std::to_string( GetCurrentProcessId() ) + ".pjg";
		HGLOBAL src_gray = make_test_gray8_dib( 17, 15 );
		char aerr[ 128 ]{};
		CHECK_COND( fn_save_a( gray_pjg_a.c_str(), src_gray, 90, aerr ) == 0 );
		GlobalFree( src_gray );

		HANDLE loaded_gray = fn_read_a( gray_pjg_a.c_str(), 0, aerr );
		CHECK_COND( loaded_gray != nullptr );
		{
			auto* bih = static_cast<const BITMAPINFOHEADER*>( GlobalLock( loaded_gray ) );
			CHECK_COND( bih != nullptr );
			CHECK_COND( bih->biWidth == 17 );
			CHECK_COND( bih->biHeight == 15 );
			CHECK_COND( bih->biBitCount == 8 );
			GlobalUnlock( loaded_gray );
		}
		GlobalFree( loaded_gray );
		DeleteFileA( gray_pjg_a.c_str() );
	}

	// 4. Finding #7 regressions: 1bpp, 4bpp, 16bpp BI_RGB (5-5-5), 32bpp BI_BITFIELDS, and top-down 24bpp DIBs
	{
		struct DibVariant {
			const wchar_t* name;
			HGLOBAL        dib;
		};
		const DibVariant variants[] = {
			{ L"var_1bpp.pjg",      make_test_indexed_dib( 16, 16, 1 ) },
			{ L"var_4bpp.pjg",      make_test_indexed_dib( 16, 16, 4 ) },
			{ L"var_16bpp_555.pjg", make_test_rgb555_16bpp_dib( 16, 16 ) },
			{ L"var_32bpp_bf.pjg",  make_test_bitfields_32bpp_dib( 16, 16 ) },
			{ L"var_topdown.pjg",   make_test_rgb24_dib( 16, 16, true ) },
		};
		for ( const auto& v : variants ) {
			const std::filesystem::path vpath = tmp_dir / v.name;
			CHECK_COND( fn_save_w( vpath.c_str(), v.dib, 90, werr ) == 0 );
			GlobalFree( v.dib );
			HANDLE h_dec = fn_read_w( vpath.c_str(), 0, werr );
			CHECK_COND( h_dec != nullptr );
			auto* bih = static_cast<const BITMAPINFOHEADER*>( GlobalLock( h_dec ) );
			CHECK_COND( bih != nullptr && bih->biWidth == 16 && bih->biHeight == 16 );
			GlobalUnlock( h_dec );
			GlobalFree( h_dec );
		}
	}

	// 5. Finding #8 regression: 32bppCMYK JPEG compressed to PJG decoded with force_grayscale (flags=1) -> 8bpp DIB
	{
		const auto cmyk_jpg = make_minimal_cmyk_jpeg_8x8();
		std::vector<std::uint8_t> cmyk_pjg_bytes;
		CHECK_COND( packjpg::convert_memory( cmyk_jpg.data(), cmyk_jpg.size(), cmyk_pjg_bytes ) );
		const std::filesystem::path cmyk_pjg = tmp_dir / L"cmyk_8x8.pjg";
		{
			std::ofstream ofs( cmyk_pjg, std::ios::binary );
			ofs.write( reinterpret_cast<const char*>( cmyk_pjg_bytes.data() ), static_cast<std::streamsize>( cmyk_pjg_bytes.size() ) );
		}
		HANDLE h_cmyk_gray = fn_read_w( cmyk_pjg.c_str(), 1u, werr );
		CHECK_COND( h_cmyk_gray != nullptr );
		auto* bih = static_cast<const BITMAPINFOHEADER*>( GlobalLock( h_cmyk_gray ) );
		CHECK_COND( bih != nullptr && bih->biWidth == 8 && bih->biHeight == 8 && bih->biBitCount == 8 );
		GlobalUnlock( h_cmyk_gray );
		GlobalFree( h_cmyk_gray );
	}

	// 6. Finding #2, #3, #4 regressions: OOB integer-wraparound DIBs rejected, 128-wchar_t errbuf canary intact, atomic SavePJG_W preserves existing file
	{
		// Finding #3: tight 128-wchar_t caller buffer surrounded by 0xDEAD stack canaries
		wchar_t guarded_err[ 260 ];
		for ( auto& wc : guarded_err ) wc = static_cast<wchar_t>( 0xDEADu );
		const std::filesystem::path missing_pjg = tmp_dir / L"does_not_exist.pjg";
		CHECK_COND( fn_read_w( missing_pjg.c_str(), 0, guarded_err ) == nullptr );
		CHECK_COND( guarded_err[ 0 ] != L'\0' && std::wcslen( guarded_err ) < PJGLIB_MSG_SIZE );
		for ( std::size_t k = PJGLIB_MSG_SIZE; k < 260; ++k ) {
			CHECK_COND( guarded_err[ k ] == static_cast<wchar_t>( 0xDEADu ) );
		}

		// Finding #2: crafted 64-byte DIBs with huge biClrUsed / biSize attempting 32-bit size_t wraparound
		HGLOBAL wrap_dib1 = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, 64 );
		CHECK_COND( wrap_dib1 != nullptr );
		{
			auto* bih = static_cast<BITMAPINFOHEADER*>( GlobalLock( wrap_dib1 ) );
			bih->biSize        = sizeof( BITMAPINFOHEADER );
			bih->biWidth       = 16;
			bih->biHeight      = 16;
			bih->biPlanes      = 1;
			bih->biBitCount    = 24;
			bih->biCompression = BI_RGB;
			bih->biClrUsed     = 0x3FFFE2ACu; // * 4 wraps 32-bit size_t!
			GlobalUnlock( wrap_dib1 );
		}
		// Finding #4: attempting SavePJG_W with wrap_dib1 onto existing rgb_pjg MUST fail AND preserve rgb_pjg 100% intact!
		CHECK_COND( fn_save_w( rgb_pjg.c_str(), wrap_dib1, 90, werr ) != 0 );
		GlobalFree( wrap_dib1 );

		HGLOBAL wrap_dib2 = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, 64 );
		CHECK_COND( wrap_dib2 != nullptr );
		{
			auto* bih = static_cast<BITMAPINFOHEADER*>( GlobalLock( wrap_dib2 ) );
			bih->biSize        = 0xFFFF8AD8u; // biSize + stride*height wraps 32-bit size_t!
			bih->biWidth       = 16;
			bih->biHeight      = 16;
			bih->biPlanes      = 1;
			bih->biBitCount    = 24;
			bih->biCompression = BI_RGB;
			GlobalUnlock( wrap_dib2 );
		}
		CHECK_COND( fn_save_w( rgb_pjg.c_str(), wrap_dib2, 90, werr ) != 0 );
		GlobalFree( wrap_dib2 );

		// Verify rgb_pjg bytes are completely untouched and zero .pjg_tmp_* files exist
		std::vector<std::uint8_t> after_failed_save;
		{
			std::ifstream ifs( rgb_pjg, std::ios::binary );
			after_failed_save.assign( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
		}
		CHECK_COND( after_failed_save == pjg_file_bytes );
		for ( const auto& entry : std::filesystem::directory_iterator( tmp_dir ) ) {
			CHECK_COND( entry.path().filename().wstring().find( L".pjg_tmp_" ) == std::wstring::npos );
		}
	}

	// 7. Susie 00IN: Findings #1, #5, #6, #8 (IsSupportedW INVALID_HANDLE_VALUE/pipe/short JS\0, SusiePictureInfo 0xCC padding, negative len & file offset)
	std::vector<std::uint8_t> probe_buf = pjg_file_bytes;
	probe_buf.resize( 2048, 0 );
	CHECK_COND( spi_supp_w( rgb_pjg.c_str(), reinterpret_cast<DWORD_PTR>( probe_buf.data() ) ) == 1 );
	CHECK_COND( spi_supp_w( L"test.jpg", reinterpret_cast<DWORD_PTR>( decoded_jpg.data() ) ) == 0 );
	std::vector<std::uint8_t> js_text( 2048, ' ' );
	std::memcpy( js_text.data(), "JS module export default 42;", 28 );
	CHECK_COND( spi_supp_w( L"script.js", reinterpret_cast<DWORD_PTR>( js_text.data() ) ) == 0 );

	// Finding #6: INVALID_HANDLE_VALUE (-1), 0, 3-byte "JS\0", and anonymous pipe handle
	CHECK_COND( spi_supp_w( L"none.pjg", 0 ) == 0 );
	CHECK_COND( spi_supp_w( L"inv.pjg", static_cast<DWORD_PTR>( -1 ) ) == 0 );
	const std::uint8_t short_js0[ 4 ] = { 'J', 'S', 0x00, 0x00 };
	CHECK_COND( spi_supp_w( L"short.pjg", reinterpret_cast<DWORD_PTR>( short_js0 ) ) == 0 );
	{
		HANDLE r_pipe = nullptr;
		HANDLE w_pipe = nullptr;
		if ( CreatePipe( &r_pipe, &w_pipe, nullptr, 256 ) ) {
			DWORD written = 0;
			WriteFile( w_pipe, pjg_file_bytes.data(), 16, &written, nullptr );
			CHECK_COND( spi_supp_w( L"pipe.pjg", reinterpret_cast<DWORD_PTR>( r_pipe ) ) == 0 );
			std::uint8_t pipe_peek[ 2 ]{};
			DWORD got = 0;
			CHECK_COND( ReadFile( r_pipe, pipe_peek, 2, &got, nullptr ) != FALSE && got == 2 );
			CHECK_COND( pipe_peek[ 0 ] == 'J' && pipe_peek[ 1 ] == 'S' );
			CloseHandle( r_pipe );
			CloseHandle( w_pipe );
		}
	}

	HANDLE hf = CreateFileW( rgb_pjg.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
	CHECK_COND( hf != INVALID_HANDLE_VALUE );
	LARGE_INTEGER seven{};
	seven.QuadPart = 7;
	CHECK_COND( SetFilePointerEx( hf, seven, nullptr, FILE_BEGIN ) != FALSE );
	CHECK_COND( spi_supp_w( rgb_pjg.c_str(), reinterpret_cast<DWORD_PTR>( hf ) ) == 1 );
	LARGE_INTEGER after_pos{};
	LARGE_INTEGER zero{};
	CHECK_COND( SetFilePointerEx( hf, zero, &after_pos, FILE_CURRENT ) != FALSE );
	CHECK_COND( after_pos.QuadPart == 7 );
	CloseHandle( hf );

	// Finding #1: poison entire SusiePictureInfo (32 bytes on x64, 26 bytes on x86) with 0xCC and verify hInfo == nullptr!
	SusiePictureInfo pinfo;
	std::memset( &pinfo, 0xCC, sizeof( pinfo ) );
	CHECK_COND( spi_pinfo_w( rgb_pjg.c_str(), 0, SPI_INPUT_FILE, &pinfo ) == SPI_ALL_RIGHT );
	CHECK_COND( pinfo.width == 24 && pinfo.height == 16 && pinfo.colorDepth == 24 );
	CHECK_COND( pinfo.hInfo == nullptr );
#ifdef _WIN64
	CHECK_COND( pinfo.dummy[ 0 ] == 0 && pinfo.dummy[ 1 ] == 0 );
#endif

	// Finding #5: negative len rejected cleanly; SPI_INPUT_FILE with len = 128 container byte offset succeeds!
	CHECK_COND( spi_pinfo_w( reinterpret_cast<LPCWSTR>( pjg_file_bytes.data() ), -1, SPI_INPUT_MEMORY, &pinfo ) == SPI_MEMORY_ERROR );
	const std::filesystem::path container_file = tmp_dir / L"macbin_container.bin";
	{
		std::ofstream ofs( container_file, std::ios::binary );
		std::vector<char> prefix( 128, 'M' );
		ofs.write( prefix.data(), static_cast<std::streamsize>( prefix.size() ) );
		ofs.write( reinterpret_cast<const char*>( pjg_file_bytes.data() ), static_cast<std::streamsize>( pjg_file_bytes.size() ) );
	}
	std::memset( &pinfo, 0xCC, sizeof( pinfo ) );
	CHECK_COND( spi_pinfo_w( container_file.c_str(), 128, SPI_INPUT_FILE, &pinfo ) == SPI_ALL_RIGHT );
	CHECK_COND( pinfo.width == 24 && pinfo.height == 16 && pinfo.hInfo == nullptr );

	HANDLE hbinfo = nullptr;
	HANDLE hbm    = nullptr;
	CHECK_COND( spi_pic_w( container_file.c_str(), 128, SPI_INPUT_FILE, &hbinfo, &hbm, nullptr, 0 ) == SPI_ALL_RIGHT );
	CHECK_COND( hbinfo != nullptr && hbm != nullptr );
	LocalFree( hbinfo );
	LocalFree( hbm );

	CHECK_COND( spi_pic_w(
		reinterpret_cast<LPCWSTR>( pjg_file_bytes.data() ),
		static_cast<LONG_PTR>( pjg_file_bytes.size() ),
		SPI_INPUT_MEMORY,
		&hbinfo,
		&hbm,
		nullptr,
		0 ) == SPI_ALL_RIGHT );
	CHECK_COND( hbinfo != nullptr && hbm != nullptr );
	{
		auto* bih = static_cast<const BITMAPINFOHEADER*>( LocalLock( hbinfo ) );
		CHECK_COND( bih != nullptr && bih->biWidth == 24 && bih->biHeight == 16 );
		LocalUnlock( hbinfo );
	}
	LocalFree( hbinfo );
	LocalFree( hbm );

	// Finding #8: early 0% progress abort must still zero *pHBInfo and *pHBm!
	hbinfo = reinterpret_cast<HANDLE>( std::uintptr_t( 0x1234u ) );
	hbm    = reinterpret_cast<HANDLE>( std::uintptr_t( 0x5678u ) );
	CHECK_COND( spi_pic_w( rgb_pjg.c_str(), 0, SPI_INPUT_FILE, &hbinfo, &hbm, abort_progress_cb, 99 ) == SPI_ABORT );
	CHECK_COND( hbinfo == nullptr && hbm == nullptr );

	CHECK_COND( spi_pic_w( rgb_pjg.c_str(), 0, SPI_INPUT_FILE, &hbinfo, &hbm, abort_progress_cb, 42 ) == SPI_ABORT );
	CHECK_COND( hbinfo == nullptr && hbm == nullptr );

	// 8. Negative / corrupted input handling
	const std::filesystem::path bad_pjg = tmp_dir / L"corrupt.pjg";
	{
		std::ofstream ofs( bad_pjg, std::ios::binary );
		ofs.write( "JS\x19\xFF\xFF\xFF", 6 );
	}
	werr[ 0 ] = L'\0';
	CHECK_COND( fn_read_w( bad_pjg.c_str(), 0, werr ) == nullptr );
	CHECK_COND( werr[ 0 ] != L'\0' );
	CHECK_COND( spi_pic_w( bad_pjg.c_str(), 0, SPI_INPUT_FILE, &hbinfo, &hbm, nullptr, 0 ) == SPI_OUT_OF_ORDER );

	FreeLibrary( mod );
	std::filesystem::remove_all( tmp_dir );
}

} // namespace

int main( int argc, char** argv )
{
	if ( argc < 2 ) {
		std::fprintf( stderr, "Usage: %s <plugin.dll> [plugin.sph/.spi ...]\n", argv[ 0 ] );
		return 2;
	}

	for ( int i = 1; i < argc; ++i ) {
		const std::filesystem::path p( argv[ i ] );
		std::printf( "  [TEST] Verifying IrfanView/Susie plugin: %s\n", argv[ i ] );
		verify_plugin_module( p.wstring().c_str() );
	}

	std::printf( "  [PASS] All IrfanView & Susie plugin tests passed!\n" );
	return 0;
}

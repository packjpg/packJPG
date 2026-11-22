#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "spi00in.h"
#include "codec_pipeline.h"

#include <windows.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
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

using SpiGetPluginInfoFn  = int ( __stdcall * )( int, LPSTR, int );
using SpiGetPluginInfoWFn = int ( __stdcall * )( int, LPWSTR, int );
using SpiIsSupportedWFn   = int ( __stdcall * )( LPCWSTR, DWORD_PTR );
using SpiGetPictureInfoWFn = int ( __stdcall * )( LPCWSTR, std::size_t, unsigned int, SusiePictureInfo* );
using SpiGetPictureWFn     = int ( __stdcall * )( LPCWSTR, std::size_t, unsigned int, HANDLE*, HANDLE*, SpiProgressCallback, LONG_PTR );

HGLOBAL make_test_rgb24_dib( int width, int height )
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
	bih->biHeight        = height;
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

int CALLBACK abort_progress_cb( int num, int denom, LONG_PTR user_data )
{
	(void) denom;
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
	CHECK_COND( std::strcmp( ver, "2.5l" ) == 0 );
	CHECK_COND( std::strstr( fmts, "PJG" ) != nullptr );

	char spi_ver[ 64 ]{};
	wchar_t spi_ext[ 64 ]{};
	CHECK_COND( spi_info_a( 0, spi_ver, sizeof( spi_ver ) ) == 4 );
	CHECK_COND( std::strcmp( spi_ver, "00IN" ) == 0 );
	CHECK_COND( spi_info_w( 2, spi_ext, 64 ) > 0 );
	CHECK_COND( std::wcscmp( spi_ext, L"*.pjg" ) == 0 );

	const std::filesystem::path tmp_dir = std::filesystem::temp_directory_path() / L"packjpg_iv_test";
	std::filesystem::create_directories( tmp_dir );

	// 2. Native SavePJG_W -> lossless PJG verification -> ReadPJG_W (24bpp & forced 8bpp)
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

	// 3. 8-bit grayscale DIB SavePJG -> ReadPJG (ANSI wrappers)
	const std::filesystem::path gray_pjg = tmp_dir / "gray8_test.pjg";
	const std::string gray_pjg_a = gray_pjg.string();
	HGLOBAL src_gray = make_test_gray8_dib( 17, 15 );
	char aerr[ 256 ]{};
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

	// 4. Susie 00IN: IsSupportedW (memory + HANDLE + ASCII "JS " rejection), GetPictureInfoW, GetPictureW
	std::vector<std::uint8_t> probe_buf = pjg_file_bytes;
	probe_buf.resize( 2048, 0 );
	CHECK_COND( spi_supp_w( rgb_pjg.c_str(), reinterpret_cast<DWORD_PTR>( probe_buf.data() ) ) == 1 );
	CHECK_COND( spi_supp_w( L"test.jpg", reinterpret_cast<DWORD_PTR>( decoded_jpg.data() ) ) == 0 );
	std::vector<std::uint8_t> js_text( 2048, ' ' );
	std::memcpy( js_text.data(), "JS module export default 42;", 28 );
	CHECK_COND( spi_supp_w( L"script.js", reinterpret_cast<DWORD_PTR>( js_text.data() ) ) == 0 );

	HANDLE hf = CreateFileW( rgb_pjg.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr );
	CHECK_COND( hf != INVALID_HANDLE_VALUE );
	if ( ( reinterpret_cast<DWORD_PTR>( hf ) & ~DWORD_PTR( 0xFFFFu ) ) == 0 ) {
		LARGE_INTEGER seven{};
		seven.QuadPart = 7;
		CHECK_COND( SetFilePointerEx( hf, seven, nullptr, FILE_BEGIN ) != FALSE );
		CHECK_COND( spi_supp_w( rgb_pjg.c_str(), reinterpret_cast<DWORD_PTR>( hf ) ) == 1 );
		LARGE_INTEGER after_pos{};
		LARGE_INTEGER zero{};
		CHECK_COND( SetFilePointerEx( hf, zero, &after_pos, FILE_CURRENT ) != FALSE );
		CHECK_COND( after_pos.QuadPart == 7 );
	}
	CloseHandle( hf );

	SusiePictureInfo pinfo{};
	CHECK_COND( spi_pinfo_w( rgb_pjg.c_str(), 0, SPI_INPUT_FILE, &pinfo ) == SPI_ALL_RIGHT );
	CHECK_COND( pinfo.width == 24 && pinfo.height == 16 && pinfo.colorDepth == 24 );



	HANDLE hbinfo = nullptr;
	HANDLE hbm    = nullptr;
	CHECK_COND( spi_pic_w(
		reinterpret_cast<LPCWSTR>( pjg_file_bytes.data() ),
		pjg_file_bytes.size(),
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

	CHECK_COND( spi_pic_w( rgb_pjg.c_str(), 0, SPI_INPUT_FILE, &hbinfo, &hbm, abort_progress_cb, 42 ) == SPI_ABORT );
	CHECK_COND( hbinfo == nullptr && hbm == nullptr );

	// 5. Negative / corrupted input handling
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

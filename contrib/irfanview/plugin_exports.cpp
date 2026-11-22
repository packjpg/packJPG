#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "pjg_wic_dib.h"
#include "spi00in.h"

#include "packjpglib.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string>
#include <vector>

#define PJG_PLUGIN_CDECL        extern "C" __declspec(dllexport)
#define PJG_PLUGIN_STDCALL(ret) extern "C" __declspec(dllexport) ret __stdcall

namespace {

constexpr std::size_t kErrBufMax = PJGLIB_MSG_SIZE; // 128 chars / wchar_ts

const char* short_version_number() noexcept
{
	const char* full = pjglib_short_name(); // e.g. "packJPG v2.5n"
	if ( full == nullptr ) {
		return "2.5n";
	}
	const char* v = std::strstr( full, " v" );
	return ( v != nullptr && v[ 2 ] != '\0' ) ? ( v + 2 ) : full;
}

int write_susie_info_a( int infono, LPSTR buf, int buflen ) noexcept
{
	if ( ( buf == nullptr ) || ( buflen <= 0 ) ) {
		return 0;
	}
	const char* src = nullptr;
	char dynamic_desc[ 128 ]{};
	switch ( infono ) {
		case 0:
			src = "00IN";
			break;
		case 1:
			std::snprintf( dynamic_desc, sizeof( dynamic_desc ), "%s IrfanView/Susie Plug-in", pjglib_short_name() );
			src = dynamic_desc;
			break;
		case 2:
			src = "*.pjg";
			break;
		case 3:
			src = "packJPG Compressed JPEG (*.pjg)";
			break;
		default:
			buf[ 0 ] = '\0';
			return 0;
	}

	const std::size_t len = std::min<std::size_t>( std::strlen( src ), static_cast<std::size_t>( buflen - 1 ) );
	std::memcpy( buf, src, len );
	buf[ len ] = '\0';
	return static_cast<int>( len );
}

int write_susie_info_w( int infono, LPWSTR buf, int buflen ) noexcept
{
	if ( ( buf == nullptr ) || ( buflen <= 0 ) ) {
		return 0;
	}
	char tmp[ 128 ]{};
	const int n = write_susie_info_a( infono, tmp, static_cast<int>( sizeof( tmp ) ) );
	if ( n <= 0 ) {
		buf[ 0 ] = L'\0';
		return 0;
	}
	pjg_iv::copy_error_w( buf, static_cast<std::size_t>( buflen ), tmp );
	return static_cast<int>( std::wcslen( buf ) );
}

int probe_susie_handle( HANDLE hf ) noexcept
{
	if ( ( hf == nullptr ) || ( hf == INVALID_HANDLE_VALUE ) ) {
		return 0;
	}
	if ( GetFileType( hf ) != FILE_TYPE_DISK ) {
		return 0;
	}
	LARGE_INTEGER zero{};
	LARGE_INTEGER orig_pos{};
	if ( !SetFilePointerEx( hf, zero, &orig_pos, FILE_CURRENT ) ) {
		return 0;
	}
	if ( !SetFilePointerEx( hf, zero, nullptr, FILE_BEGIN ) ) {
		return 0;
	}
	std::uint8_t header[ 64 ]{};
	DWORD got = 0;
	const BOOL ok = ReadFile( hf, header, sizeof( header ), &got, nullptr );
	SetFilePointerEx( hf, orig_pos, nullptr, FILE_BEGIN );
	if ( !ok || ( got < 3 ) ) {
		return 0;
	}
	return pjg_iv::has_pjg_signature( header, got ) ? 1 : 0;
}

int probe_susie_supported( DWORD_PTR dw ) noexcept
{
	if ( ( dw == 0 ) ||
	     ( dw == static_cast<DWORD_PTR>( -1 ) ) ||
	     ( dw == static_cast<DWORD_PTR>( 0xFFFFFFFFu ) ) ) {
		return 0;
	}
	// Susie 00IN convention: low 64 KB values represent a Win32 file HANDLE; higher values are typically a memory pointer.
	if ( ( dw & ~DWORD_PTR( 0xFFFFu ) ) == 0 ) {
		return probe_susie_handle( reinterpret_cast<HANDLE>( dw ) );
	}

	constexpr DWORD kReadablePageMask =
		PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
		PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	const auto* mem = reinterpret_cast<const std::uint8_t*>( dw );
	MEMORY_BASIC_INFORMATION mbi{};
	if ( ( VirtualQuery( mem, &mbi, sizeof( mbi ) ) == sizeof( mbi ) ) &&
		 ( mbi.State == MEM_COMMIT ) &&
		 ( ( mbi.Protect & kReadablePageMask ) != 0 ) &&
		 ( ( mbi.Protect & ( PAGE_NOACCESS | PAGE_GUARD ) ) == 0 ) ) {
		const auto region_end = reinterpret_cast<std::uintptr_t>( mbi.BaseAddress ) + mbi.RegionSize;
		const auto ptr_addr   = reinterpret_cast<std::uintptr_t>( mem );
		if ( region_end > ptr_addr ) {
			const std::size_t avail = static_cast<std::size_t>(
				std::min<std::uintptr_t>( region_end - ptr_addr, 64u ) );
			return pjg_iv::has_pjg_signature( mem, avail ) ? 1 : 0;
		}
	}

	// Fallback if the host passed an NT kernel file HANDLE whose numeric value exceeds 0xFFFF.
	if ( ( ( dw & 3u ) == 0 ) && ( dw <= 0x00FFFFFFu ) ) {
		return probe_susie_handle( reinterpret_cast<HANDLE>( dw ) );
	}
	return 0;
}

int load_susie_input(
	const void* buf,
	LONG_PTR len,
	unsigned int flag,
	std::vector<std::uint8_t>& storage,
	const std::uint8_t*& data_ptr,
	std::size_t& data_size )
{
	const unsigned int mode = flag & SPI_INPUT_MASK;
	if ( mode == SPI_INPUT_FILE ) {
		if ( ( buf == nullptr ) || ( len < 0 ) ) {
			return SPI_FILE_READ_ERROR;
		}
		std::string err;
		if ( !pjg_iv::read_file_bytes_w( static_cast<LPCWSTR>( buf ), storage, err, static_cast<std::uint64_t>( len ) ) ) {
			return SPI_FILE_READ_ERROR;
		}
		data_ptr  = storage.data();
		data_size = storage.size();
		return SPI_ALL_RIGHT;
	}
	if ( mode == SPI_INPUT_MEMORY ) {
		if ( ( buf == nullptr ) || ( len < 3 ) || ( static_cast<std::uint64_t>( len ) > ( 256ull << 20 ) ) ) {
			return SPI_MEMORY_ERROR;
		}
		data_ptr  = static_cast<const std::uint8_t*>( buf );
		data_size = static_cast<std::size_t>( len );
		return SPI_ALL_RIGHT;
	}
	return SPI_NO_FUNCTION;
}

int get_picture_info_impl(
	const void* buf,
	LONG_PTR len,
	unsigned int flag,
	SusiePictureInfo* lpInfo )
{
	if ( lpInfo == nullptr ) {
		return SPI_MEMORY_ERROR;
	}
	std::memset( lpInfo, 0, sizeof( SusiePictureInfo ) );

	try {
		std::vector<std::uint8_t> storage;
		const std::uint8_t* data_ptr = nullptr;
		std::size_t data_size = 0;
		const int load_rc = load_susie_input( buf, len, flag, storage, data_ptr, data_size );
		if ( load_rc != SPI_ALL_RIGHT ) {
			return load_rc;
		}

		if ( !pjg_iv::has_pjg_signature( data_ptr, data_size ) ) {
			return SPI_NOT_SUPPORT;
		}

		pjg_iv::ImageHeaderInfo info;
		std::string err;
		if ( !pjg_iv::query_pjg_info_from_memory( data_ptr, data_size, info, err ) ) {
			return SPI_OUT_OF_ORDER;
		}

		lpInfo->width      = static_cast<LONG>( info.width );
		lpInfo->height     = static_cast<LONG>( info.height );
		lpInfo->x_density  = static_cast<WORD>( std::clamp( info.dpi_x, 0, 65535 ) );
		lpInfo->y_density  = static_cast<WORD>( std::clamp( info.dpi_y, 0, 65535 ) );
		lpInfo->colorDepth = static_cast<short>( info.bit_depth );
		lpInfo->hInfo      = nullptr;
		return SPI_ALL_RIGHT;
	} catch ( const std::bad_alloc& ) {
		return SPI_NO_MEMORY;
	} catch ( ... ) {
		return SPI_OTHER_ERROR;
	}
}

int get_picture_impl(
	const void* buf,
	LONG_PTR len,
	unsigned int flag,
	HANDLE* pHBInfo,
	HANDLE* pHBm,
	SpiProgressCallback lpPrgressCallback,
	LONG_PTR lData )
{
	if ( ( pHBInfo == nullptr ) || ( pHBm == nullptr ) ) {
		return SPI_MEMORY_ERROR;
	}
	*pHBInfo = nullptr;
	*pHBm    = nullptr;

	try {
		if ( ( lpPrgressCallback != nullptr ) && ( lpPrgressCallback( 0, 100, lData ) != 0 ) ) {
			return SPI_ABORT;
		}

		std::vector<std::uint8_t> storage;
		const std::uint8_t* data_ptr = nullptr;
		std::size_t data_size = 0;
		const int load_rc = load_susie_input( buf, len, flag, storage, data_ptr, data_size );
		if ( load_rc != SPI_ALL_RIGHT ) {
			return load_rc;
		}

		std::string err;
		const int rc = pjg_iv::decode_pjg_memory_to_susie_dib( data_ptr, data_size, pHBInfo, pHBm, err );
		if ( rc != SPI_ALL_RIGHT ) {
			return rc;
		}

		if ( ( lpPrgressCallback != nullptr ) && ( lpPrgressCallback( 100, 100, lData ) != 0 ) ) {
			if ( *pHBInfo != nullptr ) { LocalFree( *pHBInfo ); *pHBInfo = nullptr; }
			if ( *pHBm != nullptr )    { LocalFree( *pHBm );    *pHBm    = nullptr; }
			return SPI_ABORT;
		}

		return SPI_ALL_RIGHT;
	} catch ( const std::bad_alloc& ) {
		return SPI_NO_MEMORY;
	} catch ( ... ) {
		return SPI_OTHER_ERROR;
	}
}

} // namespace

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	(void) hinstDLL;
	(void) fdwReason;
	(void) lpvReserved;
	return TRUE;
}

// ============================================================================
// Native IrfanView Format Plug-in Exports (__cdecl)
// ============================================================================

PJG_PLUGIN_CDECL int __cdecl GetPlugInInfo( char* version, char* formats )
{
	if ( version != nullptr ) {
		std::snprintf( version, 16, "%s", short_version_number() );
	}
	if ( formats != nullptr ) {
		std::snprintf( formats, 256, "PJG;packJPG Compressed JPEG" );
	}
	return 0;
}

PJG_PLUGIN_CDECL HANDLE __cdecl ReadPJG_W( LPCWSTR filename, UINT flags, LPWSTR errbuf )
{
	if ( errbuf != nullptr ) {
		errbuf[ 0 ] = L'\0';
	}
	try {
		std::vector<std::uint8_t> pjg_bytes;
		std::string err;
		if ( !pjg_iv::read_file_bytes_w( filename, pjg_bytes, err ) ) {
			pjg_iv::copy_error_w( errbuf, kErrBufMax, err );
			return nullptr;
		}

		const bool force_gray = ( flags & 1u ) != 0u;
		HGLOBAL h_dib = pjg_iv::decode_pjg_memory_to_packed_dib(
			pjg_bytes.data(), pjg_bytes.size(), force_gray, err );
		if ( h_dib == nullptr ) {
			pjg_iv::copy_error_w( errbuf, kErrBufMax, err );
		}
		return h_dib;
	} catch ( const std::bad_alloc& ) {
		pjg_iv::copy_error_w( errbuf, kErrBufMax, "out of memory" );
		return nullptr;
	} catch ( ... ) {
		pjg_iv::copy_error_w( errbuf, kErrBufMax, "unexpected plugin exception" );
		return nullptr;
	}
}

PJG_PLUGIN_CDECL HANDLE __cdecl ReadPJG( LPCSTR filename, UINT flags, LPSTR errbuf )
{
	if ( errbuf != nullptr ) {
		errbuf[ 0 ] = '\0';
	}
	try {
		if ( ( filename == nullptr ) || ( filename[ 0 ] == '\0' ) ) {
			pjg_iv::copy_error_a( errbuf, kErrBufMax, "empty input filename" );
			return nullptr;
		}
		const std::wstring wpath = pjg_iv::widen_acp( filename );
		if ( wpath.empty() ) {
			pjg_iv::copy_error_a( errbuf, kErrBufMax, "invalid input filename encoding" );
			return nullptr;
		}
		wchar_t werr[ kErrBufMax ]{};
		HANDLE h = ReadPJG_W( wpath.c_str(), flags, werr );
		if ( ( h == nullptr ) && ( errbuf != nullptr ) ) {
			const std::string narrow = pjg_iv::narrow_acp_from_wide( werr );
			pjg_iv::copy_error_a( errbuf, kErrBufMax, narrow );
		}
		return h;
	} catch ( const std::bad_alloc& ) {
		pjg_iv::copy_error_a( errbuf, kErrBufMax, "out of memory" );
		return nullptr;
	} catch ( ... ) {
		pjg_iv::copy_error_a( errbuf, kErrBufMax, "unexpected plugin exception" );
		return nullptr;
	}
}

PJG_PLUGIN_CDECL int __cdecl SavePJG_W( LPCWSTR filename, HANDLE hDib, int quality, LPWSTR errbuf )
{
	if ( errbuf != nullptr ) {
		errbuf[ 0 ] = L'\0';
	}
	try {
		std::vector<std::uint8_t> pjg_bytes;
		std::string err;
		if ( !pjg_iv::encode_packed_dib_to_pjg_memory( hDib, quality, pjg_bytes, err ) ) {
			pjg_iv::copy_error_w( errbuf, kErrBufMax, err );
			return 1;
		}

		if ( !pjg_iv::write_file_bytes_w( filename, pjg_bytes.data(), pjg_bytes.size(), err ) ) {
			pjg_iv::copy_error_w( errbuf, kErrBufMax, err );
			return 1;
		}

		return 0;
	} catch ( const std::bad_alloc& ) {
		pjg_iv::copy_error_w( errbuf, kErrBufMax, "out of memory" );
		return 1;
	} catch ( ... ) {
		pjg_iv::copy_error_w( errbuf, kErrBufMax, "unexpected plugin exception" );
		return 1;
	}
}

PJG_PLUGIN_CDECL int __cdecl SavePJG( LPCSTR filename, HANDLE hDib, int quality, LPSTR errbuf )
{
	if ( errbuf != nullptr ) {
		errbuf[ 0 ] = '\0';
	}
	try {
		if ( ( filename == nullptr ) || ( filename[ 0 ] == '\0' ) ) {
			pjg_iv::copy_error_a( errbuf, kErrBufMax, "empty output filename" );
			return 1;
		}
		const std::wstring wpath = pjg_iv::widen_acp( filename );
		if ( wpath.empty() ) {
			pjg_iv::copy_error_a( errbuf, kErrBufMax, "invalid output filename encoding" );
			return 1;
		}
		wchar_t werr[ kErrBufMax ]{};
		const int rc = SavePJG_W( wpath.c_str(), hDib, quality, werr );
		if ( ( rc != 0 ) && ( errbuf != nullptr ) ) {
			const std::string narrow = pjg_iv::narrow_acp_from_wide( werr );
			pjg_iv::copy_error_a( errbuf, kErrBufMax, narrow );
		}
		return rc;
	} catch ( const std::bad_alloc& ) {
		pjg_iv::copy_error_a( errbuf, kErrBufMax, "out of memory" );
		return 1;
	} catch ( ... ) {
		pjg_iv::copy_error_a( errbuf, kErrBufMax, "unexpected plugin exception" );
		return 1;
	}
}

// ============================================================================
// Susie '00IN' Image Plug-in Exports (__stdcall, undecorated via --kill-at)
// ============================================================================

PJG_PLUGIN_STDCALL(int) GetPluginInfo( int infono, LPSTR buf, int buflen )
{
	return write_susie_info_a( infono, buf, buflen );
}

PJG_PLUGIN_STDCALL(int) GetPluginInfoW( int infono, LPWSTR buf, int buflen )
{
	return write_susie_info_w( infono, buf, buflen );
}

PJG_PLUGIN_STDCALL(int) IsSupported( LPCSTR filename, DWORD_PTR dw )
{
	(void) filename;
	return probe_susie_supported( dw );
}

PJG_PLUGIN_STDCALL(int) IsSupportedW( LPCWSTR filename, DWORD_PTR dw )
{
	(void) filename;
	return probe_susie_supported( dw );
}

PJG_PLUGIN_STDCALL(int) GetPictureInfoW(
	LPCWSTR buf,
	LONG_PTR len,
	unsigned int flag,
	SusiePictureInfo* lpInfo )
{
	return get_picture_info_impl( buf, len, flag, lpInfo );
}

PJG_PLUGIN_STDCALL(int) GetPictureInfo(
	LPCSTR buf,
	LONG_PTR len,
	unsigned int flag,
	SusiePictureInfo* lpInfo )
{
	if ( lpInfo == nullptr ) {
		return SPI_MEMORY_ERROR;
	}
	std::memset( lpInfo, 0, sizeof( SusiePictureInfo ) );

	try {
		if ( ( flag & SPI_INPUT_MASK ) == SPI_INPUT_FILE ) {
			if ( buf == nullptr ) return SPI_FILE_READ_ERROR;
			const std::wstring wpath = pjg_iv::widen_acp( buf );
			if ( wpath.empty() ) return SPI_FILE_READ_ERROR;
			return get_picture_info_impl( wpath.c_str(), len, flag, lpInfo );
		}
		return get_picture_info_impl( static_cast<const void*>( buf ), len, flag, lpInfo );
	} catch ( const std::bad_alloc& ) {
		return SPI_NO_MEMORY;
	} catch ( ... ) {
		return SPI_OTHER_ERROR;
	}
}

PJG_PLUGIN_STDCALL(int) GetPictureW(
	LPCWSTR buf,
	LONG_PTR len,
	unsigned int flag,
	HANDLE* pHBInfo,
	HANDLE* pHBm,
	SpiProgressCallback lpPrgressCallback,
	LONG_PTR lData )
{
	return get_picture_impl( buf, len, flag, pHBInfo, pHBm, lpPrgressCallback, lData );
}

PJG_PLUGIN_STDCALL(int) GetPicture(
	LPCSTR buf,
	LONG_PTR len,
	unsigned int flag,
	HANDLE* pHBInfo,
	HANDLE* pHBm,
	SpiProgressCallback lpPrgressCallback,
	LONG_PTR lData )
{
	if ( ( pHBInfo != nullptr ) ) *pHBInfo = nullptr;
	if ( ( pHBm != nullptr ) )    *pHBm    = nullptr;
	if ( ( pHBInfo == nullptr ) || ( pHBm == nullptr ) ) {
		return SPI_MEMORY_ERROR;
	}

	try {
		if ( ( flag & SPI_INPUT_MASK ) == SPI_INPUT_FILE ) {
			if ( buf == nullptr ) return SPI_FILE_READ_ERROR;
			const std::wstring wpath = pjg_iv::widen_acp( buf );
			if ( wpath.empty() ) return SPI_FILE_READ_ERROR;
			return get_picture_impl( wpath.c_str(), len, flag, pHBInfo, pHBm, lpPrgressCallback, lData );
		}
		return get_picture_impl( static_cast<const void*>( buf ), len, flag, pHBInfo, pHBm, lpPrgressCallback, lData );
	} catch ( const std::bad_alloc& ) {
		return SPI_NO_MEMORY;
	} catch ( ... ) {
		return SPI_OTHER_ERROR;
	}
}

PJG_PLUGIN_STDCALL(int) GetPreviewW(
	LPCWSTR buf,
	LONG_PTR len,
	unsigned int flag,
	HANDLE* pHBInfo,
	HANDLE* pHBm,
	SpiProgressCallback lpPrgressCallback,
	LONG_PTR lData )
{
	return GetPictureW( buf, len, flag, pHBInfo, pHBm, lpPrgressCallback, lData );
}

PJG_PLUGIN_STDCALL(int) GetPreview(
	LPCSTR buf,
	LONG_PTR len,
	unsigned int flag,
	HANDLE* pHBInfo,
	HANDLE* pHBm,
	SpiProgressCallback lpPrgressCallback,
	LONG_PTR lData )
{
	return GetPicture( buf, len, flag, pHBInfo, pHBm, lpPrgressCallback, lData );
}

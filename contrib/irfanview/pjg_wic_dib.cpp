#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "pjg_wic_dib.h"
#include "spi00in.h"

#include "codec_pipeline.h"
#include "packjpglib.h"

#include <objbase.h>
#include <oaidl.h>
#include <wincodec.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pjg_iv {
namespace {

template <typename T>
class ComPtr {
public:
	ComPtr() noexcept = default;
	~ComPtr() { reset(); }

	ComPtr( const ComPtr& ) = delete;
	ComPtr& operator=( const ComPtr& ) = delete;

	T* get() const noexcept { return ptr_; }
	T* operator->() const noexcept { return ptr_; }
	explicit operator bool() const noexcept { return ptr_ != nullptr; }

	T** put() noexcept {
		reset();
		return &ptr_;
	}

	void reset() noexcept {
		if ( ptr_ != nullptr ) {
			ptr_->Release();
			ptr_ = nullptr;
		}
	}

private:
	T* ptr_ = nullptr;
};

class ScopedComInit {
public:
	ScopedComInit() noexcept {
		const HRESULT hr = CoInitializeEx( nullptr, COINIT_MULTITHREADED );
		need_uninit_ = SUCCEEDED( hr );
		ok_ = need_uninit_ || ( hr == RPC_E_CHANGED_MODE );
	}
	~ScopedComInit() {
		if ( need_uninit_ ) {
			CoUninitialize();
		}
	}

	bool ok() const noexcept { return ok_; }

private:
	bool need_uninit_ = false;
	bool ok_          = false;
};

class ScopedGlobalLock {
public:
	explicit ScopedGlobalLock( HGLOBAL h ) noexcept
		: h_( h ), ptr_( h != nullptr ? GlobalLock( h ) : nullptr ) {}
	~ScopedGlobalLock() {
		if ( ptr_ != nullptr ) {
			GlobalUnlock( h_ );
		}
	}

	void* get() const noexcept { return ptr_; }

private:
	HGLOBAL h_   = nullptr;
	void*   ptr_ = nullptr;
};

HRESULT create_wic_factory( ComPtr<IWICImagingFactory>& factory ) noexcept
{
	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		reinterpret_cast<void**>( factory.put() ) );
#if defined(CLSID_WICImagingFactory1)
	if ( FAILED( hr ) ) {
		hr = CoCreateInstance(
			CLSID_WICImagingFactory1,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			reinterpret_cast<void**>( factory.put() ) );
	}
#endif
	return hr;
}

struct DecodedImagePixels {
	UINT width     = 0;
	UINT height    = 0;
	int  bpp_bytes = 3; // 1 (8bpp gray) or 3 (24bpp BGR)
	int  dpi_x     = 0;
	int  dpi_y     = 0;
	UINT stride    = 0;
	std::vector<std::uint8_t> bottom_up_rows;
};

bool decode_jpeg_bytes_via_wic(
	const std::uint8_t* jpg_data,
	std::size_t jpg_size,
	bool force_grayscale,
	DecodedImagePixels& out,
	std::string& err )
{
	if ( ( jpg_data == nullptr ) || ( jpg_size < 4 ) ) {
		err = "reconstructed JPEG buffer is empty";
		return false;
	}

	ScopedComInit com;
	if ( !com.ok() ) {
		err = "failed to initialize Windows COM runtime";
		return false;
	}

	HGLOBAL h_mem = GlobalAlloc( GMEM_MOVEABLE, jpg_size );
	if ( h_mem == nullptr ) {
		err = "out of memory allocating JPEG stream";
		return false;
	}
	void* locked = GlobalLock( h_mem );
	if ( locked == nullptr ) {
		GlobalFree( h_mem );
		err = "failed to lock JPEG stream memory";
		return false;
	}
	std::memcpy( locked, jpg_data, jpg_size );
	GlobalUnlock( h_mem );

	ComPtr<IStream> stream;
	if ( FAILED( CreateStreamOnHGlobal( h_mem, TRUE, stream.put() ) ) ) {
		GlobalFree( h_mem );
		err = "CreateStreamOnHGlobal failed";
		return false;
	}

	ComPtr<IWICImagingFactory> factory;
	if ( FAILED( create_wic_factory( factory ) ) ) {
		err = "failed to create IWICImagingFactory";
		return false;
	}

	ComPtr<IWICBitmapDecoder> decoder;
	if ( FAILED( factory->CreateDecoderFromStream(
			stream.get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.put() ) ) ) {
		err = "WIC failed to decode reconstructed JPEG stream";
		return false;
	}

	ComPtr<IWICBitmapFrameDecode> frame;
	if ( FAILED( decoder->GetFrame( 0, frame.put() ) ) ) {
		err = "WIC failed to read JPEG frame";
		return false;
	}

	UINT width = 0;
	UINT height = 0;
	if ( FAILED( frame->GetSize( &width, &height ) ) ||
		 ( width == 0 ) || ( height == 0 ) ||
		 ( width > 32768u ) || ( height > 32768u ) ) {
		err = "invalid or unsupported JPEG dimensions";
		return false;
	}

	double rx = 0.0;
	double ry = 0.0;
	if ( SUCCEEDED( frame->GetResolution( &rx, &ry ) ) ) {
		if ( rx > 0.0 && rx < 65535.0 ) out.dpi_x = static_cast<int>( std::lround( rx ) );
		if ( ry > 0.0 && ry < 65535.0 ) out.dpi_y = static_cast<int>( std::lround( ry ) );
	}

	WICPixelFormatGUID src_fmt{};
	if ( FAILED( frame->GetPixelFormat( &src_fmt ) ) ) {
		err = "failed to query JPEG pixel format";
		return false;
	}

	const bool use_gray = force_grayscale || ( IsEqualGUID( src_fmt, GUID_WICPixelFormat8bppGray ) != FALSE );
	const WICPixelFormatGUID dst_fmt = use_gray ? GUID_WICPixelFormat8bppGray : GUID_WICPixelFormat24bppBGR;
	const int bpp_bytes = use_gray ? 1 : 3;

	const std::uint64_t raw_row = static_cast<std::uint64_t>( width ) * static_cast<std::uint64_t>( bpp_bytes );
	const std::uint64_t stride64 = ( raw_row + 3u ) & ~std::uint64_t( 3u );
	const std::uint64_t total_pixels = stride64 * static_cast<std::uint64_t>( height );
	if ( total_pixels > ( 512ull << 20 ) || total_pixels > static_cast<std::uint64_t>( SIZE_MAX / 2 ) ) {
		err = "decoded image exceeds maximum DIB allocation limit";
		return false;
	}

	ComPtr<IWICBitmapSource> pixel_src;
	if ( IsEqualGUID( src_fmt, dst_fmt ) ) {
		if ( FAILED( frame->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( pixel_src.put() ) ) ) ) {
			err = "failed to acquire IWICBitmapSource";
			return false;
		}
	}
	else {
		ComPtr<IWICFormatConverter> converter;
		if ( FAILED( factory->CreateFormatConverter( converter.put() ) ) ||
			 FAILED( converter->Initialize(
				 frame.get(), dst_fmt, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) ||
			 FAILED( converter->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( pixel_src.put() ) ) ) ) {
			err = "WIC pixel format conversion failed";
			return false;
		}
	}

	const UINT stride = static_cast<UINT>( stride64 );
	std::vector<std::uint8_t> top_down( static_cast<std::size_t>( total_pixels ), 0 );
	if ( FAILED( pixel_src->CopyPixels(
			nullptr, stride, static_cast<UINT>( top_down.size() ), top_down.data() ) ) ) {
		err = "WIC CopyPixels failed";
		return false;
	}

	out.width     = width;
	out.height    = height;
	out.bpp_bytes = bpp_bytes;
	out.stride    = stride;
	out.bottom_up_rows.assign( top_down.size(), 0 );
	for ( UINT y = 0; y < height; ++y ) {
		const std::size_t src_off = static_cast<std::size_t>( y ) * stride;
		const std::size_t dst_off = static_cast<std::size_t>( height - 1u - y ) * stride;
		std::memcpy( out.bottom_up_rows.data() + dst_off, top_down.data() + src_off, static_cast<std::size_t>( raw_row ) );
	}

	return true;
}

void populate_dib_header_and_palette(
	const DecodedImagePixels& img,
	BITMAPINFOHEADER* bih,
	RGBQUAD* palette ) noexcept
{
	std::memset( bih, 0, sizeof( BITMAPINFOHEADER ) );
	bih->biSize          = sizeof( BITMAPINFOHEADER );
	bih->biWidth         = static_cast<LONG>( img.width );
	bih->biHeight        = static_cast<LONG>( img.height );
	bih->biPlanes        = 1;
	bih->biBitCount      = static_cast<WORD>( img.bpp_bytes * 8 );
	bih->biCompression   = BI_RGB;
	bih->biSizeImage     = static_cast<DWORD>( img.bottom_up_rows.size() );
	if ( img.dpi_x > 0 ) bih->biXPelsPerMeter = static_cast<LONG>( std::lround( img.dpi_x * 39.37007874 ) );
	if ( img.dpi_y > 0 ) bih->biYPelsPerMeter = static_cast<LONG>( std::lround( img.dpi_y * 39.37007874 ) );

	if ( img.bpp_bytes == 1 && palette != nullptr ) {
		bih->biClrUsed      = 256;
		bih->biClrImportant = 256;
		for ( int i = 0; i < 256; ++i ) {
			palette[ i ].rgbBlue     = static_cast<BYTE>( i );
			palette[ i ].rgbGreen    = static_cast<BYTE>( i );
			palette[ i ].rgbRed      = static_cast<BYTE>( i );
			palette[ i ].rgbReserved = 0;
		}
	}
}

} // namespace

bool has_pjg_signature( const std::uint8_t* data, std::size_t size ) noexcept
{
	if ( ( data == nullptr ) || ( size < 3 ) ) {
		return false;
	}
	if ( ( data[ 0 ] != 'J' ) || ( data[ 1 ] != 'S' ) ) {
		return false;
	}
	std::size_t pos = 2;
	while ( pos < size ) {
		const std::uint8_t hcode = data[ pos++ ];
		if ( hcode == 0x00 ) {
			if ( pos + 8 > size ) {
				return true; // Truncated probe buffer that started with valid custom header
			}
			for ( std::size_t c = 0; c < 4; ++c ) {
				if ( data[ pos + c ] > 10 || data[ pos + 4 + c ] < 1 || data[ pos + 4 + c ] > 49 ) {
					return false;
				}
			}
			pos += 8;
		}
		else if ( ( hcode >= 0x14 ) && ( hcode <= 0x1F ) ) {
			return true;
		}
		else {
			return false;
		}
	}
	return false;
}


bool read_file_bytes_w( const wchar_t* path, std::vector<std::uint8_t>& out_bytes, std::string& err )
{
	out_bytes.clear();
	if ( ( path == nullptr ) || ( path[ 0 ] == L'\0' ) ) {
		err = "empty input filename";
		return false;
	}

	HANDLE hf = CreateFileW(
		path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hf == INVALID_HANDLE_VALUE ) {
		err = "could not open input file";
		return false;
	}

	LARGE_INTEGER fsize{};
	if ( !GetFileSizeEx( hf, &fsize ) || ( fsize.QuadPart < 0 ) || ( fsize.QuadPart > ( 256ll << 20 ) ) ) {
		CloseHandle( hf );
		err = "invalid or oversized input file";
		return false;
	}

	out_bytes.resize( static_cast<std::size_t>( fsize.QuadPart ) );
	std::size_t offset = 0;
	while ( offset < out_bytes.size() ) {
		const DWORD chunk = static_cast<DWORD>(
			std::min<std::size_t>( out_bytes.size() - offset, 1u << 20 ) );
		DWORD got = 0;
		if ( !ReadFile( hf, out_bytes.data() + offset, chunk, &got, nullptr ) || ( got != chunk ) ) {
			CloseHandle( hf );
			out_bytes.clear();
			err = "error reading input file";
			return false;
		}
		offset += got;
	}

	CloseHandle( hf );
	return true;
}

bool write_file_bytes_w( const wchar_t* path, const std::uint8_t* data, std::size_t size, std::string& err )
{
	if ( ( path == nullptr ) || ( path[ 0 ] == L'\0' ) ) {
		err = "empty output filename";
		return false;
	}
	if ( ( data == nullptr ) && ( size > 0 ) ) {
		err = "null output data buffer";
		return false;
	}

	HANDLE hf = CreateFileW(
		path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );
	if ( hf == INVALID_HANDLE_VALUE ) {
		err = "could not open output file for writing";
		return false;
	}

	std::size_t offset = 0;
	while ( offset < size ) {
		const DWORD chunk = static_cast<DWORD>( std::min<std::size_t>( size - offset, 1u << 20 ) );
		DWORD written = 0;
		if ( !WriteFile( hf, data + offset, chunk, &written, nullptr ) || ( written != chunk ) ) {
			CloseHandle( hf );
			DeleteFileW( path );
			err = "error writing output file";
			return false;
		}
		offset += written;
	}

	CloseHandle( hf );
	return true;
}

void copy_error_w( wchar_t* dst, std::size_t dst_cch, const std::string& msg ) noexcept
{
	if ( ( dst == nullptr ) || ( dst_cch == 0 ) ) {
		return;
	}
	dst[ 0 ] = L'\0';
	if ( msg.empty() ) {
		return;
	}
	const int written = MultiByteToWideChar(
		CP_UTF8, 0, msg.c_str(), -1, dst, static_cast<int>( dst_cch ) );
	if ( written <= 0 ) {
		std::size_t i = 0;
		for ( ; ( i + 1 < dst_cch ) && ( i < msg.size() ); ++i ) {
			dst[ i ] = static_cast<wchar_t>( static_cast<unsigned char>( msg[ i ] ) );
		}
		dst[ i ] = L'\0';
	}
	else {
		dst[ dst_cch - 1 ] = L'\0';
	}
}

void copy_error_a( char* dst, std::size_t dst_cch, const std::string& msg ) noexcept
{
	if ( ( dst == nullptr ) || ( dst_cch == 0 ) ) {
		return;
	}
	const std::size_t n = std::min( dst_cch - 1, msg.size() );
	if ( n > 0 ) {
		std::memcpy( dst, msg.data(), n );
	}
	dst[ n ] = '\0';
}

std::wstring widen_acp( const char* str )
{
	if ( ( str == nullptr ) || ( str[ 0 ] == '\0' ) ) {
		return {};
	}
	const int needed = MultiByteToWideChar( CP_ACP, 0, str, -1, nullptr, 0 );
	if ( needed <= 1 ) {
		return {};
	}
	std::wstring out( static_cast<std::size_t>( needed - 1 ), L'\0' );
	MultiByteToWideChar( CP_ACP, 0, str, -1, out.data(), needed );
	return out;
}

bool parse_jpeg_dimensions( const std::uint8_t* jpg_data, std::size_t jpg_size, ImageHeaderInfo& info ) noexcept
{
	if ( ( jpg_data == nullptr ) || ( jpg_size < 4 ) ||
		 ( jpg_data[ 0 ] != 0xFF ) || ( jpg_data[ 1 ] != 0xD8 ) ) {
		return false;
	}

	info = ImageHeaderInfo{};
	std::size_t pos = 2;
	while ( pos + 3 < jpg_size ) {
		while ( pos < jpg_size && jpg_data[ pos ] != 0xFF ) {
			++pos;
		}
		while ( pos < jpg_size && jpg_data[ pos ] == 0xFF ) {
			++pos;
		}
		if ( pos >= jpg_size ) break;

		const std::uint8_t marker = jpg_data[ pos++ ];
		if ( ( marker == 0xD8 ) || ( marker == 0xD9 ) || ( ( marker >= 0xD0 ) && ( marker <= 0xD7 ) ) || ( marker == 0x01 ) ) {
			if ( marker == 0xD9 ) break;
			continue;
		}
		if ( pos + 2 > jpg_size ) return false;
		const std::size_t seg_len = ( static_cast<std::size_t>( jpg_data[ pos ] ) << 8 ) | jpg_data[ pos + 1 ];
		if ( ( seg_len < 2 ) || ( pos + seg_len > jpg_size ) ) return false;

		if ( ( marker == 0xE0 ) && ( seg_len >= 14 ) &&
			 ( std::memcmp( jpg_data + pos + 2, "JFIF\0", 5 ) == 0 ) ) {
			const std::uint8_t units = jpg_data[ pos + 9 ];
			const int xd = ( static_cast<int>( jpg_data[ pos + 10 ] ) << 8 ) | jpg_data[ pos + 11 ];
			const int yd = ( static_cast<int>( jpg_data[ pos + 12 ] ) << 8 ) | jpg_data[ pos + 13 ];
			if ( units == 1 ) {
				info.dpi_x = xd;
				info.dpi_y = yd;
			}
			else if ( units == 2 ) {
				info.dpi_x = static_cast<int>( std::lround( xd * 2.54 ) );
				info.dpi_y = static_cast<int>( std::lround( yd * 2.54 ) );
			}
		}
		else if ( ( marker == 0xC0 ) || ( marker == 0xC1 ) || ( marker == 0xC2 ) ) {
			if ( seg_len < 8 ) return false;
			info.height = ( static_cast<int>( jpg_data[ pos + 3 ] ) << 8 ) | jpg_data[ pos + 4 ];
			info.width  = ( static_cast<int>( jpg_data[ pos + 5 ] ) << 8 ) | jpg_data[ pos + 6 ];
			const int cmpc = jpg_data[ pos + 7 ];
			info.bit_depth = ( cmpc == 1 ) ? 8 : 24;
			return ( info.width > 0 ) && ( info.height > 0 );
		}
		else if ( marker == 0xDA ) {
			break;
		}

		pos += seg_len;
	}
	return false;
}

bool query_pjg_info_from_memory(
	const std::uint8_t* pjg_data,
	std::size_t pjg_size,
	ImageHeaderInfo& info,
	std::string& err )
{
	if ( !has_pjg_signature( pjg_data, pjg_size ) ) {
		err = "input is not a valid packJPG (.pjg) stream";
		return false;
	}

	std::vector<std::uint8_t> jpg_bytes;
	if ( !packjpg::convert_memory( pjg_data, pjg_size, jpg_bytes, &err, false, 2 ) ) {
		if ( err.empty() ) err = "failed to decompress PJG stream";
		return false;
	}
	if ( !parse_jpeg_dimensions( jpg_bytes.data(), jpg_bytes.size(), info ) ) {
		err = "reconstructed JPEG header is malformed";
		return false;
	}
	return true;
}

HGLOBAL decode_pjg_memory_to_packed_dib(
	const std::uint8_t* pjg_data,
	std::size_t pjg_size,
	bool force_grayscale,
	std::string& err )
{
	if ( !has_pjg_signature( pjg_data, pjg_size ) ) {
		err = "input is not a valid packJPG (.pjg) stream";
		return nullptr;
	}

	std::vector<std::uint8_t> jpg_bytes;
	if ( !packjpg::convert_memory( pjg_data, pjg_size, jpg_bytes, &err, false, 2 ) ) {
		if ( err.empty() ) err = "packJPG decompression failed";
		return nullptr;
	}

	DecodedImagePixels decoded;
	if ( !decode_jpeg_bytes_via_wic( jpg_bytes.data(), jpg_bytes.size(), force_grayscale, decoded, err ) ) {
		return nullptr;
	}

	const std::size_t palette_bytes = ( decoded.bpp_bytes == 1 ) ? ( 256u * sizeof( RGBQUAD ) ) : 0u;
	const std::size_t total_dib_bytes = sizeof( BITMAPINFOHEADER ) + palette_bytes + decoded.bottom_up_rows.size();

	HGLOBAL h_dib = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, total_dib_bytes );
	if ( h_dib == nullptr ) {
		err = "out of memory allocating Windows DIB";
		return nullptr;
	}

	auto* base = static_cast<std::uint8_t*>( GlobalLock( h_dib ) );
	if ( base == nullptr ) {
		GlobalFree( h_dib );
		err = "failed to lock Windows DIB";
		return nullptr;
	}

	auto* bih = reinterpret_cast<BITMAPINFOHEADER*>( base );
	auto* pal = ( palette_bytes > 0 )
		? reinterpret_cast<RGBQUAD*>( base + sizeof( BITMAPINFOHEADER ) )
		: nullptr;
	populate_dib_header_and_palette( decoded, bih, pal );
	std::memcpy(
		base + sizeof( BITMAPINFOHEADER ) + palette_bytes,
		decoded.bottom_up_rows.data(),
		decoded.bottom_up_rows.size() );
	GlobalUnlock( h_dib );
	return h_dib;
}

int decode_pjg_memory_to_susie_dib(
	const std::uint8_t* pjg_data,
	std::size_t pjg_size,
	HANDLE* out_binfo,
	HANDLE* out_bits,
	std::string& err )
{
	if ( ( out_binfo == nullptr ) || ( out_bits == nullptr ) ) {
		err = "null Susie output handle pointer";
		return SPI_MEMORY_ERROR;
	}
	*out_binfo = nullptr;
	*out_bits  = nullptr;

	if ( !has_pjg_signature( pjg_data, pjg_size ) ) {
		err = "not a packJPG stream";
		return SPI_NOT_SUPPORT;
	}

	std::vector<std::uint8_t> jpg_bytes;
	if ( !packjpg::convert_memory( pjg_data, pjg_size, jpg_bytes, &err, false, 2 ) ) {
		return SPI_OUT_OF_ORDER;
	}

	DecodedImagePixels decoded;
	if ( !decode_jpeg_bytes_via_wic( jpg_bytes.data(), jpg_bytes.size(), false, decoded, err ) ) {
		return SPI_OUT_OF_ORDER;
	}

	const std::size_t palette_bytes = ( decoded.bpp_bytes == 1 ) ? ( 256u * sizeof( RGBQUAD ) ) : 0u;
	const std::size_t info_size = sizeof( BITMAPINFOHEADER ) + palette_bytes;

	HLOCAL h_info = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT, info_size );
	if ( h_info == nullptr ) {
		return SPI_NO_MEMORY;
	}
	HLOCAL h_bits = LocalAlloc( LMEM_MOVEABLE, decoded.bottom_up_rows.size() );
	if ( h_bits == nullptr ) {
		LocalFree( h_info );
		return SPI_NO_MEMORY;
	}

	auto* info_ptr = static_cast<std::uint8_t*>( LocalLock( h_info ) );
	void* bits_ptr = LocalLock( h_bits );
	if ( ( info_ptr == nullptr ) || ( bits_ptr == nullptr ) ) {
		if ( info_ptr != nullptr ) LocalUnlock( h_info );
		if ( bits_ptr != nullptr ) LocalUnlock( h_bits );
		LocalFree( h_info );
		LocalFree( h_bits );
		return SPI_MEMORY_ERROR;
	}

	populate_dib_header_and_palette(
		decoded,
		reinterpret_cast<BITMAPINFOHEADER*>( info_ptr ),
		( palette_bytes > 0 ) ? reinterpret_cast<RGBQUAD*>( info_ptr + sizeof( BITMAPINFOHEADER ) ) : nullptr );
	std::memcpy( bits_ptr, decoded.bottom_up_rows.data(), decoded.bottom_up_rows.size() );

	LocalUnlock( h_info );
	LocalUnlock( h_bits );

	*out_binfo = h_info;
	*out_bits  = h_bits;
	return SPI_ALL_RIGHT;
}

bool encode_packed_dib_to_pjg_memory(
	HANDLE h_dib,
	int quality,
	std::vector<std::uint8_t>& out_pjg,
	std::string& err )
{
	out_pjg.clear();
	if ( h_dib == nullptr ) {
		err = "null DIB handle";
		return false;
	}

	ScopedGlobalLock lock( h_dib );
	const auto* base = static_cast<const std::uint8_t*>( lock.get() );
	const SIZE_T total_alloc = GlobalSize( h_dib );
	if ( ( base == nullptr ) || ( total_alloc < sizeof( BITMAPINFOHEADER ) ) ) {
		err = "invalid DIB memory block";
		return false;
	}

	const auto* bih = reinterpret_cast<const BITMAPINFOHEADER*>( base );
	if ( ( bih->biSize < sizeof( BITMAPINFOHEADER ) ) ||
		 ( bih->biPlanes != 1 ) ||
		 ( bih->biCompression != BI_RGB ) ) {
		err = "unsupported DIB header or compression format";
		return false;
	}

	const int width = bih->biWidth;
	const long long raw_height = bih->biHeight;
	const bool bottom_up = ( raw_height > 0 );
	const long long abs_height_ll = bottom_up ? raw_height : -raw_height;
	if ( ( width <= 0 ) || ( width > 32768 ) || ( abs_height_ll <= 0 ) || ( abs_height_ll > 32768 ) ) {
		err = "invalid DIB dimensions";
		return false;
	}
	const int height = static_cast<int>( abs_height_ll );
	const int bpp = bih->biBitCount;
	if ( ( bpp != 8 ) && ( bpp != 24 ) && ( bpp != 32 ) ) {
		err = "only 8-bit, 24-bit, and 32-bit DIBs are supported for PJG saving";
		return false;
	}

	const std::size_t clr_used = ( bpp == 8 )
		? ( ( bih->biClrUsed > 0 && bih->biClrUsed <= 256 ) ? static_cast<std::size_t>( bih->biClrUsed ) : 256u )
		: static_cast<std::size_t>( bih->biClrUsed );
	const std::size_t header_and_pal = static_cast<std::size_t>( bih->biSize ) + clr_used * sizeof( RGBQUAD );
	const std::size_t src_stride = ( ( static_cast<std::size_t>( width ) * static_cast<std::size_t>( bpp / 8 ) ) + 3u ) & ~std::size_t( 3u );
	const std::size_t required_bytes = header_and_pal + src_stride * static_cast<std::size_t>( height );
	if ( required_bytes > total_alloc ) {
		err = "DIB buffer is smaller than declared dimensions";
		return false;
	}

	const auto* pal = ( clr_used > 0 )
		? reinterpret_cast<const RGBQUAD*>( base + bih->biSize )
		: nullptr;
	const std::uint8_t* src_pixels = base + header_and_pal;

	bool is_pure_gray_palette = ( bpp == 8 ) && ( clr_used > 0 );
	if ( is_pure_gray_palette ) {
		for ( std::size_t i = 0; i < clr_used; ++i ) {
			if ( ( pal[ i ].rgbRed != pal[ i ].rgbGreen ) || ( pal[ i ].rgbRed != pal[ i ].rgbBlue ) ) {
				is_pure_gray_palette = false;
				break;
			}
		}
	}

	const bool encode_gray = ( bpp == 8 ) && is_pure_gray_palette;
	const int dst_bpp_bytes = encode_gray ? 1 : 3;
	const std::uint64_t dst_stride64 = ( static_cast<std::uint64_t>( width ) * static_cast<std::uint64_t>( dst_bpp_bytes ) + 3u ) & ~std::uint64_t( 3u );
	const std::uint64_t dst_total64  = dst_stride64 * static_cast<std::uint64_t>( height );
	if ( ( dst_total64 > ( 512ull << 20 ) ) || ( dst_total64 > static_cast<std::uint64_t>( SIZE_MAX / 2 ) ) ) {
		err = "DIB exceeds maximum supported save buffer size";
		return false;
	}
	const UINT dst_stride = static_cast<UINT>( dst_stride64 );
	std::vector<std::uint8_t> top_down( static_cast<std::size_t>( dst_total64 ), 0 );

	for ( int y = 0; y < height; ++y ) {
		const int src_row_idx = bottom_up ? ( height - 1 - y ) : y;
		const std::uint8_t* srow = src_pixels + static_cast<std::size_t>( src_row_idx ) * src_stride;
		std::uint8_t* drow = top_down.data() + static_cast<std::size_t>( y ) * dst_stride;

		if ( encode_gray ) {
			for ( int x = 0; x < width; ++x ) {
				const std::size_t idx = std::min<std::size_t>( srow[ x ], clr_used - 1 );
				drow[ x ] = pal[ idx ].rgbRed;
			}
		}
		else if ( bpp == 8 ) {
			for ( int x = 0; x < width; ++x ) {
				const std::size_t idx = std::min<std::size_t>( srow[ x ], clr_used - 1 );
				drow[ x * 3 + 0 ] = pal[ idx ].rgbBlue;
				drow[ x * 3 + 1 ] = pal[ idx ].rgbGreen;
				drow[ x * 3 + 2 ] = pal[ idx ].rgbRed;
			}
		}
		else if ( bpp == 24 ) {
			std::memcpy( drow, srow, static_cast<std::size_t>( width ) * 3u );
		}
		else {
			for ( int x = 0; x < width; ++x ) {
				drow[ x * 3 + 0 ] = srow[ x * 4 + 0 ];
				drow[ x * 3 + 1 ] = srow[ x * 4 + 1 ];
				drow[ x * 3 + 2 ] = srow[ x * 4 + 2 ];
			}
		}
	}

	const LONG x_ppm = bih->biXPelsPerMeter;
	const LONG y_ppm = bih->biYPelsPerMeter;

	ScopedComInit com;
	if ( !com.ok() ) {
		err = "failed to initialize Windows COM runtime";
		return false;
	}

	ComPtr<IWICImagingFactory> factory;
	if ( FAILED( create_wic_factory( factory ) ) ) {
		err = "failed to create IWICImagingFactory";
		return false;
	}

	ComPtr<IStream> jpg_stream;
	if ( FAILED( CreateStreamOnHGlobal( nullptr, TRUE, jpg_stream.put() ) ) ) {
		err = "failed to allocate output JPEG memory stream";
		return false;
	}

	ComPtr<IWICBitmapEncoder> encoder;
	if ( FAILED( factory->CreateEncoder( GUID_ContainerFormatJpeg, nullptr, encoder.put() ) ) ||
		 FAILED( encoder->Initialize( jpg_stream.get(), WICBitmapEncoderNoCache ) ) ) {
		err = "failed to initialize WIC JPEG encoder";
		return false;
	}

	ComPtr<IWICBitmapFrameEncode> frame;
	ComPtr<IPropertyBag2> props;
	if ( FAILED( encoder->CreateNewFrame( frame.put(), props.put() ) ) ) {
		err = "failed to create WIC JPEG encoder frame";
		return false;
	}

	if ( props ) {
		const int clamped_q = ( quality <= 0 ) ? 90 : std::min( 100, quality );
		PROPBAG2 opt{};
		wchar_t opt_name[] = L"ImageQuality";
		opt.pstrName = opt_name;
		VARIANT var{};
		VariantInit( &var );
		var.vt     = VT_R4;
		var.fltVal = static_cast<float>( clamped_q ) / 100.0f;
		props->Write( 1, &opt, &var );
	}

	if ( FAILED( frame->Initialize( props.get() ) ) ||
		 FAILED( frame->SetSize( static_cast<UINT>( width ), static_cast<UINT>( height ) ) ) ) {
		err = "failed to configure WIC JPEG frame dimensions";
		return false;
	}

	if ( ( x_ppm > 0 ) && ( y_ppm > 0 ) ) {
		const double dpi_x = static_cast<double>( x_ppm ) / 39.37007874;
		const double dpi_y = static_cast<double>( y_ppm ) / 39.37007874;
		if ( dpi_x >= 1.0 && dpi_x <= 65535.0 && dpi_y >= 1.0 && dpi_y <= 65535.0 ) {
			frame->SetResolution( dpi_x, dpi_y );
		}
	}

	WICPixelFormatGUID target_fmt = encode_gray ? GUID_WICPixelFormat8bppGray : GUID_WICPixelFormat24bppBGR;
	if ( FAILED( frame->SetPixelFormat( &target_fmt ) ) ) {
		err = "failed to set WIC JPEG pixel format";
		return false;
	}


	const WICPixelFormatGUID src_fmt = encode_gray ? GUID_WICPixelFormat8bppGray : GUID_WICPixelFormat24bppBGR;
	if ( IsEqualGUID( target_fmt, src_fmt ) ) {
		if ( FAILED( frame->WritePixels(
				static_cast<UINT>( height ), dst_stride, static_cast<UINT>( top_down.size() ), top_down.data() ) ) ) {
			err = "WIC WritePixels failed";
			return false;
		}
	}
	else {
		ComPtr<IWICBitmap> wic_bmp;
		if ( FAILED( factory->CreateBitmapFromMemory(
				static_cast<UINT>( width ),
				static_cast<UINT>( height ),
				src_fmt,
				dst_stride,
				static_cast<UINT>( top_down.size() ),
				top_down.data(),
				wic_bmp.put() ) ) ||
			 FAILED( frame->WriteSource( wic_bmp.get(), nullptr ) ) ) {
			err = "WIC WriteSource conversion failed";
			return false;
		}
	}

	if ( FAILED( frame->Commit() ) || FAILED( encoder->Commit() ) ) {
		err = "WIC JPEG commit failed";
		return false;
	}

	STATSTG stat{};
	if ( FAILED( jpg_stream->Stat( &stat, STATFLAG_NONAME ) ) ||
		 ( stat.cbSize.QuadPart < 4 ) ||
		 ( stat.cbSize.QuadPart > ( 256ull << 20 ) ) ) {
		err = "encoded JPEG stream size is invalid";
		return false;
	}

	LARGE_INTEGER zero{};
	if ( FAILED( jpg_stream->Seek( zero, STREAM_SEEK_SET, nullptr ) ) ) {
		err = "failed to rewind encoded JPEG stream";
		return false;
	}

	std::vector<std::uint8_t> jpg_bytes( static_cast<std::size_t>( stat.cbSize.QuadPart ) );
	ULONG got = 0;
	if ( FAILED( jpg_stream->Read( jpg_bytes.data(), static_cast<ULONG>( jpg_bytes.size() ), &got ) ) ||
		 ( got != jpg_bytes.size() ) ) {
		err = "failed to read encoded JPEG bytes";
		return false;
	}

	if ( !packjpg::convert_memory( jpg_bytes.data(), jpg_bytes.size(), out_pjg, &err, false, 2 ) ) {
		if ( err.empty() ) err = "packJPG compression failed";
		return false;
	}

	return true;
}

} // namespace pjg_iv

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
#include <atomic>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <system_error>

namespace pjg_iv {
namespace {

constexpr DWORD kBiAlphaBitfields = 6u;

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
		unlock();
	}

	void* get() const noexcept { return ptr_; }

	void unlock() noexcept {
		if ( ptr_ != nullptr ) {
			GlobalUnlock( h_ );
			ptr_ = nullptr;
		}
	}

private:
	HGLOBAL h_   = nullptr;
	void*   ptr_ = nullptr;
};

HRESULT create_wic_factory( ComPtr<IWICImagingFactory>& factory ) noexcept
{
	static const GUID kClsidWicFactory1 = {
		0xcacaf262, 0x9370, 0x4615, { 0xa1, 0x3b, 0x9f, 0x55, 0x39, 0xda, 0x4c, 0x0a }
	};
	static const GUID kClsidWicFactory2 = {
		0x317d06e8, 0x5f24, 0x433d, { 0xbd, 0xf7, 0x79, 0xce, 0x68, 0xd8, 0xab, 0xc2 }
	};

	HRESULT hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		reinterpret_cast<void**>( factory.put() ) );
	if ( FAILED( hr ) ) {
		hr = CoCreateInstance(
			kClsidWicFactory1,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			reinterpret_cast<void**>( factory.put() ) );
	}
	if ( FAILED( hr ) ) {
		hr = CoCreateInstance(
			kClsidWicFactory2,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_IWICImagingFactory,
			reinterpret_cast<void**>( factory.put() ) );
	}
	return hr;
}

struct BitfieldExtractor {
	std::uint32_t mask  = 0;
	unsigned int  shift = 0;
	unsigned int  bits  = 0;
};

BitfieldExtractor make_bitfield_extractor( std::uint32_t mask ) noexcept
{
	BitfieldExtractor ex{};
	ex.mask = mask;
	if ( mask == 0 ) {
		return ex;
	}
	unsigned int s = 0;
	while ( ( s < 32u ) && ( ( ( mask >> s ) & 1u ) == 0u ) ) {
		++s;
	}
	unsigned int b = 0;
	while ( ( ( s + b ) < 32u ) && ( ( ( mask >> ( s + b ) ) & 1u ) != 0u ) ) {
		++b;
	}
	ex.shift = s;
	ex.bits  = b;
	return ex;
}

std::uint8_t extract_channel_u8( std::uint32_t pixel, const BitfieldExtractor& ex ) noexcept
{
	if ( ex.bits == 0 ) {
		return 0;
	}
	const std::uint32_t field_mask = ( ex.bits >= 32u ) ? 0xFFFFFFFFu : ( ( 1u << ex.bits ) - 1u );
	const std::uint32_t raw = ( ( pixel & ex.mask ) >> ex.shift ) & field_mask;
	if ( ex.bits >= 8u ) {
		return static_cast<std::uint8_t>( raw >> ( ex.bits - 8u ) );
	}
	const std::uint32_t max_v = ( 1u << ex.bits ) - 1u;
	return static_cast<std::uint8_t>( ( raw * 255u + ( max_v / 2u ) ) / max_v );
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
	if ( ( jpg_data == nullptr ) || ( jpg_size < 4 ) || ( jpg_size > 0xFFFFFFFFu ) ) {
		err = "reconstructed JPEG buffer is empty or oversized";
		return false;
	}

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

	ComPtr<IWICStream> wic_stream;
	if ( FAILED( factory->CreateStream( wic_stream.put() ) ) ||
		 FAILED( wic_stream->InitializeFromMemory(
			 const_cast<BYTE*>( jpg_data ), static_cast<DWORD>( jpg_size ) ) ) ) {
		err = "failed to initialize WIC memory stream";
		return false;
	}

	ComPtr<IWICBitmapDecoder> decoder;
	if ( FAILED( factory->CreateDecoderFromStream(
			wic_stream.get(), nullptr, WICDecodeMetadataCacheOnDemand, decoder.put() ) ) ) {
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
		 ( width > 65535u ) || ( height > 65535u ) ) {
		err = "invalid or unsupported JPEG dimensions";
		return false;
	}

	double rx = 0.0;
	double ry = 0.0;
	if ( SUCCEEDED( frame->GetResolution( &rx, &ry ) ) ) {
		if ( rx > 0.0 && rx <= 65535.0 ) out.dpi_x = static_cast<int>( std::lround( rx ) );
		if ( ry > 0.0 && ry <= 65535.0 ) out.dpi_y = static_cast<int>( std::lround( ry ) );
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
	ComPtr<IWICFormatConverter> converter_stage1;
	ComPtr<IWICFormatConverter> converter_stage2;
	if ( IsEqualGUID( src_fmt, dst_fmt ) ) {
		if ( FAILED( frame->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( pixel_src.put() ) ) ) ) {
			err = "failed to acquire IWICBitmapSource";
			return false;
		}
	}
	else {
		bool converted = false;
		if ( SUCCEEDED( factory->CreateFormatConverter( converter_stage1.put() ) ) &&
			 SUCCEEDED( converter_stage1->Initialize(
				 frame.get(), dst_fmt, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) &&
			 SUCCEEDED( converter_stage1->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( pixel_src.put() ) ) ) ) {
			converted = true;
		}
		else if ( use_gray ) {
			// Fallback for 32bppCMYK -> 24bppBGR -> 8bppGray when direct CMYK -> Gray is unsupported.
			converter_stage1.reset();
			if ( SUCCEEDED( factory->CreateFormatConverter( converter_stage1.put() ) ) &&
				 SUCCEEDED( converter_stage1->Initialize(
					 frame.get(), GUID_WICPixelFormat24bppBGR, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) &&
				 SUCCEEDED( factory->CreateFormatConverter( converter_stage2.put() ) ) &&
				 SUCCEEDED( converter_stage2->Initialize(
					 converter_stage1.get(), GUID_WICPixelFormat8bppGray, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) &&
				 SUCCEEDED( converter_stage2->QueryInterface( IID_IWICBitmapSource, reinterpret_cast<void**>( pixel_src.put() ) ) ) ) {
				converted = true;
			}
		}
		if ( !converted ) {
			err = "WIC pixel format conversion failed";
			return false;
		}
	}

	const UINT stride = static_cast<UINT>( stride64 );
	out.bottom_up_rows.assign( static_cast<std::size_t>( total_pixels ), 0 );
	if ( FAILED( pixel_src->CopyPixels(
			nullptr, stride, static_cast<UINT>( out.bottom_up_rows.size() ), out.bottom_up_rows.data() ) ) ) {
		out.bottom_up_rows.clear();
		err = "WIC CopyPixels failed";
		return false;
	}

	// Flip top-down WIC scanlines into bottom-up Windows DIB order in-place (zero second full-image buffer).
	if ( height > 1u ) {
		std::vector<std::uint8_t> row_tmp( stride );
		const std::size_t row_copy_bytes = static_cast<std::size_t>( stride );
		for ( UINT y = 0; y < height / 2u; ++y ) {
			std::uint8_t* top_row = out.bottom_up_rows.data() + static_cast<std::size_t>( y ) * row_copy_bytes;
			std::uint8_t* bot_row = out.bottom_up_rows.data() + static_cast<std::size_t>( height - 1u - y ) * row_copy_bytes;
			std::memcpy( row_tmp.data(), top_row, row_copy_bytes );
			std::memcpy( top_row, bot_row, row_copy_bytes );
			std::memcpy( bot_row, row_tmp.data(), row_copy_bytes );
		}
	}

	out.width     = width;
	out.height    = height;
	out.bpp_bytes = bpp_bytes;
	out.stride    = stride;
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
	if ( data[ pos ] == 0x00 ) {
		// Custom segment/threshold header: 'J','S', 0x00, nois_trs[4], segm_cnt[4], kAppVersion (>= 12 bytes total).
		if ( size < 12 ) {
			return false;
		}
		++pos;
		for ( std::size_t c = 0; c < 4; ++c ) {
			if ( ( data[ pos + c ] > 10 ) || ( data[ pos + 4 + c ] < 1 ) || ( data[ pos + 4 + c ] > 49 ) ) {
				return false;
			}
		}
		pos += 8;
	}
	return ( pos < size ) && ( data[ pos ] >= 0x14 ) && ( data[ pos ] <= 0x1F );
}

std::wstring normalize_win32_file_path_w( const wchar_t* raw_path )
{
	if ( ( raw_path == nullptr ) || ( raw_path[ 0 ] == L'\0' ) ) {
		return {};
	}
	std::wstring p( raw_path );
	if ( ( p.rfind( L"\\\\?\\", 0 ) != 0 ) && ( p.rfind( L"\\\\.\\", 0 ) != 0 ) ) {
		const DWORD need = GetFullPathNameW( raw_path, 0, nullptr, nullptr );
		if ( need > 1 ) {
			std::wstring full_buf( static_cast<std::size_t>( need ), L'\0' );
			const DWORD got = GetFullPathNameW( raw_path, need, full_buf.data(), nullptr );
			if ( ( got > 0 ) && ( got < need ) ) {
				full_buf.resize( static_cast<std::size_t>( got ) );
				p = std::move( full_buf );
			}
		}
	}
	return p;
}

std::wstring add_extended_length_prefix_if_needed( const std::wstring& full_path )
{
	if ( ( full_path.size() < 240 ) ||
	     ( full_path.rfind( L"\\\\?\\", 0 ) == 0 ) ||
	     ( full_path.rfind( L"\\\\.\\", 0 ) == 0 ) ) {
		return full_path;
	}
	std::wstring norm = full_path;
	for ( wchar_t& wc : norm ) {
		if ( wc == L'/' ) {
			wc = L'\\';
		}
	}
	if ( norm.rfind( L"\\\\", 0 ) == 0 ) {
		return L"\\\\?\\UNC\\" + norm.substr( 2 );
	}
	if ( ( norm.size() >= 3 ) && ( norm[ 1 ] == L':' ) && ( norm[ 2 ] == L'\\' ) ) {
		return L"\\\\?\\" + norm;
	}
	return full_path;
}

bool read_file_bytes_w(
	const wchar_t* path,
	std::vector<std::uint8_t>& out_bytes,
	std::string& err,
	std::uint64_t start_offset )
{
	out_bytes.clear();
	if ( ( path == nullptr ) || ( path[ 0 ] == L'\0' ) ) {
		err = "empty input filename";
		return false;
	}

	const std::wstring open_path = add_extended_length_prefix_if_needed( normalize_win32_file_path_w( path ) );
	HANDLE hf = CreateFileW(
		open_path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr );
	if ( hf == INVALID_HANDLE_VALUE ) {
		err = "could not open input file";
		return false;
	}

	LARGE_INTEGER fsize{};
	if ( !GetFileSizeEx( hf, &fsize ) || ( fsize.QuadPart < 0 ) ) {
		CloseHandle( hf );
		err = "invalid input file size";
		return false;
	}

	const std::uint64_t total_file_bytes = static_cast<std::uint64_t>( fsize.QuadPart );
	if ( start_offset > total_file_bytes ) {
		CloseHandle( hf );
		err = "input file offset exceeds file size";
		return false;
	}

	const std::uint64_t rem_bytes = total_file_bytes - start_offset;
	if ( rem_bytes > ( 256ull << 20 ) ) {
		CloseHandle( hf );
		err = "input file exceeds 256 MiB plugin limit";
		return false;
	}

	if ( start_offset > 0 ) {
		LARGE_INTEGER seek_pos{};
		seek_pos.QuadPart = static_cast<LONGLONG>( start_offset );
		if ( !SetFilePointerEx( hf, seek_pos, nullptr, FILE_BEGIN ) ) {
			CloseHandle( hf );
			err = "failed to seek to input file offset";
			return false;
		}
	}

	out_bytes.resize( static_cast<std::size_t>( rem_bytes ) );
	std::size_t offset = 0;
	while ( offset < out_bytes.size() ) {
		const DWORD chunk = static_cast<DWORD>(
			std::min<std::size_t>( out_bytes.size() - offset, 1u << 20 ) );
		DWORD got = 0;
		if ( !ReadFile( hf, out_bytes.data() + offset, chunk, &got, nullptr ) || ( got == 0 ) ) {
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

	std::wstring dst_full = normalize_win32_file_path_w( path );
	std::wstring dst_open = add_extended_length_prefix_if_needed( dst_full );

	DWORD existing_attrs = GetFileAttributesW( dst_open.c_str() );
	if ( ( existing_attrs != INVALID_FILE_ATTRIBUTES ) &&
	     ( ( existing_attrs & FILE_ATTRIBUTE_REPARSE_POINT ) != 0 ) ) {
		try {
			auto strip_extended_prefix = []( std::wstring ws ) -> std::wstring {
				if ( ws.rfind( L"\\\\?\\UNC\\", 0 ) == 0 ) {
					return L"\\\\" + ws.substr( 8 );
				}
				if ( ws.rfind( L"\\\\?\\", 0 ) == 0 ) {
					ws.erase( 0, 4 );
				}
				return ws;
			};

			std::wstring cur_full = dst_full;
			std::wstring cur_open = dst_open;
			std::error_code ec;
			if ( std::filesystem::is_symlink( std::filesystem::symlink_status( std::filesystem::path( cur_open ), ec ) ) && !ec ) {
				bool loop_detected = false;
				for ( int depth = 0; depth < 32; ++depth ) {
					const std::filesystem::path probe( cur_open );
					const auto st = std::filesystem::symlink_status( probe, ec );
					if ( ec || !std::filesystem::is_symlink( st ) ) {
						break;
					}
					const std::filesystem::path target = std::filesystem::read_symlink( probe, ec );
					if ( ec || target.empty() ) {
						break;
					}
					std::wstring next_raw;
					if ( target.is_absolute() ) {
						next_raw = strip_extended_prefix( target.wstring() );
					}
					else {
						std::filesystem::path base_dir = std::filesystem::path( cur_full ).parent_path();
						if ( !base_dir.empty() ) {
							std::error_code canon_ec;
							const auto canon_dir = std::filesystem::weakly_canonical(
								std::filesystem::path( add_extended_length_prefix_if_needed( base_dir.wstring() ) ),
								canon_ec );
							if ( !canon_ec && !canon_dir.empty() ) {
								base_dir = std::filesystem::path( strip_extended_prefix( canon_dir.wstring() ) );
							}
						}
						next_raw = ( base_dir.empty() ? target : ( base_dir / target ) ).wstring();
					}
					cur_full = normalize_win32_file_path_w( next_raw.c_str() );
					cur_open = add_extended_length_prefix_if_needed( cur_full );
					if ( depth == 31 ) {
						const auto last_st = std::filesystem::symlink_status( std::filesystem::path( cur_open ), ec );
						loop_detected = !ec && std::filesystem::is_symlink( last_st );
					}
				}
				if ( loop_detected ) {
					err = "symbolic link loop in output filename";
					return false;
				}
				dst_full = std::move( cur_full );
				dst_open = std::move( cur_open );
				existing_attrs = GetFileAttributesW( dst_open.c_str() );
			}
		} catch ( ... ) {
		}
	}

	if ( existing_attrs != INVALID_FILE_ATTRIBUTES ) {
		if ( ( existing_attrs & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {
			err = "output path is a directory";
			return false;
		}
		if ( ( existing_attrs & FILE_ATTRIBUTE_READONLY ) != 0 ) {
			err = "output file is write-protected";
			return false;
		}
	}

	std::wstring dir_prefix;
	const std::size_t slash_pos = dst_full.find_last_of( L"\\/" );
	if ( slash_pos != std::wstring::npos ) {
		dir_prefix = dst_full.substr( 0, slash_pos + 1 );
	}
	else if ( ( dst_full.size() >= 2 ) && ( dst_full[ 1 ] == L':' ) ) {
		dir_prefix = dst_full.substr( 0, 2 );
	}

	static std::atomic<std::uint64_t> temp_seq{ 0 };
	const DWORD pid = GetCurrentProcessId();
	std::wstring tmp_open;
	HANDLE hf = INVALID_HANDLE_VALUE;
	for ( unsigned int attempt = 0; attempt < 4096; ++attempt ) {
		const std::uint64_t id = temp_seq.fetch_add( 1, std::memory_order_relaxed ) + 1;
		const std::wstring tmp_raw =
			dir_prefix + L".pjg_tmp_" + std::to_wstring( pid ) + L"_" + std::to_wstring( id ) + L".tmp";
		tmp_open = add_extended_length_prefix_if_needed( tmp_raw );
		hf = CreateFileW(
			tmp_open.c_str(),
			GENERIC_WRITE,
			0,
			nullptr,
			CREATE_NEW,
			FILE_ATTRIBUTE_NORMAL,
			nullptr );
		if ( hf != INVALID_HANDLE_VALUE ) {
			break;
		}
		const DWORD gle = GetLastError();
		if ( gle != ERROR_FILE_EXISTS && gle != ERROR_ALREADY_EXISTS ) {
			err = "could not create temporary output file for writing";
			return false;
		}
	}
	if ( hf == INVALID_HANDLE_VALUE ) {
		err = "could not allocate unique temporary output filename";
		return false;
	}

	std::size_t offset = 0;
	while ( offset < size ) {
		const DWORD chunk = static_cast<DWORD>( std::min<std::size_t>( size - offset, 1u << 20 ) );
		DWORD written = 0;
		if ( !WriteFile( hf, data + offset, chunk, &written, nullptr ) || ( written != chunk ) ) {
			CloseHandle( hf );
			DeleteFileW( tmp_open.c_str() );
			err = "error writing output file";
			return false;
		}
		offset += written;
	}

	if ( !FlushFileBuffers( hf ) ) {
		CloseHandle( hf );
		DeleteFileW( tmp_open.c_str() );
		err = "error flushing output file";
		return false;
	}
	CloseHandle( hf );

	bool committed = false;
	if ( existing_attrs != INVALID_FILE_ATTRIBUTES ) {
		if ( ReplaceFileW(
		         dst_open.c_str(),
		         tmp_open.c_str(),
		         nullptr,
		         REPLACEFILE_IGNORE_MERGE_ERRORS | REPLACEFILE_IGNORE_ACL_ERRORS,
		         nullptr,
		         nullptr ) ) {
			committed = true;
		}
		else if ( ( existing_attrs & ( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ) ) != 0 ) {
			SetFileAttributesW(
				dst_open.c_str(),
				existing_attrs & ~( FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM ) );
			if ( MoveFileExW( tmp_open.c_str(), dst_open.c_str(), MOVEFILE_REPLACE_EXISTING ) ) {
				SetFileAttributesW( dst_open.c_str(), existing_attrs );
				committed = true;
			}
			else {
				SetFileAttributesW( dst_open.c_str(), existing_attrs );
			}
		}
	}

	if ( !committed && !MoveFileExW( tmp_open.c_str(), dst_open.c_str(), MOVEFILE_REPLACE_EXISTING ) ) {
		DeleteFileW( tmp_open.c_str() );
		err = "could not atomically commit output file";
		return false;
	}
	return true;
}

void copy_error_w( wchar_t* dst, std::size_t dst_cch, const std::string& msg ) noexcept
{
	if ( ( dst == nullptr ) || ( dst_cch == 0 ) ) {
		return;
	}
	dst[ 0 ] = L'\0';
	if ( msg.empty() || ( dst_cch == 1 ) ) {
		return;
	}
	try {
		int needed = MultiByteToWideChar(
			CP_UTF8, MB_ERR_INVALID_CHARS, msg.data(), static_cast<int>( msg.size() ), nullptr, 0 );
		UINT cp = CP_UTF8;
		DWORD flags = MB_ERR_INVALID_CHARS;
		if ( needed <= 0 ) {
			cp = CP_ACP;
			flags = 0;
			needed = MultiByteToWideChar( cp, flags, msg.data(), static_cast<int>( msg.size() ), nullptr, 0 );
		}
		if ( needed > 0 ) {
			std::wstring wmsg( static_cast<std::size_t>( needed ), L'\0' );
			MultiByteToWideChar( cp, flags, msg.data(), static_cast<int>( msg.size() ), wmsg.data(), needed );
			std::size_t copy_len = std::min<std::size_t>( wmsg.size(), dst_cch - 1 );
			if ( ( copy_len > 0 ) && ( copy_len < wmsg.size() ) &&
				 ( wmsg[ copy_len - 1 ] >= 0xD800 ) && ( wmsg[ copy_len - 1 ] <= 0xDBFF ) ) {
				--copy_len;
			}
			if ( copy_len > 0 ) {
				std::memcpy( dst, wmsg.data(), copy_len * sizeof( wchar_t ) );
			}
			dst[ copy_len ] = L'\0';
			return;
		}
	} catch ( ... ) {
	}

	std::size_t i = 0;
	for ( ; ( i + 1 < dst_cch ) && ( i < msg.size() ); ++i ) {
		dst[ i ] = static_cast<wchar_t>( static_cast<unsigned char>( msg[ i ] ) );
	}
	dst[ i ] = L'\0';
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

std::string narrow_acp_from_wide( const wchar_t* wstr )
{
	if ( ( wstr == nullptr ) || ( wstr[ 0 ] == L'\0' ) ) {
		return {};
	}
	int needed = WideCharToMultiByte( CP_ACP, 0, wstr, -1, nullptr, 0, nullptr, nullptr );
	UINT cp = CP_ACP;
	if ( needed <= 1 ) {
		cp = CP_UTF8;
		needed = WideCharToMultiByte( cp, 0, wstr, -1, nullptr, 0, nullptr, nullptr );
	}
	if ( needed <= 1 ) {
		return {};
	}
	std::string out( static_cast<std::size_t>( needed - 1 ), '\0' );
	WideCharToMultiByte( cp, 0, wstr, -1, out.data(), needed, nullptr, nullptr );
	return out;
}

bool parse_jpeg_dimensions( const std::uint8_t* jpg_data, std::size_t jpg_size, ImageHeaderInfo& info ) noexcept
{
	if ( ( jpg_data == nullptr ) || ( jpg_size < 4 ) ||
		 ( jpg_data[ 0 ] != 0xFF ) || ( jpg_data[ 1 ] != 0xD8 ) ) {
		return false;
	}

	info = ImageHeaderInfo{};
	bool found_sof = false;
	std::size_t pos = 2;
	while ( pos < jpg_size ) {
		while ( ( pos < jpg_size ) && ( jpg_data[ pos ] != 0xFF ) ) {
			++pos;
		}
		while ( ( pos < jpg_size ) && ( jpg_data[ pos ] == 0xFF ) ) {
			++pos;
		}
		if ( pos >= jpg_size ) {
			break;
		}

		const std::uint8_t marker = jpg_data[ pos++ ];
		if ( ( marker == 0x00 ) || ( marker == 0x01 ) || ( marker == 0xD8 ) ||
			 ( ( marker >= 0xD0 ) && ( marker <= 0xD7 ) ) ) {
			continue;
		}
		if ( ( marker == 0xD9 ) || ( marker == 0xDA ) ) {
			break;
		}
		if ( pos + 2 > jpg_size ) {
			return false;
		}
		const std::size_t seg_len = ( static_cast<std::size_t>( jpg_data[ pos ] ) << 8 ) | jpg_data[ pos + 1 ];
		if ( ( seg_len < 2 ) || ( pos + seg_len > jpg_size ) ) {
			return false;
		}

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
			if ( seg_len < 8 ) {
				return false;
			}
			info.height = ( static_cast<int>( jpg_data[ pos + 3 ] ) << 8 ) | jpg_data[ pos + 4 ];
			info.width  = ( static_cast<int>( jpg_data[ pos + 5 ] ) << 8 ) | jpg_data[ pos + 6 ];
			const int cmpc = jpg_data[ pos + 7 ];
			info.bit_depth = ( cmpc == 1 ) ? 8 : 24;
			found_sof = ( info.width > 0 ) && ( info.height > 0 );
		}

		pos += seg_len;
	}
	return found_sof && ( info.width > 0 ) && ( info.height > 0 );
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
	if ( !packjpg::convert_memory( pjg_data, pjg_size, jpg_bytes, &err, false, 1 ) ) {
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
	if ( !packjpg::convert_memory( pjg_data, pjg_size, jpg_bytes, &err, false, 1 ) ) {
		if ( err.empty() ) err = "packJPG decompression failed";
		return nullptr;
	}

	DecodedImagePixels decoded;
	if ( !decode_jpeg_bytes_via_wic( jpg_bytes.data(), jpg_bytes.size(), force_grayscale, decoded, err ) ) {
		return nullptr;
	}
	std::vector<std::uint8_t>().swap( jpg_bytes );

	const std::size_t palette_bytes = ( decoded.bpp_bytes == 1 ) ? ( 256u * sizeof( RGBQUAD ) ) : 0u;
	const std::size_t total_dib_bytes = sizeof( BITMAPINFOHEADER ) + palette_bytes + decoded.bottom_up_rows.size();

	HGLOBAL h_dib = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT, total_dib_bytes );
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
	if ( !packjpg::convert_memory( pjg_data, pjg_size, jpg_bytes, &err, false, 1 ) ) {
		return SPI_OUT_OF_ORDER;
	}

	DecodedImagePixels decoded;
	if ( !decode_jpeg_bytes_via_wic( jpg_bytes.data(), jpg_bytes.size(), false, decoded, err ) ) {
		return SPI_OUT_OF_ORDER;
	}
	std::vector<std::uint8_t>().swap( jpg_bytes );

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
	const std::uint64_t alloc64 = static_cast<std::uint64_t>( GlobalSize( h_dib ) );
	if ( ( base == nullptr ) || ( alloc64 < sizeof( BITMAPINFOHEADER ) ) ) {
		err = "invalid DIB memory block";
		return false;
	}

	const auto* bih = reinterpret_cast<const BITMAPINFOHEADER*>( base );
	const DWORD bi_size = bih->biSize;
	const DWORD comp    = bih->biCompression;
	if ( ( bi_size < sizeof( BITMAPINFOHEADER ) ) ||
		 ( bi_size > 4096u ) ||
		 ( static_cast<std::uint64_t>( bi_size ) > alloc64 ) ||
		 ( bih->biPlanes != 1 ) ) {
		err = "unsupported DIB header";
		return false;
	}

	const int bpp = bih->biBitCount;
	const bool valid_rgb = ( comp == BI_RGB ) &&
		( ( bpp == 1 ) || ( bpp == 4 ) || ( bpp == 8 ) || ( bpp == 16 ) || ( bpp == 24 ) || ( bpp == 32 ) );
	const bool valid_bitfields = ( ( comp == BI_BITFIELDS ) || ( comp == kBiAlphaBitfields ) ) &&
		( ( bpp == 16 ) || ( bpp == 32 ) );
	if ( !valid_rgb && !valid_bitfields ) {
		err = "unsupported DIB bit depth or compression format";
		return false;
	}

	const int width = bih->biWidth;
	const long long raw_height = static_cast<long long>( bih->biHeight );
	const bool bottom_up = ( raw_height > 0 );
	const long long abs_height_ll = bottom_up ? raw_height : -raw_height;
	if ( ( width <= 0 ) || ( width > 65535 ) || ( abs_height_ll <= 0 ) || ( abs_height_ll > 65535 ) ) {
		err = "invalid DIB dimensions";
		return false;
	}
	const int height = static_cast<int>( abs_height_ll );

	std::uint64_t extra_mask_bytes = 0;
	if ( valid_bitfields ) {
		if ( bi_size == sizeof( BITMAPINFOHEADER ) ) {
			extra_mask_bytes = ( comp == kBiAlphaBitfields ) ? ( 4ull * sizeof( DWORD ) ) : ( 3ull * sizeof( DWORD ) );
		}
		else if ( bi_size < 52u ) {
			err = "invalid DIB bitfields header size";
			return false;
		}
	}

	const std::uint64_t max_pal = ( bpp <= 8 ) ? ( std::uint64_t( 1u ) << bpp ) : 256ull;
	std::uint64_t clr_used64 = 0;
	if ( bpp <= 8 ) {
		if ( bih->biClrUsed == 0 ) {
			clr_used64 = max_pal;
		}
		else if ( bih->biClrUsed <= max_pal ) {
			clr_used64 = static_cast<std::uint64_t>( bih->biClrUsed );
		}
		else {
			err = "invalid DIB palette size";
			return false;
		}
	}
	else {
		if ( bih->biClrUsed > 256u ) {
			err = "invalid DIB color table size";
			return false;
		}
		clr_used64 = static_cast<std::uint64_t>( bih->biClrUsed );
	}

	const std::uint64_t pal_offset64   = static_cast<std::uint64_t>( bi_size ) + extra_mask_bytes;
	const std::uint64_t pixel_offset64 = pal_offset64 + clr_used64 * sizeof( RGBQUAD );
	const std::uint64_t row_bits64     = static_cast<std::uint64_t>( width ) * static_cast<std::uint64_t>( bpp );
	const std::uint64_t src_stride64   = ( ( row_bits64 + 31ull ) / 32ull ) * 4ull;
	const std::uint64_t pixel_bytes64  = src_stride64 * static_cast<std::uint64_t>( height );
	if ( ( pixel_offset64 > alloc64 ) || ( pixel_bytes64 > ( alloc64 - pixel_offset64 ) ) ) {
		err = "DIB buffer is smaller than declared dimensions";
		return false;
	}

	BitfieldExtractor r_ext{};
	BitfieldExtractor g_ext{};
	BitfieldExtractor b_ext{};
	if ( bpp == 16 && comp == BI_RGB ) {
		r_ext = make_bitfield_extractor( 0x00007C00u );
		g_ext = make_bitfield_extractor( 0x000003E0u );
		b_ext = make_bitfield_extractor( 0x0000001Fu );
	}
	else if ( valid_bitfields ) {
		// Both BITMAPINFOHEADER + trailing masks and BITMAPV4/V5HEADER place R/G/B masks at byte offset 40..51.
		std::uint32_t masks[ 3 ]{};
		std::memcpy( masks, base + 40, sizeof( masks ) );
		if ( bpp == 16 ) {
			masks[ 0 ] &= 0xFFFFu;
			masks[ 1 ] &= 0xFFFFu;
			masks[ 2 ] &= 0xFFFFu;
		}
		if ( ( masks[ 0 ] == 0 ) && ( masks[ 1 ] == 0 ) && ( masks[ 2 ] == 0 ) ) {
			err = "DIB bitfield masks are all zero";
			return false;
		}
		r_ext = make_bitfield_extractor( masks[ 0 ] );
		g_ext = make_bitfield_extractor( masks[ 1 ] );
		b_ext = make_bitfield_extractor( masks[ 2 ] );
	}

	const std::size_t clr_used   = static_cast<std::size_t>( clr_used64 );
	const std::size_t src_stride = static_cast<std::size_t>( src_stride64 );
	const auto* pal = ( clr_used > 0 )
		? reinterpret_cast<const RGBQUAD*>( base + static_cast<std::size_t>( pal_offset64 ) )
		: nullptr;
	const std::uint8_t* src_pixels = base + static_cast<std::size_t>( pixel_offset64 );

	bool is_pure_gray_palette = ( bpp <= 8 ) && ( clr_used > 0 );
	if ( is_pure_gray_palette ) {
		for ( std::size_t i = 0; i < clr_used; ++i ) {
			if ( ( pal[ i ].rgbRed != pal[ i ].rgbGreen ) || ( pal[ i ].rgbRed != pal[ i ].rgbBlue ) ) {
				is_pure_gray_palette = false;
				break;
			}
		}
	}

	const bool encode_gray = ( bpp <= 8 ) && is_pure_gray_palette;
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

		if ( bpp == 1 ) {
			for ( int x = 0; x < width; ++x ) {
				const std::size_t bit = ( srow[ x >> 3 ] >> ( 7 - ( x & 7 ) ) ) & 1u;
				const std::size_t idx = std::min<std::size_t>( bit, clr_used - 1 );
				if ( encode_gray ) {
					drow[ x ] = pal[ idx ].rgbRed;
				}
				else {
					drow[ x * 3 + 0 ] = pal[ idx ].rgbBlue;
					drow[ x * 3 + 1 ] = pal[ idx ].rgbGreen;
					drow[ x * 3 + 2 ] = pal[ idx ].rgbRed;
				}
			}
		}
		else if ( bpp == 4 ) {
			for ( int x = 0; x < width; ++x ) {
				const std::uint8_t byte_v = srow[ x >> 1 ];
				const std::size_t nib = ( ( x & 1 ) == 0 ) ? ( byte_v >> 4 ) : ( byte_v & 0x0Fu );
				const std::size_t idx = std::min<std::size_t>( nib, clr_used - 1 );
				if ( encode_gray ) {
					drow[ x ] = pal[ idx ].rgbRed;
				}
				else {
					drow[ x * 3 + 0 ] = pal[ idx ].rgbBlue;
					drow[ x * 3 + 1 ] = pal[ idx ].rgbGreen;
					drow[ x * 3 + 2 ] = pal[ idx ].rgbRed;
				}
			}
		}
		else if ( bpp == 8 ) {
			for ( int x = 0; x < width; ++x ) {
				const std::size_t idx = std::min<std::size_t>( srow[ x ], clr_used - 1 );
				if ( encode_gray ) {
					drow[ x ] = pal[ idx ].rgbRed;
				}
				else {
					drow[ x * 3 + 0 ] = pal[ idx ].rgbBlue;
					drow[ x * 3 + 1 ] = pal[ idx ].rgbGreen;
					drow[ x * 3 + 2 ] = pal[ idx ].rgbRed;
				}
			}
		}
		else if ( bpp == 16 ) {
			for ( int x = 0; x < width; ++x ) {
				std::uint16_t px16 = 0;
				std::memcpy( &px16, srow + static_cast<std::size_t>( x ) * 2u, sizeof( px16 ) );
				drow[ x * 3 + 0 ] = extract_channel_u8( px16, b_ext );
				drow[ x * 3 + 1 ] = extract_channel_u8( px16, g_ext );
				drow[ x * 3 + 2 ] = extract_channel_u8( px16, r_ext );
			}
		}
		else if ( bpp == 24 ) {
			std::memcpy( drow, srow, static_cast<std::size_t>( width ) * 3u );
		}
		else if ( valid_bitfields ) {
			for ( int x = 0; x < width; ++x ) {
				std::uint32_t px32 = 0;
				std::memcpy( &px32, srow + static_cast<std::size_t>( x ) * 4u, sizeof( px32 ) );
				drow[ x * 3 + 0 ] = extract_channel_u8( px32, b_ext );
				drow[ x * 3 + 1 ] = extract_channel_u8( px32, g_ext );
				drow[ x * 3 + 2 ] = extract_channel_u8( px32, r_ext );
			}
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
	lock.unlock();

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
	std::vector<std::uint8_t>().swap( top_down );

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

	if ( !packjpg::convert_memory( jpg_bytes.data(), jpg_bytes.size(), out_pjg, &err, false, 1 ) ) {
		if ( err.empty() ) err = "packJPG compression failed";
		return false;
	}

	return true;
}

} // namespace pjg_iv

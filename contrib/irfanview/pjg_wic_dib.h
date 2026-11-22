#ifndef PACKJPG_IRFANVIEW_PJG_WIC_DIB_H
#define PACKJPG_IRFANVIEW_PJG_WIC_DIB_H

#include <windows.h>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace pjg_iv {

struct ImageHeaderInfo {
	int width      = 0;
	int height     = 0;
	int bit_depth  = 24;
	int dpi_x      = 0;
	int dpi_y      = 0;
};

// Returns true if the leading bytes look like a valid packJPG v2.x stream ("JS" + valid header).
bool has_pjg_signature( const std::uint8_t* data, std::size_t size ) noexcept;

// Reads a file (optionally starting at `start_offset` bytes for Susie archive containers) from a UTF-16 Windows path.
bool read_file_bytes_w(
	const wchar_t* path,
	std::vector<std::uint8_t>& out_bytes,
	std::string& err,
	std::uint64_t start_offset = 0 );

// Writes an entire buffer atomically to a UTF-16 Windows path via a sibling temporary file + MoveFileExW.
bool write_file_bytes_w( const wchar_t* path, const std::uint8_t* data, std::size_t size, std::string& err );

// Converts UTF-8 / ASCII plugin error messages into wide or narrow caller buffers safely without writing past end.
void copy_error_w( wchar_t* dst, std::size_t dst_cch, const std::string& msg ) noexcept;
void copy_error_a( char* dst, std::size_t dst_cch, const std::string& msg ) noexcept;
std::wstring widen_acp( const char* str );
std::string narrow_acp_from_wide( const wchar_t* wstr );

// Parses reconstructed JPEG header markers (SOF0/1/2 + JFIF APP0) without full pixel decoding.
bool parse_jpeg_dimensions( const std::uint8_t* jpg_data, std::size_t jpg_size, ImageHeaderInfo& info ) noexcept;

// Decompresses a .pjg buffer in memory via packjpglib and extracts JPEG dimensions.
bool query_pjg_info_from_memory(
	const std::uint8_t* pjg_data,
	std::size_t pjg_size,
	ImageHeaderInfo& info,
	std::string& err );

// Decodes a .pjg memory buffer into a packed Windows DIB (moveable HGLOBAL: BITMAPINFOHEADER + palette + pixels)
// suitable for IrfanView's native ReadPJG_W / ReadPJG ABI.
HGLOBAL decode_pjg_memory_to_packed_dib(
	const std::uint8_t* pjg_data,
	std::size_t pjg_size,
	bool force_grayscale,
	std::string& err );

// Decodes a .pjg memory buffer into split Susie 00IN handles (BITMAPINFO + pixel bits).
int decode_pjg_memory_to_susie_dib(
	const std::uint8_t* pjg_data,
	std::size_t pjg_size,
	HANDLE* out_binfo,
	HANDLE* out_bits,
	std::string& err );

// Encodes a packed Windows DIB (1/4/8/16/24/32bpp BI_RGB or 16/32bpp BI_BITFIELDS) to JPEG via WIC
// at `quality` (1..100) and losslessly recompresses the JPEG into a .pjg byte buffer via packjpglib.
bool encode_packed_dib_to_pjg_memory(
	HANDLE h_dib,
	int quality,
	std::vector<std::uint8_t>& out_pjg,
	std::string& err );

} // namespace pjg_iv

#endif

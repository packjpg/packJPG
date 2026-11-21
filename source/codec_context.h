#ifndef CODEC_CONTEXT_H
#define CODEC_CONTEXT_H

#include "aricoder.h"
#include "bitops.h"
#include "dct8x8.h"
#include "pjpgtbl.h"

#include <array>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// Program & format metadata
inline constexpr unsigned char kAppVersion = 25;
inline constexpr const char* kSubVersion   = "k";
inline constexpr const char* kAppTitle     = "packJPG";
inline constexpr const char* kAppName      = "packjpg";
inline constexpr const char* kVersionDate  = "01/22/2016";
inline constexpr const char* kAuthor       = "Matthias Stirner / Se";
inline constexpr const char* kWebsite      = "http://packjpg.encode.ru/";
inline constexpr const char* kCopyright    = "2006-2016 HTW Aalen University & Matthias Stirner";
inline constexpr const char* kEmail        = "packjpg (at) matthiasstirner.com";
inline constexpr const char* kPjgExt       = "pjg";
inline constexpr const char* kJpgExt       = "jpg";
inline constexpr unsigned char kPjgMagic[ 2 ] = { 'J', 'S' };

inline constexpr const char* kMemErrMsg = "out of memory error";
inline constexpr const char* kFrdErrMsg = "could not read file / file not found: %s";
inline constexpr const char* kFwrErrMsg = "could not write file / file write-protected: %s";
inline constexpr std::size_t kMsgSize   = 128;
inline constexpr int kBarLen            = 36;

// Typed inline math & JPEG VLI helpers replacing preprocessor macros
inline std::unique_ptr<model_s> init_model_s( int max_s, int max_c, int max_o ) {
	return std::make_unique<model_s>( max_s, max_c, max_o, 255 );
}

inline std::unique_ptr<model_b> init_model_b( int max_c, int max_o ) {
	return std::make_unique<model_b>( max_c, max_o, 255 );
}

constexpr int envli( int s, int v ) noexcept {
	return ( v > 0 ) ? v : ( v - 1 ) + ( 1 << s );
}

constexpr int devli( int s, int n ) noexcept {
	return ( s == 0 ) ? 0 : ( ( n >= ( 1 << ( s - 1 ) ) ) ? n : ( n + 1 - ( 1 << s ) ) );
}

constexpr int e_envli( int s, int v ) noexcept {
	return v - ( 1 << s );
}

constexpr int e_devli( int s, int n ) noexcept {
	return n + ( 1 << s );
}

template <typename T>
constexpr int abs_val( T v ) noexcept {
	return ( v < 0 ) ? -static_cast<int>( v ) : static_cast<int>( v );
}

constexpr int b_short( std::uint8_t v1, std::uint8_t v2 ) noexcept {
	return ( static_cast<int>( v1 ) << 8 ) + static_cast<int>( v2 );
}

constexpr int bitlen_1024p( int v ) noexcept {
	return ( v <= 0 ) ? 0 : ( ( v > 1024 ) ? 11 : pbitlen_0_1024[ v ] );
}

constexpr int bitlen_2048n( int v ) noexcept {
	return ( v < -2048 ) ? 12 : ( ( v > 2047 ) ? 11 : pbitlen_n2048_2047[ v + 2048 ] );
}

constexpr int jpg_vli_bitlen( int v ) noexcept {
	return ( ( v >= -2048 ) && ( v <= 2047 ) )
		? pbitlen_n2048_2047[ v + 2048 ]
		: bit_length_u32( static_cast<unsigned int>( abs_val( v ) ) );
}

constexpr int clamped( int l, int h, int v ) noexcept {
	return ( v < l ) ? l : ( ( v > h ) ? h : v );
}

enum class StageId {
	None,
	CheckFile,
	ReadJpeg,
	MergeJpeg,
	DecodeJpeg,
	RecodeJpeg,
	AdaptIcos,
	PredictDc,
	UnpredictDc,
	CheckValueRange,
	CalcZdstLists,
	PackPjg,
	UnpackPjg,
	SwapStreams,
	CompareOutput,
	ResetBuffers,
	DumpHdr,
	DumpHuf,
	DumpColl,
	DumpZdst,
	DumpErrFile,
	DumpInfo,
	DumpDist,
	DumpPgm
};

constexpr const char* stage_status_text( StageId stage ) noexcept
{
	switch ( stage ) {
		case StageId::None:            return "unknown action";
		case StageId::CheckFile:       return "Determining filetype";
		case StageId::ReadJpeg:        return "Reading header & image data";
		case StageId::MergeJpeg:       return "Merging header & image data";
		case StageId::DecodeJpeg:      return "Decompressing JPEG image data";
		case StageId::RecodeJpeg:      return "Recompressing JPEG image data";
		case StageId::AdaptIcos:       return "Adapting DCT precalc. tables";
		case StageId::PredictDc:       return "Applying prediction to DC";
		case StageId::UnpredictDc:     return "Removing prediction from DC";
		case StageId::CheckValueRange: return "Checking values range";
		case StageId::CalcZdstLists:   return "Calculating zero dist lists";
		case StageId::PackPjg:         return "Compressing data to PJG";
		case StageId::UnpackPjg:       return "Uncompressing data from PJG";
		case StageId::SwapStreams:     return "Swapping input/output streams";
		case StageId::CompareOutput:   return "Verifying output stream";
		case StageId::ResetBuffers:    return "Resetting program";
		case StageId::DumpHdr:         return "Writing header data to file";
		case StageId::DumpHuf:         return "Writing huffman data to file";
		case StageId::DumpColl:        return "Writing collections to files";
		case StageId::DumpZdst:        return "Writing zdist lists to files";
		case StageId::DumpErrFile:     return "Writing error info to file";
		case StageId::DumpInfo:        return "Writing info to files";
		case StageId::DumpDist:        return "Writing distributions to files";
		case StageId::DumpPgm:         return "Writing converted image to pgm";
	}
	return "Function description missing!";
}

struct ComponentInfo {
	unsigned short* qtable = nullptr; // quantization table
	int huffdc = -1; // no of huffman table (DC)
	int huffac = -1; // no of huffman table (AC)
	int sfv    = -1; // sample factor vertical
	int sfh    = -1; // sample factor horizontal
	int mbs    = -1; // blocks in mcu
	int bcv    = -1; // block count vertical (interleaved)
	int bch    = -1; // block count horizontal (interleaved)
	int bc     = -1; // block count (all) (interleaved)
	int ncv    = -1; // block count vertical (non interleaved)
	int nch    = -1; // block count horizontal (non interleaved)
	int nc     = -1; // block count (all) (non interleaved)
	int sid    = -1; // statistical identity
	int jid    = -1; // jpeg internal id
};

struct HuffCodes {
	unsigned short cval[ 256 ]{};
	unsigned short clen[ 256 ]{};
	unsigned short max_eobrun = 0;
};

struct HuffTree {
	unsigned short l[ 512 ]{};
	unsigned short r[ 512 ]{};
};

/* -----------------------------------------------
	Self-contained, reentrant per-session codec state
	----------------------------------------------- */
struct CodecContext {
	// Quantization and Huffman tables
	std::array<std::array<unsigned short, 64>, 4> qtables{};
	HuffCodes     hcodes[ 2 ][ 4 ]{};
	HuffTree      htrees[ 2 ][ 4 ]{};
	unsigned char htset[ 2 ][ 4 ]{};

	// Header, scan, and trailing data buffers (RAII)
	std::vector<unsigned char> grbgdata;
	std::vector<unsigned char> hdrdata;
	std::vector<unsigned char> huffdata;

	std::vector<unsigned int>  rstp;
	std::vector<unsigned int>  scnp;
	std::vector<unsigned char> rst_err;
	bool has_rst_err = false;
	int  rstc        = 0;
	int  scnc        = 0;
	int  rsti        = 0;
	int  padbit      = -1;

	// Coefficient & zero-distribution storage (RAII)
	std::array<std::vector<unsigned char>, 4> zdstdata;
	std::array<std::vector<unsigned char>, 4> eobxhigh;
	std::array<std::vector<unsigned char>, 4> eobyhigh;
	std::array<std::vector<unsigned char>, 4> zdstxlow;
	std::array<std::vector<unsigned char>, 4> zdstylow;
	std::array<std::array<std::vector<short>, 64>, 4> colldata;

	std::array<const unsigned char*, 4> freqscan{ stdscan, stdscan, stdscan, stdscan };
	unsigned char zsrtscan[ 4 ][ 64 ]{};

	int adpt_idct_8x8[ 4 ][ 8 * 8 * 8 * 8 ]{};
	int adpt_idct_1x8[ 4 ][ 1 * 1 * 8 * 8 ]{};
	int adpt_idct_8x1[ 4 ][ 8 * 8 * 1 * 1 ]{};

	// Image & MCU geometry
	std::array<ComponentInfo, 4> cmpnfo{};
	int cmpc      = 0;
	int imgwidth  = 0;
	int imgheight = 0;
	int sfhm      = 0;
	int sfvm      = 0;
	int mcuv      = 0;
	int mcuh      = 0;
	int mcuc      = 0;

	// Current scan state
	int cs_cmpc = 0;
	std::array<int, 4> cs_cmp{};
	int cs_from = 0;
	int cs_to   = 0;
	int cs_sah  = 0;
	int cs_sal  = 0;

	// Stream & file info
	std::string jpgfilename;
	std::string pjgfilename;
	int jpgfilesize = 0;
	int pjgfilesize = 0;
	int jpegtype    = 0; // 0->unknown, 1->sequential, 2->progressive
	int filetype    = F_UNK;
	std::unique_ptr<Reader> str_in;
	std::unique_ptr<Writer> str_out;

	// Codec settings
	int  err_tol   = 1;
	bool disc_meta = false;
	bool auto_set  = true;
	int  action    = A_COMPRESS;
	std::array<unsigned char, 4> nois_trs{ 6, 6, 6, 6 };
	std::array<unsigned char, 4> segm_cnt{ 10, 10, 10, 10 };

#if defined(DEV_INFOS)
	int dev_size_hdr = 0;
	std::array<int, 4> dev_size_cmp{};
	std::array<int, 4> dev_size_zsr{};
	std::array<int, 4> dev_size_dc{};
	std::array<int, 4> dev_size_ach{};
	std::array<int, 4> dev_size_acl{};
	std::array<int, 4> dev_size_zdh{};
	std::array<int, 4> dev_size_zdl{};
#endif

	// Diagnostic state
	std::string errormessage = "no errormessage specified";
	StageId     error_stage  = StageId::None;
	int         errorlevel   = 0; // 0->ok, 1->warning, 2->fatal

	unsigned short quant( int cm, int bp ) const noexcept {
		return cmpnfo[ cm ].qtable[ bp ];
	}

	int max_v( int cm, int bp ) const noexcept {
		const unsigned short q = quant( cm, bp );
		return ( q > 0 ) ? ( ( freqmax[ bp ] + q - 1 ) / q ) : 0;
	}

#if defined(__GNUC__) || defined(__clang__)
	__attribute__((format(printf, 2, 3)))
#endif
	void set_errormessage( const char* fmt, ... ) {
		va_list args;
		va_start( args, fmt );
		va_list args_copy;
		va_copy( args_copy, args );
		const int needed = std::vsnprintf( nullptr, 0, fmt, args_copy );
		va_end( args_copy );
		if ( needed > 0 ) {
			std::vector<char> buf( static_cast<std::size_t>( needed ) + 1 );
			std::vsnprintf( buf.data(), buf.size(), fmt, args );
			errormessage.assign( buf.data(), static_cast<std::size_t>( needed ) );
		} else {
			errormessage.clear();
		}
		va_end( args );
	}

	bool read_header_segment( unsigned int hpos, unsigned char& type, unsigned int& len ) {
		const auto hdrs = hdrdata.size();
		if ( ( hpos > hdrs ) || ( hdrs - hpos < 4 ) ) {
			errormessage = "unexpected end of header data";
			errorlevel = 2;
			return false;
		}
		type = hdrdata[ hpos + 1 ];
		len = static_cast<unsigned int>( 2 + b_short( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] ) );
		if ( ( len < 4 ) || ( len > hdrs - hpos ) ) {
			errormessage = "unexpected end of header data";
			errorlevel = 2;
			return false;
		}
		return true;
	}

	bool reset_buffers() {
		hdrdata.clear();
		huffdata.clear();
		grbgdata.clear();
		rst_err.clear();
		has_rst_err = false;
		rstp.clear();
		scnp.clear();

		for ( int cmp = 0; cmp < 4; cmp++ ) {
			zdstdata[ cmp ].clear();
			eobxhigh[ cmp ].clear();
			eobyhigh[ cmp ].clear();
			zdstxlow[ cmp ].clear();
			zdstylow[ cmp ].clear();
			freqscan[ cmp ] = stdscan;
			for ( int bpos = 0; bpos < 64; bpos++ ) {
				colldata[ cmp ][ bpos ].clear();
			}
			cmpnfo[ cmp ] = ComponentInfo{};
		}

		imgwidth  = 0;
		imgheight = 0;
		cmpc      = 0;

		sfhm = 0;
		sfvm = 0;
		mcuc = 0;
		mcuh = 0;
		mcuv = 0;
		rsti = 0;
		rstc = 0;
		scnc = 0;

		for ( int i = 0; i < 4; i++ ) {
			htset[ 0 ][ i ] = 0;
			htset[ 1 ][ i ] = 0;
			qtables[ i ].fill( 0 );
		}

		jpegtype = 0;
		padbit   = -1;
		return true;
	}
};

#endif

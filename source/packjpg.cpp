/*
packJPG v2.5k (01/22/2016)
~~~~~~~~~~~~~~~~~~~~~~~~~~

packJPG is a compression program specially designed for further
compression of JPEG images without causing any further loss. Typically
it reduces the file size of a JPEG file by 20%.


LGPL v3 license and special permissions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

All programs in this package are free software; you can redistribute 
them and/or modify them under the terms of the GNU Lesser General Public 
License as published by the Free Software Foundation; either version 3 
of the License, or (at your option) any later version. 

The package is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranty of 
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser 
General Public License for more details at 
http://www.gnu.org/copyleft/lgpl.html. 

If the LGPL v3 license is not compatible with your software project you 
might contact us and ask for a special permission to use the packJPG 
library under different conditions. In any case, usage of the packJPG 
algorithm under the LGPL v3 or above is highly advised and special 
permissions will only be given where necessary on a case by case basis. 
This offer is aimed mainly at closed source freeware developers seeking 
to add PJG support to their software projects. 

Copyright 2006...2014 by HTW Aalen University and Matthias Stirner.


Usage of packJPG
~~~~~~~~~~~~~~~~

JPEG files are compressed and PJG files are decompressed using this
command:

 "packJPG [file(s)]"

packJPG recognizes file types on its own and decides whether to compress
(JPG) or decompress (PJG). For unrecognized file types no action is
taken. Files are recognized by content, not by extension.

packJPG supports wildcards like "*.*" and drag and drop of multiple 
files. Filenames for output files are created automatically. In default
mode, files are never overwritten. If a filename is already in use,
packJPG creates a new filename by adding underscores.

If "-" is used as a filename input from stdin is assumed and output is
written to stdout. This can be useful for example if jpegtran is to be
used as a preprocessor.

Usage examples:

 "packJPG *.pjg"
 "packJPG lena.jpg"
 "packJPG kodim??.jpg"
 "packJPG - < sail.pjg > sail.jpg"


Command line switches
~~~~~~~~~~~~~~~~~~~~~

 -ver  verify files after processing
 -v?   level of verbosity; 0,1 or 2 is allowed (default 0)
 -np   no pause after processing files
 -o    overwrite existing files
 -p    proceed on warnings
 -d    discard meta-info

By default, compression is cancelled on warnings. If warnings are 
skipped by using "-p", most files with warnings can also be compressed, 
but JPEG files reconstructed from PJG files might not be bitwise 
identical with the original JPEG files. There won't be any loss to 
image data or quality however.

Unnecessary meta information can be discarded using "-d". This reduces 
compressed files' sizes. Be warned though, reconstructed files won't be 
bitwise identical with the original files and meta information will be 
lost forever. As with "-p" there won't be any loss to image data or 
quality. 

There is no known case in which a file compressed by packJPG (without 
the "-p" option, see above) couldn't be reconstructed to exactly the 
state it was before. If you want an additional layer of safety you can 
also use the verify option "-ver". In this mode, files are compressed, 
then decompressed and the decompressed file compared to the original 
file. If this test doesn't pass there will be an error message and the 
compressed file won't be written to the drive. 

Please note that the "-ver" option should never be used in conjunction 
with the "-d" and/or "-p" options. As stated above, the "-p" and "-d" 
options will most likely lead to reconstructed JPG files not being 
bitwise identical to the original JPG files. In turn, the verification 
process may fail on various files although nothing actually went wrong. 

Usage examples:

 "packJPG -v1 -o baboon.pjg"
 "packJPG -ver lena.jpg"
 "packJPG -d tiffany.jpg"
 "packJPG -p *.jpg"


Known Limitations 
~~~~~~~~~~~~~~~~~ 

packJPG is a compression program specially for JPEG files, so it doesn't 
compress other file types.

packJPG has low error tolerance. JPEG files might not work with packJPG 
even if they work perfectly with other image processing software. The 
command line switch "-p" can be used to increase error tolerance and 
compatibility.

If you try to drag and drop to many files at once, there might be a 
windowed error message about missing privileges. In that case you can 
try it again with less files or consider using the command prompt. 
packJPG has been tested to work perfectly with thousands of files from 
the command line. This issue also happens with drag and drop in other 
applications, so it might not be a limitation of packJPG but a 
limitation of Windows.

Compressed PJG files are not compatible between different packJPG 
versions. You will get an error message if you try to decompress PJG 
files with a different version than the one used for compression. You 
may download older versions of packJPG from:
http://www.elektronik.htw-aalen.de/packJPG/binaries/old/


Open source release / developer info
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The packJPG source codes is found inside the "source" subdirectory. 
Additional documents aimed to developers, containing detailed 
instructions on compiling the source code and using special 
functionality, are included in the "packJPG" subdirectory. 
 

History
~~~~~~~

v1.9a (04/20/2007) (non public)
 - first released version
 - only for testing purposes

v2.0  (05/28/2007) (public)
 - first public version of packJPG
 - minor improvements to overall compression
 - minor bugfixes

v2.2  (08/05/2007) (public)
 - around 40% faster compression & decompression
 - major improvements to overall compression (around 2% on average)
 - reading from stdin, writing to stdout
 - smaller executable
 - minor bugfixes
 - various minor improvements
 
v2.3  (09/18/2007) (public)
 - compatibility with JPEG progressive mode
 - compatibility with JPEG extended sequential mode
 - compatibility with the CMYK color space
 - compatibility with older CPUs
 - around 15% faster compression & decompression 
 - new switch: [-d] (discard meta-info)
 - various bugfixes

v2.3a (11/21/2007) (public)
 - crash issue with certain images fixed
 - compatibility with packJPG v2.3 maintained

v2.3b (12/20/2007) (public)
 - some minor errors in the packJPG library fixed
 - compatibility with packJPG v2.3 maintained
 
v2.4 (03/24/2010) (public)
 - major improvements (1%...2%) to overall compression
 - around 10% faster compression & decompression
 - major improvements to JPG compatibility
 - size of executable reduced to ~33%
 - new switch: [-ver] (verify file after processing)
 - new switch: [-np] (no pause after processing)
 - new progress bar output mode
 - arithmetic coding routines rewritten from scratch
 - various smaller improvements to numerous to list here
 - new SFX (self extracting) archive format
 
v2.5 (11/11/2011) (public)
 - improvements (~0.5%) to overall compression
 - several minor bugfixes
 - major code cleanup
 - removed packJPX from the package
 - added packARC to the package
 - packJPG is now open source!
 
v2.5a (11/21/11) (public)
 - source code compatibility improvements (Gerhard Seelmann)
 - avoid some compiler warnings (Gerhard Seelmann)
 - source code clean up (Gerhard Seelmann)
 
v2.5b (01/27/12) (public)
 - further removal of redundant code
 - some fixes for the packJPG static library
 - compiler fix for Mac OS (thanks to Sergio Lopez)
 - improved compression ratio calculation
 - eliminated the need for temp files
 
v2.5c (04/13/12) (public)
 - various source code optimizations
 
v2.5d (07/03/12) (public)
 - fixed a rare bug with progressive JPEG
 
v2.5e (07/03/12) (public)
 - some minor source code optimizations
 - changed packJPG licensing to LGPL
 - moved packARC to a separate package
 
v2.5f (02/24/13) (public)
 - fixed a minor bug in the JPG parser (thanks to Stephan Busch)
 
v2.5g (09/14/13) (public)
 - fixed a rare crash bug with manipulated JPEG files
 
v2.5h (12/07/13) (public)
 - added a warning for inefficient huffman coding (thanks to Moinak Ghosh)
 
v2.5i (12/26/13) (public)
 - fixed possible crash with malformed JPEG (thanks to Moinak Ghosh)
 
v2.5j (01/15/14) (public)
 - various source code optimizations (using cppcheck)

v2.5k (01/22/16) (public)
 - Updated contact info
 - fixed a minor bug


Acknowledgements
~~~~~~~~~~~~~~~~

packJPG is the result of countless hours of research and development. It
is part of my final year project for Hochschule Aalen.

Prof. Dr. Gerhard Seelmann from Hochschule Aalen supported my
development of packJPG with his extensive knowledge in the field of data
compression. Without his advice, packJPG would not be possible.

The official developer blog for packJPG is hosted by encode.ru.

packJPG logo and icon are designed by Michael Kaufmann.


Contact
~~~~~~~

The official developer blog for packJPG:
 http://packjpg.encode.ru/
 
For questions and bug reports:
 packjpg (at) matthiasstirner.com


____________________________________
packJPG by Matthias Stirner, 01/2016
*/

#include <array>
#include <chrono>
#include <memory>
#include <numeric>
#include <tuple>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "bitops.h"
#include "aricoder.h"
#include "pjpgtbl.h"
#include "dct8x8.h"

#if defined BUILD_DLL // define BUILD_LIB from the compiler options if you want to compile a DLL!
	#define BUILD_LIB
#endif

#if defined BUILD_LIB // define BUILD_LIB as compiler option if you want to compile a library!
	#include "packjpglib.h"
#endif

#define INTERN static

#define INIT_MODEL_S(a,b,c) new model_s( a, b, c, 255 )
#define INIT_MODEL_B(a,b)   new model_b( a, b, 255 )

// #define USE_PLOCOI // uncomment to use loco-i predictor instead of 1DDCT predictor
// #define DEV_BUILD // uncomment to include developer functions
// #define DEV_INFOS // uncomment to include developer information

int bitlen(int v) {
	int length = 0;
	while ((v >> length) != 0) {
		length++;
	}
	return length;
}

constexpr std::int16_t fdiv2(std::int16_t v, int p) {
	return (v < 0) ? -((-v) >> p) : (v >> p);
}

constexpr int envli(int s, int v) {
	return (v > 0) ? v : v - 1 + (1 << s);
}

constexpr int devli(int s, int n) {
	return (n >= 1 << (s - 1)) ? n : n + 1 - (1 << s);
}

constexpr int e_envli(int s, int v) {
	return v - (1 << s);
}

constexpr int e_devli(int s, int n) {
	return n + (1 << s);
}

constexpr int pack(std::uint8_t left, std::uint8_t right) {
	return (int(left) << 8) + int(right);
}

constexpr int bitlen1024p(int v) {
	return pbitlen_0_1024[v];
}

constexpr int bitlen2048n(int v) {
	return pbitlen_n2048_2047[v + 2048];
}

constexpr int clamp(int val, int lo, int hi) {
	return (val < lo) ? lo : (val > hi ? hi : val);
}

const std::string MEM_ERRMSG("out of memory error");
const std::string FRD_ERRMSG("could not read file / file not found: %s");
const std::string FWR_ERRMSG("could not write file / file write-protected: %s");

/* -----------------------------------------------
Enums for use in packJPG processing
----------------------------------------------- */

enum Action {
	A_COMPRESS = 1,
	A_SPLIT_DUMP = 2,
	A_COLL_DUMP = 3,
	A_FCOLL_DUMP = 4,
	A_ZDST_DUMP = 5,
	A_TXT_INFO = 6,
	A_DIST_INFO = 7,
	A_PGM_DUMP = 8
};

enum FileType {
	F_JPG = 1,
	F_PJG = 2,
	F_UNK = 3
};

/* -----------------------------------------------
	struct declarations
	----------------------------------------------- */

struct componentInfo {
	unsigned short* qtable; // quantization table
	int huffdc; // no of huffman table (DC)
	int huffac; // no of huffman table (AC)
	int sfv; // sample factor vertical
	int sfh; // sample factor horizontal	
	int mbs; // blocks in mcu
	int bcv; // block count vertical (interleaved)
	int bch; // block count horizontal (interleaved)
	int bc;  // block count (all) (interleaved)
	int ncv; // block count vertical (non interleaved)
	int nch; // block count horizontal (non interleaved)
	int nc;  // block count (all) (non interleaved)
	int sid; // statistical identity
	int jid; // jpeg internal id
};

struct HuffCodes {
	std::array<std::uint16_t, 256> cval = { 0 };
	std::array<std::uint16_t, 256> clen = { 0 };
	std::uint16_t max_eobrun = 0;
};

struct HuffTree {
	std::array<std::uint16_t, 256> l = { 0 };
	std::array<std::uint16_t, 256> r = { 0 };
};

enum JpegType {
	UNKNOWN = 0,
	SEQUENTIAL = 1,
	PROGRESSIVE = 2
};


/* -----------------------------------------------
	function declarations: main interface
	----------------------------------------------- */
#if !defined( BUILD_LIB )
INTERN void initialize_options( int argc, char** argv );
INTERN void process_ui();
static std::string get_status( bool (*function)() );
INTERN void show_help();
#endif
INTERN void process_file();
INTERN void execute( bool (*function)() );


/* -----------------------------------------------
	function declarations: main functions
	----------------------------------------------- */
#if !defined( BUILD_LIB )
INTERN bool check_file();
INTERN bool swap_streams();
INTERN bool compare_output();
#endif
INTERN bool reset_buffers();
INTERN bool predict_dc();
INTERN bool unpredict_dc();
INTERN bool calc_zdst_lists();

namespace jpg {

enum CodingStatus {
	OKAY = 0,
	RESTART = 1,
	ERROR = -1,
	DONE = 2
};

HuffCodes hcodes[2][4]; // huffman codes
HuffTree htrees[2][4]; // huffman decoding trees
bool htset[2][4]; // Indicates whether the corresponding huffman table has been created.

char padbit = -1; // padbit (for huffman coding)
int scan_count = 0; // count of scans
int rsti = 0; // restart interval

std::vector<std::uint32_t> scnp; // scan start positions in huffdata
std::vector<std::uint32_t> rstp; // restart markers positions in huffdata
std::vector<std::uint8_t> rst_err; // number of wrong-set RST markers per scan

// Parses header for imageinfo.
bool setup_imginfo();
// JFIF header rebuilding routine.
bool rebuild_header();

// Calculates next position for MCU.
jpg::CodingStatus next_mcupos(int* mcu, int* cmp, int* csc, int* sub, int* dpos, int* rstw);
// Calculates next position (non interleaved).
jpg::CodingStatus next_mcuposn(int cmpt, int* dpos, int* rstw);

namespace jfif {

// Parses JFIF segment, returning true if the segment is valid in packjpg and the parse was successful, false otherwise.
bool parse_jfif(unsigned char type, unsigned int len, const unsigned char* segment);

// Helper function that parses DHT segments, returning true if the parse succeeds.
bool parse_dht(unsigned char type, unsigned int len, const unsigned char* segment);
// Constructs Huffman codes from DHT data.
HuffCodes build_huffcodes(const unsigned char* clen, const unsigned char* cval);
// Constructs a Huffman tree from the given Huffman codes.
HuffTree build_hufftree(const HuffCodes& codes);

// Helper function that parses DQT segments, returning true if the parse succeeds.
bool parse_dqt(unsigned len, const unsigned char* segment);
// Helper function that parses SOS segments, returning true if the parse succeeds.
bool parse_sos(const unsigned char* segment);
// Helper function that parses SOF0/SOF1/SOF2 segments, returning true if the parse succeeds.
bool parse_sof(unsigned char type, const unsigned char* segment);
// Helper function that parses DRI segments.
void parse_dri(const unsigned char* segment);
}

namespace encode {
// JPEG encoding routine.
bool recode();
// Merges header & image data to jpeg.
bool merge();

// Sequential block encoding routine.
int block_seq(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& dctbl, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block);
// Progressive DC encoding routine.
void dc_prg_fs(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& dctbl, const std::array<std::int16_t, 64>& block);
// Progressive AC encoding routine.
int ac_prg_fs(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block,
              int* eobrun, int from, int to);
// Progressive DC SA encoding routine.
void dc_prg_sa(const std::unique_ptr<abitwriter>& huffw, const std::array<std::int16_t, 64>& block);
// Progressive AC SA encoding routine.
int ac_prg_sa(const std::unique_ptr<abitwriter>& huffw, const std::unique_ptr<abytewriter>& storw, const HuffCodes& actbl,
              const std::array<std::int16_t, 64>& block, int* eobrun, int from, int to);
// Run of EOB encoding routine.
void eobrun(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& actbl, int* eobrun);
// Correction bits encoding routine.
void crbits(const std::unique_ptr<abitwriter>& huffw, const std::unique_ptr<abytewriter>& storw);
}

namespace decode {
// Read in header and image data.
bool read();
// JPEG decoding routine.
bool decode();
// Checks range of values, error if out of bounds.
bool check_value_range();

// Sequential block decoding routine.
int block_seq(const std::unique_ptr<abitreader>& huffr, const HuffTree& dctree, const HuffTree& actree, short* block);
// Progressive DC decoding routine.
jpg::CodingStatus dc_prg_fs(const std::unique_ptr<abitreader>& huffr, const HuffTree& dctree, short* block);
// Progressive AC decoding routine.
int ac_prg_fs(const std::unique_ptr<abitreader>& huffr, const HuffTree& actree, short* block,
              int* eobrun, int from, int to);
// Progressive DC SA decoding routine.
void dc_prg_sa(const std::unique_ptr<abitreader>& huffr, short* block);
// Progressive AC SA decoding routine.
int ac_prg_sa(const std::unique_ptr<abitreader>& huffr, const HuffTree& actree, short* block,
              int* eobrun, int from, int to);
// Run of EOB SA decoding routine.
void eobrun_sa(const std::unique_ptr<abitreader>& huffr, short* block, int* eobrun, int from, int to);

// Skips the eobrun, calculates next position.
jpg::CodingStatus skip_eobrun(int cmpt, int* dpos, int* rstw, int* eobrun);
// Returns next the next code(from huffman tree and data).
int next_huffcode(const std::unique_ptr<abitreader>& huffr, const HuffTree& ctree);
}
}

/* -----------------------------------------------
	function declarations: pjg-specific
	----------------------------------------------- */

namespace pjg {
	unsigned char* zdstdata[4] = { nullptr }; // zero distribution (# of non-zeroes) lists (for higher 7x7 block)
	unsigned char* eobxhigh[4] = { nullptr }; // eob in x direction (for higher 7x7 block)
	unsigned char* eobyhigh[4] = { nullptr }; // eob in y direction (for higher 7x7 block)
	unsigned char* zdstxlow[4] = { nullptr }; // # of non zeroes for first row
	unsigned char* zdstylow[4] = { nullptr }; // # of non zeroes for first column

	unsigned char* freqscan[4] = { nullptr }; // optimized order for frequency scans (only pointers to scans)
	unsigned char  zsrtscan[4][64];	// zero optimized frequency scan

	namespace encode {
		bool encode();

		// Optimizes DHT segments for compression.
		void optimize_dht(int hpos, const int len);
		// Optimizes DQT segments for compression.
		void optimize_dqt(int hpos, const int len);
		// Optimizes JFIF header for compression.
		void optimize_header();

		void zstscan(const std::unique_ptr<aricoder>& enc, int cmp);
		void zdst_high(const std::unique_ptr<aricoder>& enc, int cmp);
		void zdst_low(const std::unique_ptr<aricoder>& enc, int cmp);
		void dc(const std::unique_ptr<aricoder>& enc, int cmp);
		void ac_high(const std::unique_ptr<aricoder>& enc, int cmp);
		void ac_low(const std::unique_ptr<aricoder>& enc, int cmp);
		bool generic(const std::unique_ptr<aricoder>& enc, unsigned char* data, int len);
		void bit(const std::unique_ptr<aricoder>& enc, unsigned char bit);


		// Get zero sort frequency scan vector.
		void get_zerosort_scan(unsigned char* sv, int cmp);

	}

	namespace decode {
		bool decode();

		// Undoes DHT segment optimizations.
		void deoptimize_dht(int hpos, int segment_length);
		// Undoes DQT segment optimizations.
		void deoptimize_dqt(int hpos, int segment_length);
		// Undoes DHT and DQT (header) optimizations.
		void deoptimize_header();

	void zstscan(const std::unique_ptr<aricoder>& dec, int cmp);
	void zdst_high(const std::unique_ptr<aricoder>& dec, int cmp);
	void zdst_low(const std::unique_ptr<aricoder>& dec, int cmp);
	void dc(const std::unique_ptr<aricoder>& dec, int cmp);
	void ac_high(const std::unique_ptr<aricoder>& dec, int cmp);
	void ac_low(const std::unique_ptr<aricoder>& dec, int cmp);
	bool generic(const std::unique_ptr<aricoder>& dec, unsigned char** data, int* len);
	std::vector<std::uint8_t> generic(const std::unique_ptr<aricoder>& dec);
	std::uint8_t bit(const std::unique_ptr<aricoder>& dec);
	}

	void aavrg_prepare(unsigned short** abs_coeffs, int* weights, unsigned short* abs_store, int cmp);
	int aavrg_context(unsigned short** abs_coeffs, int* weights, int pos, int p_y, int p_x, int r_x);
	int lakh_context(signed short** coeffs_x, signed short** coeffs_a, int* pred_cf, int pos);
std::pair<int, int> get_context_nnb(int pos, int w);
}

/*
* Discrete cosine transform (DCT) and Inverse discrete cosine transform (IDCT) functions and data.
*
*/
namespace dct {
	short* colldata[4][64] = { { nullptr } }; // Collection sorted DCT coefficients.

	int adpt_idct_8x8[4][8 * 8 * 8 * 8]; // precalculated/adapted values for idct (8x8)
	int adpt_idct_1x8[4][1 * 1 * 8 * 8]; // precalculated/adapted values for idct (1x8)
	int adpt_idct_8x1[4][8 * 8 * 1 * 1]; // precalculated/adapted values for idct (8x1)

	bool adapt_icos();

#if !defined(BUILD_LIB) && defined(DEV_BUILD)
	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_8x8(int cmp, int dpos, int ix, int iy);
#endif
	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_1x8(int cmp, int dpos, int ix, int iy);
	// Inverse DCT transform using precalc tables (fast).
	int idct_2d_fst_8x1(int cmp, int dpos, int ix, int iy);
}

namespace predictor {
#if defined( USE_PLOCOI )
	// Returns predictor for collection data.
	int dc_coll_predictor(int cmp, int dpos);
	// loco-i predictor.
	int plocoi(int a, int b, int c);
#else
	// 1D DCT predictor for DC coefficients.
	int dc_1ddct_predictor(int cmp, int dpos);
#endif
}


/* -----------------------------------------------
	function declarations: miscelaneous helpers
	----------------------------------------------- */
#if !defined(BUILD_LIB)
static void progress_bar(int current, int last);
static std::string create_filename(const std::string& oldname, const std::string& new_extension);
static std::string unique_filename(const std::string& oldname, const std::string& new_extension);
#endif
static bool file_exists(const std::string& filename);


/* -----------------------------------------------
	function declarations: developers functions
	----------------------------------------------- */

// these are developers functions, they are not needed
// in any way to compress jpg or decompress pjg
#if !defined(BUILD_LIB) && defined(DEV_BUILD)
enum CollectionMode {
	STD = 0, // standard collections
	DHF = 1, // sequential order collections, 'dhufs'
	SQU = 2, // square collections
	UNC = 3, // uncollections
	SQU_ALT = 4, // square collections / alt order (even/uneven)
	UNC_ALT = 5 // uncollections / alt order (even/uneven)
};

static CollectionMode coll_mode = CollectionMode::STD; // Write mode for collections.

INTERN bool dump_hdr();
INTERN bool dump_huf();
INTERN bool dump_coll();
INTERN bool dump_zdst();
INTERN bool dump_file( const std::string& base, const std::string& ext, void* data, int bpv, int size );
INTERN bool dump_errfile();
INTERN bool dump_info();
INTERN bool dump_dist();
INTERN bool dump_pgm();
#endif


/* -----------------------------------------------
	global variables: library only variables
	----------------------------------------------- */
#if defined(BUILD_LIB)
INTERN int lib_in_type  = -1;
INTERN int lib_out_type = -1;
#endif


/* -----------------------------------------------
	global variables: data storage
	----------------------------------------------- */

INTERN unsigned short qtables[4][64];				// quantization tables

static std::vector<std::uint8_t> grbgdata; // garbage data
INTERN unsigned char* hdrdata          =   nullptr;   // header data
static std::vector<std::uint8_t> huffdata; // huffman coded data
INTERN int            hdrs             =    0  ;   // size of header



/* -----------------------------------------------
	global variables: info about image
	----------------------------------------------- */

// separate info for each color component
INTERN componentInfo cmpnfo[ 4 ];

constexpr int QUANT(int cm, int bp) {
	return cmpnfo[cm].qtable[bp];
}

constexpr int MAX_V(int cm, int bp) {
	return (QUANT(cm, bp) > 0) ? (freqmax[bp] + QUANT(cm, bp) - 1) / QUANT(cm, bp) : 0;
}
namespace image {
int cmpc = 0; // component count
int imgwidth = 0; // width of image
int imgheight = 0; // height of image

int sfhm = 0; // max horizontal sample factor
int sfvm = 0; // max verical sample factor
int mcuv = 0; // mcus per line
int mcuh = 0; // mcus per collumn
int mcuc = 0; // count of mcus
}


/* -----------------------------------------------
	global variables: info about current scan
	----------------------------------------------- */

namespace curr_scan {
int cmpc = 0; // component count in current scan
std::array<int, 4> cmp = {0}; // component numbers  in current scan
int from = 0; // begin - band of current scan ( inclusive )
int to = 0; // end - band of current scan ( inclusive )
int sah = 0; // successive approximation bit pos high
int sal = 0; // successive approximation bit pos low
}
	

/* -----------------------------------------------
	global variables: info about files
	----------------------------------------------- */
	
INTERN std::string jpgfilename;	// name of JPEG file
INTERN std::string pjgfilename;	// name of PJG file
INTERN int    jpgfilesize;			// size of JPEG file
INTERN int    pjgfilesize;			// size of PJG file
static JpegType jpegtype = JpegType::UNKNOWN; // type of JPEG coding
INTERN FileType filetype;				// type of current file
INTERN iostream* str_in  = nullptr;	// input stream
INTERN iostream* str_out = nullptr;	// output stream

#if !defined(BUILD_LIB)
INTERN iostream* str_str = nullptr;	// storage stream

static std::vector<std::string> filelist; // list of files to process 
INTERN int    file_no  = 0;			// number of current file

INTERN std::vector<std::string> err_list; // list of error messages 
INTERN std::vector<int> err_tp; // list of error types
#endif

#if defined(DEV_INFOS)
INTERN int    dev_size_hdr      = 0;
INTERN int    dev_size_cmp[ 4 ] = { 0 };
INTERN int    dev_size_zsr[ 4 ] = { 0 };
INTERN int    dev_size_dc[ 4 ]  = { 0 };
INTERN int    dev_size_ach[ 4 ] = { 0 };
INTERN int    dev_size_acl[ 4 ] = { 0 };
INTERN int    dev_size_zdh[ 4 ] = { 0 };
INTERN int    dev_size_zdl[ 4 ] = { 0 };
#endif


/* -----------------------------------------------
	global variables: messages
	----------------------------------------------- */

INTERN char errormessage [ 128 ];
INTERN bool (*errorfunction)();
INTERN int  errorlevel;
// meaning of errorlevel:
// -1 -> wrong input
// 0 -> no error
// 1 -> warning
// 2 -> fatal error


/* -----------------------------------------------
	global variables: settings
	----------------------------------------------- */

#if !defined(BUILD_LIB)
INTERN int  verbosity  = -1;	// level of verbosity
INTERN bool overwrite  = false;	// overwrite files yes / no
INTERN bool wait_exit  = true;	// pause after finished yes / no
INTERN int  verify_lv  = 0;		// verification level ( none (0), simple (1), detailed output (2) )
INTERN int  err_tol    = 1;		// error threshold ( proceed on warnings yes (2) / no (1) )
INTERN bool disc_meta  = false;	// discard meta-info yes / no

INTERN bool developer  = false;	// allow developers functions yes/no
INTERN bool auto_set   = true;	// automatic find best settings yes/no
INTERN Action action = Action::A_COMPRESS;// what to do with JPEG/PJG files

INTERN FILE*  msgout   = stdout;// stream for output of messages
INTERN bool   pipe_on  = false;	// use stdin/stdout instead of filelist
#else
INTERN int  err_tol    = 1;		// error threshold ( proceed on warnings yes (2) / no (1) )
INTERN bool disc_meta  = false;	// discard meta-info yes / no
INTERN bool auto_set   = true;	// automatic find best settings yes/no
INTERN Action action = Action::A_COMPRESS;// what to do with JPEG/PJG files
#endif

INTERN unsigned char nois_trs[ 4 ] = {6,6,6,6}; // bit pattern noise threshold
INTERN unsigned char segm_cnt[ 4 ] = {10,10,10,10}; // number of segments
#if !defined(BUILD_LIB)
INTERN unsigned char orig_set[ 8 ] = { 0 }; // store array for settings
#endif

namespace program_info {
	const unsigned char appversion = 25;
	const std::string subversion = "k";
	const std::string apptitle = "packJPG";
	const std::string appname = "packjpg";
	const std::string versiondate = "01/22/2016";
	const std::string author = "Matthias Stirner / Se";
#if !defined(BUILD_LIB)
	const std::string website = "http://packjpg.encode.ru/";
	const std::string copyright = "2006-2016 HTW Aalen University & Matthias Stirner";
	const std::string email = "packjpg (at) matthiasstirner.com";

	const std::string pjg_ext = "pjg";
	const std::string jpg_ext = "jpg";
	const std::array<std::uint8_t, 2> pjg_magic = { 'J', 'S' };
#endif
}


/* -----------------------------------------------
	main-function
	----------------------------------------------- */

#if !defined(BUILD_LIB)
int main( int argc, char** argv )
{	
	sprintf( errormessage, "no errormessage specified" );
		
	int error_cnt = 0;
	int warn_cnt  = 0;
	
	double acc_jpgsize = 0;
	double acc_pjgsize = 0;
		
	errorlevel = 0;
	
	
	// read options from command line
	initialize_options( argc, argv );
	
	// write program info to screen
	fprintf( msgout,  "\n--> %s v%i.%i%s (%s) by %s <--\n",
	         program_info::apptitle.c_str(), program_info::appversion / 10, program_info::appversion % 10, program_info::subversion.c_str(), program_info::versiondate.c_str(), program_info::author.c_str());
	fprintf( msgout, "Copyright %s\nAll rights reserved\n\n", program_info::copyright.c_str() );
	
	// check if user input is wrong, show help screen if it is
	if (filelist.empty() ||
		( ( !developer ) && ( (action != Action::A_COMPRESS) || (!auto_set) || (verify_lv > 1) ) ) ) {
		show_help();
		return -1;
	}
	
	// display warning if not using automatic settings
	if ( !auto_set ) {
		fprintf( msgout,  " custom compression settings: \n" );
		fprintf( msgout,  " -------------------------------------------------\n" );
		fprintf( msgout,  " no of segments    ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
				segm_cnt[0], segm_cnt[1], segm_cnt[2], segm_cnt[3] );
		fprintf( msgout,  " noise threshold   ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
				nois_trs[0], nois_trs[1], nois_trs[2], nois_trs[3] );
		fprintf( msgout,  " -------------------------------------------------\n\n" );
	}
	
	// (re)set program has to be done first
	reset_buffers();
	
	// process file(s) - this is the main function routine

	auto begin = std::chrono::steady_clock::now();
	for ( file_no = 0; file_no < filelist.size(); file_no++ ) {
		// process current file
		process_ui();
		// store error message and type if any
		if ( errorlevel > 0 ) {
			err_tp[file_no] = errorlevel;
			err_list[file_no] = errormessage;
		}
		// count errors / warnings / file sizes
		if ( errorlevel >= err_tol ) error_cnt++;
		else {
			if ( errorlevel == 1 ) warn_cnt++;
			acc_jpgsize += jpgfilesize;
			acc_pjgsize += pjgfilesize;
		}
	}
	auto end = std::chrono::steady_clock::now();
	
	// errors summary: only needed for -v2 or progress bar
	if ( ( verbosity == -1 ) || ( verbosity == 2 ) ) {
		// print summary of errors to screen
		if ( error_cnt > 0 ) {
			fprintf( stderr, "\n\nfiles with errors:\n" );
			fprintf( stderr, "------------------\n" );
			for ( file_no = 0; file_no < filelist.size(); file_no++ ) {
				if ( err_tp[ file_no ] >= err_tol ) {
					fprintf( stderr, "%s (%s)\n", filelist[ file_no ].c_str(), err_list[ file_no ].c_str());
				}
			}
		}
		// print summary of warnings to screen
		if ( warn_cnt > 0 ) {
			fprintf( stderr, "\n\nfiles with warnings:\n" );
			fprintf( stderr, "------------------\n" );
			for ( file_no = 0; file_no < filelist.size(); file_no++ ) {
				if ( err_tp[ file_no ] == 1 ) {
					fprintf( stderr, "%s (%s)\n", filelist[ file_no ].c_str(), err_list[ file_no ].c_str());
				}
			}
		}
	}
	
	// show statistics
	fprintf( msgout,  "\n\n-> %i file(s) processed, %i error(s), %i warning(s)\n",
		filelist.size(), error_cnt, warn_cnt );
	if ( (filelist.size() > error_cnt ) && ( verbosity != 0 ) &&
	 ( action == Action::A_COMPRESS ) ) {
		acc_jpgsize /= 1024.0;
		acc_pjgsize /= 1024.0;
		std::chrono::duration<double> duration = end - begin;
		double total = duration.count();
		
		fprintf( msgout,  " --------------------------------- \n" );
		if ( total > 0 ) {
			fprintf( msgout,  " total time       : %8.2f sec\n", total );
			int kbps = acc_jpgsize / total;
			fprintf( msgout,  " avg. kbyte per s : %8i byte\n", kbps );
		} else {
			fprintf( msgout,  " total time       : %8s sec\n", "N/A" );
			fprintf( msgout,  " avg. kbyte per s : %8s byte\n", "N/A" );
		}
		double cr = (acc_jpgsize > 0) ? (100.0 * acc_pjgsize / acc_jpgsize) : 0;
		fprintf( msgout,  " avg. comp. ratio  : %8.2f %%\n", cr );		
		fprintf( msgout,  " --------------------------------- \n" );
		#if defined(DEV_INFOS)
		if ( acc_jpgsize > 0 ) { 
			fprintf( msgout,  " header %%          : %8.2f %%\n", 100.0 * dev_size_hdr / acc_jpgsize );
			if ( dev_size_cmp[0] > 0 ) fprintf( msgout,  " component [0] %%   : %8.2f %%\n", 100.0 * dev_size_cmp[0] / acc_jpgsize );
			if ( dev_size_cmp[1] > 0 ) fprintf( msgout,  " component [1] %%   : %8.2f %%\n", 100.0 * dev_size_cmp[1] / acc_jpgsize );
			if ( dev_size_cmp[2] > 0 ) fprintf( msgout,  " component [2] %%   : %8.2f %%\n", 100.0 * dev_size_cmp[2] / acc_jpgsize );
			if ( dev_size_cmp[3] > 0 ) fprintf( msgout,  " component [3] %%   : %8.2f %%\n", 100.0 * dev_size_cmp[3] / acc_jpgsize );
			fprintf( msgout,  " --------------------------------- \n" );
			for ( int i = 0; i < 4; i++ ) {
				if ( dev_size_cmp[i] == 0 ) break;
				fprintf( msgout,  " ac coeffs h [%i] %% : %8.2f %%\n", i, 100.0 * dev_size_ach[i] / acc_jpgsize );
				fprintf( msgout,  " ac coeffs l [%i] %% : %8.2f %%\n", i, 100.0 * dev_size_acl[i] / acc_jpgsize );
				fprintf( msgout,  " dc coeffs   [%i] %% : %8.2f %%\n", i, 100.0 * dev_size_dc[i] / acc_jpgsize );
				fprintf( msgout,  " zero dist h [%i] %% : %8.2f %%\n", i, 100.0 * dev_size_zdh[i] / acc_jpgsize );
				fprintf( msgout,  " zero dist l [%i] %% : %8.2f %%\n", i, 100.0 * dev_size_zdl[i] / acc_jpgsize );
				fprintf( msgout,  " zero sort   [%i] %% : %8.2f %%\n", i, 100.0 * dev_size_zsr[i] / acc_jpgsize );
				fprintf( msgout,  " --------------------------------- \n" );
			}
		}
		#endif
	}
	
	// pause before exit
	if ( wait_exit && ( msgout != stderr ) ) {
		fprintf( msgout, "\n\n< press ENTER >\n" );
		fgetc( stdin );
	}
	
	
	return 0;
}
#endif

/* ----------------------- Begin of library only functions -------------------------- */

/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
	
#if defined(BUILD_LIB)
EXPORT bool pjglib_convert_stream2stream( char* msg )
{
	// process in main function
	return pjglib_convert_stream2mem( nullptr, nullptr, msg ); 
}

/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
EXPORT bool pjglib_convert_file2file( char* in, char* out, char* msg )
{
	// init streams
	pjglib_init_streams( (void*) in, 0, 0, (void*) out, 0 );
	
	// process in main function
	return pjglib_convert_stream2mem( nullptr, nullptr, msg ); 
}

/* -----------------------------------------------
	DLL export converter function
	----------------------------------------------- */
EXPORT bool pjglib_convert_stream2mem( unsigned char** out_file, unsigned int* out_size, char* msg )
{
	// use automatic settings
	auto_set = true;
	
	// (re)set buffers
	reset_buffers();
	action = Action::A_COMPRESS;
	
	// main compression / decompression routines
	auto begin = std::chrono::steady_clock::now();
	
	// process one file
	process_file();
	
	// fetch pointer and size of output (only for memory output)
	if ( ( errorlevel < err_tol ) && ( lib_out_type == 1 ) &&
		 ( out_file != nullptr ) && ( out_size != nullptr ) ) {
		*out_size = str_out->getsize();
		*out_file = str_out->getptr();
	}
	
	// close iostreams
	if ( str_in  != nullptr ) delete( str_in  ); str_in  = nullptr;
	if ( str_out != nullptr ) delete( str_out ); str_out = nullptr;
	
	auto end = std::chrono::steady_clock::now();
	
	// copy errormessage / remove files if error (and output is file)
	if ( errorlevel >= err_tol ) {
		if ( lib_out_type == 0 ) {
			if ( filetype == FileType::F_JPG ) {
				if ( file_exists( pjgfilename ) ) remove( pjgfilename.c_str());
			} else if ( filetype == FileType::F_PJG ) {
				if ( file_exists( jpgfilename ) ) remove( jpgfilename.c_str());
			}
		}
		if ( msg != nullptr ) strcpy( msg, errormessage );
		return false;
	}
	
	// get compression info
	auto duration = end - begin;
	auto total = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
	float cr = ( jpgfilesize > 0 ) ? ( 100.0 * pjgfilesize / jpgfilesize ) : 0;
	
	// write success message else
	if ( msg != nullptr ) {
		switch( filetype )
		{
			case FileType::F_JPG:
				sprintf( msg, "Compressed to %s (%.2f%%) in %ims",
					pjgfilename.c_str(), cr, ( total >= 0 ) ? total : -1 );
				break;
			case FileType::F_PJG:
				sprintf( msg, "Decompressed to %s (%.2f%%) in %ims",
					jpgfilename.c_str(), cr, ( total >= 0 ) ? total : -1 );
				break;
			case FileType::F_UNK:
				sprintf( msg, "Unknown filetype" );
				break;	
		}
	}
	
	
	return true;
}

/* -----------------------------------------------
	DLL export init input (file/mem)
	----------------------------------------------- */
EXPORT void pjglib_init_streams( void* in_src, int in_type, int in_size, void* out_dest, int out_type )
{
	/* a short reminder about input/output stream types:
	
	if input is file
	----------------
	in_scr -> name of input file
	in_type -> 0
	in_size -> ignore
	
	if input is memory
	------------------
	in_scr -> array containg data
	in_type -> 1
	in_size -> size of data array
	
	if input is *FILE (f.e. stdin)
	------------------------------
	in_src -> stream pointer
	in_type -> 2
	in_size -> ignore
	
	vice versa for output streams! */
	
	unsigned char buffer[ 2 ];
	
	// (re)set errorlevel
	errorfunction = nullptr;
	errorlevel = 0;
	jpgfilesize = 0;
	pjgfilesize = 0;
	
	// open input stream, check for errors
	str_in = new iostream( in_src, StreamType(in_type), in_size, StreamMode::kRead );
	if ( str_in->chkerr() ) {
		sprintf( errormessage, "error opening input stream" );
		errorlevel = 2;
		return;
	}	
	
	// open output stream, check for errors
	str_out = new iostream( out_dest, StreamType(out_type), 0, StreamMode::kWrite);
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "error opening output stream" );
		errorlevel = 2;
		return;
	}
	
	// clear filenames if needed
	jpgfilename = "";
	pjgfilename = "";
	
	// check input stream
	str_in->read( buffer, 2 );
	if ( ( buffer[0] == 0xFF ) && ( buffer[1] == 0xD8 ) ) {
		// file is JPEG
		filetype = FileType::F_JPG;
		// copy filenames
		jpgfilename = (in_type == 0) ? (char*)in_src : "JPG in memory";
		pjgfilename = (out_type == 0) ? (char*)out_dest : "PJG in memory";
	}
	else if ( (buffer[0] == program_info::pjg_magic[0]) && (buffer[1] == program_info::pjg_magic[1]) ) {
		// file is PJG
		filetype = FileType::F_PJG;
		// copy filenames
		pjgfilename = (in_type == 0) ? (char*)in_src : "PJG in memory";
		jpgfilename = (out_type == 0) ? (char*)out_dest : "JPG in memory";
	}
	else {
		// file is neither
		filetype = FileType::F_UNK;
		sprintf( errormessage, "filetype of input stream is unknown" );
		errorlevel = 2;
		return;
	}
	
	// store types of in-/output
	lib_in_type  = in_type;
	lib_out_type = out_type;
}

/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
EXPORT const char* pjglib_version_info()
{
	static char v_info[ 256 ];
	
	// copy version info to string
	sprintf( v_info, "--> %s library v%i.%i%s (%s) by %s <--",
		program_info::apptitle.c_str(), program_info::appversion / 10, program_info::appversion % 10, program_info::subversion.c_str(), program_info::versiondate.c_str(), program_info::author.c_str());
			
	return (const char*) v_info;
}

/* -----------------------------------------------
	DLL export version information
	----------------------------------------------- */
EXPORT const char* pjglib_short_name()
{
	static char v_name[ 256 ];
	
	// copy version info to string
	sprintf( v_name, "%s v%i.%i%s",
		program_info::apptitle.c_str(), program_info::appversion / 10, program_info::appversion % 10, program_info::subversion.c_str());
			
	return (const char*) v_name;
}
#endif

/* ----------------------- End of libary only functions -------------------------- */

/* ----------------------- Begin of main interface functions -------------------------- */


/* -----------------------------------------------
	reads in commandline arguments
	----------------------------------------------- */
	
#if !defined(BUILD_LIB)	
INTERN void initialize_options( int argc, char** argv )
{	
	int tmp_val;
	int i;
	
	// read in arguments
	while ( --argc > 0 ) {
		argv++;
		std::string arg = *argv;
		// switches begin with '-'
		if (arg == "-p") {
			err_tol = 2;
		}
		else if (arg == "-d") {
			disc_meta = true;
		}		
		else if (arg == "-ver") {
			verify_lv = ( verify_lv < 1 ) ? 1 : verify_lv;
		}
		else if ( sscanf(arg.c_str(), "-v%i", &tmp_val ) == 1 ){
			verbosity = tmp_val;
			verbosity = ( verbosity < 0 ) ? 0 : verbosity;
			verbosity = ( verbosity > 2 ) ? 2 : verbosity;			
		}
		else if (arg == "-vp") {
			verbosity = -1;
		}
		else if (arg == "-np") {
			wait_exit = false;
		}
		else if (arg == "-o") {
			overwrite = true;
		}
		#if defined(DEV_BUILD)
		else if (arg == "-dev") {
			developer = true;
		}
		else if (arg == "-test") {
			verify_lv = 2;
		}
		else if ( sscanf(arg.c_str(), "-t%i,%i", &i, &tmp_val ) == 2 ) {
			i = ( i < 0 ) ? 0 : i;
			i = ( i > 3 ) ? 3 : i;
			tmp_val = ( tmp_val < 0  ) ?  0 : tmp_val;
			tmp_val = ( tmp_val > 10 ) ? 10 : tmp_val;
			nois_trs[ i ] = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-s%i,%i", &i, &tmp_val ) == 2 ) {
			i = ( i < 0 ) ? 0 : i;
			i = ( i > 3 ) ? 3 : i;
			tmp_val = ( tmp_val <  1 ) ?  1 : tmp_val;
			tmp_val = ( tmp_val > 49 ) ? 49 : tmp_val;
			segm_cnt[ i ] = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-t%i", &tmp_val ) == 1 ) {
			tmp_val = ( tmp_val < 0  ) ?  0 : tmp_val;
			tmp_val = ( tmp_val > 10 ) ? 10 : tmp_val;
			nois_trs[0] = tmp_val;
			nois_trs[1] = tmp_val;
			nois_trs[2] = tmp_val;
			nois_trs[3] = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-s%i", &tmp_val ) == 1 ) {
			tmp_val = ( tmp_val <  1 ) ?  1 : tmp_val;
			tmp_val = ( tmp_val > 64 ) ? 64 : tmp_val;
			segm_cnt[0] = tmp_val;
			segm_cnt[1] = tmp_val;
			segm_cnt[2] = tmp_val;
			segm_cnt[3] = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-coll%i", &tmp_val ) == 1 ) {
			tmp_val = std::max(tmp_val, 0);
			tmp_val = std::min(tmp_val, 5);
			coll_mode = CollectionMode(tmp_val);
			action = Action::A_COLL_DUMP;
		}
		else if ( sscanf(arg.c_str(), "-fcol%i", &tmp_val ) == 1 ) {
			tmp_val = std::max(tmp_val, 0);
			tmp_val = std::min(tmp_val, 5);
			coll_mode = CollectionMode(tmp_val);
			action = Action::A_FCOLL_DUMP;
		}
		else if (arg == "-split") {
			action = Action::A_SPLIT_DUMP;
		}
		else if (arg == "-zdst") {
			action = Action::A_ZDST_DUMP;
		}	
		else if (arg == "-info") {
			action = Action::A_TXT_INFO;
		}
		else if (arg == "-dist") {
			action = Action::A_DIST_INFO;
		}
		else if (arg == "-pgm") {
			action = Action::A_PGM_DUMP;
		}
	   	else if (arg == "-comp") {
			action = Action::A_COMPRESS;
		}
		#endif
		else if (arg == "-") {
			// switch standard message out stream
			msgout = stderr;
			// use "-" as placeholder for stdin
			filelist.push_back("-");
		}
		else {
			// if argument is not switch, it's a filename
			filelist.push_back(arg);
		}		
	}
	
	// alloc arrays for error messages and types storage
	err_list = std::vector<std::string>(filelist.size());
	err_tp   = std::vector<int>(filelist.size());
	
	// backup settings - needed to restore original setting later
	if ( !auto_set ) {
		orig_set[ 0 ] = nois_trs[ 0 ];
		orig_set[ 1 ] = nois_trs[ 1 ];
		orig_set[ 2 ] = nois_trs[ 2 ];
		orig_set[ 3 ] = nois_trs[ 3 ];
		orig_set[ 4 ] = segm_cnt[ 0 ];
		orig_set[ 5 ] = segm_cnt[ 1 ];
		orig_set[ 6 ] = segm_cnt[ 2 ];
		orig_set[ 7 ] = segm_cnt[ 3 ];
	}
	else {
		for ( i = 0; i < 8; i++ )
			orig_set[ i ] = 0;
	}	
}

/* -----------------------------------------------
	UI for processing one file
	----------------------------------------------- */
INTERN void process_ui()
{

	errorfunction = nullptr;
	errorlevel = 0;
	jpgfilesize = 0;
	pjgfilesize = 0;	
	#if !defined(DEV_BUILD)
	action = Action::A_COMPRESS;
	#endif
	
	// compare file name, set pipe if needed
	if ( filelist[ file_no ] == "-" && ( action == Action::A_COMPRESS ) ) {
		pipe_on = true;
		filelist[ file_no ] = "STDIN";
	}
	else {		
		pipe_on = false;
	}

	std::string actionmsg;
	if ( verbosity >= 0 ) { // standard UI
		fprintf( msgout,  "\nProcessing file %i of %u \"%s\" -> ",
					file_no + 1, filelist.size(), filelist[ file_no ].c_str() );
		
		if ( verbosity > 1 )
			fprintf( msgout,  "\n----------------------------------------" );
		
		// check input file and determine filetype
		execute( check_file );
		
		// get specific action message
		if ( filetype == FileType::F_UNK ) actionmsg = "unknown filetype";
		else switch ( action ) {
			case Action::A_COMPRESS:	actionmsg = ( filetype == FileType::F_JPG ) ? "Compressing" : "Decompressing";	break;
			case Action::A_SPLIT_DUMP:	actionmsg = "Splitting"; break;			
			case Action::A_COLL_DUMP:	actionmsg = "Extracting Colls"; break;
			case Action::A_FCOLL_DUMP:	actionmsg = "Extracting FColls"; break;
			case Action::A_ZDST_DUMP:	actionmsg = "Extracting ZDST lists"; break;			
			case Action::A_TXT_INFO:	actionmsg = "Extracting info"; break;		
			case Action::A_DIST_INFO:	actionmsg = "Extracting distributions";	break;		
			case Action::A_PGM_DUMP:	actionmsg = "Converting"; break;
		}
		
		if ( verbosity < 2 ) fprintf( msgout, "%s -> ", actionmsg.c_str() );
	}
	else { // progress bar UI
		// update progress message
		fprintf( msgout, "Processing file %2i of %2u ", file_no + 1, filelist.size());
		progress_bar( file_no, filelist.size());
		fprintf( msgout, "\r" );
		execute( check_file );
	}
	fflush( msgout );
	
	
	// main function routine
	auto begin = std::chrono::steady_clock::now();
	
	// streams are initiated, start processing file
	process_file();
	
	// close iostreams
	if ( str_in  != nullptr ) delete( str_in  ); str_in  = nullptr;
	if ( str_out != nullptr ) delete( str_out ); str_out = nullptr;
	if ( str_str != nullptr ) delete( str_str ); str_str = nullptr;
	// delete if broken or if output not needed
	if ( ( !pipe_on ) && ( ( errorlevel >= err_tol ) || ( action != Action::A_COMPRESS ) ) ) {
		if ( filetype == FileType::F_JPG ) {
			if ( file_exists( pjgfilename ) ) remove( pjgfilename.c_str() );
		} else if ( filetype == FileType::F_PJG ) {
			if ( file_exists( jpgfilename ) ) remove( jpgfilename.c_str());
		}
	}
	
	auto end = std::chrono::steady_clock::now();
	
	// speed and compression ratio calculation
	float cr = ( jpgfilesize > 0 ) ? ( 100.0 * pjgfilesize / jpgfilesize ) : 0;
	
	if ( verbosity >= 0 ) { // standard UI
		if ( verbosity > 1 )
			fprintf( msgout,  "\n----------------------------------------" );
		
		// display success/failure message
		std::string errtypemsg;
		switch ( verbosity ) {
			case 0:			
				if ( errorlevel < err_tol ) {
					if ( action == Action::A_COMPRESS ) fprintf( msgout,  "%.2f%%", cr );
					else fprintf( msgout, "DONE" );
				}
				else fprintf( msgout,  "ERROR" );
				if ( errorlevel > 0 ) fprintf( msgout,  "\n" );
				break;
			
			case 1:
				fprintf( msgout, "%s\n",  ( errorlevel < err_tol ) ? "DONE" : "ERROR" );
				break;
			
			case 2:
				if ( errorlevel < err_tol ) fprintf( msgout,  "\n-> %s OK\n", actionmsg.c_str());
				else  fprintf( msgout,  "\n-> %s ERROR\n", actionmsg.c_str());
				break;
		}
		
		// set type of error message
		switch ( errorlevel ) {
			case 0:	errtypemsg = "none"; break;
			case 1: errtypemsg = ( err_tol > 1 ) ?  "warning (ignored)" : "warning (skipped file)"; break;
			case 2: errtypemsg = "fatal error"; break;
		}
		
		// error/ warning message
		if ( errorlevel > 0 ) {
			fprintf( msgout, " %s -> %s:\n", get_status( errorfunction ).c_str(), errtypemsg.c_str());
			fprintf( msgout, " %s\n", errormessage );
		}
		if ((verbosity > 0) && (errorlevel < err_tol) && (action == Action::A_COMPRESS)) {
			auto duration = end - begin;
			auto total = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			if ( total >= 0 ) {
				fprintf( msgout,  " time taken  : %7lld msec\n", total );
				int bpms = (total > 0) ? (jpgfilesize / total) : jpgfilesize;
				fprintf( msgout,  " byte per ms : %7i byte\n", bpms );
			}
			else {
				fprintf( msgout,  " time taken  : %7s msec\n", "N/A" );
				fprintf( msgout,  " byte per ms : %7s byte\n", "N/A" );
			}
			fprintf( msgout,  " comp. ratio : %7.2f %%\n", cr );		
		}	
		if ( ( verbosity > 1 ) && ( action == Action::A_COMPRESS ) )
			fprintf( msgout,  "\n" );
	}
	else { // progress bar UI
		// if this is the last file, update progress bar one last time
		if ( file_no + 1 == filelist.size()) {
			// update progress message
			fprintf( msgout, "Processed %2i of %2u files ", file_no + 1, filelist.size());
			progress_bar( 1, 1 );
			fprintf( msgout, "\r" );
		}	
	}
}

/* -----------------------------------------------
	gets statusmessage for function
	----------------------------------------------- */
INTERN inline std::string get_status( bool (*function)() )
{	
	if ( function == nullptr ) {
		return "unknown action";
	} else if ( function == *check_file ) {
		return "Determining filetype";
	} else if ( function == *jpg::decode::read ) {
		return "Reading header & image data";
	} else if ( function == *jpg::encode::merge ) {
		return "Merging header & image data";
	} else if ( function == *jpg::decode::decode ) {
		return "Decompressing JPEG image data";
	} else if ( function == *jpg::encode::recode ) {
		return "Recompressing JPEG image data";
	} else if ( function == *dct::adapt_icos ) {
		return "Adapting DCT precalc. tables";
	} else if ( function == *predict_dc ) {
		return "Applying prediction to DC";
	} else if ( function == *unpredict_dc ) {
		return "Removing prediction from DC";
	} else if ( function == *jpg::decode::check_value_range ) {
		return "Checking values range";
	} else if ( function == *calc_zdst_lists ) {
		return "Calculating zero dist lists";
	} else if ( function == *pjg::encode::encode ) {
		return "Compressing data to PJG";
	} else if ( function == *pjg::decode::decode ) {
		return "Uncompressing data from PJG";
	} else if ( function == *swap_streams ) {
		return "Swapping input/output streams";
	} else if ( function == *compare_output ) {
		return "Verifying output stream";
	} else if ( function == *reset_buffers ) {
		return "Resetting program";
	}
	#if defined(DEV_BUILD)
	else if ( function == *dump_hdr ) {
		return "Writing header data to file";
	} else if ( function == *dump_huf ) {
		return "Writing huffman data to file";
	} else if ( function == *dump_coll ) {
		return "Writing collections to files";
	} else if ( function == *dump_zdst ) {
		return "Writing zdist lists to files";
	} else if ( function == *dump_errfile ) {
		return "Writing error info to file";
	} else if ( function == *dump_info ) {
		return "Writing info to files";
	} else if ( function == *dump_dist ) {
		return "Writing distributions to files";
	} else if ( function == *dump_pgm ) {
		return "Writing converted image to pgm";
	}
	#endif
	else {
		return "Function description missing!";
	}
}

/* -----------------------------------------------
	shows help in case of wrong input
	----------------------------------------------- */
INTERN void show_help()
{	
	fprintf( msgout, "\n" );
	fprintf( msgout, "Website: %s\n", program_info::website.c_str() );
	fprintf( msgout, "Email  : %s\n", program_info::email.c_str() );
	fprintf( msgout, "\n" );
	fprintf( msgout, "Usage: %s [switches] [filename(s)]", program_info::appname.c_str());
	fprintf( msgout, "\n" );
	fprintf( msgout, "\n" );
	fprintf( msgout, " [-ver]   verify files after processing\n" );
	fprintf( msgout, " [-v?]    set level of verbosity (max: 2) (def: 0)\n" );
	fprintf( msgout, " [-np]    no pause after processing files\n" );
	fprintf( msgout, " [-o]     overwrite existing files\n" );
	fprintf( msgout, " [-p]     proceed on warnings\n" );
	fprintf( msgout, " [-d]     discard meta-info\n" );
	#if defined(DEV_BUILD)
	if ( developer ) {
	fprintf( msgout, "\n" );
	fprintf( msgout, " [-s?]    set global number of segments (1<=s<=49)\n" );
	fprintf( msgout, " [-t?]    set global noise threshold (0<=t<=10)\n" );
	fprintf( msgout, "\n" );	
	fprintf( msgout, " [-s?,?]  set number of segments for component\n" );
	fprintf( msgout, " [-t?,?]  set noise threshold for component\n" );
	fprintf( msgout, "\n" );
	fprintf( msgout, " [-test]  test algorithms, alert if error\n" );
	fprintf( msgout, " [-split] split jpeg (to header & image data)\n" );
	fprintf( msgout, " [-coll?] write collections (0=std,1=dhf,2=squ,3=unc)\n" );
	fprintf( msgout, " [-fcol?] write predicted collections (see above)\n" );
	fprintf( msgout, " [-zdst]  write zero distribution lists\n" );	
	fprintf( msgout, " [-info]  write debug info to .nfo file\n" );	
	fprintf( msgout, " [-dist]  write distribution data to file\n" );
	fprintf( msgout, " [-pgm]   convert and write to pgm files\n" );
	}
	#endif
	fprintf( msgout, "\n" );
	fprintf( msgout, "Examples: \"%s -v1 -o baboon.%s\"\n", program_info::appname.c_str(), program_info::pjg_ext.c_str() );
	fprintf( msgout, "          \"%s -p *.%s\"\n", program_info::appname.c_str(), program_info::jpg_ext.c_str() );
}
#endif


/* -----------------------------------------------
	processes one file
	----------------------------------------------- */

INTERN void process_file()
{	
	if ( filetype == FileType::F_JPG ) {
		switch ( action ) {
			case Action::A_COMPRESS:
				execute( jpg::decode::read );
				execute( jpg::decode::decode );
				execute( jpg::decode::check_value_range );
				execute( dct::adapt_icos );
				execute( predict_dc );
				execute( calc_zdst_lists );
				execute( pjg::encode::encode );
				#if !defined(BUILD_LIB)	
				if ( verify_lv > 0 ) { // verifcation
					execute( reset_buffers );
					execute( swap_streams );
					execute( pjg::decode::decode );
					execute( dct::adapt_icos );
					execute( unpredict_dc );
					execute( jpg::encode::recode );
					execute( jpg::encode::merge );
					execute( compare_output );
				}
				#endif
				break;
				
			#if !defined(BUILD_LIB) && defined(DEV_BUILD)
			case Action::A_SPLIT_DUMP:
				execute( jpg::decode::read );
				execute( dump_hdr );
				execute( dump_huf );
				break;
				
			case Action::A_COLL_DUMP:
				execute( jpg::decode::read );
				execute( jpg::decode::decode );
				execute( dump_coll );
				break;
				
			case Action::A_FCOLL_DUMP:
				execute( jpg::decode::read );
				execute( jpg::decode::decode );
				execute( jpg::decode::check_value_range );
				execute( dct::adapt_icos );
				execute( predict_dc );
				execute( dump_coll );
				break;
				
			case Action::A_ZDST_DUMP:
				execute( jpg::decode::read );
				execute( jpg::decode::decode );
				execute( jpg::decode::check_value_range );
				execute( dct::adapt_icos );
				execute( predict_dc );
				execute( calc_zdst_lists );
				execute( dump_zdst );
				break;
				
			case Action::A_TXT_INFO:
				execute( jpg::decode::read );
				execute( dump_info );
				break;
				
			case Action::A_DIST_INFO:
				execute( jpg::decode::read );
				execute( jpg::decode::decode );
				execute( jpg::decode::check_value_range );
				execute( dct::adapt_icos );
				execute( predict_dc );
				execute( dump_dist );
				break;
			
			case Action::A_PGM_DUMP:
				execute( jpg::decode::read );
				execute( jpg::decode::decode );
				execute(dct::adapt_icos );
				execute( dump_pgm );
				break;
			#else
			default:
				break;
			#endif
		}
	}
	else if ( filetype == FileType::F_PJG )	{
		switch ( action )
		{
			case Action::A_COMPRESS:
				execute( pjg::decode::decode );
				execute( dct::adapt_icos );
				execute( unpredict_dc );
				execute( jpg::encode::recode );
				execute( jpg::encode::merge );
				#if !defined(BUILD_LIB)
				if ( verify_lv > 0 ) { // verify
					execute( reset_buffers );
					execute( swap_streams );
					execute( jpg::decode::read );
					execute( jpg::decode::decode );
					execute( jpg::decode::check_value_range );
					execute(dct::adapt_icos );
					execute( predict_dc );
					execute( calc_zdst_lists );
					execute( pjg::encode::encode );
					execute( compare_output );
				}
				#endif
				break;
				
			#if !defined(BUILD_LIB) && defined(DEV_BUILD)
			case Action::A_SPLIT_DUMP:
				execute( pjg::decode::decode);
				execute( dct::adapt_icos );
				execute( unpredict_dc );
				execute( jpg::encode::recode );
				execute( dump_hdr );
				execute( dump_huf );
				break;
				
			case Action::A_COLL_DUMP:
				execute( pjg::decode::decode);
				execute(dct::adapt_icos );
				execute( unpredict_dc );
				execute( dump_coll );
				break;
				
			case Action::A_FCOLL_DUMP:
				execute( pjg::decode::decode);
				execute( dump_coll );
				break;
				
			case Action::A_ZDST_DUMP:
				execute( pjg::decode::decode);
				execute( dump_zdst );
				break;
			
			case Action::A_TXT_INFO:
				execute( pjg::decode::decode);
				execute( dump_info );
				break;
			
			case Action::A_DIST_INFO:
				execute( pjg::decode::decode);
				execute( dump_dist );
				break;
			
			case Action::A_PGM_DUMP:
				execute( pjg::decode::decode);
				execute( dct::adapt_icos );
				execute( unpredict_dc );
				execute( dump_pgm );
				break;
			#else
			default:
				break;
			#endif
		}
	}	
	#if !defined(BUILD_LIB) && defined(DEV_BUILD)
	// write error file if verify lv > 1
	if ( ( verify_lv > 1 ) && ( errorlevel >= err_tol ) )
		dump_errfile();
	#endif
	// reset buffers
	reset_buffers();
}


/* -----------------------------------------------
	main-function execution routine
	----------------------------------------------- */

INTERN void execute( bool (*function)() )
{
	if ( errorlevel < err_tol ) {
		#if !defined BUILD_LIB
		
		// write statusmessage
		if ( verbosity == 2 ) {
			fprintf( msgout,  "\n%s ", get_status( function ).c_str() );
			for ( int i = strlen( get_status( function ).c_str()); i <= 30; i++ )
				fprintf( msgout,  " " );			
		}
		
		// set starttime
		auto begin = std::chrono::steady_clock::now();
		// call function
		bool success = ( *function )();
		// set endtime
		auto end = std::chrono::steady_clock::now();
		
		if ( ( errorlevel > 0 ) && ( errorfunction == nullptr ) )
			errorfunction = function;
		
		// write time or failure notice
		if ( success ) {
			auto duration = end - begin;
			auto total = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			if ( verbosity == 2 ) fprintf( msgout,  "%7lldms", ( total >= 0 ) ? total : -1 );
		}
		else {
			errorfunction = function;
			if ( verbosity == 2 ) fprintf( msgout,  "%8s", "ERROR" );
		}
		#else
		// call function
		( *function )();
		
		// store errorfunction if needed
		if ( ( errorlevel > 0 ) && ( errorfunction == nullptr ) )
			errorfunction = function;
		#endif
	}
}

/* ----------------------- End of main interface functions -------------------------- */

/* ----------------------- Begin of main functions -------------------------- */


/* -----------------------------------------------
	check file and determine filetype
	----------------------------------------------- */

#if !defined(BUILD_LIB)
INTERN bool check_file()
{	
	unsigned char fileid[ 2 ] = { 0, 0 };
	const std::string& filename = filelist[ file_no ];
	
	
	// open input stream, check for errors
	str_in = new iostream( (void*) filename.c_str(), ( !pipe_on ) ? StreamType::kFile : StreamType::kStream, 0, StreamMode::kRead );
	if ( str_in->chkerr() ) {
		sprintf( errormessage, FRD_ERRMSG.c_str(), filename.c_str());
		errorlevel = 2;
		return false;
	}
	
	// free memory from filenames if needed
	jpgfilename = "";
	pjgfilename = "";
	
	// immediately return error if 2 bytes can't be read
	if ( str_in->read( fileid, 2 ) != 2 ) { 
		filetype = FileType::F_UNK;
		sprintf( errormessage, "file doesn't contain enough data" );
		errorlevel = 2;
		return false;
	}
	
	// check file id, determine filetype
	if ( ( fileid[0] == 0xFF ) && ( fileid[1] == 0xD8 ) ) {
		// file is JPEG
		filetype = FileType::F_JPG;
		// create filenames
		if ( !pipe_on ) {
			jpgfilename = filename;
			pjgfilename = ( overwrite ) ?
				create_filename( filename, program_info::pjg_ext ) :
				unique_filename(filename, program_info::pjg_ext);
		}
		else {
			jpgfilename = create_filename( "STDIN", "" );
			pjgfilename = create_filename( "STDOUT", "" );
		}
		// open output stream, check for errors
		str_out = new iostream( (void*) pjgfilename.c_str(), ( !pipe_on ) ? StreamType::kFile : StreamType::kStream, 0, StreamMode::kWrite );
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG.c_str(), pjgfilename.c_str() );
			errorlevel = 2;
			return false;
		}
		// JPEG specific settings - restore original settings
		if ( orig_set[ 0 ] == 0 )
			auto_set = true;
		else {	
			nois_trs[ 0 ] = orig_set[ 0 ];
			nois_trs[ 1 ] = orig_set[ 1 ];
			nois_trs[ 2 ] = orig_set[ 2 ];
			nois_trs[ 3 ] = orig_set[ 3 ];
			segm_cnt[ 0 ] = orig_set[ 4 ];
			segm_cnt[ 1 ] = orig_set[ 5 ];
			segm_cnt[ 2 ] = orig_set[ 6 ];
			segm_cnt[ 3 ] = orig_set[ 7 ];
			auto_set = false;
		}
	}
	else if ( ( fileid[0] == program_info::pjg_magic[0] ) && ( fileid[1] == program_info::pjg_magic[1] ) ) {
		// file is PJG
		filetype = FileType::F_PJG;
		// create filenames
		if ( !pipe_on ) {
			pjgfilename = filename;
			jpgfilename = ( overwrite ) ?
				create_filename( filename, program_info::jpg_ext) :
				unique_filename( filename, program_info::jpg_ext);
		}
		else {
			jpgfilename = create_filename( "STDOUT", "" );
			pjgfilename = create_filename( "STDIN", "" );
		}
		// open output stream, check for errors
		str_out = new iostream( (void*) jpgfilename.c_str(), ( !pipe_on ) ? StreamType::kFile : StreamType::kStream, 0, StreamMode::kWrite );
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG.c_str(), jpgfilename.c_str());
			errorlevel = 2;
			return false;
		}
		// PJG specific settings - auto unless specified otherwise
		auto_set = true;
	}
	else {
		// file is neither
		filetype = FileType::F_UNK;
		sprintf( errormessage, "filetype of file \"%s\" is unknown", filename.c_str());
		errorlevel = 2;
		return false;		
	}
	
	
	return true;
}

/* -----------------------------------------------
	swap streams / init verification
	----------------------------------------------- */
	
INTERN bool swap_streams()	
{
	unsigned char dmp[ 2 ];
	
	// store input stream
	str_str = str_in;
	str_str->rewind();
	
	// replace input stream by output stream / switch mode for reading / read first bytes
	str_in = str_out;
	str_in->switch_mode();
	str_in->read( dmp, 2 );
	
	// open new stream for output / check for errors
	str_out = new iostream( nullptr, StreamType::kMemory, 0, StreamMode::kWrite );
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "error opening comparison stream" );
		errorlevel = 2;
		return false;
	}
	
	
	return true;
}

/* -----------------------------------------------
	comparison between input & output
	----------------------------------------------- */

INTERN bool compare_output()
{
	unsigned char* buff_ori;
	unsigned char* buff_cmp;
	int bsize = 1024;
	int dsize;
	int i, b;
	
	
	// init buffer arrays
	buff_ori = ( unsigned char* ) calloc( bsize, sizeof( char ) );
	buff_cmp = ( unsigned char* ) calloc( bsize, sizeof( char ) );
	if ( ( buff_ori == nullptr ) || ( buff_cmp == nullptr ) ) {
		if ( buff_ori != nullptr ) free( buff_ori );
		if ( buff_cmp != nullptr ) free( buff_cmp );
		sprintf( errormessage, MEM_ERRMSG.c_str() );
		errorlevel = 2;
		return false;
	}
	
	// switch output stream mode / check for stream errors
	str_out->switch_mode();
	while ( true ) {
		if ( str_out->chkerr() )
			sprintf( errormessage, "error in comparison stream" );
		else if ( str_in->chkerr() )
			sprintf( errormessage, "error in output stream" );
		else if ( str_str->chkerr() )
			sprintf( errormessage, "error in input stream" );
		else break;
		errorlevel = 2;
		return false;
	}
	
	// compare sizes
	dsize = str_str->getsize();
	if ( str_out->getsize() != dsize ) {
		sprintf( errormessage, "file sizes do not match" );
		errorlevel = 2;
		return false;
	}
	
	// compare files byte by byte
	for ( i = 0; i < dsize; i++ ) {
		b = i % bsize;
		if ( b == 0 ) {
			str_str->read( buff_ori, bsize );
			str_out->read( buff_cmp, bsize );
		}
		if ( buff_ori[ b ] != buff_cmp[ b ] ) {
			sprintf( errormessage, "difference found at 0x%X", i );
			errorlevel = 2;
			return false;
		}
	}
	
	// free buffers
	free( buff_ori );
	free( buff_cmp );
	
	
	return true;
}
#endif


/* -----------------------------------------------
	set each variable to its initial value
	----------------------------------------------- */

INTERN bool reset_buffers()
{
	int cmp, bpos;
	int i;
	
	
	// -- free buffers --
	
	// free buffers & set pointers nullptr
	if ( hdrdata  != nullptr ) free ( hdrdata );
	huffdata.clear();
	grbgdata.clear();
	jpg::rst_err.clear();
	jpg::rstp.clear();
	jpg::scnp.clear();
	hdrdata   = nullptr;
	
	// free image arrays
	for ( cmp = 0; cmp < 4; cmp++ )	{
		if ( pjg::zdstdata[ cmp ] != nullptr ) free( pjg::zdstdata[cmp] );
		if ( pjg::eobxhigh[ cmp ] != nullptr) free( pjg::eobxhigh[cmp] );
		if ( pjg::eobyhigh[ cmp ] != nullptr) free( pjg::eobyhigh[cmp] );
		if ( pjg::zdstxlow[ cmp ] != nullptr) free( pjg::zdstxlow[cmp] );
		if ( pjg::zdstylow[ cmp ] != nullptr) free( pjg::zdstylow[cmp] );
		pjg::zdstdata[ cmp ] = nullptr;
		pjg::eobxhigh[ cmp ] = nullptr;
		pjg::eobyhigh[ cmp ] = nullptr;
		pjg::zdstxlow[ cmp ] = nullptr;
		pjg::zdstylow[ cmp ] = nullptr;
		pjg::freqscan[ cmp ] = (unsigned char*) stdscan.data();
		
		for ( bpos = 0; bpos < 64; bpos++ ) {
			if ( dct::colldata[ cmp ][ bpos ] != nullptr ) free( dct::colldata[cmp][bpos] );
			dct::colldata[ cmp ][ bpos ] = nullptr;
		}		
	}
	
	
	// -- set variables --
	
	// preset componentinfo
	for ( cmp = 0; cmp < 4; cmp++ ) {
		cmpnfo[ cmp ].sfv = -1;
		cmpnfo[ cmp ].sfh = -1;
		cmpnfo[ cmp ].mbs = -1;
		cmpnfo[ cmp ].bcv = -1;
		cmpnfo[ cmp ].bch = -1;
		cmpnfo[ cmp ].bc  = -1;
		cmpnfo[ cmp ].ncv = -1;
		cmpnfo[ cmp ].nch = -1;
		cmpnfo[ cmp ].nc  = -1;
		cmpnfo[ cmp ].sid = -1;
		cmpnfo[ cmp ].jid = -1;
		cmpnfo[ cmp ].qtable = nullptr;
		cmpnfo[ cmp ].huffdc = -1;
		cmpnfo[ cmp ].huffac = -1;
	}
	
	// preset imgwidth / imgheight / component count 
	image::imgwidth  = 0;
	image::imgheight = 0;
	image::cmpc      = 0;
	
	// preset mcu info variables / restart interval
	image::sfhm      = 0;
	image::sfvm      = 0;
	image::mcuc      = 0;
	image::mcuh      = 0;
	image::mcuv      = 0;
	jpg::rsti      = 0;
	
	// reset quantization / huffman tables
	for ( i = 0; i < 4; i++ ) {
		jpg::htset[ 0 ][ i ] = false;
		jpg::htset[ 1 ][ i ] = false;
		for ( bpos = 0; bpos < 64; bpos++ )
			qtables[ i ][ bpos ] = 0;
	}
	
	// preset jpegtype
	jpegtype  = JpegType::UNKNOWN;
	
	// reset jpg::padbit
	jpg::padbit = -1;
	
	
	return true;
}
	
bool jpg::decode::read()
{
	unsigned char  type = 0x00; // type of current marker segment
	unsigned int   len  = 0; // length of current marker segment
	unsigned int   crst = 0; // current rst marker counter
	unsigned int   cpos = 0; // rst marker counter
	unsigned char  tmp;	
	
	// preset count of scans
	jpg::scan_count = 0;
	
	// start headerwriter
	auto hdrw = std::make_unique<abytewriter>(4096);
	hdrs = 0; // size of header data, start with 0
	
	// start huffman writer
	auto huffw = std::make_unique<abytewriter>(0);
	
	// alloc memory for segment data first
	std::vector<std::uint8_t> segment(1024);
	
	// JPEG reader loop
	while ( true ) {
		if ( type == 0xDA ) { // if last marker was sos
			// switch to huffman data reading mode
			cpos = 0;
			crst = 0;
			while ( true ) {
				// read byte from imagedata
				if ( str_in->read_byte(&tmp) == 0 )
					break;
					
				// non-0xFF loop
				if ( tmp != 0xFF ) {
					crst = 0;
					while ( tmp != 0xFF ) {
						huffw->write( tmp );
						if ( str_in->read_byte(&tmp) == 0 )
							break;
					}
				}
				
				// treatment of 0xFF
				if ( tmp == 0xFF ) {
					if ( str_in->read_byte(&tmp) == 0 )
						break; // read next byte & check
					if ( tmp == 0x00 ) {
						crst = 0;
						// no zeroes needed -> ignore 0x00. write 0xFF
						huffw->write( 0xFF );
					}
					else if ( tmp == 0xD0 + ( cpos % 8 ) ) { // restart marker
						// increment rst counters
						cpos++;
						crst++;
					}
					else { // in all other cases leave it to the header parser routines
						// store number of wrongly set rst markers
						if ( crst > 0 ) {
							if (jpg::rst_err.empty()) {
								jpg::rst_err.resize(jpg::scan_count + 1);
							}
						}
						if (!jpg::rst_err.empty()) {
							// realloc and set only if needed
							jpg::rst_err.resize(jpg::scan_count + 1);
							if ( crst > 255 ) {
								sprintf( errormessage, "Severe false use of RST markers (%i)", (int) crst );
								errorlevel = 1;
								crst = 255;
							}
							jpg::rst_err[ jpg::scan_count ] = crst;							
						}
						// end of current scan
						jpg::scan_count++;
						// on with the header parser routines
						segment[ 0 ] = 0xFF;
						segment[ 1 ] = tmp;
						break;
					}
				}
				else {
					// otherwise this means end-of-file, so break out
					break;
				}
			}
		}
		else {
			// read in next marker
			if ( str_in->read( segment.data(), 2 ) != 2 ) break;
			if ( segment[ 0 ] != 0xFF ) {
				// ugly fix for incorrect marker segment sizes
				sprintf( errormessage, "size mismatch in marker segment FF %2X", type );
				errorlevel = 2;
				if ( type == 0xFE ) { //  if last marker was COM try again
					if ( str_in->read( segment.data(), 2 ) != 2 ) break;
					if ( segment[ 0 ] == 0xFF ) errorlevel = 1;
				}
				if ( errorlevel == 2 ) {
					return false;
				}
			}
		}
		
		// read segment type
		type = segment[ 1 ];
		
		// if EOI is encountered make a quick exit
		if ( type == 0xD9 ) {
			// get pointer for header data & size
			hdrdata  = hdrw->getptr();
			hdrs     = hdrw->getpos();
			// get pointer for huffman data & size
			auto hdata = huffw->getptr();
			auto hdata_length = huffw->getpos();
			huffdata = std::vector<std::uint8_t>(hdata, hdata + hdata_length);
			// everything is done here now
			break;			
		}
		
		// read in next segments' length and check it
		if ( str_in->read( segment.data() + 2, 2 ) != 2 ) break;
		len = 2 + pack( segment[ 2 ], segment[ 3 ] );
		if ( len < 4 ) break;
		
		// realloc segment data if needed
		if ( segment.size() < len ) {
			segment.resize(len);
		}
		
		// read rest of segment, store back in header writer
		if ( str_in->read( ( segment.data() + 4 ), ( len - 4 ) ) !=
			( unsigned short ) ( len - 4 ) ) break;
		hdrw->write_n( segment.data(), len );
	}
	// JPEG reader loop end
	
	// check if everything went OK
	if ( ( hdrs == 0 ) || huffdata.empty() ) {
		sprintf( errormessage, "unexpected end of data encountered" );
		errorlevel = 2;
		return false;
	}
	
	// store garbage after EOI if needed
	bool garbage_avail = str_in->read_byte(&tmp);
	if (garbage_avail) {

		auto grbgw = std::make_unique<abytewriter>( 1024 );
		grbgw->write( tmp );
		while( true ) {
			len = str_in->read( segment.data(), segment.capacity() );
			if ( len == 0 ) break;
			grbgw->write_n( segment.data(), len );
		}
		auto grbgptr = grbgw->getptr();
		auto grbg_size = grbgw->getpos();
		grbgdata = std::vector<std::uint8_t>(grbgptr, grbgptr + grbg_size);
	}
	
	// get filesize
	jpgfilesize = str_in->getsize();	
	
	// parse header for image info
	if ( !jpg::setup_imginfo() ) {
		return false;
	}
	
	
	return true;
}

bool jpg::encode::merge() {
	int hpos = 0; // current position in header
	int rpos = 0; // current restart marker position
	int scan = 1; // number of current scan	

	// write SOI
	constexpr std::array<std::uint8_t, 2> SOI = {0xFF, 0xD8};
	str_out->write(SOI.data(), 2);

	// JPEG writing loop
	while (true) {
		// store current header position
		std::uint32_t tmp = hpos;

		// seek till start-of-scan
		std::uint8_t type; // type of current marker segment
		for (type = 0x00; type != 0xDA;) {
			if (hpos >= hdrs) {
				break;
			}
			type = hdrdata[hpos + 1];
			int len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] ); // length of current marker segment
			hpos += len;
		}

		// write header data to file
		str_out->write(hdrdata + tmp, hpos - tmp);

		// get out if last marker segment type was not SOS
		if (type != 0xDA) {
			break;
		}


		// (re)set corrected rst pos
		std::uint32_t cpos = 0; // in scan corrected rst marker position

		// write & expand huffman coded image data
		// ipos is the current position in image data.
		for (std::uint32_t ipos = jpg::scnp[scan - 1]; ipos < jpg::scnp[scan]; ipos++) {
			// write current byte
			str_out->write_byte(huffdata[ipos]);
			// check current byte, stuff if needed
			if (huffdata[ipos] == 0xFF) {
				str_out->write_byte(std::uint8_t(0)); // 0xFF stuff value
			}
			// insert restart markers if needed
			if (!jpg::rstp.empty()) {
				if (ipos == jpg::rstp[rpos]) {
					const std::uint8_t rst = 0xD0 + (cpos % 8); // Restart marker
					constexpr std::uint8_t mrk = 0xFF; // marker start
					str_out->write_byte(mrk);
					str_out->write_byte(rst);
					rpos++;
					cpos++;
				}
			}
		}
		// insert false rst markers at end if needed
		if (!jpg::rst_err.empty()) {
			while (jpg::rst_err[scan - 1] > 0) {
				const std::uint8_t rst = 0xD0 + (cpos % 8); // Restart marker
				constexpr std::uint8_t mrk = 0xFF; // marker start
				str_out->write_byte(mrk);
				str_out->write_byte(rst);
				cpos++;
				jpg::rst_err[scan - 1]--;
			}
		}

		// proceed with next scan
		scan++;
	}

	// write EOI
	constexpr std::array<std::uint8_t, 2> EOI = {0xFF, 0xD9}; // EOI segment
	str_out->write(EOI.data(), 2);

	// write garbage if needed
	if (!grbgdata.empty()) {
		str_out->write(grbgdata.data(), grbgdata.size());
	}

	// errormessage if write error
	if (str_out->chkerr()) {
		sprintf(errormessage, "write error, possibly drive is full");
		errorlevel = 2;
		return false;
	}

	// get filesize
	jpgfilesize = str_out->getsize();

	return true;
}

bool jpg::decode::decode()
{	
	unsigned int hpos = 0; // current position in header
	
	short block[64]; // store block for coeffs
	
	// open huffman coded image data for input in abitreader
	auto huffr = std::make_unique<abitreader>(huffdata.data(), huffdata.size()); // bitwise reader for image data
	
	// preset count of scans
	jpg::scan_count = 0;
	
	// JPEG decompression loop
	while ( true )
	{
		// seek till start-of-scan, parse only DHT, DRI and SOS
		std::uint8_t type; // type of current marker segment
		for ( type = 0x00; type != 0xDA; ) {
			if ( ( int ) hpos >= hdrs ) break;
			type = hdrdata[ hpos + 1 ];
			std::uint32_t len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] ); // length of current marker segment
			if ( ( type == 0xC4 ) || ( type == 0xDA ) || ( type == 0xDD ) ) {
				if ( !jpg::jfif::parse_jfif( type, len, &( hdrdata[ hpos ] ) ) ) {
					return false;
				}
			}
			hpos += len;
		}
		
		// get out if last marker segment type was not SOS
		if ( type != 0xDA ) break;
		
		// check if huffman tables are available
		for (int csc = 0; csc < curr_scan::cmpc; csc++) {
			int cmp = curr_scan::cmp[ csc ];
			if ( ( ( curr_scan::sal == 0 ) && !jpg::htset[ 0 ][ cmpnfo[cmp].huffdc ] ) ||
				 ( ( curr_scan::sah >  0 ) && !jpg::htset[ 1 ][ cmpnfo[cmp].huffac ] ) ) {
				sprintf( errormessage, "huffman table missing in scan%i", jpg::scan_count );
				errorlevel = 2;
				return false;
			}
		}
		
		
		// intial variables set for decoding
		int cmp  = curr_scan::cmp[ 0 ];
		int csc  = 0;
		int mcu  = 0;
		int sub  = 0;
		int dpos = 0;
		
		// JPEG imagedata decoding routines
		while ( true )
		{			
			// (re)set last DCs for diff coding
			std::array<int, 4> lastdc = { 0 }; // last dc for each component
			
			// (re)set status
			int eob = 0;
			jpg::CodingStatus status = jpg::CodingStatus::OKAY;
			
			// (re)set eobrun
			int eobrun  = 0; // run of eobs
			int peobrun = 0; // previous eobrun
			
			// (re)set rst wait counter
			int rstw = jpg::rsti; // restart wait counter
			
			// decoding for interleaved data
			if ( curr_scan::cmpc > 1 )
			{				
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential interleaved decoding <---
					while ( status == jpg::CodingStatus::OKAY ) {
						// decode block
						eob = jpg::decode::block_seq( huffr,
						                              jpg::htrees[ 0 ][ cmpnfo[cmp].huffdc ],
						                              jpg::htrees[ 1 ][ cmpnfo[cmp].huffdc ],
						                              block );
						
						// check for non optimal coding
						if ( ( eob > 1 ) && ( block[ eob - 1 ] == 0 ) ) {
							sprintf( errormessage, "reconstruction of inefficient coding not supported" );
							errorlevel = 1;
						}
						
						// fix dc
						block[ 0 ] += lastdc[ cmp ];
						lastdc[ cmp ] = block[ 0 ];
						
						// copy to dct::colldata
						for (int bpos = 0; bpos < eob; bpos++ )
							dct::colldata[ cmp ][ bpos ][ dpos ] = block[ bpos ];
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
						else status = jpg::next_mcupos(&mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
				else if ( curr_scan::sah == 0 ) {
					// ---> progressive interleaved DC decoding <---
					// ---> succesive approximation first stage <---
					while ( status == jpg::CodingStatus::OKAY ) {
						status = jpg::decode::dc_prg_fs( huffr,
						                                 jpg::htrees[0][cmpnfo[cmp].huffdc],
						                                 block );
						
						// fix dc for diff coding
						dct::colldata[cmp][0][dpos] = block[0] + lastdc[ cmp ];
						lastdc[ cmp ] = dct::colldata[cmp][0][dpos];
						
						// bitshift for succesive approximation
						dct::colldata[cmp][0][dpos] <<= curr_scan::sal;
						
						// next mcupos if no error happened
						if ( status != jpg::CodingStatus::ERROR )
							status = jpg::next_mcupos( &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
				else {
					// ---> progressive interleaved DC decoding <---
					// ---> succesive approximation later stage <---					
					while ( status == jpg::CodingStatus::OKAY ) {
						// decode next bit
						jpg::decode::dc_prg_sa(huffr, block);
						
						// shift in next bit
						dct::colldata[cmp][0][dpos] += block[0] << curr_scan::sal;
						
						status = jpg::next_mcupos(&mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
			}
			else // decoding for non interleaved data
			{
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential non interleaved decoding <---
					while ( status == jpg::CodingStatus::OKAY ) {
						// decode block
						eob = jpg::decode::block_seq( huffr,
						                              jpg::htrees[ 0 ][ cmpnfo[cmp].huffdc ],
						                              jpg::htrees[ 1 ][ cmpnfo[cmp].huffdc ],
						                              block );
						
						// check for non optimal coding
						if ( ( eob > 1 ) && ( block[ eob - 1 ] == 0 ) ) {
							sprintf( errormessage, "reconstruction of inefficient coding not supported" );
							errorlevel = 1;
						}
						
						// fix dc
						block[ 0 ] += lastdc[ cmp ];
						lastdc[ cmp ] = block[ 0 ];
						
						// copy to dct::colldata
						for (int bpos = 0; bpos < eob; bpos++ )
							dct::colldata[ cmp ][ bpos ][ dpos ] = block[ bpos ];
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
						else status = jpg::next_mcuposn(cmp, &dpos, &rstw);
					}
				}
				else if ( curr_scan::to == 0 ) {					
					if ( curr_scan::sah == 0 ) {
						// ---> progressive non interleaved DC decoding <---
						// ---> succesive approximation first stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							status = jpg::decode::dc_prg_fs( huffr,
							                                 jpg::htrees[0][cmpnfo[cmp].huffdc],
							                                 block );
								
							// fix dc for diff coding
							dct::colldata[cmp][0][dpos] = block[0] + lastdc[ cmp ];
							lastdc[ cmp ] = dct::colldata[cmp][0][dpos];
							
							// bitshift for succesive approximation
							dct::colldata[cmp][0][dpos] <<= curr_scan::sal;
							
							// check for errors, increment dpos otherwise
							if ( status != jpg::CodingStatus::ERROR )
								status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}
					}
					else {
						// ---> progressive non interleaved DC decoding <---
						// ---> succesive approximation later stage <---
						while( status == jpg::CodingStatus::OKAY ) {
							// decode next bit
							jpg::decode::dc_prg_sa(huffr, block);
							
							// shift in next bit
							dct::colldata[cmp][0][dpos] += block[0] << curr_scan::sal;
							
							// increment dpos
							status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}
					}
				}
				else {
					if ( curr_scan::sah == 0 ) {
						// ---> progressive non interleaved AC decoding <---
						// ---> succesive approximation first stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							if ( eobrun == 0 ) {
								// decode block
								eob = jpg::decode::ac_prg_fs( huffr,
								                              jpg::htrees[1][cmpnfo[cmp].huffac],
								                              block, &eobrun, curr_scan::from, curr_scan::to );
								
								if ( eobrun > 0 ) {
									// check for non optimal coding
									if ( ( eob == curr_scan::from )  && ( peobrun > 0 ) &&
										( peobrun <	jpg::hcodes[ 1 ][ cmpnfo[cmp].huffac ].max_eobrun - 1 ) ) {
										sprintf( errormessage,
											"reconstruction of inefficient coding not supported" );
										errorlevel = 1;
									}
									peobrun = eobrun;
									eobrun--;
								} else peobrun = 0;
							
								// copy to colldata
								for (int bpos = curr_scan::from; bpos < eob; bpos++)
									dct::colldata[ cmp ][ bpos ][ dpos ] = block[ bpos ] << curr_scan::sal;
							} else eobrun--;
							
							// check for errors
							if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
							else status = jpg::decode::skip_eobrun(cmp, &dpos, &rstw, &eobrun);
							
							// proceed only if no error encountered
							if ( status == jpg::CodingStatus::OKAY )
								status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}
					}
					else {
						// ---> progressive non interleaved AC decoding <---
						// ---> succesive approximation later stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							// copy from colldata
							for (int bpos = curr_scan::from; bpos <= curr_scan::to; bpos++)
								block[ bpos ] = dct::colldata[ cmp ][ bpos ][ dpos ];
							
							if ( eobrun == 0 ) {
								// decode block (long routine)
								eob = jpg::decode::ac_prg_sa( huffr,
								                              jpg::htrees[1][cmpnfo[cmp].huffac],
								                              block, &eobrun, curr_scan::from, curr_scan::to );
								
								if ( eobrun > 0 ) {
									// check for non optimal coding
									if ( ( eob == curr_scan::from ) && ( peobrun > 0 ) &&
										( peobrun < jpg::hcodes[ 1 ][ cmpnfo[cmp].huffac ].max_eobrun - 1 ) ) {
										sprintf( errormessage,
											"reconstruction of inefficient coding not supported" );
										errorlevel = 1;
									}
									
									// store eobrun
									peobrun = eobrun;
									eobrun--;
								} else peobrun = 0;
							}
							else {
								// decode block (short routine)
								jpg::decode::eobrun_sa(huffr, block, &eobrun, curr_scan::from, curr_scan::to);
								eob = 0;
								eobrun--;
							}
								
							// copy back to colldata
							for (int bpos = curr_scan::from; bpos <= curr_scan::to; bpos++)
								dct::colldata[ cmp ][ bpos ][ dpos ] += block[ bpos ] << curr_scan::sal;
							
							// proceed only if no error encountered
							if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
							else status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}
					}
				}
			}			
			
			// unpad huffman reader / check jpg::padbit
			if ( jpg::padbit != -1 ) {
				if ( jpg::padbit != huffr->unpad( jpg::padbit ) ) {
					sprintf( errormessage, "inconsistent use of jpg::padbits" );
					jpg::padbit = 1;
					errorlevel = 1;
				}
			}
			else {
				jpg::padbit = huffr->unpad( jpg::padbit );
			}
			
			// evaluate status
			if ( status == jpg::CodingStatus::ERROR ) {
				sprintf( errormessage, "decode error in scan%i / mcu%i",
					jpg::scan_count, ( curr_scan::cmpc > 1 ) ? mcu : dpos );
				errorlevel = 2;
				return false;
			}
			else if ( status == jpg::CodingStatus::DONE ) {
				jpg::scan_count++; // increment scan counter
				break; // leave decoding loop, everything is done here
			}
		}
	}
	
	// check for missing data
	if ( huffr->peof() > 0 ) {
		sprintf( errormessage, "coded image data truncated / too short" );
		errorlevel = 1;
	}
	
	// check for surplus data
	if ( !huffr->eof()) {
		sprintf( errormessage, "surplus data found after coded image data" );
		errorlevel = 1;
	}
	
	
	return true;
}

bool jpg::encode::recode()
{	
	int hpos = 0; // current position in header
	
	std::array<std::int16_t, 64> block; // store block for coeffs
	
	// open huffman coded image data in abitwriter
	auto huffw = std::make_unique<abitwriter>(0); // bitwise writer for image data
	huffw->set_fillbit( jpg::padbit );
	
	// init storage writer
	auto storw = std::make_unique<abytewriter>(0); // bytewise writer for storage of correction bits
	
	// preset count of scans and restarts
	jpg::scan_count = 0;
	int rstc = 0; // count of restart markers
	
	// JPEG decompression loop
	while ( true )
	{
		// seek till start-of-scan, parse only DHT, DRI and SOS
		std::uint8_t type; // type of current marker segment
		for ( type = 0x00; type != 0xDA; ) {
			if (hpos >= hdrs) {
				break;
			}
			type = hdrdata[ hpos + 1 ];
			std::uint32_t len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] ); // length of current marker segment
			if ( ( type == 0xC4 ) || ( type == 0xDA ) || ( type == 0xDD ) ) {
				if ( !jpg::jfif::parse_jfif( type, len, &( hdrdata[ hpos ] ) ) ) {
					return false;
				}
				hpos += len;
			}
			else {
				hpos += len;
				continue;
			}			
		}
		
		// get out if last marker segment type was not SOS
		if ( type != 0xDA ) break;
		
		
		// (re)alloc scan positons array
		jpg::scnp.resize(jpg::scan_count + 2);
		
		// (re)alloc restart marker positons array if needed
		if ( jpg::rsti > 0 ) {
			int tmp = rstc + ( ( curr_scan::cmpc > 1 ) ?
				( image::mcuc / jpg::rsti ) : ( cmpnfo[ curr_scan::cmp[ 0 ] ].bc / jpg::rsti ) );
			jpg::rstp.resize(tmp + 1);
		}		
		
		// intial variables set for encoding
		int cmp  = curr_scan::cmp[ 0 ];
		int csc  = 0;
		int mcu  = 0;
		int sub  = 0;
		int dpos = 0;
		
		// store scan position
		jpg::scnp[ jpg::scan_count ] = huffw->getpos();
		
		// JPEG imagedata encoding routines
		while ( true )
		{
			// (re)set last DCs for diff coding
			std::array<int, 4> lastdc = { 0 }; // last dc for each component
			
			// (re)set status
			jpg::CodingStatus status = jpg::CodingStatus::OKAY;
			
			// (re)set eobrun
			int eobrun = 0; // run of eobs
			
			// (re)set rst wait counter
			int rstw = jpg::rsti; // restart wait counter
			
			// encoding for interleaved data
			if ( curr_scan::cmpc > 1 )
			{				
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential interleaved encoding <---
					while ( status == jpg::CodingStatus::OKAY ) {
						// copy from colldata
						for (int bpos = 0; bpos < 64; bpos++)
							block[ bpos ] = dct::colldata[ cmp ][ bpos ][ dpos ];
						
						// diff coding for dc
						block[ 0 ] -= lastdc[ cmp ];
						lastdc[ cmp ] = dct::colldata[ cmp ][ 0 ][ dpos ];
						
						// encode block
						int eob = jpg::encode::block_seq( huffw,
						                              jpg::hcodes[0][cmpnfo[cmp].huffac],
						                              jpg::hcodes[1][cmpnfo[cmp].huffac],
						                              block );
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
						else status = jpg::next_mcupos( &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
				else if ( curr_scan::sah == 0 ) {
					// ---> progressive interleaved DC encoding <---
					// ---> succesive approximation first stage <---
					while ( status == jpg::CodingStatus::OKAY ) {
						// diff coding & bitshifting for dc 
						int tmp = dct::colldata[ cmp ][ 0 ][ dpos ] >> curr_scan::sal;
						block[ 0 ] = tmp - lastdc[ cmp ];
						lastdc[ cmp ] = tmp;
						
						// encode dc
						jpg::encode::dc_prg_fs(huffw,
						                       jpg::hcodes[0][cmpnfo[cmp].huffdc],
						                       block);
						
						// next mcupos
						status = jpg::next_mcupos( &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
				else {
					// ---> progressive interleaved DC encoding <---
					// ---> succesive approximation later stage <---
					while ( status == jpg::CodingStatus::OKAY ) {
						// fetch bit from current bitplane
						block[ 0 ] = BITN(dct::colldata[ cmp ][ 0 ][ dpos ], curr_scan::sal );
						
						// encode dc correction bit
						jpg::encode::dc_prg_sa(huffw, block);
						
						status = jpg::next_mcupos( &mcu, &cmp, &csc, &sub, &dpos, &rstw );
					}
				}
			}
			else // encoding for non interleaved data
			{
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential non interleaved encoding <---
					while ( status == jpg::CodingStatus::OKAY ) {
						// copy from colldata
						for (int bpos = 0; bpos < 64; bpos++)
							block[ bpos ] = dct::colldata[ cmp ][ bpos ][ dpos ];
						
						// diff coding for dc
						block[ 0 ] -= lastdc[ cmp ];
						lastdc[ cmp ] = dct::colldata[ cmp ][ 0 ][ dpos ];
						
						// encode block
						int eob = jpg::encode::block_seq( huffw,
						                              jpg::hcodes[0][cmpnfo[cmp].huffac],
						                              jpg::hcodes[1][cmpnfo[cmp].huffac],
						                              block );
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
						else status = jpg::next_mcuposn(cmp, &dpos, &rstw);	
					}
				}
				else if ( curr_scan::to == 0 ) {
					if ( curr_scan::sah == 0 ) {
						// ---> progressive non interleaved DC encoding <---
						// ---> succesive approximation first stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							// diff coding & bitshifting for dc 
							int tmp = dct::colldata[ cmp ][ 0 ][ dpos ] >> curr_scan::sal;
							block[ 0 ] = tmp - lastdc[ cmp ];
							lastdc[ cmp ] = tmp;
							
							// encode dc
							jpg::encode::dc_prg_fs(huffw,
							                       jpg::hcodes[0][cmpnfo[cmp].huffdc],
							                       block);							
							
							// check for errors, increment dpos otherwise
							status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}
					}
					else {
						// ---> progressive non interleaved DC encoding <---
						// ---> succesive approximation later stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							// fetch bit from current bitplane
							block[ 0 ] = BITN(dct::colldata[ cmp ][ 0 ][ dpos ], curr_scan::sal );
							
							// encode dc correction bit
							jpg::encode::dc_prg_sa(huffw, block);
							
							// next mcupos if no error happened
							status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}
					}
				}
				else {
					if ( curr_scan::sah == 0 ) {
						// ---> progressive non interleaved AC encoding <---
						// ---> succesive approximation first stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							// copy from colldata
							for (int bpos = curr_scan::from; bpos <= curr_scan::to; bpos++)
								block[ bpos ] =
									fdiv2(dct::colldata[ cmp ][ bpos ][ dpos ], curr_scan::sal );
							
							// encode block
							int eob = jpg::encode::ac_prg_fs( huffw,
							                              jpg::hcodes[1][cmpnfo[cmp].huffac],
							                              block, &eobrun, curr_scan::from, curr_scan::to );
							
							// check for errors, proceed if no error encountered
							if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
							else status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}						
						
						// encode remaining eobrun
						jpg::encode::eobrun(huffw, jpg::hcodes[1][cmpnfo[cmp].huffac], &eobrun);
					}
					else {
						// ---> progressive non interleaved AC encoding <---
						// ---> succesive approximation later stage <---
						while ( status == jpg::CodingStatus::OKAY ) {
							// copy from colldata
							for (int bpos = curr_scan::from; bpos <= curr_scan::to; bpos++)
								block[ bpos ] =
									fdiv2(dct::colldata[ cmp ][ bpos ][ dpos ], curr_scan::sal );
							
							// encode block
							int eob = jpg::encode::ac_prg_sa( huffw, storw,
							                              jpg::hcodes[1][cmpnfo[cmp].huffac],
							                              block, &eobrun, curr_scan::from, curr_scan::to );
							
							// check for errors, proceed if no error encountered
							if ( eob < 0 ) status = jpg::CodingStatus::ERROR;
							else status = jpg::next_mcuposn(cmp, &dpos, &rstw);
						}						
						
						// encode remaining eobrun
						jpg::encode::eobrun(huffw, jpg::hcodes[1][cmpnfo[cmp].huffac], &eobrun);
							
						// encode remaining correction bits
						jpg::encode::crbits( huffw, storw );
					}
				}
			}
			
			// pad huffman writer
			huffw->pad();
			
			// evaluate status
			if ( status == jpg::CodingStatus::ERROR ) {
				sprintf( errormessage, "encode error in scan%i / mcu%i",
					jpg::scan_count, ( curr_scan::cmpc > 1 ) ? mcu : dpos );
				errorlevel = 2;
				return false;
			}
			else if ( status == jpg::CodingStatus::DONE ) {
				jpg::scan_count++; // increment scan counter
				break; // leave decoding loop, everything is done here
			}
			else if ( status == jpg::CodingStatus::RESTART ) {
				if ( jpg::rsti > 0 ) // store jpg::rstp & stay in the loop
					jpg::rstp[ rstc++ ] = huffw->getpos() - 1;
			}
		}
	}
	
	// safety check for error in huffwriter
	if ( huffw->error ()) {
		sprintf(errormessage, MEM_ERRMSG.c_str());
		errorlevel = 2;
		return false;
	}
	
	// get data into huffdata
	auto hdata = huffw->getptr();
	auto hdata_length = huffw->getpos();
	huffdata = std::vector<std::uint8_t>(hdata, hdata + hdata_length);
	
	// store last scan & restart positions
	jpg::scnp[ jpg::scan_count ] = huffdata.size();
	if (!jpg::rstp.empty()) {
		jpg::rstp[rstc] = huffdata.size();
	}
	
	
	return true;
}


/* -----------------------------------------------
	adapt ICOS tables for quantizer tables
	----------------------------------------------- */
	
bool dct::adapt_icos()
{
	std::array<std::uint16_t, 64> quant; // local copy of quantization	
	
	
	for (int cmp = 0; cmp < image::cmpc; cmp++ ) {
		// make a local copy of the quantization values, check
		for (int ipos = 0; ipos < 64; ipos++) {
			quant[ipos] = QUANT(cmp, zigzag[ipos]);
			if (quant[ipos] >= 2048) { // if this is true, it can be safely assumed (for 8 bit JPEG), that all coefficients are zero
				quant[ipos] = 0;
			}
		}
		// adapt idct 8x8 table
		for (int ipos = 0; ipos < 64 * 64; ipos++) {
			dct::adpt_idct_8x8[cmp][ipos] = dct::icos_idct_8x8[ipos] * quant[ipos % 64];
		}
		// adapt idct 1x8 table
		for (int ipos = 0; ipos < 8 * 8; ipos++) {
			dct::adpt_idct_1x8[cmp][ipos] = dct::icos_idct_1x8[ipos] * quant[(ipos % 8) * 8];
		}
		// adapt idct 8x1 table
		for (int ipos = 0; ipos < 8 * 8; ipos++) {
			dct::adpt_idct_8x1[cmp][ipos] = dct::icos_idct_1x8[ipos] * quant[ipos % 8];
		}
	}
	
	return true;
}


/* -----------------------------------------------
	filter DC coefficients
	----------------------------------------------- */

INTERN bool predict_dc()
{
	signed short* coef;
	int absmaxp;
	int absmaxn;
	int corr_f;
	int cmp, dpos;	
	
	
	// apply prediction, store prediction error instead of DC
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		absmaxp = MAX_V( cmp, 0 );
		absmaxn = -absmaxp;
		corr_f = ( ( 2 * absmaxp ) + 1 );
		
		for ( dpos = cmpnfo[cmp].bc - 1; dpos > 0; dpos-- )	{
			coef = &(dct::colldata[cmp][0][dpos]);
			#if defined( USE_PLOCOI )
			(*coef) -= predictor::dc_coll_predictor( cmp, dpos ); // loco-i predictor
			#else
			(*coef) -= predictor::dc_1ddct_predictor( cmp, dpos ); // 1d dct
			#endif
			
			// fix range
			if ( (*coef) > absmaxp ) (*coef) -= corr_f;
			else if ( (*coef) < absmaxn ) (*coef) += corr_f;
		}
	}
	
	return true;
}


/* -----------------------------------------------
	unpredict DC coefficients
	----------------------------------------------- */

INTERN bool unpredict_dc()
{	
	signed short* coef;
	int absmaxp;
	int absmaxn;
	int corr_f;
	int cmp, dpos;
	
	
	// remove prediction, store DC instead of prediction error
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		absmaxp = MAX_V( cmp, 0 );
		absmaxn = -absmaxp;
		corr_f = ( ( 2 * absmaxp ) + 1 );
		
		for ( dpos = 1; dpos < cmpnfo[cmp].bc; dpos++ ) {
			coef = &(dct::colldata[cmp][0][dpos]);
			#if defined( USE_PLOCOI )
			(*coef) += predictor::dc_coll_predictor( cmp, dpos ); // loco-i predictor
			#else
			(*coef) += predictor::dc_1ddct_predictor( cmp, dpos ); // 1d dct predictor
			#endif
			
			// fix range
			if ( (*coef) > absmaxp ) (*coef) -= corr_f;
			else if ( (*coef) < absmaxn ) (*coef) += corr_f;
		}
	}
	
	
	return true;
}

INTERN bool jpg::decode::check_value_range()
{
	int absmax;
	int cmp, bpos, dpos;
	
	// out of range should never happen with unmodified JPEGs
	for ( cmp = 0; cmp < image::cmpc; cmp++ )
	for ( bpos = 0; bpos < 64; bpos++ ) {
		absmax = MAX_V( cmp, bpos );
		for ( dpos = 0; dpos < cmpnfo[cmp].bc; dpos++ )
		if ( ( dct::colldata[cmp][bpos][dpos] > absmax ) ||
			 ( dct::colldata[cmp][bpos][dpos] < -absmax ) ) {
			sprintf( errormessage, "value out of range error: cmp%i, frq%i, val %i, max %i",
					cmp, bpos, dct::colldata[cmp][bpos][dpos], absmax );
			errorlevel = 2;
			return false;
		}
	}
	
	
	return true;
}


/* -----------------------------------------------
	calculate zero distribution lists
	----------------------------------------------- */
	
INTERN bool calc_zdst_lists()
{
	int cmp, bpos, dpos;
	int b_x, b_y;
	
	
	// this functions counts, for each DCT block, the number of non-zero coefficients
	for ( cmp = 0; cmp < image::cmpc; cmp++ )
	{
		// preset zdstlist
		memset( pjg::zdstdata[cmp], 0, cmpnfo[cmp].bc * sizeof( char ) );
		
		// calculate # on non-zeroes per block (separately for lower 7x7 block & first row/collumn)
		for ( bpos = 1; bpos < 64; bpos++ ) {
			b_x = unzigzag[ bpos ] % 8;
			b_y = unzigzag[ bpos ] / 8;
			if ( b_x == 0 ) {
				for ( dpos = 0; dpos < cmpnfo[cmp].bc; dpos++ )
					if (dct::colldata[cmp][bpos][dpos] != 0 ) pjg::zdstylow[cmp][dpos]++;
			}
			else if ( b_y == 0 ) {
				for ( dpos = 0; dpos < cmpnfo[cmp].bc; dpos++ )
					if (dct::colldata[cmp][bpos][dpos] != 0 ) pjg::zdstxlow[cmp][dpos]++;
			}
			else {
				for ( dpos = 0; dpos < cmpnfo[cmp].bc; dpos++ )
					if (dct::colldata[cmp][bpos][dpos] != 0 ) pjg::zdstdata[cmp][dpos]++;
			}
		}
	}
	
	
	return true;
}


/* -----------------------------------------------
	packs all parts to compressed pjg
	----------------------------------------------- */
	
bool pjg::encode::encode()
{
	unsigned char hcode;
	int cmp;
	#if defined(DEV_INFOS)
	int dev_size = 0;
	#endif
	
	
	// PJG-Header
	str_out->write(program_info::pjg_magic.data(), 2 );
	
	// store settings if not auto
	if ( !auto_set ) {
		hcode = 0x00;
		str_out->write_byte(hcode);
		str_out->write( nois_trs, 4 );
		str_out->write( segm_cnt, 4 );
	}
	
	// store version number
	hcode = program_info::appversion;
	str_out->write_byte(hcode);
	
	
	// init arithmetic compression
	auto encoder = std::make_unique<aricoder>(str_out, StreamMode::kWrite);
	
	// discard meta information from header if option set
	if ( disc_meta )
		if ( !jpg::rebuild_header() ) return false;	
	// optimize header for compression
	pjg::encode::optimize_header();
	// set padbit to 1 if previously unset
	if (jpg::padbit == -1 )	jpg::padbit = 1;
	
	// encode JPG header
	#if !defined(DEV_INFOS)	
	if ( !pjg::encode::generic( encoder, hdrdata, hdrs ) ) return false;
	#else
	dev_size = str_out->getpos();
	if ( !pjg::encode::generic( encoder, hdrdata, hdrs ) ) return false;
	dev_size_hdr += str_out->getpos() - dev_size;
	#endif
	// store padbit (padbit can't be retrieved from the header)
	pjg::encode::bit(encoder, jpg::padbit);
	// also encode one bit to signal false/correct use of RST markers
	pjg::encode::bit(encoder, jpg::rst_err.empty() ? 0 : 1);
	// encode # of false set RST markers per scan
	if ( !jpg::rst_err.empty() )
		if ( !pjg::encode::generic( encoder, jpg::rst_err.data(), jpg::scan_count ) ) return false;
	
	// encode actual components data
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		#if !defined(DEV_INFOS)
		// encode frequency scan ('zero-sort-scan')
		pjg::encode::zstscan(encoder, cmp);
		// encode zero-distribution-lists for higher (7x7) ACs
		pjg::encode::zdst_high(encoder, cmp);
		// encode coefficients for higher (7x7) ACs
		pjg::encode::ac_high(encoder, cmp);
		// encode zero-distribution-lists for lower ACs
		pjg::encode::zdst_low(encoder, cmp);
		// encode coefficients for first row / collumn ACs
		pjg::encode::ac_low(encoder, cmp);
		// encode coefficients for DC
		pjg::encode::dc(encoder, cmp);
		#else
		dev_size = str_out->getpos();
		// encode frequency scan ('zero-sort-scan')
		pjg::encode::zstscan(encoder, cmp);
		dev_size_zsr[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode zero-distribution-lists for higher (7x7) ACs
		pjg::encode::zdst_high(encoder, cmp);
		dev_size_zdh[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode coefficients for higher (7x7) ACs
		pjg::encode::ac_high(encoder, cmp);
		dev_size_ach[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode zero-distribution-lists for lower ACs
		pjg::encode::zdst_low(encoder, cmp);
		dev_size_zdl[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode coefficients for first row / collumn ACs
		pjg::encode::ac_low(encoder, cmp);
		dev_size_acl[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode coefficients for DC
		pjg::encode::dc(encoder, cmp);
		dev_size_dc[ cmp ] += str_out->getpos() - dev_size;
		dev_size_cmp[ cmp ] = 
			dev_size_zsr[ cmp ] + dev_size_zdh[ cmp ] +	dev_size_zdl[ cmp ] +
			dev_size_ach[ cmp ] + dev_size_acl[ cmp ] +	dev_size_dc[ cmp ];
		#endif
	}
	
	// encode checkbit for garbage (0 if no garbage, 1 if garbage has to be coded)
	pjg::encode::bit(encoder, !grbgdata.empty() ? 1 : 0);
	// encode garbage data only if needed
	if (!grbgdata.empty())
		if ( !pjg::encode::generic( encoder, grbgdata.data(), grbgdata.size()) ) return false;
	
	// finalize arithmetic compression
	//delete( encoder );
	
	
	// errormessage if write error
	if ( str_out->chkerr() ) {
		sprintf( errormessage, "write error, possibly drive is full" );
		errorlevel = 2;		
		return false;
	}
	
	// get filesize
	pjgfilesize = str_out->getsize();
	
	
	return true;
}


/* -----------------------------------------------
	unpacks compressed pjg to dct::colldata
	----------------------------------------------- */
	
bool pjg::decode::decode()
{
	unsigned char hcode;
	int cmp;
	
	
	// check header codes ( maybe position in other function ? )
	while( true ) {
		str_in->read_byte(&hcode);
		if ( hcode == 0x00 ) {
			// retrieve compression settings from file
			str_in->read( nois_trs, 4 );
			str_in->read( segm_cnt, 4 );
			auto_set = false;
		}
		else if ( hcode >= 0x14 ) {
			// compare version number
			if ( hcode != program_info::appversion ) {
				sprintf( errormessage, "incompatible file, use %s v%i.%i",
					program_info::appname.c_str(), hcode / 10, hcode % 10 );
				errorlevel = 2;
				return false;
			}
			else break;
		}
		else {
			sprintf( errormessage, "unknown header code, use newer version of %s", program_info::appname.c_str());
			errorlevel = 2;
			return false;
		}
	}
	
	
	// init arithmetic compression
	auto decoder = std::make_unique<aricoder>(str_in, StreamMode::kRead);
	
	// decode JPG header
	if ( !pjg::decode::generic( decoder, &hdrdata, &hdrs ) ) return false;
	// retrieve padbit from stream
	jpg::padbit = pjg::decode::bit(decoder);
	// decode one bit that signals false /correct use of RST markers
	auto cb = pjg::decode::bit(decoder);
	// decode # of false set RST markers per scan only if available
	if ( cb == 1 ) {
		jpg::rst_err = pjg::decode::generic(decoder);
	}
	
	// undo header optimizations
	pjg::decode::deoptimize_header();
	// discard meta information from header if option set
	if ( disc_meta )
		if ( !jpg::rebuild_header() ) return false;
	// parse header for image-info
	if ( !jpg::setup_imginfo() ) return false;
	
	// decode actual components data
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		// decode frequency scan ('zero-sort-scan')
		pjg::decode::zstscan(decoder, cmp);
		// decode zero-distribution-lists for higher (7x7) ACs
		pjg::decode::zdst_high(decoder, cmp);
		// decode coefficients for higher (7x7) ACs
		pjg::decode::ac_high(decoder, cmp);
		// decode zero-distribution-lists for lower ACs
		pjg::decode::zdst_low(decoder, cmp);
		// decode coefficients for first row / collumn ACs
		pjg::decode::ac_low(decoder, cmp);
		// decode coefficients for DC
		pjg::decode::dc(decoder, cmp);
	}
	
	// retrieve checkbit for garbage (0 if no garbage, 1 if garbage has to be coded)
	auto garbage_exists = pjg::decode::bit(decoder);
	
	// decode garbage data only if available
	if (garbage_exists != 0) {
		grbgdata = pjg::decode::generic(decoder);
	}
	
	// finalize arithmetic compression
	//delete( decoder );
	
	
	// get filesize
	pjgfilesize = str_in->getsize();
	
	
	return true;
}

/* ----------------------- End of main functions -------------------------- */

/* ----------------------- Begin of JPEG specific functions -------------------------- */

bool jpg::setup_imginfo()
{
	unsigned char  type = 0x00; // type of current marker segment
	unsigned int   len  = 0; // length of current marker segment
	unsigned int   hpos = 0; // position in header
	
	int cmp, bpos;
	int i;
	
	// header parser loop
	while ( ( int ) hpos < hdrs ) {
		type = hdrdata[ hpos + 1 ];
		len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] );
		// do not parse DHT & DRI
		if ( ( type != 0xDA ) && ( type != 0xC4 ) && ( type != 0xDD ) ) {
			if ( !jpg::jfif::parse_jfif( type, len, &( hdrdata[ hpos ] ) ) )
				return false;
		}
		hpos += len;
	}
	
	// check if information is complete
	if (image::cmpc == 0 ) {
		sprintf( errormessage, "header contains incomplete information" );
		errorlevel = 2;
		return false;
	}
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		if ( ( cmpnfo[cmp].sfv == 0 ) ||
			 ( cmpnfo[cmp].sfh == 0 ) ||
			 ( cmpnfo[cmp].qtable == nullptr ) ||
			 ( cmpnfo[cmp].qtable[0] == 0 ) ||
			 ( jpegtype == JpegType::UNKNOWN ) ) {
			sprintf( errormessage, "header information is incomplete" );
			errorlevel = 2;
			return false;
		}
	}
	
	// do all remaining component info calculations
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		if ( cmpnfo[ cmp ].sfh > image::sfhm ) image::sfhm = cmpnfo[ cmp ].sfh;
		if ( cmpnfo[ cmp ].sfv > image::sfvm ) image::sfvm = cmpnfo[ cmp ].sfv;
	}
	image::mcuv = ( int ) ceil( (float)image::imgheight / (float) ( 8 * image::sfhm ) );
	image::mcuh = ( int ) ceil( (float)image::imgwidth  / (float) ( 8 * image::sfvm ) );
	image::mcuc  = image::mcuv * image::mcuh;
	for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
		cmpnfo[ cmp ].mbs = cmpnfo[ cmp ].sfv * cmpnfo[ cmp ].sfh;		
		cmpnfo[ cmp ].bcv = image::mcuv * cmpnfo[ cmp ].sfh;
		cmpnfo[ cmp ].bch = image::mcuh * cmpnfo[ cmp ].sfv;
		cmpnfo[ cmp ].bc  = cmpnfo[ cmp ].bcv * cmpnfo[ cmp ].bch;
		cmpnfo[ cmp ].ncv = ( int ) ceil( (float)image::imgheight *
							( (float) cmpnfo[ cmp ].sfh / ( 8.0 * image::sfhm ) ) );
		cmpnfo[ cmp ].nch = ( int ) ceil( (float)image::imgwidth *
							( (float) cmpnfo[ cmp ].sfv / ( 8.0 * image::sfvm ) ) );
		cmpnfo[ cmp ].nc  = cmpnfo[ cmp ].ncv * cmpnfo[ cmp ].nch;
	}
	
	// decide components' statistical ids
	if (image::cmpc <= 3 ) {
		for ( cmp = 0; cmp < image::cmpc; cmp++ ) cmpnfo[ cmp ].sid = cmp;
	}
	else {
		for ( cmp = 0; cmp < image::cmpc; cmp++ ) cmpnfo[ cmp ].sid = 0;
	}
	
	// alloc memory for further operations
	for ( cmp = 0; cmp < image::cmpc; cmp++ )
	{
		// alloc memory for colls
		for ( bpos = 0; bpos < 64; bpos++ ) {
			dct::colldata[cmp][bpos] = (short int*) calloc ( cmpnfo[cmp].bc, sizeof( short ) );
			if (dct::colldata[cmp][bpos] == nullptr) {
				sprintf( errormessage, MEM_ERRMSG.c_str() );
				errorlevel = 2;
				return false;
			}
		}
		
		// alloc memory for zdstlist / eob x / eob y
		pjg::zdstdata[cmp] = (unsigned char*) calloc( cmpnfo[cmp].bc, sizeof( char ) );
		pjg::eobxhigh[cmp] = (unsigned char*) calloc( cmpnfo[cmp].bc, sizeof( char ) );
		pjg::eobyhigh[cmp] = (unsigned char*) calloc( cmpnfo[cmp].bc, sizeof( char ) );
		pjg::zdstxlow[cmp] = (unsigned char*) calloc( cmpnfo[cmp].bc, sizeof( char ) );
		pjg::zdstylow[cmp] = (unsigned char*) calloc( cmpnfo[cmp].bc, sizeof( char ) );
		if ( ( pjg::zdstdata[cmp] == nullptr) ||
			( pjg::eobxhigh[cmp] == nullptr) || ( pjg::eobyhigh[cmp] == nullptr) ||
			( pjg::zdstxlow[cmp] == nullptr) || ( pjg::zdstylow[cmp] == nullptr) ) {
			sprintf( errormessage, MEM_ERRMSG.c_str());
			errorlevel = 2;
			return false;
		}
	}
	
	// also decide automatic settings here
	if ( auto_set ) {
		for ( cmp = 0; cmp < image::cmpc; cmp++ ) {
			for ( i = 0;
				conf_sets[ i ][ cmpnfo[cmp].sid ] > (unsigned int) cmpnfo[ cmp ].bc;
				i++ );
			segm_cnt[ cmp ] = conf_segm;
			nois_trs[ cmp ] = conf_ntrs[ i ][ cmpnfo[cmp].sid ];
		}
	}
	
	
	return true;
}

// Builds Huffman trees and codes.
bool jpg::jfif::parse_dht(unsigned char type, unsigned int len, const unsigned char* segment) {
	int hpos = 4; // current position in segment, start after segment header
	// build huffman trees & codes
	while (hpos < len) {
		int lval = LBITS(segment[hpos], 4);
		int rval = RBITS(segment[hpos], 4);
		if (lval < 0 || lval >= 2 || rval < 0 || rval >= 4) {
			break;
		}

		hpos++;
		// build huffman codes & trees
		jpg::hcodes[lval][rval] = jpg::jfif::build_huffcodes(&(segment[hpos + 0]), &(segment[hpos + 16]));
		jpg::htrees[lval][rval] = jpg::jfif::build_hufftree(jpg::hcodes[lval][rval]);
		jpg::htset[lval][rval] = true;

		int skip = 16;
		for (int i = 0; i < 16; i++) {
			skip += (int)segment[hpos + i];
		}
		hpos += skip;
	}

	if (hpos != len) {
		// if we get here, something went wrong
		sprintf(errormessage, "size mismatch in dht marker");
		errorlevel = 2;
		return false;
	}
	return true;
}

// Copy quantization tables to internal memory
bool jpg::jfif::parse_dqt(unsigned len, const unsigned char* segment) {
	int hpos = 4; // current position in segment, start after segment header
	while (hpos < len) {
		int lval = LBITS( segment[ hpos ], 4 );
		int rval = RBITS( segment[ hpos ], 4 );
		if (lval < 0 || lval >= 2) {
			break;
		}
		if (rval < 0 || rval >= 4) {
			break;
		}
		hpos++;
		if (lval == 0) { // 8 bit precision
			for (int i = 0; i < 64; i++) {
				qtables[rval][i] = (unsigned short) segment[hpos + i];
				if (qtables[rval][i] == 0) {
					break;
				}
			}
			hpos += 64;
		} else { // 16 bit precision
			for (int i = 0; i < 64; i++) {
				qtables[rval][i] =
					pack( segment[ hpos + (2*i) ], segment[ hpos + (2*i) + 1 ] );
				if (qtables[rval][i] == 0) {
					break;
				}
			}
			hpos += 128;
		}
	}

	if (hpos != len) {
		// if we get here, something went wrong
		sprintf(errormessage, "size mismatch in dqt marker");
		errorlevel = 2;
		return false;
	}
	return true;
}

// define restart interval
void jpg::jfif::parse_dri(const unsigned char* segment) {
	int hpos = 4; // current position in segment, start after segment header
	jpg::rsti = pack( segment[ hpos ], segment[ hpos + 1 ] );
}

bool jpg::jfif::parse_sof(unsigned char type, const unsigned char* segment) {
	int hpos = 4; // current position in segment, start after segment header

	// set JPEG coding type
	if (type == 0xC2) {
		jpegtype = JpegType::PROGRESSIVE;
	} else {
		jpegtype = JpegType::SEQUENTIAL;
	}

	// check data precision, only 8 bit is allowed
	int lval = segment[hpos];
	if (lval != 8) {
		sprintf(errormessage, "%i bit data precision is not supported", lval);
		errorlevel = 2;
		return false;
	}

	// image size, height & component count
	image::imgheight = pack(segment[hpos + 1], segment[hpos + 2]);
	image::imgwidth = pack(segment[hpos + 3], segment[hpos + 4]);
	image::cmpc = segment[hpos + 5];
	if ((image::imgwidth == 0) || (image::imgheight == 0)) {
		sprintf(errormessage, "resolution is %ix%i, possible malformed JPEG", image::imgwidth, image::imgheight);
		errorlevel = 2;
		return false;
	}
	if (image::cmpc > 4) {
		sprintf(errormessage, "image has %i components, max 4 are supported", image::cmpc);
		errorlevel = 2;
		return false;
	}

	hpos += 6;
	// components contained in image
	for (int cmp = 0; cmp < image::cmpc; cmp++) {
		cmpnfo[cmp].jid = segment[hpos];
		cmpnfo[cmp].sfv = LBITS(segment[hpos + 1], 4);
		cmpnfo[cmp].sfh = RBITS(segment[hpos + 1], 4);
		cmpnfo[cmp].qtable = qtables[segment[hpos + 2]];
		hpos += 3;
	}

	return true;
}

bool jpg::jfif::parse_sos(const unsigned char* segment) {
	int hpos = 4; // current position in segment, start after segment header
	curr_scan::cmpc = segment[hpos];
	if (curr_scan::cmpc > image::cmpc) {
		sprintf(errormessage, "%i components in scan, only %i are allowed",
			curr_scan::cmpc, image::cmpc);
		errorlevel = 2;
		return false;
	}
	hpos++;
	for (int i = 0; i < curr_scan::cmpc; i++) {
		int cmp;
		for (cmp = 0; (segment[hpos] != cmpnfo[cmp].jid) && (cmp < image::cmpc); cmp++);
		if (cmp == image::cmpc) {
			sprintf(errormessage, "component id mismatch in start-of-scan");
			errorlevel = 2;
			return false;
		}
		curr_scan::cmp[i] = cmp;
		cmpnfo[cmp].huffdc = LBITS(segment[hpos + 1], 4);
		cmpnfo[cmp].huffac = RBITS(segment[hpos + 1], 4);
		if ((cmpnfo[cmp].huffdc < 0) || (cmpnfo[cmp].huffdc >= 4) ||
			(cmpnfo[cmp].huffac < 0) || (cmpnfo[cmp].huffac >= 4)) {
			sprintf(errormessage, "huffman table number mismatch");
			errorlevel = 2;
			return false;
		}
		hpos += 2;
	}
	curr_scan::from = segment[hpos + 0];
	curr_scan::to = segment[hpos + 1];
	curr_scan::sah = LBITS(segment[hpos + 2], 4);
	curr_scan::sal = RBITS(segment[hpos + 2], 4);
	// check for errors
	if ((curr_scan::from > curr_scan::to) || (curr_scan::from > 63) || (curr_scan::to > 63)) {
		sprintf(errormessage, "spectral selection parameter out of range");
		errorlevel = 2;
		return false;
	}
	if ((curr_scan::sah >= 12) || (curr_scan::sal >= 12)) {
		sprintf(errormessage, "successive approximation parameter out of range");
		errorlevel = 2;
		return false;
	}
	return true;
}

bool jpg::jfif::parse_jfif(unsigned char type, unsigned int len, const unsigned char* segment)
{
	
	switch ( type )
	{
		case 0xC4: // DHT segment
			return jpg::jfif::parse_dht(type, len, segment);
		
		case 0xDB: // DQT segment
			return jpg::jfif::parse_dqt(len, segment);
			
		case 0xDD: // DRI segment
			jpg::jfif::parse_dri(segment);
			return true;
			
		case 0xDA: // SOS segment
			// prepare next scan
			return jpg::jfif::parse_sos(segment);
		
		case 0xC0: // SOF0 segment
			// coding process: baseline DCT
			
		case 0xC1: // SOF1 segment
			// coding process: extended sequential DCT
		
		case 0xC2: // SOF2 segment
			// coding process: progressive DCT

			return jpg::jfif::parse_sof(type, segment);
		
		case 0xC3: // SOF3 segment
			// coding process: lossless sequential
			sprintf( errormessage, "sof3 marker found, image is coded lossless" );
			errorlevel = 2;
			return false;
		
		case 0xC5: // SOF5 segment
			// coding process: differential sequential DCT
			sprintf( errormessage, "sof5 marker found, image is coded diff. sequential" );
			errorlevel = 2;
			return false;
		
		case 0xC6: // SOF6 segment
			// coding process: differential progressive DCT
			sprintf( errormessage, "sof6 marker found, image is coded diff. progressive" );
			errorlevel = 2;
			return false;
		
		case 0xC7: // SOF7 segment
			// coding process: differential lossless
			sprintf( errormessage, "sof7 marker found, image is coded diff. lossless" );
			errorlevel = 2;
			return false;
			
		case 0xC9: // SOF9 segment
			// coding process: arithmetic extended sequential DCT
			sprintf( errormessage, "sof9 marker found, image is coded arithm. sequential" );
			errorlevel = 2;
			return false;
			
		case 0xCA: // SOF10 segment
			// coding process: arithmetic extended sequential DCT
			sprintf( errormessage, "sof10 marker found, image is coded arithm. progressive" );
			errorlevel = 2;
			return false;
			
		case 0xCB: // SOF11 segment
			// coding process: arithmetic extended sequential DCT
			sprintf( errormessage, "sof11 marker found, image is coded arithm. lossless" );
			errorlevel = 2;
			return false;
			
		case 0xCD: // SOF13 segment
			// coding process: arithmetic differntial sequential DCT
			sprintf( errormessage, "sof13 marker found, image is coded arithm. diff. sequential" );
			errorlevel = 2;
			return false;
			
		case 0xCE: // SOF14 segment
			// coding process: arithmetic differential progressive DCT
			sprintf( errormessage, "sof14 marker found, image is coded arithm. diff. progressive" );
			errorlevel = 2;
			return false;
		
		case 0xCF: // SOF15 segment
			// coding process: arithmetic differntial lossless
			sprintf( errormessage, "sof15 marker found, image is coded arithm. diff. lossless" );
			errorlevel = 2;
			return false;
			
		case 0xE0: // APP0 segment	
		case 0xE1: // APP1 segment
		case 0xE2: // APP2 segment
		case 0xE3: // APP3 segment
		case 0xE4: // APP4 segment
		case 0xE5: // APP5 segment
		case 0xE6: // APP6 segment
		case 0xE7: // APP7 segment
		case 0xE8: // APP8 segment
		case 0xE9: // APP9 segment
		case 0xEA: // APP10 segment
		case 0xEB: // APP11 segment
		case 0xEC: // APP12 segment
		case 0xED: // APP13 segment
		case 0xEE: // APP14 segment
		case 0xEF: // APP15 segment
		case 0xFE: // COM segment
			// do nothing - return true
			return true;
			
		case 0xD0: // RST0 segment
		case 0xD1: // RST1 segment
		case 0xD2: // RST2 segment
		case 0xD3: // RST3 segment
		case 0xD4: // RST4 segment
		case 0xD5: // RST5 segment
		case 0xD6: // RST6 segment
		case 0xD7: // RST7 segment
			// return errormessage - RST is out of place here
			sprintf( errormessage, "rst marker found out of place" );
			errorlevel = 2;
			return false;
		
		case 0xD8: // SOI segment
			// return errormessage - start-of-image is out of place here
			sprintf( errormessage, "soi marker found out of place" );
			errorlevel = 2;
			return false;
		
		case 0xD9: // EOI segment
			// return errormessage - end-of-image is out of place here
			sprintf( errormessage, "eoi marker found out of place" );
			errorlevel = 2;
			return false;
			
		default: // unknown marker segment
			// return warning
			sprintf( errormessage, "unknown marker found: FF %2X", type );
			errorlevel = 1;
			return true;
	}
}

bool jpg::rebuild_header()
{		
	unsigned char  type = 0x00; // type of current marker segment
	unsigned int   len  = 0; // length of current marker segment
	unsigned int   hpos = 0; // position in header	
	
	
	// start headerwriter
	auto hdrw = std::make_unique<abytewriter>( 4096 ); // new header writer
	
	// header parser loop
	while ( ( int ) hpos < hdrs ) {
		type = hdrdata[ hpos + 1 ];
		len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] );
		// discard any unneeded meta info
		if ( ( type == 0xDA ) || ( type == 0xC4 ) || ( type == 0xDB ) ||
			 ( type == 0xC0 ) || ( type == 0xC1 ) || ( type == 0xC2 ) ||
			 ( type == 0xDD ) ) {
			hdrw->write_n( &(hdrdata[ hpos ]), len );
		}
		hpos += len;
	}
	
	// replace current header with the new one
	free( hdrdata );
	hdrdata = hdrw->getptr();
	hdrs    = hdrw->getpos();	
	
	return true;
}

int jpg::decode::block_seq(const std::unique_ptr<abitreader>& huffr, const HuffTree& dctree, const HuffTree& actree, short* block)
{
	unsigned short n;
	unsigned char  s;
	unsigned char  z;
	int eob = 64;
	int bpos;
	int hc;
	
	
	// decode dc
	if (jpg::decode::dc_prg_fs(huffr, dctree, block) == jpg::CodingStatus::ERROR) {
		return -1; // Return error
	}
	
	// decode ac
	for ( bpos = 1; bpos < 64; )
	{
		// decode next
		hc = jpg::decode::next_huffcode( huffr, actree );
		// analyse code
		if ( hc > 0 ) {
			z = LBITS( hc, 4 );
			s = RBITS( hc, 4 );
			n = huffr->read( s );
			if ( ( z + bpos ) >= 64 )
				return -1; // run is to long
			while ( z > 0 ) { // write zeroes
				block[ bpos++ ] = 0;
				z--;
			}
			block[ bpos++ ] = ( short ) devli( s, n ); // decode cvli
		}
		else if ( hc == 0 ) { // EOB
			eob = bpos;			
			// while( bpos < 64 ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		}
		else {
			return -1; // return error
		}
	}
	
	
	// return position of eob
	return eob;
}

int jpg::encode::block_seq(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& dctbl, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block)
{
	// encode DC
	jpg::encode::dc_prg_fs(huffw, dctbl, block);

	// encode AC
	int z = 0;
	for (int bpos = 1; bpos < 64; bpos++) {
		// if nonzero is encountered
		if (block[bpos] != 0) {
			// write remaining zeroes
			while (z >= 16) {
				huffw->write(actbl.cval[0xF0], actbl.clen[0xF0]);
				z -= 16;
			}
			// vli encode
			int s = bitlen2048n( block[ bpos ] );
			std::uint16_t n = envli( s, block[ bpos ] );
			int hc = ((z << 4) + s);
			// write to huffman writer
			huffw->write(actbl.cval[hc], actbl.clen[hc]);
			huffw->write(n, s);
			// reset zeroes
			z = 0;
		} else { // increment zero counter
			z++;
		}
	}
	// write eob if needed
	if (z > 0) {
		huffw->write(actbl.cval[0x00], actbl.clen[0x00]);
	}

	return 64 - z;
}

jpg::CodingStatus jpg::decode::dc_prg_fs(const std::unique_ptr<abitreader>& huffr, const HuffTree& dctree, short* block)
{
	// decode dc
	int hc = jpg::decode::next_huffcode(huffr, dctree);
	if (hc < 0) {
		return jpg::CodingStatus::ERROR; // return error
	}
	int s = hc;
	std::uint16_t n = huffr->read(s);
	block[0] = devli( s, n );

	// return 0 if everything is ok
	return jpg::CodingStatus::OKAY;
}

void jpg::encode::dc_prg_fs(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& dctbl, const std::array<std::int16_t, 64>& block)
{
	// encode DC	
	int s = bitlen2048n( block[ 0 ] );
	std::uint16_t n = envli( s, block[ 0 ] );
	huffw->write(dctbl.cval[s], dctbl.clen[s]);
	huffw->write(n, s);
}

int jpg::decode::ac_prg_fs(const std::unique_ptr<abitreader>& huffr, const HuffTree& actree, short* block, int* eobrun, int from, int to)
{
	unsigned short n;
	unsigned char  s;
	unsigned char  z;
	int eob = to + 1;
	int bpos;
	int hc;
	int l;
	int r;
	
	
	// decode ac
	for ( bpos = from; bpos <= to; )
	{
		// decode next
		hc = jpg::decode::next_huffcode( huffr, actree );
		if ( hc < 0 ) return -1;
		l = LBITS( hc, 4 );
		r = RBITS( hc, 4 );
		// analyse code
		if ( ( l == 15 ) || ( r > 0 ) ) { // decode run/level combination
			z = l;
			s = r;
			n = huffr->read( s );
			if ( ( z + bpos ) > to )
				return -1; // run is to long			
			while ( z > 0 ) { // write zeroes
				block[ bpos++ ] = 0;
				z--;
			}			
			block[ bpos++ ] = ( short ) devli( s, n ); // decode cvli
		}
		else { // decode eobrun
			eob = bpos;
			s = l;
			n = huffr->read( s );
			(*eobrun) = e_devli( s, n );			
			// while( bpos <= to ) // fill remaining block with zeroes
			//	block[ bpos++ ] = 0;
			break;
		}
	}
	
	
	// return position of eob
	return eob;
}

int jpg::encode::ac_prg_fs(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block, int* eobrun, int from, int to)
{
	unsigned short n;
	unsigned char  s;
	unsigned char  z;
	int bpos;
	int hc;
	
	// encode AC
	z = 0;
	for ( bpos = from; bpos <= to; bpos++ )
	{
		// if nonzero is encountered
		if ( block[ bpos ] != 0 ) {
			// encode eobrun
			jpg::encode::eobrun(huffw, actbl, eobrun);
			// write remaining zeroes
			while ( z >= 16 ) {
				huffw->write( actbl.cval[ 0xF0 ], actbl.clen[ 0xF0 ] );
				z -= 16;
			}			
			// vli encode
			s = bitlen2048n( block[ bpos ] );
			n = envli( s, block[ bpos ] );
			hc = ( ( z << 4 ) + s );
			// write to huffman writer
			huffw->write( actbl.cval[ hc ], actbl.clen[ hc ] );
			huffw->write( n, s );
			// reset zeroes
			z = 0;
		}
		else { // increment zero counter
			z++;
		}
	}
	
	// check eob, increment eobrun if needed
	if ( z > 0 ) {
		(*eobrun)++;
		// check eobrun, encode if needed
		if ( (*eobrun) == actbl.max_eobrun )
			jpg::encode::eobrun(huffw, actbl, eobrun);
		return 1 + to - z;		
	}
	else {
		return 1 + to;
	}
}

void jpg::decode::dc_prg_sa(const std::unique_ptr<abitreader>& huffr, short* block)
{
	// decode next bit of dc coefficient
	block[ 0 ] = huffr->read_bit();
}

void jpg::encode::dc_prg_sa(const std::unique_ptr<abitwriter>& huffw, const std::array<std::int16_t, 64>& block)
{
	// enocode next bit of dc coefficient
	huffw->write_bit(block[0]);
}

int jpg::decode::ac_prg_sa(const std::unique_ptr<abitreader>& huffr, const HuffTree& actree, short* block, int* eobrun, int from, int to)
{
	unsigned short n;
	unsigned char  s;
	signed char    z;
	signed char    v;
	int bpos = from;
	int eob = to;
	int hc;
	int l;
	int r;
	
	
	// decode AC succesive approximation bits
	if ( (*eobrun) == 0 ) while ( bpos <= to )
	{
		// decode next
		hc = jpg::decode::next_huffcode( huffr, actree );
		if ( hc < 0 ) return -1;
		l = LBITS( hc, 4 );
		r = RBITS( hc, 4 );
		// analyse code
		if ( ( l == 15 ) || ( r > 0 ) ) { // decode run/level combination
			z = l;
			s = r;
			if ( s == 0 ) v = 0;
			else if ( s == 1 ) {
				n = huffr->read_bit();
				v = ( n == 0 ) ? -1 : 1; // fast decode vli
			}
			else return -1; // decoding error
			// write zeroes / write correction bits
			while ( true ) {
				if ( block[ bpos ] == 0 ) { // skip zeroes / write value
					if ( z > 0 ) z--;
					else {
						block[ bpos++ ] = v;
						break;
					}
				}
				else { // read correction bit
					n = huffr->read_bit();
					block[ bpos ] = ( block[ bpos ] > 0 ) ? n : -n;
				}
				if ( bpos++ >= to ) return -1; // error check					
			}
		}
		else { // decode eobrun
			eob = bpos;
			s = l;
			n = huffr->read( s );
			(*eobrun) = e_devli( s, n );
			break;
		}
	}
	
	// read after eob correction bits
	if ( (*eobrun) > 0 ) {
		for ( ; bpos <= to; bpos++ ) {
			if ( block[ bpos ] != 0 ) {
				n = huffr->read_bit();
				block[ bpos ] = ( block[ bpos ] > 0 ) ? n : -n;
			}
		}
	}
	
	// return eob
	return eob;
}

int jpg::encode::ac_prg_sa(const std::unique_ptr<abitwriter>& huffw, const std::unique_ptr<abytewriter>& storw, const HuffCodes& actbl, const std::array<std::int16_t, 64>& block, int* eobrun, int from, int to)
{
	unsigned short n;
	unsigned char  s;
	unsigned char  z;
	int eob = from;
	int bpos;
	int hc;
	
	// check if block contains any newly nonzero coefficients and find out position of eob
	for ( bpos = to; bpos >= from; bpos-- )	{
		if ( ( block[ bpos ] == 1 ) || ( block[ bpos ] == -1 ) ) {
			eob = bpos + 1;
			break;
		}
	}
	
	// encode eobrun if needed
	if ( ( eob > from ) && ( (*eobrun) > 0 ) ) {
		jpg::encode::eobrun(huffw, actbl, eobrun);
		jpg::encode::crbits( huffw, storw );
	}
	
	// encode AC
	z = 0;
	for ( bpos = from; bpos < eob; bpos++ )
	{
		// if zero is encountered
		if ( block[ bpos ] == 0 ) {
			z++; // increment zero counter
			if ( z == 16 ) { // write zeroes if needed
				huffw->write( actbl.cval[ 0xF0 ], actbl.clen[ 0xF0 ] );
				jpg::encode::crbits( huffw, storw );
				z = 0;
			}
		}
		// if nonzero is encountered
		else if ( ( block[ bpos ] == 1 ) || ( block[ bpos ] == -1 ) ) {
			// vli encode			
			s = bitlen2048n( block[ bpos ] );
			n = envli( s, block[ bpos ] );
			hc = ( ( z << 4 ) + s );
			// write to huffman writer
			huffw->write( actbl.cval[ hc ], actbl.clen[ hc ] );
			huffw->write( n, s );
			// write correction bits
			jpg::encode::crbits( huffw, storw );
			// reset zeroes
			z = 0;
		}
		else { // store correction bits
			n = block[ bpos ] & 0x1;
			storw->write( n );
		}
	}
	
	// fast processing after eob
	for ( ;bpos <= to; bpos++ )
	{
		if ( block[ bpos ] != 0 ) { // store correction bits
			n = block[ bpos ] & 0x1;
			storw->write( n );
		}
	}
	
	// check eob, increment eobrun if needed
	if ( eob <= to ) {
		(*eobrun)++;	
		// check eobrun, encode if needed
		if ( (*eobrun) == actbl.max_eobrun ) {
			jpg::encode::eobrun(huffw, actbl, eobrun);
			jpg::encode::crbits( huffw, storw );		
		}
	}	
	
	// return eob
	return eob;
}

void jpg::decode::eobrun_sa(const std::unique_ptr<abitreader>& huffr, short* block, int* eobrun, int from, int to)
{
	unsigned short n;
	int bpos;
	
	
	// fast eobrun decoding routine for succesive approximation
	for ( bpos = from; bpos <= to; bpos++ ) {
		if ( block[ bpos ] != 0 ) {
			n = huffr->read_bit();
			block[ bpos ] = ( block[ bpos ] > 0 ) ? n : -n;
		}
	}
}

void jpg::encode::eobrun(const std::unique_ptr<abitwriter>& huffw, const HuffCodes& actbl, int* eobrun)
{
	unsigned short n;
	unsigned char  s;
	int hc;
	
	
	if ( (*eobrun) > 0 ) {
		while ( (*eobrun) > actbl.max_eobrun ) {
			huffw->write( actbl.cval[ 0xE0 ], actbl.clen[ 0xE0 ] );
			huffw->write(e_envli( 14, 32767 ), 14 );
			(*eobrun) -= actbl.max_eobrun;
		}
		s = bitlen((*eobrun));
		s--;
		n = e_envli( s, (*eobrun) );
		hc = ( s << 4 );
		huffw->write( actbl.cval[ hc ], actbl.clen[ hc ] );
		huffw->write( n, s );
		(*eobrun) = 0;
	}
}

void jpg::encode::crbits(const std::unique_ptr<abitwriter>& huffw, const std::unique_ptr<abytewriter>& storw)
{	
	unsigned char* data;
	int len;
	int i;
	
	
	// peek into data from abytewriter	
	len = storw->getpos();
	if ( len == 0 ) return;
	data = storw->peekptr();
	
	// write bits to huffwriter
	for ( i = 0; i < len; i++ )
		huffw->write_bit(data[i]);
	
	// reset abytewriter, discard data
	storw->reset();
}

int jpg::decode::next_huffcode(const std::unique_ptr<abitreader>& huffr, const HuffTree& ctree)
{	
	int node = 0;
	
	
	while ( node < 256 ) {
		node = ( huffr->read_bit() == 1 ) ?
				ctree.r[ node ] : ctree.l[ node ];
		if ( node == 0 ) break;
	}
	
	return ( node - 256 );
}

jpg::CodingStatus jpg::next_mcupos(int* mcu, int* cmp, int* csc, int* sub, int* dpos, int* rstw)
{
	jpg::CodingStatus sta = jpg::CodingStatus::OKAY;
	
	
	// increment all counts where needed
	if ( ( ++(*sub) ) >= cmpnfo[(*cmp)].mbs ) {
		(*sub) = 0;
		
		if ( ( ++(*csc) ) >= curr_scan::cmpc ) {
			(*csc) = 0;
			(*cmp) = curr_scan::cmp[ 0 ];
			(*mcu)++;
			if ( (*mcu) >= image::mcuc ) sta = jpg::CodingStatus::DONE;
			else if ( jpg::rsti > 0 )
				if ( --(*rstw) == 0 ) sta = jpg::CodingStatus::RESTART;
		}
		else {
			(*cmp) = curr_scan::cmp[(*csc)];
		}
	}
	
	// get correct position in image ( x & y )
	if ( cmpnfo[(*cmp)].sfh > 1 ) { // to fix mcu order
		(*dpos)  = ( (*mcu) / image::mcuh ) * cmpnfo[(*cmp)].sfh + ( (*sub) / cmpnfo[(*cmp)].sfv );
		(*dpos) *= cmpnfo[(*cmp)].bch;
		(*dpos) += ( (*mcu) % image::mcuh ) * cmpnfo[(*cmp)].sfv + ( (*sub) % cmpnfo[(*cmp)].sfv );
	}
	else if ( cmpnfo[(*cmp)].sfv > 1 ) {
		// simple calculation to speed up things if simple fixing is enough
		(*dpos) = ( (*mcu) * cmpnfo[(*cmp)].mbs ) + (*sub);
	}
	else {
		// no calculations needed without subsampling
		(*dpos) = (*mcu);
	}
	
	
	return sta;
}

jpg::CodingStatus jpg::next_mcuposn(int cmpt, int* dpos, int* rstw)
{
	// increment position
	(*dpos)++;
	
	// fix for non interleaved mcu - horizontal
	if ( cmpnfo[cmpt].bch != cmpnfo[cmpt].nch ) {
		if ( (*dpos) % cmpnfo[cmpt].bch == cmpnfo[cmpt].nch )
			(*dpos) += ( cmpnfo[cmpt].bch - cmpnfo[cmpt].nch );
	}
	
	// fix for non interleaved mcu - vertical
	if ( cmpnfo[cmpt].bcv != cmpnfo[cmpt].ncv ) {
		if ( (*dpos) / cmpnfo[cmpt].bch == cmpnfo[cmpt].ncv )
			(*dpos) = cmpnfo[cmpt].bc;
	}
	
	// check position
	if ( (*dpos) >= cmpnfo[cmpt].bc ) return jpg::CodingStatus::DONE;
	else if ( jpg::rsti > 0 )
		if ( --(*rstw) == 0 ) return jpg::CodingStatus::RESTART;
	

	return jpg::CodingStatus::OKAY;
}

jpg::CodingStatus jpg::decode::skip_eobrun(int cmpt, int* dpos, int* rstw, int* eobrun)
{
	if ( (*eobrun) > 0 ) // error check for eobrun
	{		
		// compare rst wait counter if needed
		if ( jpg::rsti > 0 ) {
			if ( (*eobrun) > (*rstw) )
				return jpg::CodingStatus::ERROR;
			else
				(*rstw) -= (*eobrun);
		}
		
		// fix for non interleaved mcu - horizontal
		if ( cmpnfo[cmpt].bch != cmpnfo[cmpt].nch ) {
			(*dpos) += ( ( ( (*dpos) % cmpnfo[cmpt].bch ) + (*eobrun) ) /
						cmpnfo[cmpt].nch ) * ( cmpnfo[cmpt].bch - cmpnfo[cmpt].nch );
		}
		
		// fix for non interleaved mcu - vertical
		if ( cmpnfo[cmpt].bcv != cmpnfo[cmpt].ncv ) {
			if ( (*dpos) / cmpnfo[cmpt].bch >= cmpnfo[cmpt].ncv )
				(*dpos) += ( cmpnfo[cmpt].bcv - cmpnfo[cmpt].ncv ) *
						cmpnfo[cmpt].bch;
		}		
		
		// skip blocks 
		(*dpos) += (*eobrun);
		
		// reset eobrun
		(*eobrun) = 0;
		
		// check position
		if ( (*dpos) == cmpnfo[cmpt].bc ) return jpg::CodingStatus::DONE;
		else if ( (*dpos) > cmpnfo[cmpt].bc ) return jpg::CodingStatus::ERROR;
		else if ( jpg::rsti > 0 ) 
			if ( (*rstw) == 0 ) return jpg::CodingStatus::RESTART;
	}
	
	return jpg::CodingStatus::OKAY;
}

static HuffCodes jpg::jfif::build_huffcodes(const unsigned char* clen, const unsigned char* cval) {
	HuffCodes codes;
	int k = 0;
	int code = 0;
	
	// symbol-value of code is its position in the table
	for (int i = 0; i < 16; i++) {
		for (int j = 0; j < (int)clen[i]; j++) {
			codes.clen[(int)cval[k]] = 1 + i;
			codes.cval[(int)cval[k]] = code;

			k++;
			code++;
		}
		code = code << 1;
	}
	
	// find out eobrun max value
	for (int i = 14; i >= 0; i--) {
		if (codes.clen[i << 4] > 0) {
			codes.max_eobrun = (2 << i) - 1;
			break;
		}
	}
	return codes;
}

static HuffTree jpg::jfif::build_hufftree(const HuffCodes& hc) {
	HuffTree tree;
	// initial value for next free place
	int nextfree = 1;

	// work through every code creating links between the nodes (represented through ints)
	for (int i = 0; i < 256; i++) {
		// (re)set current node
		int node = 0;
		// go through each code & store path
		for (int j = hc.clen[i] - 1; j > 0; j--) {
			if (BITN(hc.cval[i], j) == 1) {
				if (tree.r[node] == 0) {
					tree.r[node] = nextfree++;
				}
				node = tree.r[node];
			} else {
				if (tree.l[node] == 0) {
					tree.l[node] = nextfree++;
				}
				node = tree.l[node];
			}
		}
		// last link is number of targetvalue + 256
		if (hc.clen[i] > 0) {
			if (BITN(hc.cval[i], 0) == 1) {
				tree.r[node] = i + 256;
			} else {
				tree.l[node] = i + 256;
			}
		}
	}
	return tree;
}

/* ----------------------- End of JPEG specific functions -------------------------- */

/* ----------------------- End of PJG specific functions -------------------------- */


/* -----------------------------------------------
	encodes frequency scanorder to pjg
	----------------------------------------------- */
void pjg::encode::zstscan(const std::unique_ptr<aricoder>& enc, int cmp)
{
	// calculate zero sort scan
	pjg::encode::get_zerosort_scan( pjg::zsrtscan[cmp], cmp );
	
	// preset freqlist
	std::array<std::uint8_t, 64> freqlist;
	std::copy(std::begin(stdscan), std::end(stdscan), std::begin(freqlist));
		
	// init model
	auto model = INIT_MODEL_S(64, 64, 1);
	
	// encode scanorder
	for (int i = 1; i < 64; i++ )
	{			
		// reduce range of model
		model->exclude_symbols(64 - i);
		
		// compare remaining list to remainnig scan
		int tpos = 0; // True position.
		int c;
		for ( c = i; c < 64; c++ ) {
			// search next val != 0 in list
			for ( tpos++; freqlist[ tpos ] == 0; tpos++ );
			// get out if not a match
			if ( freqlist[ tpos ] != pjg::zsrtscan[ cmp ][ c ] ) break;				
		}
		if ( c == 64 ) {
			// remaining list is in sorted scanorder
			// encode zero and make a quick exit
			enc->encode_ari( model, 0 );
			break;
		}
		
		// list is not in sorted order -> next pos hat to be encoded
		int cpos = 1; // Coded position.
		// encode position
		for ( tpos = 0; freqlist[ tpos ] != pjg::zsrtscan[ cmp ][ i ]; tpos++ )
			if ( freqlist[ tpos ] != 0 ) cpos++;
		// remove from list
		freqlist[ tpos ] = 0;
		
		// encode coded position in list
		enc->encode_ari( model, cpos );
		model->shift_context( cpos );		
	}
	
	// delete model
	delete model;
	
	// set zero sort scan as pjg::freqscan
	pjg::freqscan[ cmp ] = pjg::zsrtscan[ cmp ];
}


/* -----------------------------------------------
	encodes # of non zeroes to pjg (high)
	----------------------------------------------- */
void pjg::encode::zdst_high(const std::unique_ptr<aricoder>& enc, int cmp)
{
	// init model, constants
	auto model = INIT_MODEL_S(49 + 1, 25 + 1, 1);
	const unsigned char* zdstls = pjg::zdstdata[ cmp ];
	const int w = cmpnfo[cmp].bch;
	const int bc = cmpnfo[cmp].bc;

	// arithmetic encode zero-distribution-list
	for (int dpos = 0; dpos < bc; dpos++) {
		// context modelling - use average of above and left as context
		auto coords = get_context_nnb(dpos, w);
		coords.first = (coords.first >= 0) ? zdstls[coords.first] : 0;
		coords.second = (coords.second >= 0) ? zdstls[coords.second] : 0;
		// shift context
		model->shift_context((coords.first + coords.second + 2) / 4);
		// encode symbol
		enc->encode_ari(model, zdstls[dpos]);
	}
	
	// clean up
	delete model;
}


/* -----------------------------------------------
	encodes # of non zeroes to pjg (low)
	----------------------------------------------- */
void pjg::encode::zdst_low(const std::unique_ptr<aricoder>& enc, int cmp)
{
	// init model, constants
	auto model = INIT_MODEL_S(8, 8, 2);
	const unsigned char* zdstls_x = pjg::zdstxlow[ cmp ];
	const unsigned char* zdstls_y = pjg::zdstylow[ cmp ];
	const unsigned char* ctx_eobx = pjg::eobxhigh[ cmp ];
	const unsigned char* ctx_eoby = pjg::eobyhigh[ cmp ];
	const unsigned char* ctx_zdst = pjg::zdstdata[ cmp ];
	const int bc = cmpnfo[cmp].bc;
	
	// arithmetic encode zero-distribution-list (first row)
	for (int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[dpos] + 3 ) / 7 ); // shift context
		model->shift_context( ctx_eobx[dpos] ); // shift context
		enc->encode_ari( model, zdstls_x[ dpos ] ); // encode symbol
	}
	// arithmetic encode zero-distribution-list (first collumn)
	for (int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[dpos] + 3 ) / 7 ); // shift context
		model->shift_context( ctx_eoby[dpos] ); // shift context
		enc->encode_ari( model, zdstls_y[ dpos ] ); // encode symbol
	}
	
	// clean up
	delete model;
}


/* -----------------------------------------------
	encodes DC coefficients to pjg
	----------------------------------------------- */
void pjg::encode::dc(const std::unique_ptr<aricoder>& enc, int cmp)
{	
	unsigned short* c_absc[ 6 ]; // quick access array for contexts
	int c_weight[ 6 ]; // weighting for contexts

	// decide segmentation setting
	const unsigned char* segm_tab = segm_tables[ segm_cnt[ cmp ] - 1 ];
	
	// get max absolute value/bit length
	const int max_val = MAX_V( cmp, 0 ); // Max value.
	const int max_len = bitlen1024p( max_val ); // Max bitlength.
	
	// init models for bitlenghts and -patterns	
	auto mod_len = INIT_MODEL_S(max_len + 1, std::max(int(segm_cnt[cmp]), max_len + 1), 2);
	auto mod_res = INIT_MODEL_B(std::max(int(segm_cnt[cmp]), 16), 2);
	auto mod_sgn = INIT_MODEL_B(1, 0);
	
	// set width/height of each band
	const int bc = cmpnfo[cmp].bc;
	const int w = cmpnfo[cmp].bch;
	
	// allocate memory for absolute values storage
	std::vector<unsigned short> absv_store(bc); // absolute coefficients values storage
	
	// set up context quick access array
	pjg::aavrg_prepare( c_absc, c_weight, absv_store.data(), cmp );
	
	// locally store pointer to coefficients and zero distribution list
	const short* coeffs = dct::colldata[ cmp ][ 0 ]; // Pointer to current coefficent data.
	const unsigned char* zdstls = pjg::zdstdata[ cmp ];	 // Pointer to zero distribution list.
	
	// arithmetic compression loop
	for (int dpos = 0; dpos < bc; dpos++ )
	{		
		//calculate x/y positions in band
		const int p_y = dpos / w;
		// r_y = h - ( p_y + 1 );
		const int p_x = dpos % w;
		const int r_x = w - ( p_x + 1 );
		
		// get segment-number from zero distribution list and segmentation set
		const int snum = segm_tab[ zdstls[dpos] ];
		// calculate contexts (for bit length)
		const int ctx_avr = pjg::aavrg_context( c_absc, c_weight, dpos, p_y, p_x, r_x ); // Average context
		const int ctx_len = bitlen1024p( ctx_avr ); // Bitlength context.
		// shift context / do context modelling (segmentation is done per context)
		shift_model( mod_len, ctx_len, snum );
		
		// simple treatment if coefficient is zero
		if ( coeffs[ dpos ] == 0 ) {
			// encode bit length (0) of current coefficient			
			enc->encode_ari( mod_len, 0 );
		}
		else {
			// get absolute val, sign & bit length for current coefficient
			const int absv = std::abs( coeffs[dpos] );
			const int clen = bitlen1024p( absv );
			const int sgn = ( coeffs[dpos] > 0 ) ? 0 : 1;
			// encode bit length of current coefficient
			enc->encode_ari( mod_len, clen );
			// encoding of residual
			// first set bit must be 1, so we start at clen - 2
			for (int bp = clen - 2; bp >= 0; bp-- ) {
				shift_model( mod_res, snum, bp ); // shift in 2 contexts
				// encode/get bit
				const int bt = BITN( absv, bp );
				enc->encode_ari( mod_res, bt );
			}
			// encode sign
			enc->encode_ari( mod_sgn, sgn );
			// store absolute value
			absv_store[ dpos ] = absv;
		}
	}
	
	// clear models
	delete mod_len;
	delete mod_res;
	delete mod_sgn;
}


/* -----------------------------------------------
	encodes high (7x7) AC coefficients to pjg
	----------------------------------------------- */
void pjg::encode::ac_high(const std::unique_ptr<aricoder>& enc, int cmp)
{	
	unsigned short* c_absc[ 6 ]; // quick access array for contexts
	int c_weight[ 6 ]; // weighting for contexts
	
	// decide segmentation setting
	const unsigned char* segm_tab = segm_tables[ segm_cnt[ cmp ] - 1 ];
	
	// init models for bitlenghts and -patterns
	auto mod_len = INIT_MODEL_S(11, std::max(11, int(segm_cnt[cmp])), 2);
	auto mod_res = INIT_MODEL_B(std::max(int(segm_cnt[cmp]), 16), 2);
	auto mod_sgn = INIT_MODEL_B(9, 1);
	
	// set width/height of each band
	const int bc = cmpnfo[cmp].bc;
	const int w = cmpnfo[cmp].bch;
	
	// allocate memory for absolute values & signs storage
	std::vector<unsigned short> absv_store(bc);	// absolute coefficients values storage
	std::vector<unsigned char> sgn_store(bc); // sign storage for context	
	std::vector<unsigned char> zdstls(pjg::zdstdata[cmp], pjg::zdstdata[cmp] + bc); // copy of zero distribution list
	
	// set up quick access arrays for signs context
	unsigned char* sgn_nbh = sgn_store.data() - 1; // Left signs neighbor.
	unsigned char* sgn_nbv = sgn_store.data() - w; // Upper signs neighbor.
	
	// locally store pointer to eob x / eob y
	unsigned char* eob_x = pjg::eobxhigh[ cmp ]; // Pointer to x eobs.
	unsigned char* eob_y = pjg::eobyhigh[ cmp ]; // Pointer to y eobs.
	
	// preset x/y eobs
	std::fill(eob_x, eob_x + bc, unsigned char(0));
	std::fill(eob_y, eob_y + bc, unsigned char(0));
	
	// work through lower 7x7 bands in order of pjg::freqscan
	for (int i = 1; i < 64; i++ )
	{		
		// work through blocks in order of frequency scan
		const int bpos = (int) pjg::freqscan[cmp][i];
		const int b_x = unzigzag[ bpos ] % 8;
		const int b_y = unzigzag[ bpos ] / 8;
	
		if ( ( b_x == 0 ) || ( b_y == 0 ) )
			continue; // process remaining coefficients elsewhere
	
		// preset absolute values/sign storage
		std::fill(std::begin(absv_store), std::end(absv_store), unsigned short(0));
		std::fill(std::begin(sgn_store), std::end(sgn_store), unsigned char(0));
		
		// set up average context quick access arrays
		pjg::aavrg_prepare( c_absc, c_weight, absv_store.data(), cmp );
		
		// locally store pointer to coefficients
		const short* coeffs = dct::colldata[ cmp ][ bpos ]; // Pointer to current coefficent data.
		
		// get max bit length
		const int max_val = MAX_V( cmp, bpos ); // Max value.
		const int max_len = bitlen1024p( max_val ); // Max bitlength.
		
		// arithmetic compression loo
		for (int dpos = 0; dpos < bc; dpos++ )
		{		
			// skip if beyound eob
			if ( zdstls[dpos] == 0 )
				continue;
		
			//calculate x/y positions in band
			const int p_y = dpos / w;
			const int p_x = dpos % w;
			const int r_x = w - ( p_x + 1 );
		
			// get segment-number from zero distribution list and segmentation set
			const int snum = segm_tab[ zdstls[dpos] ];
			// calculate contexts (for bit length)
			const int ctx_avr = pjg::aavrg_context( c_absc, c_weight, dpos, p_y, p_x, r_x ); // Average context.
			const int ctx_len = bitlen1024p( ctx_avr ); // Bitlength context.
			// shift context / do context modelling (segmentation is done per context)
			shift_model( mod_len, ctx_len, snum );
			mod_len->exclude_symbols(max_len);		
		
			// simple treatment if coefficient is zero
			if ( coeffs[ dpos ] == 0 ) {
				// encode bit length (0) of current coefficien
				enc->encode_ari( mod_len, 0 );
			}
			else {
				// get absolute val, sign & bit length for current coefficient
				const int absv = std::abs( coeffs[dpos] );
				const int clen = bitlen1024p( absv );
				const int sgn = ( coeffs[dpos] > 0 ) ? 0 : 1;
				// encode bit length of current coefficient				
				enc->encode_ari( mod_len, clen );
				// encoding of residual
				// first set bit must be 1, so we start at clen - 2
				for (int bp = clen - 2; bp >= 0; bp-- ) { 
					shift_model( mod_res, snum, bp ); // shift in 2 contexts
					// encode/get bit
					const int bt = BITN( absv, bp );
					enc->encode_ari( mod_res, bt );
				}
				// encode sign				
				int ctx_sgn = ( p_x > 0 ) ? sgn_nbh[ dpos ] : 0; // Sign context.
				if ( p_y > 0 ) ctx_sgn += 3 * sgn_nbv[ dpos ]; // IMPROVE !!!!!!!!!!!
				mod_sgn->shift_context( ctx_sgn );
				enc->encode_ari( mod_sgn, sgn );
				// store absolute value/sign, decrement zdst
				absv_store[ dpos ] = absv;
				sgn_store[ dpos ] = sgn + 1;
				zdstls[dpos]--;
				// recalculate x/y eob				
				if ( b_x > eob_x[dpos] ) eob_x[dpos] = b_x;
				if ( b_y > eob_y[dpos] ) eob_y[dpos] = b_y;
			}
		}
		// flush models
		mod_len->flush_model();
		mod_res->flush_model();
		mod_sgn->flush_model();
	}
	
	// clear models
	delete mod_len;
	delete mod_res;
	delete mod_sgn;
}


/* -----------------------------------------------
	encodes first row/col AC coefficients to pjg
	----------------------------------------------- */
void pjg::encode::ac_low(const std::unique_ptr<aricoder>& enc, int cmp)
{	
	
	short* coeffs_x[ 8 ]; // prediction coeffs - current block
	short* coeffs_a[ 8 ]; // prediction coeffs - neighboring block
	int pred_cf[ 8 ]; // prediction multipliers
	
	// init models for bitlenghts and -patterns
	auto mod_len = INIT_MODEL_S(11, std::max(int(segm_cnt[cmp]), 11), 2);
	auto mod_res = INIT_MODEL_B(1 << 4, 2);
	auto mod_top = INIT_MODEL_B(1 << std::max(4, int(nois_trs[cmp])), 3);
	auto mod_sgn = INIT_MODEL_B(11, 1);
	
	// set width/height of each band
	const int bc = cmpnfo[cmp].bc;
	const int w = cmpnfo[cmp].bch;
	
	// work through each first row / first collumn band
	for (int i = 2; i < 16; i++ )
	{		
		// alternate between first row and first collumn
		int b_x = ( i % 2 == 0 ) ? i / 2 : 0;
		int b_y = ( i % 2 == 1 ) ? i / 2 : 0;
		const int bpos = (int) zigzag[ b_x + (8*b_y) ];
		
		// locally store pointer to band coefficients
		const short* coeffs = dct::colldata[ cmp ][ bpos ]; // Pointer to current coefficent data.
		// store pointers to prediction coefficients
		int p_x, p_y;
		int* edge_c; // edge criteria
		unsigned char* zdstls; // Pointer to row/col # of non-zeroes.
		if ( b_x == 0 ) {
			for ( ; b_x < 8; b_x++ ) {
				coeffs_x[ b_x ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ];
				coeffs_a[ b_x ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ] - 1;
				pred_cf[ b_x ] = dct::icos_base_8x8[ b_x * 8 ] * QUANT ( cmp, zigzag[b_x+(8*b_y)] );
			}
			zdstls = pjg::zdstylow[ cmp ];
			edge_c = &p_x;
		}
		else { // if ( b_y == 0 )
			for ( ; b_y < 8; b_y++ ) {
				coeffs_x[ b_y ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ];
				coeffs_a[ b_y ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ] - w;
				pred_cf[ b_y ] = dct::icos_base_8x8[ b_y * 8 ] * QUANT ( cmp, zigzag[b_x+(8*b_y)] );
			}
			zdstls = pjg::zdstxlow[ cmp ];
			edge_c = &p_y;
		}
		
		// get max bit length / other info
		const int max_valp = MAX_V( cmp, bpos ); // Max value (positive).
		const int max_valn = -max_valp; // Max value (negative).
		const int max_len = bitlen1024p( max_valp ); // Max bitlength
		const int thrs_bp = ( max_len > nois_trs[cmp] ) ? max_len - nois_trs[cmp] : 0; // residual threshold bitplane	
		
		// arithmetic compression loop
		for (int dpos = 0; dpos < bc; dpos++ )
		{
			// skip if beyound eob
			if (zdstls[dpos] == 0) {
				continue;
			}
			
			// calculate x/y positions in band
			p_y = dpos / w;
			p_x = dpos % w;
			
			// edge treatment / calculate LAKHANI context
			int ctx_lak; // lakhani context
			if ((*edge_c) > 0) {
				ctx_lak = pjg::lakh_context(coeffs_x, coeffs_a, pred_cf, dpos);
			} else {
				ctx_lak = 0;
			}
			ctx_lak = clamp(ctx_lak, max_valn, max_valp);
			const int ctx_len = bitlen2048n( ctx_lak ); // Context for bitlength.
			
			// shift context / do context modelling (segmentation is done per context)
			shift_model( mod_len, ctx_len, zdstls[ dpos ] );
			mod_len->exclude_symbols(max_len);			
			
			// simple treatment if coefficient is zero
			if ( coeffs[ dpos ] == 0 ) {
				// encode bit length (0) of current coefficient
				enc->encode_ari( mod_len, 0 );
			} else {
				// get absolute val, sign & bit length for current coefficient
				const int absv = std::abs( coeffs[dpos] );
				const int clen = bitlen2048n( absv );
				const int sgn = ( coeffs[dpos] > 0 ) ? 0 : 1;
				// encode bit length of current coefficient
				enc->encode_ari( mod_len, clen );
				// encoding of residual
				int bp = clen - 2; // first set bit must be 1, so we start at clen - 2
				int ctx_res = ( bp >= thrs_bp ) ? 1 : 0; // Bitplane context for residual.
				const int ctx_abs = std::abs( ctx_lak ); // Absolute context.
				const int ctx_sgn = ( ctx_lak == 0 ) ? 0 : ( ctx_lak > 0 ) ? 1 : 2; // Context for sign.
				for ( ; bp >= thrs_bp; bp-- ) {						
					shift_model( mod_top, ctx_abs >> thrs_bp, ctx_res, clen - thrs_bp ); // shift in 3 contexts
					// encode/get bit
					const int bt = BITN( absv, bp );
					enc->encode_ari( mod_top, bt );
					// update context
					ctx_res = ctx_res << 1;
					if ( bt ) ctx_res |= 1; 
				}
				for ( ; bp >= 0; bp-- ) {
					shift_model( mod_res, zdstls[ dpos ], bp ); // shift in 2 contexts
					// encode/get bit
					const int bt = BITN( absv, bp );
					enc->encode_ari( mod_res, bt );
				}
				// encode sign
				shift_model( mod_sgn, ctx_len, ctx_sgn );
				enc->encode_ari( mod_sgn, sgn );
				// decrement # of non zeroes
				zdstls[ dpos ]--;
			}
		}
		// flush models
		mod_len->flush_model();
		mod_res->flush_model();
		mod_top->flush_model();
		mod_sgn->flush_model();
	}
	
	// clear models
	delete mod_len;
	delete mod_res;
	delete mod_top;
	delete mod_sgn;
}


/* -----------------------------------------------
	encodes a stream of generic (8bit) data to pjg
	----------------------------------------------- */
bool pjg::encode::generic( const std::unique_ptr<aricoder>& enc, unsigned char* data, int len )
{
	// arithmetic encode data
	auto model = INIT_MODEL_S(256 + 1, 256, 1);
	for (int i = 0; i < len; i++ ) {
		enc->encode_ari( model, data[ i ] );
		model->shift_context( data[ i ] );
	}
	// encode end-of-data symbol (256)
	enc->encode_ari( model, 256 );
	delete model;
	
	return true;
}


/* -----------------------------------------------
	encodes one bit to pjg
	----------------------------------------------- */
void pjg::encode::bit(const std::unique_ptr<aricoder>& enc, unsigned char bit)
{
	// encode one bit
	auto model = INIT_MODEL_B(1, -1);
	enc->encode_ari( model, bit );
	delete model;
}


/* -----------------------------------------------
	encodes frequency scanorder to pjg
	----------------------------------------------- */
void pjg::decode::zstscan(const std::unique_ptr<aricoder>& dec, int cmp)
{		
	int tpos; // true position
	
	// set first position in zero sort scan
	pjg::zsrtscan[ cmp ][ 0 ] = 0;
	
	// preset freqlist
	std::array<std::uint8_t, 64> freqlist;
	std::copy(std::begin(stdscan), std::end(stdscan), std::begin(freqlist));
		
	// init model
	auto model = INIT_MODEL_S(64, 64, 1);
	
	// encode scanorder
	for (int i = 1; i < 64; i++ )
	{			
		// reduce range of model
		model->exclude_symbols(64 - i);
		
		// decode symbol
		int cpos = dec->decode_ari(model ); // coded position	
		model->shift_context( cpos );
		
		if ( cpos == 0 ) {
			// remaining list is identical to scan
			// fill the scan & make a quick exit				
			for ( tpos = 0; i < 64; i++ ) {
				while ( freqlist[ ++tpos ] == 0 );
				pjg::zsrtscan[ cmp ][ i ] = freqlist[ tpos ];
			}
			break;
		}
		
		// decode position from list
		for ( tpos = 0; tpos < 64; tpos++ ) {
			if ( freqlist[ tpos ] != 0 ) cpos--;
			if ( cpos == 0 ) break;
		}
			
		// write decoded position to zero sort scan
		pjg::zsrtscan[ cmp ][ i ] = freqlist[ tpos ];
		// remove from list
		freqlist[ tpos ] = 0;
	}
	
	// delete model
	delete model;		
	
	// set zero sort scan as pjg::freqscan
	pjg::freqscan[ cmp ] = pjg::zsrtscan[ cmp ];
}


/* -----------------------------------------------
	decodes # of non zeroes from pjg (high)
	----------------------------------------------- */
void pjg::decode::zdst_high(const std::unique_ptr<aricoder>& dec, int cmp)
{		
	// init model, constants
	auto model = INIT_MODEL_S(49 + 1, 25 + 1, 1);
	unsigned char* zdstls = pjg::zdstdata[ cmp ];
	const int w = cmpnfo[cmp].bch;
	const int bc = cmpnfo[cmp].bc;
	
	// arithmetic decode zero-distribution-list
	for (int dpos = 0; dpos < bc; dpos++)	{
		// context modelling - use average of above and left as context		
		auto coords = get_context_nnb(dpos, w);
		coords.first = (coords.first >= 0) ? zdstls[coords.first] : 0;
		coords.second = (coords.second >= 0) ? zdstls[coords.second] : 0;
		// shift context
		model->shift_context((coords.first + coords.second + 2) / 4);
		// decode symbol
		zdstls[dpos] = dec->decode_ari(model);
	}
	
	// clean up
	delete model;
}


/* -----------------------------------------------
	decodes # of non zeroes from pjg (low)
	----------------------------------------------- */
void pjg::decode::zdst_low(const std::unique_ptr<aricoder>& dec, int cmp)
{
	// init model, constants
	auto model = INIT_MODEL_S(8, 8, 2);

	unsigned char* zdstls_x = pjg::zdstxlow[ cmp ];
	unsigned char* zdstls_y = pjg::zdstylow[ cmp ];

	const unsigned char* ctx_eobx = pjg::eobxhigh[ cmp ];
	const unsigned char* ctx_eoby = pjg::eobyhigh[ cmp ];
	const unsigned char* ctx_zdst = pjg::zdstdata[ cmp ];
	const int bc = cmpnfo[cmp].bc;
	
	// arithmetic encode zero-distribution-list (first row)
	for (int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[dpos] + 3 ) / 7 ); // shift context
		model->shift_context( ctx_eobx[dpos] ); // shift context
		zdstls_x[ dpos ] = dec->decode_ari(model ); // decode symbol
	}
	// arithmetic encode zero-distribution-list (first collumn)
	for (int dpos = 0; dpos < bc; dpos++ ) {
		model->shift_context( ( ctx_zdst[dpos] + 3 ) / 7 ); // shift context
		model->shift_context( ctx_eoby[dpos] ); // shift context
		zdstls_y[ dpos ] = dec->decode_ari(model ); // decode symbol
	}
	
	// clean up
	delete model;
}


/* -----------------------------------------------
	decodes DC coefficients from pjg
	----------------------------------------------- */
void pjg::decode::dc(const std::unique_ptr<aricoder>& dec, int cmp)
{	
	unsigned short* c_absc[ 6 ]; // quick access array for contexts
	int c_weight[ 6 ]; // weighting for contexts
	
	// decide segmentation setting
	const unsigned char* segm_tab = segm_tables[ segm_cnt[ cmp ] - 1 ];
	
	// get max absolute value/bit length
	const int max_val = MAX_V( cmp, 0 ); // Max value.
	const int max_len = bitlen1024p( max_val ); // Max bitlength.
	
	// init models for bitlenghts and -patterns
	auto mod_len = INIT_MODEL_S(max_len + 1, std::max(int(segm_cnt[cmp]), max_len + 1), 2);
	auto mod_res = INIT_MODEL_B(std::max(int(segm_cnt[cmp]), 16), 2);
	auto mod_sgn = INIT_MODEL_B(1, 0);
	
	// set width/height of each band
	const int bc = cmpnfo[cmp].bc;
	const int w = cmpnfo[cmp].bch;
	
	// allocate memory for absolute values storage
	std::vector<unsigned short> absv_store(bc); // absolute coefficients values storage
	
	// set up context quick access array
	pjg::aavrg_prepare( c_absc, c_weight, absv_store.data(), cmp );
	
	// locally store pointer to coefficients and zero distribution list
	short* coeffs = dct::colldata[ cmp ][ 0 ]; // Pointer to current coefficent data.
	const unsigned char* zdstls = pjg::zdstdata[ cmp ]; // Pointer to zero distribution list.
	
	// arithmetic compression loop
	for (int dpos = 0; dpos < bc; dpos++ )
	{		
		//calculate x/y positions in band
		const int p_y = dpos / w;
		// r_y = h - ( p_y + 1 );
		const int p_x = dpos % w;
		const int r_x = w - ( p_x + 1 );
		
		// get segment-number from zero distribution list and segmentation set
		const int snum = segm_tab[ zdstls[dpos] ];
		// calculate contexts (for bit length)
		const int ctx_avr = pjg::aavrg_context( c_absc, c_weight, dpos, p_y, p_x, r_x ); // Average context
		const int ctx_len = bitlen1024p( ctx_avr ); // Bitlength context				
		// shift context / do context modelling (segmentation is done per context)
		shift_model( mod_len, ctx_len, snum );
		// decode bit length of current coefficient
		const int clen = dec->decode_ari(mod_len );
		
		// simple treatment if coefficient is zero
		if ( clen == 0 ) {
			// coeffs[ dpos ] = 0;
		}
		else {
			// decoding of residual
			int absv = 1;
			// first set bit must be 1, so we start at clen - 2
			for (int bp = clen - 2; bp >= 0; bp-- ) {
				shift_model( mod_res, snum, bp ); // shift in 2 contexts
				// decode bit
				const int bt = dec->decode_ari(mod_res );
				// update absv
				absv = absv << 1;
				if ( bt ) absv |= 1; 
			}
			// decode sign
			const int sgn = dec->decode_ari(mod_sgn );
			// copy to colldata
			coeffs[ dpos ] = ( sgn == 0 ) ? absv : -absv;
			// store absolute value/sign
			absv_store[ dpos ] = absv;
		}
	}
	
	// clear models
	delete mod_len;
	delete mod_res;
	delete mod_sgn;
}


/* -----------------------------------------------
	decodes high (7x7) AC coefficients to pjg
	----------------------------------------------- */
void pjg::decode::ac_high(const std::unique_ptr<aricoder>& dec, int cmp)
{	
	unsigned short* c_absc[ 6 ]; // quick access array for contexts
	int c_weight[ 6 ]; // weighting for contexts
	
	// decide segmentation setting
	const unsigned char* segm_tab = segm_tables[ segm_cnt[ cmp ] - 1 ];
	
	// init models for bitlenghts and -patterns
	auto mod_len = INIT_MODEL_S(11, std::max(int(segm_cnt[cmp]), 11), 2);
	auto mod_res = INIT_MODEL_B(std::max(int(segm_cnt[cmp]), 16), 2);
	auto mod_sgn = INIT_MODEL_B(9, 1);
	
	// set width/height of each band
	const int bc = cmpnfo[cmp].bc;
	const int w = cmpnfo[cmp].bch;
	
	// allocate memory for absolute values & signs storage
	std::vector<unsigned short> absv_store(bc); // absolute coefficients values storage
	std::vector<unsigned char> sgn_store(bc); // sign storage for context	
	std::vector<unsigned char> zdstls(pjg::zdstdata[cmp], pjg::zdstdata[cmp] + bc); // copy of zero distribution list
	
	// set up quick access arrays for signs context
	unsigned char* sgn_nbh = sgn_store.data() - 1; // Left signs neighbor.
	unsigned char* sgn_nbv = sgn_store.data() - w; // Upper signs neighbor.
	
	// locally store pointer to eob x / eob y
	unsigned char* eob_x = pjg::eobxhigh[ cmp ]; // Pointer to x eobs.
	unsigned char* eob_y = pjg::eobyhigh[ cmp ]; // Pointer to y eobs.
	
	// preset x/y eobs
	std::fill(eob_x, eob_x + bc, unsigned char(0));
	std::fill(eob_y, eob_y + bc, unsigned char(0));
	
	// work through lower 7x7 bands in order of pjg::freqscan
	for (int i = 1; i < 64; i++ )
	{		
		// work through blocks in order of frequency scan
		const int bpos = (int) pjg::freqscan[cmp][i];
		const int b_x = unzigzag[ bpos ] % 8;
		const int b_y = unzigzag[ bpos ] / 8;
		
		if ( ( b_x == 0 ) || ( b_y == 0 ) )
				continue; // process remaining coefficients elsewhere
		
		// preset absolute values/sign storage
		std::fill(std::begin(absv_store), std::end(absv_store), unsigned short(0));
		std::fill(std::begin(sgn_store), std::end(sgn_store), unsigned char(0));
		
		// set up average context quick access arrays
		pjg::aavrg_prepare( c_absc, c_weight, absv_store.data(), cmp );
		
		// locally store pointer to coefficients
		short* coeffs = dct::colldata[ cmp ][ bpos ]; // Pointer to current coefficent data.
		
		// get max bit length
		const int max_val = MAX_V( cmp, bpos ); // Max value.
		const int max_len = bitlen1024p( max_val ); // Max bitlength.
		
		// arithmetic compression loop
		for (int dpos = 0; dpos < bc; dpos++ )
		{
			// skip if beyound eob
			if ( zdstls[dpos] == 0 )
				continue;
			
			//calculate x/y positions in band
			const int p_y = dpos / w;
			const int p_x = dpos % w;
			const int r_x = w - ( p_x + 1 );
			
			// get segment-number from zero distribution list and segmentation set
			const int snum = segm_tab[ zdstls[dpos] ];
			// calculate contexts (for bit length)
			const int ctx_avr = pjg::aavrg_context( c_absc, c_weight, dpos, p_y, p_x, r_x ); // Average context.
			const int ctx_len = bitlen1024p( ctx_avr ); // Bitlength context.
			// shift context / do context modelling (segmentation is done per context)
			shift_model( mod_len, ctx_len, snum );
			mod_len->exclude_symbols(max_len);
			
			// decode bit length of current coefficient
			const int clen = dec->decode_ari(mod_len );
			// simple treatment if coefficient is zero
			if ( clen == 0 ) {
				// coeffs[ dpos ] = 0;
			}
			else {
				// decoding of residual
				int absv = 1;
				// first set bit must be 1, so we start at clen - 2
				for (int bp = clen - 2; bp >= 0; bp-- ) {
					shift_model( mod_res, snum, bp ); // shift in 2 contexts
					// decode bit
					const int bt = dec->decode_ari(mod_res );
					// update absv
					absv = absv << 1;
					if ( bt ) absv |= 1; 
				}
				// decode sign
				int ctx_sgn = ( p_x > 0 ) ? sgn_nbh[ dpos ] : 0; // Sign context.
				if ( p_y > 0 ) ctx_sgn += 3 * sgn_nbv[ dpos ]; // IMPROVE! !!!!!!!!!!!
				mod_sgn->shift_context( ctx_sgn );
				const int sgn = dec->decode_ari(mod_sgn );
				// copy to colldata
				coeffs[ dpos ] = ( sgn == 0 ) ? absv : -absv;
				// store absolute value/sign, decrement zdst
				absv_store[ dpos ] = absv;
				sgn_store[ dpos ] = sgn + 1;
				zdstls[dpos]--;
				// recalculate x/y eob
				if ( b_x > eob_x[dpos] ) eob_x[dpos] = b_x;
				if ( b_y > eob_y[dpos] ) eob_y[dpos] = b_y;	
			}
		}
		// flush models
		mod_len->flush_model();
		mod_res->flush_model();
		mod_sgn->flush_model();
	}
	
	// clear models
	delete mod_len;
	delete mod_res;
	delete mod_sgn;
}


/* -----------------------------------------------
	decodes high (7x7) AC coefficients to pjg
	----------------------------------------------- */
void pjg::decode::ac_low(const std::unique_ptr<aricoder>& dec, int cmp)
{	
	signed short* coeffs_x[ 8 ]; // prediction coeffs - current block
	signed short* coeffs_a[ 8 ]; // prediction coeffs - neighboring block
	int pred_cf[ 8 ]; // prediction multipliers
	
	// init models for bitlenghts and -patterns
	auto mod_len = INIT_MODEL_S(11, std::max(int(segm_cnt[cmp]), 11), 2);
	auto mod_res = INIT_MODEL_B(1 << 4, 2);
	auto mod_top = INIT_MODEL_B(1 << std::max(4, int(nois_trs[cmp])), 3);
	auto mod_sgn = INIT_MODEL_B(11, 1);
	
	// set width/height of each band
	const int bc = cmpnfo[cmp].bc;
	const int w = cmpnfo[cmp].bch;
	
	// work through each first row / first collumn band
	for (int i = 2; i < 16; i++ )
	{		
		// alternate between first row and first collumn
		int b_x = ( i % 2 == 0 ) ? i / 2 : 0;
		int b_y = ( i % 2 == 1 ) ? i / 2 : 0;
		const int bpos = (int) zigzag[ b_x + (8*b_y) ];
		
		// locally store pointer to band coefficients
		short* coeffs = dct::colldata[ cmp ][ bpos ]; // Pointer to current coefficent data.
		// store pointers to prediction coefficients
		int p_x, p_y;
		int* edge_c; // edge criteria
		unsigned char* zdstls; // Pointer to row/col # of non-zeroes.
		if ( b_x == 0 ) {
			for ( ; b_x < 8; b_x++ ) {
				coeffs_x[ b_x ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ];
				coeffs_a[ b_x ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ] - 1;
				pred_cf[ b_x ] = dct::icos_base_8x8[ b_x * 8 ] * QUANT ( cmp, zigzag[b_x+(8*b_y)] );
			} b_x = 0;
			zdstls = pjg::zdstylow[ cmp ];
			edge_c = &p_x;
		}
		else { // if ( b_y == 0 )
			for ( ; b_y < 8; b_y++ ) {
				coeffs_x[ b_y ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ];
				coeffs_a[ b_y ] = dct::colldata[ cmp ][ zigzag[b_x+(8*b_y)] ] - w;
				pred_cf[ b_y ] = dct::icos_base_8x8[ b_y * 8 ] * QUANT ( cmp, zigzag[b_x+(8*b_y)] );
			} b_y = 0;
			zdstls = pjg::zdstxlow[ cmp ];
			edge_c = &p_y;
		}
		
		// get max bit length / other info
		const int max_valp = MAX_V( cmp, bpos ); // Max value (positive).
		const int max_valn = -max_valp; // Max value (negative).
		const int max_len = bitlen1024p( max_valp ); // Max bitlength.
		const int thrs_bp = ( max_len > nois_trs[cmp] ) ? max_len - nois_trs[cmp] : 0; // Residual threshold bitplane.
		
		// arithmetic compression loop
		for (int dpos = 0; dpos < bc; dpos++ )
		{
			// skip if beyound eob
			if ( zdstls[ dpos ] == 0 )
				continue;
			
			//calculate x/y positions in band
			p_y = dpos / w;
			p_x = dpos % w;
			
			// edge treatment / calculate LAKHANI context
			int ctx_lak; // Lakhani context.
			if ( (*edge_c) > 0 )
				ctx_lak = pjg::lakh_context( coeffs_x, coeffs_a, pred_cf, dpos );
			else ctx_lak = 0;
			ctx_lak = clamp(ctx_lak, max_valn, max_valp);
			const int ctx_len = bitlen2048n( ctx_lak ); // Bitlength context.				
			// shift context / do context modelling (segmentation is done per context)
			shift_model( mod_len, ctx_len, zdstls[ dpos ] );
			mod_len->exclude_symbols(max_len);
			
			// decode bit length of current coefficient
			const int clen = dec->decode_ari(mod_len );
			// simple treatment if coefficients == 0
			if ( clen == 0 ) {
				// coeffs[ dpos ] = 0;
			}
			else {
				// decoding of residual
				int bp = clen - 2; // first set bit must be 1, so we start at clen - 2
				int ctx_res = ( bp >= thrs_bp ) ? 1 : 0; // Bit plane context for residual.
				const int ctx_abs = std::abs( ctx_lak ); // Absolute context.
				const int ctx_sgn = ( ctx_lak == 0 ) ? 0 : ( ctx_lak > 0 ) ? 1 : 2; // Context for sign.
				for ( ; bp >= thrs_bp; bp-- ) {						
					shift_model( mod_top, ctx_abs >> thrs_bp, ctx_res, clen - thrs_bp ); // shift in 3 contexts
					// decode bit
					const int bt = dec->decode_ari(mod_top );
					// update context
					ctx_res = ctx_res << 1;
					if ( bt ) ctx_res |= 1; 
				}
				int absv = ( ctx_res == 0 ) ? 1 : ctx_res; // !!!!
				for ( ; bp >= 0; bp-- ) {
					shift_model( mod_res, zdstls[ dpos ], bp ); // shift in 2 contexts
					// decode bit
					const int bt = dec->decode_ari(mod_res );
					// update absv
					absv = absv << 1;
					if ( bt ) absv |= 1; 
				}
				// decode sign
				shift_model( mod_sgn, zdstls[ dpos ], ctx_sgn );
				const int sgn = dec->decode_ari(mod_sgn );
				// copy to colldata
				coeffs[ dpos ] = ( sgn == 0 ) ? absv : -absv;
				// decrement # of non zeroes
				zdstls[ dpos ]--;
			}
		}
		// flush models
		mod_len->flush_model();
		mod_res->flush_model();
		mod_top->flush_model();
		mod_sgn->flush_model();
	}
	
	// clear models
	delete mod_len;
	delete mod_res;
	delete mod_top;
	delete mod_sgn;
}


/* -----------------------------------------------
	deodes a stream of generic (8bit) data from pjg
	----------------------------------------------- */
bool pjg::decode::generic( const std::unique_ptr<aricoder>& dec, unsigned char** data, int* len )
{
	// start byte writer
	auto bwrt = std::make_unique<abytewriter>(1024);
	
	// decode header, ending with 256 symbol
	auto model = INIT_MODEL_S(256 + 1, 256, 1);
	while ( true ) {
		const int c = dec->decode_ari(model );
		if ( c == 256 ) break;
		bwrt->write( (unsigned char) c );
		model->shift_context( c );
	}
	delete model;
	
	// check for out of memory
	if ( bwrt->error() ) {
		sprintf( errormessage, MEM_ERRMSG.c_str());
		errorlevel = 2;
		return false;
	}
	
	// get data/length and close byte writer
	(*data) = bwrt->getptr();
	if ( len != nullptr ) (*len) = bwrt->getpos();
	
	return true;
}

static std::vector<std::uint8_t> pjg::decode::generic(const std::unique_ptr<aricoder>& dec) {
	auto bwrt = std::make_unique<abytewriter>(1024);
	auto model = INIT_MODEL_S(256 + 1, 256, 1);
	while (true) {
		int c = dec->decode_ari(model);
		if (c == 256) {
			break;
		}
		bwrt->write((unsigned char)c);
		model->shift_context(c);
	}

	// check for out of memory
	if (bwrt->error()) {
		sprintf(errormessage, MEM_ERRMSG.c_str());
		errorlevel = 2;
		return std::vector<std::uint8_t>();
	}

	auto data = bwrt->getptr();
	auto length = bwrt->getpos();
	return std::vector<std::uint8_t>(data, data + length);

}


/* -----------------------------------------------
	decodes one bit from pjg
	----------------------------------------------- */
std::uint8_t pjg::decode::bit(const std::unique_ptr<aricoder>& dec)
{
	auto model = INIT_MODEL_B(1, -1);
	std::uint8_t bit = dec->decode_ari(model); // This conversion is okay since there are only 2 symbols in the model.
	delete model;
	return bit;
}

void pjg::encode::get_zerosort_scan(unsigned char* sv, int cmpt)  {
	// Preset the unsorted scan index:
	std::array<uint8_t, 64> index;
	std::iota(std::begin(index), std::end(index), uint8_t(0)); // Initialize the unsorted scan with indices 0, 1, ..., 63.

	// Count the number of zeroes for each frequency:
	const int bc = cmpnfo[cmpt].bc;
	std::array<uint32_t, 64> zeroDist; // Distribution of zeroes per band.
	std::transform(dct::colldata[cmpt],
	               dct::colldata[cmpt] + 64,
	               std::begin(zeroDist),
	               [&](const short* freq) {
		               return std::count(freq, freq + bc, 0);
	               });

	// Sort in ascending order according to the number of zeroes per band:
	std::sort(std::begin(index) + 1, // Skip the first element.
	          std::end(index),
	          [&](const uint32_t& a, const uint32_t& b) {
		          return zeroDist[a] < zeroDist[b];
	          }
	);

	std::copy(std::begin(index), std::end(index), sv);
}

void pjg::encode::optimize_dqt(int hpos, int segment_length) {
	const int fpos = hpos + segment_length; // End of marker position.
	hpos += 4; // Skip marker and segment length data.
	while (hpos < fpos) {
		const int i = LBITS(hdrdata[hpos], 4);
		hpos++;
		// table found
		if (i == 1) { // get out for 16 bit precision
			hpos += 128;
			continue;
		}
		// do diff coding for 8 bit precision
		for (int sub_pos = 63; sub_pos > 0; sub_pos--) {
			hdrdata[hpos + sub_pos] -= hdrdata[hpos + sub_pos - 1];
		}

		hpos += 64;
	}
}

void pjg::encode::optimize_dht(int hpos, int segment_length) {
	const int fpos = hpos + segment_length; // End of marker position.
	hpos += 4; // Skip marker and segment length data.
	while (hpos < fpos) {
		hpos++;
		// table found - compare with each of the four standard tables		
		for (int i = 0; i < 4; i++) {
			int sub_pos;
			for (sub_pos = 0; sub_pos < std_huff_lengths[i]; sub_pos++) {
				if (hdrdata[hpos + sub_pos] != std_huff_tables[i][sub_pos]) {
					break;
				}
			}
			// check if comparison ok
			if (sub_pos != std_huff_lengths[i]) {
				continue;
			}

			// if we get here, the table matches the standard table
			// number 'i', so it can be replaced
			hdrdata[hpos + 0] = std_huff_lengths[i] - 16 - i;
			hdrdata[hpos + 1] = i;
			for (sub_pos = 2; sub_pos < std_huff_lengths[i]; sub_pos++) {
				hdrdata[hpos + sub_pos] = 0x00;
			}
			// everything done here, so leave
			break;
		}

		int skip = 16; // Num bytes to skip.
		for (int i = 0; i < 16; i++) {
			skip += int(hdrdata[hpos + i]);
		}
		hpos += skip;
	}
}

void pjg::encode::optimize_header() {
	int hpos = 0; // Current position in the header.

	// Header parser loop:
	while (hpos < hdrs) {
		const std::uint8_t type = hdrdata[hpos + 1]; // Type of the current marker segment.
		const int len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] ); // Length of the current marker segment.
		if (type == 0xC4) { // DHT segment:
			optimize_dht(hpos, len);
		} else if (type == 0xDB) { // DQT segment:
			optimize_dqt(hpos, len);
		} else {
			// Skip other segments.
		}
		hpos += len;
	}
}


void pjg::decode::deoptimize_dqt(int hpos, int segment_length) {
	int fpos = hpos + segment_length;
	hpos += 4; // Skip marker and segment length data.
	while (hpos < fpos) {
		const int i = LBITS( hdrdata[ hpos ], 4 );
		hpos++;
		// table found
		if (i == 1) { // get out for 16 bit precision
			hpos += 128;
			continue;
		}
		// undo diff coding for 8 bit precision
		for (int sub_pos = 1; sub_pos < 64; sub_pos++) {
			hdrdata[hpos + sub_pos] += hdrdata[hpos + sub_pos - 1];
		}

		hpos += 64;
	}
}

void pjg::decode::deoptimize_dht(int hpos, int segment_length) {
	const int fpos = hpos + segment_length; // End of segment in hdrdata.
	hpos += 4; // Skip marker and segment length data.
	while (hpos < fpos) {
		hpos++;
		// table found - check if modified
		if (hdrdata[hpos] > 2) {
			// reinsert the standard table
			const int i = hdrdata[hpos + 1];
			for (int sub_pos = 0; sub_pos < std_huff_lengths[i]; sub_pos++) {
				hdrdata[hpos + sub_pos] = std_huff_tables[i][sub_pos];
			}
		}

		int skip = 16; // Num bytes to skip.
		for (int i = 0; i < 16; i++) {
			skip += int(hdrdata[hpos + i]);
		}
		hpos += skip;
	}
}

void pjg::decode::deoptimize_header() {
	int hpos = 0; // Current position in the header.

	// Header parser loop:
	while (hpos < hdrs) {
		const std::uint8_t type = hdrdata[hpos + 1]; // Type of current marker segment.
		const int len = 2 + pack( hdrdata[ hpos + 2 ], hdrdata[ hpos + 3 ] ); // Length of current marker segment.

		if (type == 0xC4) { // DHT segment.
			deoptimize_dht(hpos, len);
		} else if (type == 0xDB) { // DQT segment.
			deoptimize_dqt(hpos, len);
		} else {
			// Skip other segments.
		}
		hpos += len;
	}
}


/* -----------------------------------------------
	preparations for special average context
	----------------------------------------------- */
void pjg::aavrg_prepare( unsigned short** abs_coeffs, int* weights, unsigned short* abs_store, int cmp )
{
	int w = cmpnfo[cmp].bch;
	
	// set up quick access arrays for all prediction positions
	abs_coeffs[ 0 ] = abs_store + (  0 + ((-2)*w) ); // top-top
	abs_coeffs[ 1 ] = abs_store + ( -1 + ((-1)*w) ); // top-left
	abs_coeffs[ 2 ] = abs_store + (  0 + ((-1)*w) ); // top
	abs_coeffs[ 3 ] = abs_store + (  1 + ((-1)*w) ); // top-right
	abs_coeffs[ 4 ] = abs_store + ( -2 + (( 0)*w) ); // left-left
	abs_coeffs[ 5 ] = abs_store + ( -1 + (( 0)*w) ); // left
	// copy context weighting factors
	weights[ 0 ] = abs_ctx_weights_lum[ 0 ][ 0 ][ 2 ]; // top-top
	weights[ 1 ] = abs_ctx_weights_lum[ 0 ][ 1 ][ 1 ]; // top-left
	weights[ 2 ] = abs_ctx_weights_lum[ 0 ][ 1 ][ 2 ]; // top
	weights[ 3 ] = abs_ctx_weights_lum[ 0 ][ 1 ][ 3 ]; // top-right
	weights[ 4 ] = abs_ctx_weights_lum[ 0 ][ 2 ][ 0 ]; // left-left
	weights[ 5 ] = abs_ctx_weights_lum[ 0 ][ 2 ][ 1 ]; // left
}


/* -----------------------------------------------
	special average context used in coeff encoding
	----------------------------------------------- */
int pjg::aavrg_context( unsigned short** abs_coeffs, int* weights, int pos, int p_y, int p_x, int r_x )
{
	int ctx_avr = 0; // AVERAGE context
	int w_ctx = 0; // accumulated weight of context
	int w_curr; // current weight of context
	
	
	// different cases due to edge treatment
	if ( p_y >= 2 ) {
		w_curr = weights[ 0 ]; ctx_avr += abs_coeffs[ 0 ][ pos ] * w_curr; w_ctx += w_curr;
		w_curr = weights[ 2 ]; ctx_avr += abs_coeffs[ 2 ][ pos ] * w_curr; w_ctx += w_curr;
		if ( p_x >= 2 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_coeffs[ 1 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 4 ]; ctx_avr += abs_coeffs[ 4 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_coeffs[ 5 ][ pos ] * w_curr; w_ctx += w_curr;
		}
		else if ( p_x == 1 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_coeffs[ 1 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_coeffs[ 5 ][ pos ] * w_curr; w_ctx += w_curr;
		}
		if ( r_x >= 1 ) {
			w_curr = weights[ 3 ]; ctx_avr += abs_coeffs[ 3 ][ pos ] * w_curr; w_ctx += w_curr;
		}
	}
	else if ( p_y == 1 ) {
		w_curr = weights[ 2 ]; ctx_avr += abs_coeffs[ 2 ][ pos ] * w_curr; w_ctx += w_curr;
		if ( p_x >= 2 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_coeffs[ 1 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 4 ]; ctx_avr += abs_coeffs[ 4 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_coeffs[ 5 ][ pos ] * w_curr; w_ctx += w_curr;
		}
		else if ( p_x == 1 ) {
			w_curr = weights[ 1 ]; ctx_avr += abs_coeffs[ 1 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_coeffs[ 5 ][ pos ] * w_curr; w_ctx += w_curr;
		}
		if ( r_x >= 1 ) {
			w_curr = weights[ 3 ]; ctx_avr += abs_coeffs[ 3 ][ pos ] * w_curr; w_ctx += w_curr;
		}
	}
	else {
		if ( p_x >= 2 ) {
			w_curr = weights[ 4 ]; ctx_avr += abs_coeffs[ 4 ][ pos ] * w_curr; w_ctx += w_curr;
			w_curr = weights[ 5 ]; ctx_avr += abs_coeffs[ 5 ][ pos ] * w_curr; w_ctx += w_curr;
		}
		else if ( p_x == 1 ) {
			w_curr = weights[ 5 ]; ctx_avr += abs_coeffs[ 5 ][ pos ] * w_curr; w_ctx += w_curr;
		}
	}
	
	// return average context
	return ( w_ctx != 0 ) ? ( ctx_avr + ( w_ctx / 2 ) ) / w_ctx : 0;
}


/* -----------------------------------------------
	lakhani ac context used in coeff encoding
	----------------------------------------------- */
int pjg::lakh_context( signed short** coeffs_x, signed short** coeffs_a, int* pred_cf, int pos )
{
	int pred = 0;
	
	// calculate partial prediction
	pred -= ( coeffs_x[ 1 ][ pos ] + coeffs_a[ 1 ][ pos ] ) * pred_cf[ 1 ];
	pred -= ( coeffs_x[ 2 ][ pos ] - coeffs_a[ 2 ][ pos ] ) * pred_cf[ 2 ];
	pred -= ( coeffs_x[ 3 ][ pos ] + coeffs_a[ 3 ][ pos ] ) * pred_cf[ 3 ];
	pred -= ( coeffs_x[ 4 ][ pos ] - coeffs_a[ 4 ][ pos ] ) * pred_cf[ 4 ];
	pred -= ( coeffs_x[ 5 ][ pos ] + coeffs_a[ 5 ][ pos ] ) * pred_cf[ 5 ];
	pred -= ( coeffs_x[ 6 ][ pos ] - coeffs_a[ 6 ][ pos ] ) * pred_cf[ 6 ];
	pred -= ( coeffs_x[ 7 ][ pos ] + coeffs_a[ 7 ][ pos ] ) * pred_cf[ 7 ];
	// normalize / quantize partial prediction
	pred = ( ( pred > 0 ) ? ( pred + (pred_cf[0]/2) ) : ( pred - (pred_cf[0]/2) ) ) / pred_cf[ 0 ];
	// complete prediction
	pred += coeffs_a[ 0 ][ pos ];
	
	return pred;
}


/* -----------------------------------------------
	Calculates coordinates for nearest neighbor context
	----------------------------------------------- */
std::pair<int, int> pjg::get_context_nnb(int pos, int w)
{
	// this function calculates and returns coordinates for
	// a simple 2D context
	std::pair<int, int> coords;
	if (pos == 0) {
		coords = std::make_pair<int, int>(-1, -1);
	} else if ((pos % w) == 0) {
		if (pos >= (w << 1)) {
			coords = std::make_pair<int, int>(pos - (w << 1), pos - w);
		} else {
			coords = std::make_pair<int, int>(pos - w, pos - w);
		}
	} else if (pos < w) {
		if (pos >= 2) {
			coords = std::make_pair<int, int>(pos - 1, pos - 2);
		} else {
			coords = std::make_pair<int, int>(pos - 1, pos - 1);
		}
	} else {
		coords = std::make_pair<int, int>(pos - 1, pos - w);
	}
	return coords;
}

/* ----------------------- End of PJG specific functions -------------------------- */

/* ----------------------- Begin of DCT specific functions -------------------------- */

#if !defined(BUILD_LIB) && defined(DEV_BUILD)
int dct::idct_2d_fst_8x8(int cmp, int dpos, int ix, int iy) {
	// calculate start index
	const int ixy = ((iy << 3) + ix) << 6;

	// begin transform
	int idct = 0;
	idct += dct::colldata[cmp][0][dpos] * dct::adpt_idct_8x8[cmp][ixy + 0];
	idct += dct::colldata[cmp][1][dpos] * dct::adpt_idct_8x8[cmp][ixy + 1];
	idct += dct::colldata[cmp][5][dpos] * dct::adpt_idct_8x8[cmp][ixy + 2];
	idct += dct::colldata[cmp][6][dpos] * dct::adpt_idct_8x8[cmp][ixy + 3];
	idct += dct::colldata[cmp][14][dpos] * dct::adpt_idct_8x8[cmp][ixy + 4];
	idct += dct::colldata[cmp][15][dpos] * dct::adpt_idct_8x8[cmp][ixy + 5];
	idct += dct::colldata[cmp][27][dpos] * dct::adpt_idct_8x8[cmp][ixy + 6];
	idct += dct::colldata[cmp][28][dpos] * dct::adpt_idct_8x8[cmp][ixy + 7];
	idct += dct::colldata[cmp][2][dpos] * dct::adpt_idct_8x8[cmp][ixy + 8];
	idct += dct::colldata[cmp][4][dpos] * dct::adpt_idct_8x8[cmp][ixy + 9];
	idct += dct::colldata[cmp][7][dpos] * dct::adpt_idct_8x8[cmp][ixy + 10];
	idct += dct::colldata[cmp][13][dpos] * dct::adpt_idct_8x8[cmp][ixy + 11];
	idct += dct::colldata[cmp][16][dpos] * dct::adpt_idct_8x8[cmp][ixy + 12];
	idct += dct::colldata[cmp][26][dpos] * dct::adpt_idct_8x8[cmp][ixy + 13];
	idct += dct::colldata[cmp][29][dpos] * dct::adpt_idct_8x8[cmp][ixy + 14];
	idct += dct::colldata[cmp][42][dpos] * dct::adpt_idct_8x8[cmp][ixy + 15];
	idct += dct::colldata[cmp][3][dpos] * dct::adpt_idct_8x8[cmp][ixy + 16];
	idct += dct::colldata[cmp][8][dpos] * dct::adpt_idct_8x8[cmp][ixy + 17];
	idct += dct::colldata[cmp][12][dpos] * dct::adpt_idct_8x8[cmp][ixy + 18];
	idct += dct::colldata[cmp][17][dpos] * dct::adpt_idct_8x8[cmp][ixy + 19];
	idct += dct::colldata[cmp][25][dpos] * dct::adpt_idct_8x8[cmp][ixy + 20];
	idct += dct::colldata[cmp][30][dpos] * dct::adpt_idct_8x8[cmp][ixy + 21];
	idct += dct::colldata[cmp][41][dpos] * dct::adpt_idct_8x8[cmp][ixy + 22];
	idct += dct::colldata[cmp][43][dpos] * dct::adpt_idct_8x8[cmp][ixy + 23];
	idct += dct::colldata[cmp][9][dpos] * dct::adpt_idct_8x8[cmp][ixy + 24];
	idct += dct::colldata[cmp][11][dpos] * dct::adpt_idct_8x8[cmp][ixy + 25];
	idct += dct::colldata[cmp][18][dpos] * dct::adpt_idct_8x8[cmp][ixy + 26];
	idct += dct::colldata[cmp][24][dpos] * dct::adpt_idct_8x8[cmp][ixy + 27];
	idct += dct::colldata[cmp][31][dpos] * dct::adpt_idct_8x8[cmp][ixy + 28];
	idct += dct::colldata[cmp][40][dpos] * dct::adpt_idct_8x8[cmp][ixy + 29];
	idct += dct::colldata[cmp][44][dpos] * dct::adpt_idct_8x8[cmp][ixy + 30];
	idct += dct::colldata[cmp][53][dpos] * dct::adpt_idct_8x8[cmp][ixy + 31];
	idct += dct::colldata[cmp][10][dpos] * dct::adpt_idct_8x8[cmp][ixy + 32];
	idct += dct::colldata[cmp][19][dpos] * dct::adpt_idct_8x8[cmp][ixy + 33];
	idct += dct::colldata[cmp][23][dpos] * dct::adpt_idct_8x8[cmp][ixy + 34];
	idct += dct::colldata[cmp][32][dpos] * dct::adpt_idct_8x8[cmp][ixy + 35];
	idct += dct::colldata[cmp][39][dpos] * dct::adpt_idct_8x8[cmp][ixy + 36];
	idct += dct::colldata[cmp][45][dpos] * dct::adpt_idct_8x8[cmp][ixy + 37];
	idct += dct::colldata[cmp][52][dpos] * dct::adpt_idct_8x8[cmp][ixy + 38];
	idct += dct::colldata[cmp][54][dpos] * dct::adpt_idct_8x8[cmp][ixy + 39];
	idct += dct::colldata[cmp][20][dpos] * dct::adpt_idct_8x8[cmp][ixy + 40];
	idct += dct::colldata[cmp][22][dpos] * dct::adpt_idct_8x8[cmp][ixy + 41];
	idct += dct::colldata[cmp][33][dpos] * dct::adpt_idct_8x8[cmp][ixy + 42];
	idct += dct::colldata[cmp][38][dpos] * dct::adpt_idct_8x8[cmp][ixy + 43];
	idct += dct::colldata[cmp][46][dpos] * dct::adpt_idct_8x8[cmp][ixy + 44];
	idct += dct::colldata[cmp][51][dpos] * dct::adpt_idct_8x8[cmp][ixy + 45];
	idct += dct::colldata[cmp][55][dpos] * dct::adpt_idct_8x8[cmp][ixy + 46];
	idct += dct::colldata[cmp][60][dpos] * dct::adpt_idct_8x8[cmp][ixy + 47];
	idct += dct::colldata[cmp][21][dpos] * dct::adpt_idct_8x8[cmp][ixy + 48];
	idct += dct::colldata[cmp][34][dpos] * dct::adpt_idct_8x8[cmp][ixy + 49];
	idct += dct::colldata[cmp][37][dpos] * dct::adpt_idct_8x8[cmp][ixy + 50];
	idct += dct::colldata[cmp][47][dpos] * dct::adpt_idct_8x8[cmp][ixy + 51];
	idct += dct::colldata[cmp][50][dpos] * dct::adpt_idct_8x8[cmp][ixy + 52];
	idct += dct::colldata[cmp][56][dpos] * dct::adpt_idct_8x8[cmp][ixy + 53];
	idct += dct::colldata[cmp][59][dpos] * dct::adpt_idct_8x8[cmp][ixy + 54];
	idct += dct::colldata[cmp][61][dpos] * dct::adpt_idct_8x8[cmp][ixy + 55];
	idct += dct::colldata[cmp][35][dpos] * dct::adpt_idct_8x8[cmp][ixy + 56];
	idct += dct::colldata[cmp][36][dpos] * dct::adpt_idct_8x8[cmp][ixy + 57];
	idct += dct::colldata[cmp][48][dpos] * dct::adpt_idct_8x8[cmp][ixy + 58];
	idct += dct::colldata[cmp][49][dpos] * dct::adpt_idct_8x8[cmp][ixy + 59];
	idct += dct::colldata[cmp][57][dpos] * dct::adpt_idct_8x8[cmp][ixy + 60];
	idct += dct::colldata[cmp][58][dpos] * dct::adpt_idct_8x8[cmp][ixy + 61];
	idct += dct::colldata[cmp][62][dpos] * dct::adpt_idct_8x8[cmp][ixy + 62];
	idct += dct::colldata[cmp][63][dpos] * dct::adpt_idct_8x8[cmp][ixy + 63];

	return idct;
}
#endif

int dct::idct_2d_fst_8x1(int cmp, int dpos, int ix, int iy) {
	// calculate start index
	const int ixy = ix << 3;

	// begin transform
	int idct = 0;
	idct += dct::colldata[cmp][0][dpos] * dct::adpt_idct_8x1[cmp][ixy + 0];
	idct += dct::colldata[cmp][1][dpos] * dct::adpt_idct_8x1[cmp][ixy + 1];
	idct += dct::colldata[cmp][5][dpos] * dct::adpt_idct_8x1[cmp][ixy + 2];
	idct += dct::colldata[cmp][6][dpos] * dct::adpt_idct_8x1[cmp][ixy + 3];
	idct += dct::colldata[cmp][14][dpos] * dct::adpt_idct_8x1[cmp][ixy + 4];
	idct += dct::colldata[cmp][15][dpos] * dct::adpt_idct_8x1[cmp][ixy + 5];
	idct += dct::colldata[cmp][27][dpos] * dct::adpt_idct_8x1[cmp][ixy + 6];
	idct += dct::colldata[cmp][28][dpos] * dct::adpt_idct_8x1[cmp][ixy + 7];

	return idct;
}

int dct::idct_2d_fst_1x8(int cmp, int dpos, int ix, int iy) {
	// calculate start index
	const int ixy = iy << 3;

	// begin transform
	int idct = 0;
	idct += dct::colldata[cmp][0][dpos] * dct::adpt_idct_1x8[cmp][ixy + 0];
	idct += dct::colldata[cmp][2][dpos] * dct::adpt_idct_1x8[cmp][ixy + 1];
	idct += dct::colldata[cmp][3][dpos] * dct::adpt_idct_1x8[cmp][ixy + 2];
	idct += dct::colldata[cmp][9][dpos] * dct::adpt_idct_1x8[cmp][ixy + 3];
	idct += dct::colldata[cmp][10][dpos] * dct::adpt_idct_1x8[cmp][ixy + 4];
	idct += dct::colldata[cmp][20][dpos] * dct::adpt_idct_1x8[cmp][ixy + 5];
	idct += dct::colldata[cmp][21][dpos] * dct::adpt_idct_1x8[cmp][ixy + 6];
	idct += dct::colldata[cmp][35][dpos] * dct::adpt_idct_1x8[cmp][ixy + 7];

	return idct;
}

/* ----------------------- End of DCT specific functions -------------------------- */

/* ----------------------- Begin of prediction functions -------------------------- */

#if defined(USE_PLOCOI)
int predictor::dc_coll_predictor(int cmp, int dpos)
{
	const short* coeffs = dct::colldata[cmp][0];
	const int w = cmpnfo[cmp].bch;
	int a = 0;
	int b = 0;
	int c = 0;
	
	if (dpos < w) {
		a = coeffs[dpos - 1];
	} else if ((dpos%w) == 0) {
		b = coeffs[dpos - w];
	} else {
		a = coeffs[dpos - 1];
		b = coeffs[dpos - w];
		c = coeffs[dpos - 1 - w];
	}
	
	return predictor::plocoi(a, b, c);
}

int predictor::plocoi(int a, int b, int c)
{
	// a -> left; b -> above; c -> above-left

	const int min = std::min(a, b);
	const int max = std::max(a, b);

	if (c >= max) return min;
	if (c <= min) return max;

	return a + b - c;
}

#endif

#if !defined(USE_PLOCOI)
int predictor::dc_1ddct_predictor(int cmp, int dpos) {
	const int w = cmpnfo[cmp].bch;
	const int px = dpos % w;
	const int py = dpos / w;

	// Store current block DC coefficient:
	const short swap = dct::colldata[cmp][0][dpos];
	dct::colldata[cmp][0][dpos] = short(0);

	// Calculate prediction:
	int pred = 0;
	if (px > 0 && py > 0) {
		const int pa = dct::idct_2d_fst_8x1(cmp, dpos - 1, 7, 0);
		const int xa = dct::idct_2d_fst_8x1(cmp, dpos, 0, 0);

		const int pb = dct::idct_2d_fst_1x8(cmp, dpos - w, 0, 7);
		const int xb = dct::idct_2d_fst_1x8(cmp, dpos, 0, 0);

		pred = ((pa - xa) + (pb - xb)) * 4;
	} else if (px > 0) {
		const int pa = dct::idct_2d_fst_8x1(cmp, dpos - 1, 7, 0);
		const int xa = dct::idct_2d_fst_8x1(cmp, dpos, 0, 0);

		pred = (pa - xa) * 8;
	} else if (py > 0) {
		const int pb = dct::idct_2d_fst_1x8(cmp, dpos - w, 0, 7);
		const int xb = dct::idct_2d_fst_1x8(cmp, dpos, 0, 0);

		pred = (pb - xb) * 8;
	}

	// Write back current DCT coefficient:
	dct::colldata[cmp][0][dpos] = swap;

	// Clamp and quantize predictor:
	pred = clamp(pred, -(1024 * dct::DCT_RSC_FACTOR), 1016 * dct::DCT_RSC_FACTOR);
	pred = pred / QUANT(cmp, 0);
	pred = dct::DCT_RESCALE(pred);

	return pred;
}
#endif

/* ----------------------- End of prediction functions -------------------------- */

/* ----------------------- Begin of miscellaneous helper functions -------------------------- */


/* -----------------------------------------------
	displays progress bar on screen
	----------------------------------------------- */
#if !defined(BUILD_LIB)
static void progress_bar( int current, int last )
{
	constexpr int BARLEN = 36;
	int barpos = ((current * BARLEN) + (last / 2)) / last;

	// generate progress bar
	fprintf(msgout, "[");
	for (int i = 0; i < BARLEN; i++) {
		if (i < barpos) {
			#if defined(_WIN32)
			fprintf(msgout, "\xFE");
			#else
			fprintf(msgout, "X");
			#endif
		} else {
			fprintf(msgout, " ");
		}
	}
	fprintf(msgout, "]");
}

/* -----------------------------------------------
	Replaces the file extension of oldname (if any) with new_extension.
	----------------------------------------------- */

static std::string create_filename(const std::string& oldname, const std::string& new_extension) {
	auto filename_base = oldname.substr(0, oldname.find_last_of("."));
	auto filename = filename_base + "." + new_extension;
	return filename;
}

/* -----------------------------------------------
	Replaces the file extension of oldname (if any) with new_extension.
	If such a file already exists, then underscores are appended to the filename (e.g., filename_.ext)
	until the filename chosen does not already exist.
	----------------------------------------------- */

static std::string unique_filename(const std::string& oldname, const std::string& new_extension) {
	auto filename_base = oldname.substr(0, oldname.find_last_of("."));
	auto filename = filename_base + "." + new_extension;
	while (file_exists(filename)) {
		filename_base += "_";
		filename = filename_base + "." + new_extension;
	}
	return filename;
}
#endif

/* -----------------------------------------------
checks if a file exists
----------------------------------------------- */
static bool file_exists(const std::string& filename) {
	// needed for both, executable and library
	FILE* fp = fopen(filename.c_str(), "rb");

	if (fp == nullptr) {
		return false;
	} else {
		fclose(fp);
		return true;
	}
}

/* ----------------------- End of miscellaneous helper functions -------------------------- */

/* ----------------------- Begin of developers functions -------------------------- */


/* -----------------------------------------------
	Writes header file
	----------------------------------------------- */
#if !defined(BUILD_LIB) && defined(DEV_BUILD)
INTERN bool dump_hdr() {
	const std::string ext = "hdr";
	const auto basename = filelist[file_no];

	if (!dump_file(basename, ext, hdrdata, 1, hdrs)) {
		return false;
	}

	return true;
}

/* -----------------------------------------------
	Writes huffman coded file
	----------------------------------------------- */
INTERN bool dump_huf() {
	const std::string ext = "huf";
	const auto basename = filelist[file_no];

	if (!dump_file(basename, ext, huffdata.data(), 1, huffdata.size())) {
		return false;
	}

	return true;
}

/* -----------------------------------------------
	Writes collections of DCT coefficients
	----------------------------------------------- */
INTERN bool dump_coll()
{
	const std::array<std::string, 4> ext = { "coll0", "coll1", "coll2", "coll3" };
	const auto& base = filelist[file_no];

	for (int cmp = 0; cmp < image::cmpc; cmp++) {
		// create filename
		const auto fn = create_filename(base, ext[cmp]);

		// open file for output
		FILE* fp = fopen(fn.c_str(), "wb");
		if (fp == nullptr) {
			sprintf(errormessage, FWR_ERRMSG.c_str(), fn.c_str());
			errorlevel = 2;
			return false;
		}

		int dpos;
		switch (coll_mode) {

		case CollectionMode::STD:
			for (int bpos = 0; bpos < 64; bpos++) {
				fwrite(dct::colldata[cmp][bpos], sizeof(short), cmpnfo[cmp].bc, fp);
			}
			break;

		case CollectionMode::DHF:
			for (dpos = 0; dpos < cmpnfo[cmp].bc; dpos++) {
				for (int bpos = 0; bpos < 64; bpos++) {
					fwrite(&(dct::colldata[cmp][bpos][dpos]), sizeof(short), 1, fp);
				}
			}
			break;

		case CollectionMode::SQU:
			dpos = 0;
			for (int i = 0; i < 64; ) {
				const int bpos = zigzag[i++];
				fwrite(&(dct::colldata[cmp][bpos][dpos]), sizeof(short),
					cmpnfo[cmp].bch, fp);
				if ((i % 8) == 0) {
					dpos += cmpnfo[cmp].bch;
					if (dpos >= cmpnfo[cmp].bc) {
						dpos = 0;
					} else {
						i -= 8;
					}
				}
			}
			break;

		case CollectionMode::UNC:
			for (int i = 0; i < (cmpnfo[cmp].bcv * 8); i++) {
				for (int j = 0; j < (cmpnfo[cmp].bch * 8); j++) {
					const int bpos = zigzag[((i % 8) * 8) + (j % 8)];
					dpos = ((i / 8) * cmpnfo[cmp].bch) + (j / 8);
					fwrite(&(dct::colldata[cmp][bpos][dpos]), sizeof(short), 1, fp);
				}
			}
			break;

		case CollectionMode::SQU_ALT:
			dpos = 0;
			for (int i = 0; i < 64; ) {
				int bpos = even_zigzag[i++];
				fwrite(&(dct::colldata[cmp][bpos][dpos]), sizeof(short),
					cmpnfo[cmp].bch, fp);
				if ((i % 8) == 0) {
					dpos += cmpnfo[cmp].bch;
					if (dpos >= cmpnfo[cmp].bc) {
						dpos = 0;
					} else {
						i -= 8;
					}
				}
			}
			break;

		case CollectionMode::UNC_ALT:
			for (int i = 0; i < (cmpnfo[cmp].bcv * 8); i++) {
				for (int j = 0; j < (cmpnfo[cmp].bch * 8); j++) {
					const int bpos = even_zigzag[((i % 8) * 8) + (j % 8)];
					dpos = ((i / 8) * cmpnfo[cmp].bch) + (j / 8);
					fwrite(&(dct::colldata[cmp][bpos][dpos]), sizeof(short), 1, fp);
				}
			}
			break;
		}

		fclose(fp);
	}

	return true;
}

/* -----------------------------------------------
	Writes zero distribution data to file;
	----------------------------------------------- */
INTERN bool dump_zdst() {
	const std::array<std::string, 4> ext = { "zdst0", "zdst1", "zdst2", "zdst3" };
	const auto basename = filelist[file_no];

	for (int cmp = 0; cmp < image::cmpc; cmp++) {
		if (!dump_file(basename, ext[cmp], pjg::zdstdata[cmp], 1, cmpnfo[cmp].bc)) {
			return false;
		}
	}

	return true;
}

/* -----------------------------------------------
	Writes to file
	----------------------------------------------- */
INTERN bool dump_file(const std::string& base, const std::string& ext, void* data, int bpv, int size) {
	// create filename
	const auto fn = create_filename(base, ext);

	// open file for output
	FILE* fp = fopen(fn.c_str(), "wb");
	if (fp == nullptr) {
		sprintf(errormessage, FWR_ERRMSG.c_str(), fn.c_str());
		errorlevel = 2;
		return false;
	}

	// write & close
	fwrite(data, bpv, size, fp);
	fclose(fp);

	return true;
}

/* -----------------------------------------------
	Writes error info file
	----------------------------------------------- */
INTERN bool dump_errfile() {
	// return immediately if theres no error
	if (errorlevel == 0) {
		return true;
	}

	// create filename based on errorlevel
	std::string fn;
	if (errorlevel == 1) {
		fn = create_filename(filelist[file_no], "wrn.nfo");
	} else {
		fn = create_filename(filelist[file_no], "err.nfo");
	}

	// open file for output
	FILE* fp = fopen(fn.c_str(), "w");
	if (fp == nullptr) {
		sprintf(errormessage, FWR_ERRMSG.c_str(), fn.c_str());
		errorlevel = 2;
		return false;
	}

	// write status and errormessage to file
	fprintf(fp, "--> error (level %i) in file \"%s\" <--\n", errorlevel, filelist[file_no].c_str());
	fprintf(fp, "\n");
	// write error specification to file
	fprintf(fp, " %s -> %s:\n", get_status(errorfunction).c_str(),
		(errorlevel == 1) ? "warning" : "error");
	fprintf(fp, " %s\n", errormessage);

	// done, close file
	fclose(fp);

	return true;
}

/* -----------------------------------------------
	Writes info to textfile
	----------------------------------------------- */
INTERN bool dump_info() {
	// create filename
	const auto fn = create_filename(filelist[file_no], "nfo");

	// open file for output
	FILE* fp = fopen(fn.c_str(), "w");
	if (fp == nullptr) {
		sprintf(errormessage, FWR_ERRMSG.c_str(), fn.c_str());
		errorlevel = 2;
		return false;
	}

	// info about image
	fprintf( fp, "<Infofile for JPEG image %s>\n\n\n", jpgfilename.c_str());
	fprintf( fp, "coding process: %s\n", ( jpegtype == JpegType::SEQUENTIAL ) ? "sequential" : "progressive" );
	// fprintf( fp, "no of scans: %i\n", jpg::scan_count );
	fprintf( fp, "imageheight: %i / imagewidth: %i\n", image::imgheight, image::imgwidth );
	fprintf( fp, "component count: %i\n", image::cmpc );
	fprintf( fp, "mcu count: %i/%i/%i (all/v/h)\n\n", image::mcuc, image::mcuv, image::mcuh );
	
	// info about header
	fprintf(fp, "\nfile header structure:\n");
	fprintf(fp, " type  length   hpos\n");
	// header parser loop
	int hpos; // Position in the header.
	int len = 0; // Length of current marker segment.
	for (hpos = 0; hpos < hdrs; hpos += len) {
		std::uint8_t type = hdrdata[hpos + 1]; // Type of current marker segment.
		len = 2 + pack(hdrdata[hpos + 2], hdrdata[hpos + 3]);
		fprintf(fp, " FF%2X  %6i %6i\n", (int)type, len, hpos);
	}
	fprintf(fp, " _END       0 %6i\n", hpos);
	fprintf(fp, "\n");

	// info about compression settings	
	fprintf(fp, "\ncompression settings:\n");
	fprintf(fp, " no of segments    ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
		segm_cnt[0], segm_cnt[1], segm_cnt[2], segm_cnt[3]);
	fprintf(fp, " noise threshold   ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
		nois_trs[0], nois_trs[1], nois_trs[2], nois_trs[3]);
	fprintf(fp, "\n");

	// info about components
	for (int cmp = 0; cmp < image::cmpc; cmp++) {
		fprintf(fp, "\n");
		fprintf(fp, "component number %i ->\n", cmp);
		fprintf(fp, "sample factors: %i/%i (v/h)\n", cmpnfo[cmp].sfv, cmpnfo[cmp].sfh);
		fprintf(fp, "blocks per mcu: %i\n", cmpnfo[cmp].mbs);
		fprintf(fp, "block count (mcu): %i/%i/%i (all/v/h)\n",
			cmpnfo[cmp].bc, cmpnfo[cmp].bcv, cmpnfo[cmp].bch);
		fprintf(fp, "block count (sng): %i/%i/%i (all/v/h)\n",
			cmpnfo[cmp].nc, cmpnfo[cmp].ncv, cmpnfo[cmp].nch);
		fprintf(fp, "quantiser table ->");
		for (int i = 0; i < 64; i++) {
			int bpos = zigzag[i];
			if ((i % 8) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%4i, ", QUANT(cmp, bpos));
		}
		fprintf(fp, "\n");
		fprintf(fp, "maximum values ->");
		for (int i = 0; i < 64; i++) {
			int bpos = zigzag[i];
			if ((i % 8) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%4i, ", MAX_V(cmp, bpos));
		}
		fprintf(fp, "\n\n");
	}

	fclose(fp);

	return true;
}

/* -----------------------------------------------
	Writes distribution for use in valdist.h
	----------------------------------------------- */
INTERN bool dump_dist() {
	// create filename
	const auto fn = create_filename(filelist[file_no], "dist");

	// open file for output
	FILE* fp = fopen(fn.c_str(), "wb");
	if (fp == nullptr) {
		sprintf(errormessage, FWR_ERRMSG.c_str(), fn.c_str());
		errorlevel = 2;
		return false;
	}

	// calculate & write distributions for each frequency
	for (int cmp = 0; cmp < image::cmpc; cmp++) {
		for (int bpos = 0; bpos < 64; bpos++) {
			std::array<int, 1024 + 1> dist = { 0 };
			// get distribution
			for (int dpos = 0; dpos < cmpnfo[cmp].bc; dpos++) {
				dist[std::abs(dct::colldata[cmp][bpos][dpos])]++;
			}
			// write to file
			fwrite(dist.data(), sizeof(int), dist.size(), fp);
		}
	}

	// close file
	fclose(fp);

	return true;
}

/* -----------------------------------------------
	Do inverse DCT and write pgms
	----------------------------------------------- */
INTERN bool dump_pgm() {
	const std::array<std::string, 4> ext = { "cmp0.pgm", "cmp1.pgm", "cmp2.pgm", "cmp3.pgm" };

	for (int cmp = 0; cmp < image::cmpc; cmp++) {
		// create filename
		const auto fn = create_filename(filelist[file_no], ext[cmp]);

		// open file for output
		FILE* fp = fopen(fn.c_str(), "wb");
		if (fp == nullptr) {
			sprintf(errormessage, FWR_ERRMSG.c_str(), fn.c_str());
			errorlevel = 2;
			return false;
		}

		// alloc memory for image data
		std::vector<std::uint8_t> imgdata(cmpnfo[cmp].bc * 64);

		for (int dpos = 0; dpos < cmpnfo[cmp].bc; dpos++) {
			// do inverse DCT, store in imgdata
			int dcpos = (((dpos / cmpnfo[cmp].bch) * cmpnfo[cmp].bch) << 6) +
				((dpos % cmpnfo[cmp].bch) << 3);
			for (int y = 0; y < 8; y++) {
				int ypos = dcpos + (y * (cmpnfo[cmp].bch << 3));
				for (int x = 0; x < 8; x++) {
					int xpos = ypos + x;
					int pix_v = dct::idct_2d_fst_8x8(cmp, dpos, x, y);
					pix_v = dct::DCT_RESCALE(pix_v);
					pix_v = pix_v + 128;
					imgdata[xpos] = std::uint8_t(clamp(pix_v, 0, 255));
				}
			}
		}

		// write PGM header
		fprintf(fp, "P5\n");
		fprintf(fp, "# created by %s v%i.%i%s (%s) by %s\n",
			program_info::apptitle.c_str(),
			program_info::appversion / 10,
			program_info::appversion % 10,
			program_info::subversion.c_str(),
			program_info::versiondate.c_str(),
			program_info::author.c_str());
		fprintf(fp, "%i %i\n", cmpnfo[cmp].bch * 8, cmpnfo[cmp].bcv * 8);
		fprintf(fp, "255\n");

		// write image data
		fwrite(imgdata.data(), sizeof(char), imgdata.size(), fp);

		// close file
		fclose(fp);
	}

	return true;
}
#endif

/* ----------------------- End of developers functions -------------------------- */

/* ----------------------- End of file -------------------------- */

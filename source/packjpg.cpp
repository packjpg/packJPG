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
 -w	   wait after processing files
 -o    overwrite existing files
 -p    proceed on warnings

By default, compression is cancelled on warnings. If warnings are 
skipped by using "-p", most files with warnings can also be compressed, 
but JPEG files reconstructed from PJG files might not be bitwise 
identical with the original JPEG files. There won't be any loss to 
image data or quality however.

There is no known case in which a file compressed by packJPG (without 
the "-p" option, see above) couldn't be reconstructed to exactly the 
state it was before. If you want an additional layer of safety you can 
also use the verify option "-ver". In this mode, files are compressed, 
then decompressed and the decompressed file compared to the original 
file. If this test doesn't pass there will be an error message and the 
compressed file won't be written to the drive. 

Please note that the "-ver" option should never be used in conjunction 
with the "-p" option. As stated above, the "-p" 
will most likely lead to reconstructed JPG files not being 
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

#include <algorithm>
#include <array>
#include <chrono>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <cstdio>

#include "action.h"
#include "aricoder.h"
#include "bitops.h"
#include "component.h"
#include "dct8x8.h"
#include "filetype.h"
#include "huffcodes.h"
#include "hufftree.h"
#include "jfifparse.h"
#include "jpgdecoder.h"
#include "jpgencoder.h"
#include "jpgreader.h"
#include "pjgdecoder.h"
#include "pjgencoder.h"
#include "pjpgtbl.h"
#include "scaninfo.h"
#include "segment.h"

#if defined BUILD_DLL // define BUILD_LIB from the compiler options if you want to compile a DLL!
	#define BUILD_LIB
#endif

#if defined BUILD_LIB // define BUILD_LIB as compiler option if you want to compile a library!
	#include "packjpglib.h"
#endif

// #define DEV_BUILD // uncomment to include developer functions
// #define DEV_INFOS // uncomment to include developer information

const std::string FRD_ERRMSG("could not read file / file not found: %s");
const std::string FWR_ERRMSG("could not write file / file write-protected: %s");

/* -----------------------------------------------
global variables: messages
----------------------------------------------- */

static char errormessage[128];
static bool(*errorfunction)();
static int  errorlevel;
// meaning of errorlevel:
// -1 -> wrong input
// 0 -> no error
// 1 -> warning
// 2 -> fatal error

/* -----------------------------------------------
global variables: data storage
----------------------------------------------- */

static std::array<std::array<std::uint16_t, 64>, 4> qtables; // quantization tables

static std::vector<std::uint8_t> grbgdata; // garbage data
static std::vector<Segment> segments; // Header segments.
static std::vector<std::uint8_t> huffdata; // huffman coded data

/* -----------------------------------------------
global variables: info about image
----------------------------------------------- */

static std::vector<Component> cmpnfo; // separate info for each color component

/* -----------------------------------------------
global variables: info about files
----------------------------------------------- */

static std::string jpgfilename;	// name of JPEG file
static std::string pjgfilename;	// name of PJG file
static int    jpgfilesize;			// size of JPEG file
static int    pjgfilesize;			// size of PJG file
static JpegType jpegtype = JpegType::UNKNOWN; // type of JPEG coding
static FileType filetype;				// type of current file
static std::unique_ptr<iostream> str_in;	// input stream
static std::unique_ptr<iostream> str_out;	// output stream


/* -----------------------------------------------
	function declarations: main interface
	----------------------------------------------- */
#if !defined( BUILD_LIB )
static void initialize_options( int argc, char** argv );
static void process_ui();
static std::string get_status( bool (*function)() );
static void show_help();
#endif
static void process_file();
static void execute( bool (*function)() );


/* -----------------------------------------------
	function declarations: main functions
	----------------------------------------------- */
#if !defined( BUILD_LIB )
static bool check_file();
static bool swap_streams();
static bool compare_output();
#endif
static bool reset_buffers();

/* -----------------------------------------------
filter DC coefficients
----------------------------------------------- */

static bool predict_dc() {
	// apply prediction, store prediction error instead of DC
	for (auto& cmpt : cmpnfo) {
		cmpt.predict_dc();
	}
	return true;
}


/* -----------------------------------------------
unpredict DC coefficients
----------------------------------------------- */

static bool unpredict_dc() {
	// remove prediction, store DC instead of prediction error
	for (auto& cmpt : cmpnfo) {
		cmpt.unpredict_dc();
	}
	return true;
}

/* -----------------------------------------------
calculate zero distribution lists
----------------------------------------------- */

static bool calc_zdst_lists() {
	// this functions counts, for each DCT block, the number of non-zero coefficients
	for (auto& cmpt : cmpnfo) {
		cmpt.calc_zdst_lists();
	}

	return true;
}

namespace image {
	int imgwidth = 0; // width of image
	int imgheight = 0; // height of image

	int mcuh = 0; // mcus per collumn
	int mcuc = 0; // count of mcus
}


namespace jfif {
	// Helper function that parses SOF0/SOF1/SOF2 segments.
	inline void parse_sof(Marker type, const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header

					  // set JPEG coding type
		if (type == Marker::kSOF2) {
			jpegtype = JpegType::PROGRESSIVE;
		}
		else {
			jpegtype = JpegType::SEQUENTIAL;
		}

		// check data precision, only 8 bit is allowed
		int lval = segment[hpos];
		if (lval != 8) {
			throw std::runtime_error(std::to_string(lval) + " bit data precision is not supported");
		}

		// image size, height & component count
		image::imgheight = pack(segment[hpos + 1], segment[hpos + 2]);
		image::imgwidth = pack(segment[hpos + 3], segment[hpos + 4]);
		cmpnfo.resize(segment[hpos + 5]);
		if ((image::imgwidth == 0) || (image::imgheight == 0)) {
			throw std::runtime_error("resolution is " + std::to_string(image::imgwidth) + "x" + std::to_string(image::imgheight) + ", possible malformed JPEG");
		}
		if (cmpnfo.size() > 4) {
			throw std::runtime_error("image has " + std::to_string(cmpnfo.size()) + " components, max 4 are supported");
		}

		hpos += 6;
		// components contained in image
		for (auto& cmpt : cmpnfo) {
			cmpt.jid = segment[hpos];
			cmpt.sfv = bitops::LBITS(segment[hpos + 1], 4);
			cmpt.sfh = bitops::RBITS(segment[hpos + 1], 4);
			cmpt.qtable = qtables[segment[hpos + 2]];
			hpos += 3;
		}
	}

	// Helper function that parses SOS segments.
	inline ScanInfo parse_sos(const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		ScanInfo scan_info;
		scan_info.cmpc = segment[hpos];
		if (scan_info.cmpc > cmpnfo.size()) {
			throw std::range_error(std::to_string(scan_info.cmpc) + " components in scan, only " + std::to_string(cmpnfo.size()) + " are allowed");
		}
		hpos++;
		for (int i = 0; i < scan_info.cmpc; i++) {
			int cmp;
			for (cmp = 0; (segment[hpos] != cmpnfo[cmp].jid) && (cmp < cmpnfo.size()); cmp++);
			if (cmp == cmpnfo.size()) {
				throw std::range_error("component id mismatch in start-of-scan");
			}
			auto& cmpt = cmpnfo[cmp];
			scan_info.cmp[i] = cmp;
			cmpt.huffdc = bitops::LBITS(segment[hpos + 1], 4);
			cmpt.huffac = bitops::RBITS(segment[hpos + 1], 4);
			if ((cmpt.huffdc < 0) || (cmpt.huffdc >= 4) ||
				(cmpt.huffac < 0) || (cmpt.huffac >= 4)) {
				throw std::range_error("huffman table number mismatch");
			}
			hpos += 2;
		}
		scan_info.from = segment[hpos + 0];
		scan_info.to = segment[hpos + 1];
		scan_info.sah = bitops::LBITS(segment[hpos + 2], 4);
		scan_info.sal = bitops::RBITS(segment[hpos + 2], 4);
		// check for errors
		if ((scan_info.from > scan_info.to) || (scan_info.from > 63) || (scan_info.to > 63)) {
			throw std::range_error("spectral selection parameter out of range");
		}
		if ((scan_info.sah >= 12) || (scan_info.sal >= 12)) {
			throw std::range_error("successive approximation parameter out of range");
		}
		return scan_info;
	}

	// Throws an error if the jfif segment is not valid in packjpg.
	inline void parse_jfif(const Segment& segment)
	{
		switch (segment.get_type()) {
		case Marker::kDHT:
			break;
		case Marker::kDQT:
			try {
				jfif::parse_dqt(qtables, segment.get_data());
			} catch (const std::runtime_error&) {
				throw;
			}
			break;
		case Marker::kDRI:
			break;
		case Marker::kSOS:
			break;
		case Marker::kSOF0:
			// coding process: baseline DCT
		case Marker::kSOF1:
			// coding process: extended sequential DCT
		case Marker::kSOF2:
			// coding process: progressive DCT
			try {
				jfif::parse_sof(segment.get_type(), segment.get_data());
			} catch (const std::runtime_error&) {
				throw;
			}
			break;
		case Marker::kSOF3:
			// coding process: lossless sequential
			throw std::runtime_error("sof3 marker found, image is coded lossless");
		case Marker::kSOF5:
			// coding process: differential sequential DCT
			throw std::runtime_error("sof5 marker found, image is coded diff. sequential");
		case Marker::kSOF6:
			// coding process: differential progressive DCT
			throw std::runtime_error("sof6 marker found, image is coded diff. progressive");
		case Marker::kSOF7:
			// coding process: differential lossless
			throw std::runtime_error("sof7 marker found, image is coded diff. lossless");
		case Marker::kSOF9:
			// coding process: arithmetic extended sequential DCT
			throw std::runtime_error("sof9 marker found, image is coded arithm. sequential");
		case Marker::kSOF10:
			// coding process: arithmetic extended sequential DCT
			throw std::runtime_error("sof10 marker found, image is coded arithm. progressive");
		case Marker::kSOF11:
			// coding process: arithmetic extended sequential DCT
			throw std::runtime_error("sof11 marker found, image is coded arithm. lossless");
		case Marker::kSOF13:
			// coding process: arithmetic differntial sequential DCT
			throw std::runtime_error("sof13 marker found, image is coded arithm. diff. sequential");
		case Marker::kSOF14:
			// coding process: arithmetic differential progressive DCT
			throw std::runtime_error("sof14 marker found, image is coded arithm. diff. progressive");
		case Marker::kSOF15:
			// coding process: arithmetic differntial lossless
			throw std::runtime_error("sof15 marker found, image is coded arithm. diff. lossless");
		case Marker::kAPP0:
		case Marker::kAPP1:
		case Marker::kAPP2:
		case Marker::kAPP3:
		case Marker::kAPP4:
		case Marker::kAPP5:
		case Marker::kAPP6:
		case Marker::kAPP7:
		case Marker::kAPP8:
		case Marker::kAPP9:
		case Marker::kAPP10:
		case Marker::kAPP11:
		case Marker::kAPP12:
		case Marker::kAPP13:
		case Marker::kAPP14:
		case Marker::kAPP15:
		case Marker::kCOM:
			break;
		case Marker::kRST0:
		case Marker::kRST1:
		case Marker::kRST2:
		case Marker::kRST3:
		case Marker::kRST4:
		case Marker::kRST5:
		case Marker::kRST6:
		case Marker::kRST7:
			// return errormessage - RST is out of place here
			throw std::runtime_error("rst marker found out of place");
		case Marker::kSOI:
			// return errormessage - start-of-image is out of place here
			throw std::runtime_error("soi marker found out of place");
		case Marker::kEOI:
			// return errormessage - end-of-image is out of place here
			throw std::runtime_error("eoi marker found out of place");
		default: // unknown marker segment
			throw std::runtime_error("unknown marker found");
		}
	}
}

namespace jpg {

char padbit = -1; // padbit (for huffman coding)

std::vector<std::uint8_t> rst_err; // number of wrong-set RST markers per scan

// Parses header for imageinfo.
void setup_imginfo();

// Calculates next position for MCU.
CodingStatus next_mcupos(const ScanInfo& scan_info, int rsti, int* mcu, int* cmp, int* csc, int* sub, int* dpos, int* rstw);
// Calculates next position (non interleaved).
CodingStatus next_mcuposn(const Component& cmpt, int rsti, int* dpos, int* rstw);

namespace encode {
	std::unique_ptr<JpgEncoder> jpeg_encoder = std::make_unique<JpgEncoder>();
	// JPEG encoding routine.
	bool recode() {
		try {
			jpeg_encoder->recode();
		}
		catch (const std::exception& e) {
			std::strcpy(errormessage, e.what());
			errorlevel = 2;
			return false;
		}
		return true;
	}
	// Merges header & image data to jpeg.
	bool merge() {
		try {
			jpeg_encoder->merge();
		} catch (const std::exception& e) {
			std::strcpy(errormessage, e.what());
			errorlevel = 2;
			return false;
		}
		return true;
	}
}

namespace decode {
	std::unique_ptr<JpgDecoder> jpeg_decoder = std::make_unique<JpgDecoder>();

	// Read in header and image data.
	bool read() {
		auto reader = std::make_unique<JpgReader>();
		try {
			reader->read();
		} catch (const std::exception& e) {
			std::strcpy(errormessage, e.what());
			errorlevel = 2;
			return false;
		}
		return true;
	}
	// JPEG decoding routine.
	bool decode() {
		try {
			jpeg_decoder->decode(jpegtype, segments, cmpnfo, huffdata);
		} catch (const std::exception& e) {
			std::strcpy(errormessage, e.what());
			errorlevel = 2;
			return false;
		}
		return true;
	}
	// Checks range of values, error if out of bounds.
	bool check_value_range() {
		try {
			jpeg_decoder->check_value_range(cmpnfo);
		} catch (const std::exception& e) {
			std::strcpy(errormessage, e.what());
			errorlevel = 2;
			return false;
		}
		return true;
	}

}
}

/* -----------------------------------------------
	function declarations: pjg-specific
	----------------------------------------------- */

namespace pjg {
	namespace encode {
		bool encode() {
			try {
				auto pjg_encoder = std::make_unique<PjgEncoder>(str_out);
				pjg_encoder->encode();
			} catch (const std::exception& e) {
				std::strcpy(errormessage, e.what());
				errorlevel = 2;
				return false;
			}
			return true;
		}
	}

	namespace decode {
		bool decode() {
			try {
				auto pjg_decoder = std::make_unique<PjgDecoder>(str_in);
				pjg_decoder->decode();
			} catch (const std::exception& e) {
				std::strcpy(errormessage, e.what());
				errorlevel = 2;
				return false;
			}
			return true;
		}
	}
}

/*
* Discrete cosine transform (DCT) and Inverse discrete cosine transform (IDCT) functions and data.
*
*/
namespace dct {
	/* -----------------------------------------------
	adapt ICOS tables for quantizer tables
	----------------------------------------------- */
	bool adapt_icos() {
		for (auto& cmpt : cmpnfo) {
			cmpt.adapt_icos();
		}
		return true;
	}
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
enum class CollectionMode {
	STD = 0, // standard collections
	DHF = 1, // sequential order collections, 'dhufs'
	SQU = 2, // square collections
	UNC = 3, // uncollections
	SQU_ALT = 4, // square collections / alt order (even/uneven)
	UNC_ALT = 5 // uncollections / alt order (even/uneven)
};

static CollectionMode coll_mode = CollectionMode::STD; // Write mode for collections.

static bool dump_hdr();
static bool dump_huf();
static bool dump_coll();
static bool dump_zdst();
static bool dump_file( const std::string& base, const std::string& ext, void* data, int bpv, int size );
static bool dump_errfile();
static bool dump_info();
static bool dump_dist();
static bool dump_pgm();
#endif


/* -----------------------------------------------
	global variables: library only variables
	----------------------------------------------- */
#if defined(BUILD_LIB)
static int lib_in_type  = -1;
static int lib_out_type = -1;
#endif

#if !defined(BUILD_LIB)
static std::unique_ptr<iostream> str_str;	// storage stream

static std::vector<std::string> filelist; // list of files to process 
static int    file_no  = 0;			// number of current file

#endif

#if defined(DEV_INFOS)
static int    dev_size_hdr      = 0;
static int    dev_size_cmp[ 4 ]{};
static int    dev_size_zsr[ 4 ]{};
static int    dev_size_dc[ 4 ] {};
static int    dev_size_ach[ 4 ]{};
static int    dev_size_acl[ 4 ]{};
static int    dev_size_zdh[ 4 ]{};
static int    dev_size_zdl[ 4 ]{};
#endif


/* -----------------------------------------------
	global variables: settings
	----------------------------------------------- */

static int  err_tol = 1;		// error threshold ( proceed on warnings yes (2) / no (1) )
static bool auto_set = true;	// automatic find best settings yes/no
static Action action = Action::A_COMPRESS;// what to do with JPEG/PJG files

#if !defined(BUILD_LIB)
static int  verbosity  = -1;	// level of verbosity
static bool overwrite  = false;	// overwrite files yes / no
static bool wait_on_finish  = false;	// pause after finished yes / no
static int  verify_lv  = 0;		// verification level ( none (0), simple (1), detailed output (2) )

static bool developer  = false;	// allow developers functions yes/sno

static FILE*  msgout   = stdout;// stream for output of messages
static bool   pipe_on  = false;	// use stdin/stdout instead of filelist

#endif

namespace program_info {
	const std::uint8_t appversion = 25;
	const std::string subversion = "k";
	const std::string apptitle = "packJPG";
	const std::string appname = "packjpg";
	const std::string versiondate = "01/22/2016";
	const std::array<std::uint8_t, 2> pjg_magic{ 'J', 'S' };

	const std::string author = "Matthias Stirner / Se";
	const std::string website = "http://packjpg.encode.ru/";
	const std::string copyright = "2006-2016 HTW Aalen University & Matthias Stirner";
	const std::string email = "packjpg (at) matthiasstirner.com";

	const std::string pjg_ext = "pjg";
	const std::string jpg_ext = "jpg";
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
				cmpnfo[0].segm_cnt, cmpnfo[1].segm_cnt, cmpnfo[2].segm_cnt, cmpnfo[3].segm_cnt);
		fprintf( msgout,  " noise threshold   ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
				cmpnfo[0].nois_trs, cmpnfo[1].nois_trs, cmpnfo[2].nois_trs, cmpnfo[3].nois_trs );
		fprintf( msgout,  " -------------------------------------------------\n\n" );
	}
	
	// (re)set program has to be done first
	reset_buffers();
	
	// process file(s) - this is the main function routine
	static std::vector<std::string> err_list(filelist.size()); // list of error messages 
	static std::vector<int> err_tp(filelist.size()); // list of error types

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
	if ( wait_on_finish && ( msgout != stderr ) ) {
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
		const auto& data = str_out->get_data();
		auto arr = new unsigned char[data.size()];
		std::copy(std::begin(data), std::end(data), arr);
		*out_file = arr;
	}
	
	// close iostreams
	str_in.reset(nullptr);
	str_out.reset(nullptr);
	
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
	
	std::uint8_t buffer[ 2 ];
	
	// (re)set errorlevel
	errorfunction = nullptr;
	errorlevel = 0;
	jpgfilesize = 0;
	pjgfilesize = 0;
	
	// open input stream, check for errors
	StreamType in_ty = StreamType(in_type);
	if (in_ty == StreamType::kFile) {
		std::string file_path((char*)in_src);
		str_out = std::make_unique<FileStream>(file_path, StreamMode::kRead);
	} else if (in_ty == StreamType::kMemory) {
		std::vector<std::uint8_t> data((std::uint8_t*)in_src, (std::uint8_t*)in_src + in_size);
		str_in = std::make_unique<MemStream>(data, StreamMode::kRead);
	} else { // Stream
		str_in = std::make_unique<MemStream>(StreamMode::kRead);
	}
	if ( str_in->chkerr() ) {
		sprintf( errormessage, "error opening input stream" );
		errorlevel = 2;
		return;
	}	
	
	// open output stream, check for errors
	StreamType out_ty = StreamType(out_type);
	if (out_ty == StreamType::kFile) {
		std::string file_path((char*)out_dest);
		str_out = std::make_unique<FileStream>(file_path, StreamMode::kWrite);
	} else if (out_ty == StreamType::kMemory) {
		str_out = std::make_unique<MemStream>(std::vector<std::uint8_t>() , StreamMode::kWrite);
	} else { // Stream
		str_out = std::make_unique<MemStream>(StreamMode::kWrite);
	}
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
static void initialize_options( int argc, char** argv )
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
		} else if (arg == "-ver") {
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
		else if (arg == "-w") {
			wait_on_finish = true;
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
			cmpnfo[i].nois_trs = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-s%i,%i", &i, &tmp_val ) == 2 ) {
			i = ( i < 0 ) ? 0 : i;
			i = ( i > 3 ) ? 3 : i;
			tmp_val = ( tmp_val <  1 ) ?  1 : tmp_val;
			tmp_val = ( tmp_val > 49 ) ? 49 : tmp_val;
			cmpnfo[i].segm_cnt = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-t%i", &tmp_val ) == 1 ) {
			tmp_val = ( tmp_val < 0  ) ?  0 : tmp_val;
			tmp_val = ( tmp_val > 10 ) ? 10 : tmp_val;
			cmpnfo[0].nois_trs = tmp_val;
			cmpnfo[1].nois_trs = tmp_val;
			cmpnfo[2].nois_trs = tmp_val;
			cmpnfo[3].nois_trs = tmp_val;
			auto_set = false;
		}
		else if ( sscanf(arg.c_str(), "-s%i", &tmp_val ) == 1 ) {
			tmp_val = ( tmp_val <  1 ) ?  1 : tmp_val;
			tmp_val = ( tmp_val > 64 ) ? 64 : tmp_val;
			cmpnfo[0].segm_cnt = tmp_val;
			cmpnfo[1].segm_cnt = tmp_val;
			cmpnfo[2].segm_cnt = tmp_val;
			cmpnfo[3].segm_cnt = tmp_val;
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
}

/* -----------------------------------------------
	UI for processing one file
	----------------------------------------------- */
static void process_ui()
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
	str_in.reset(nullptr);
	str_out.reset(nullptr);
	str_str.reset(nullptr);
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
static inline std::string get_status( bool (*function)() )
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
static void show_help()
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
	fprintf( msgout, " [-w]		wait after processing files\n" );
	fprintf( msgout, " [-o]     overwrite existing files\n" );
	fprintf( msgout, " [-p]     proceed on warnings\n" );
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
static void process_file()
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
static void execute( bool (*function)() )
{
	if ( errorlevel < err_tol ) {
		#if !defined BUILD_LIB
		
		// write statusmessage
		if ( verbosity == 2 ) {
			fprintf( msgout,  "\n%s ", get_status( function ).c_str() );
			for ( int i = get_status(function).length(); i <= 30; i++ )
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
static bool check_file()
{	
	std::uint8_t fileid[ 2 ]{};
	const std::string& filename = filelist[ file_no ];
	
	
	// open input stream, check for errors
	if (pipe_on) {
		str_in = std::make_unique<MemStream>(StreamMode::kRead);
	} else {
		str_in = std::make_unique<FileStream>(filename, StreamMode::kRead);
	}
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
		if (pipe_on) {
			str_out = std::make_unique<MemStream>(StreamMode::kWrite);
		}
		else {
			str_out = std::make_unique<FileStream>(pjgfilename, StreamMode::kWrite);
		}
		if ( str_out->chkerr() ) {
			sprintf( errormessage, FWR_ERRMSG.c_str(), pjgfilename.c_str() );
			errorlevel = 2;
			return false;
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
		if (pipe_on) {
			str_out = std::make_unique<MemStream>(StreamMode::kWrite);
		} else {
			str_out = std::make_unique<FileStream>(jpgfilename, StreamMode::kWrite);
		}
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
static bool swap_streams()	
{
	std::uint8_t dmp[ 2 ];
	
	// store input stream
	str_str = std::move(str_in);
	str_str->rewind();
	
	// replace input stream by output stream / switch mode for reading / read first bytes
	str_in = std::move(str_out);
	str_in->switch_mode();
	str_in->read( dmp, 2 );
	
	// open new stream for output / check for errors
	str_out = std::make_unique<MemStream>(std::vector<std::uint8_t>(), StreamMode::kWrite);
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
static bool compare_output() {
	const auto& input_data = str_str->get_data();
	const auto& verif_data = str_out->get_data();
	if (input_data.size() != verif_data.size()) {
		sprintf(errormessage, "expected size: %u, verif size: %u", input_data.size(), verif_data.size());
		errorlevel = 2;
		return false;
	}

	const auto result = std::mismatch(std::begin(input_data),
	                                  std::end(input_data),
	                                  std::begin(verif_data),
	                                  std::end(verif_data));
	if (result.first != std::end(input_data)
		|| result.second != std::end(verif_data)) {
		const auto first_diff = std::distance(std::begin(input_data), result.first);
		sprintf(errormessage, "difference found at byte position %u", first_diff);
		errorlevel = 2;
		return false;
	}

	return true;
}
#endif


/* -----------------------------------------------
	set each variable to its initial value
	----------------------------------------------- */

static bool reset_buffers() {
	// free buffers & set pointers nullptr
	segments.clear();
	huffdata.clear();
	grbgdata.clear();
	jpg::rst_err.clear();

	jpg::encode::jpeg_encoder = std::make_unique<JpgEncoder>();

	cmpnfo.clear();

	// preset imgwidth / imgheight / component count 
	image::imgwidth = 0;
	image::imgheight = 0;

	// preset mcu info variables / restart interval
	image::mcuc = 0;
	image::mcuh = 0;

	// reset quantization
	for (auto& qtable : qtables) {
		qtable.fill(0);
	}

	// preset jpegtype
	jpegtype = JpegType::UNKNOWN;

	// reset padbit
	jpg::padbit = -1;

	return true;
}

void JpgReader::read_sos(const std::unique_ptr<abytewriter>& huffw, std::vector<std::uint8_t>& segment) {
	// switch to huffman data reading mode
	std::uint32_t cpos = 0; // rst marker counter
	std::uint32_t crst = 0; // current rst marker counter
	while (true) {
		// read byte from imagedata
		std::uint8_t tmp = str_in->read_byte();

		// non-0xFF loop
		if (tmp != 0xFF) {
			crst = 0;
			while (tmp != 0xFF) {
				huffw->write(tmp);
				tmp = str_in->read_byte();
			}
		}

		// treatment of 0xFF
		if (tmp == 0xFF) {
			tmp = str_in->read_byte();
			if (tmp == 0x00) {
				crst = 0;
				// no zeroes needed -> ignore 0x00. write 0xFF
				huffw->write(0xFF);
			} else if (tmp == 0xD0 + (cpos % 8)) { // restart marker
				// increment rst counters
				cpos++;
				crst++;
			} else { // in all other cases leave it to the header parser routines
				// store number of wrongly set rst markers
				if (crst > 0) {
					if (jpg::rst_err.empty()) {
						jpg::rst_err.resize(scan_count + 1);
					}
				}
				if (!jpg::rst_err.empty()) {
					// realloc and set only if needed
					jpg::rst_err.resize(scan_count + 1);
					if (crst > 255) {
						throw std::runtime_error("Severe false use of RST markers (" + std::to_string(crst) + ")");
						// crst = 255;
					}
					jpg::rst_err[scan_count] = crst;
				}
				// end of current scan
				scan_count++;
				// on with the header parser routines
				segment[0] = 0xFF;
				segment[1] = tmp;
				break;
			}
		} else {
			// otherwise this means end-of-file, so break out
			break;
		}
	}
}

void JpgReader::read() {
	scan_count = 0;
	// start headerwriter
	auto hdrw = std::make_unique<abytewriter>(4096);

	// start huffman writer
	auto huffw = std::make_unique<abytewriter>(0);

	// alloc memory for segment data first
	std::vector<std::uint8_t> segment(1024);

	// JPEG reader loop
	Marker type = Marker::kINVALID;
	while (true) {
		if (type == Marker::kSOS) { // if last marker was sos
			try {
				read_sos(huffw, segment);
			} catch (const std::runtime_error&) {
				throw;
			}
		} else {
			// read in next marker
			if (str_in->read(segment, 2) != 2) {
				break;
			}
			if (segment[0] != 0xFF) {
				// ugly fix for incorrect marker segment sizes
				throw std::runtime_error("size mismatch in marker segment FF");
				/*errorlevel = 2;
				if (type == Marker::kCOM) { //  if last marker was COM try again
					if (str_in->read(segment, 2) != 2) {
						break;
					}
					if (segment[0] == 0xFF) {
						errorlevel = 1;
					}
				}
				if (errorlevel == 2) {
					return false;
				}*/
			}
		}

		// read segment type
		type = static_cast<Marker>(segment[1]); // TODO: add enum correctness checks?

		// if EOI is encountered make a quick exit
		if (type == Marker::kEOI) {
			// get pointer for header data & size
			segments = Segment::parse_segments(hdrw->get_data());
			// get pointer for huffman data & size
			huffdata = huffw->get_data();
			// everything is done here now
			break;
		}

		// read in next segments' length and check it
		if (str_in->read(segment, 2, 2) != 2) {
			break;
		}
		uint32_t len = 2 + jfif::pack(segment[2], segment[3]); // Length of current marker segment.
		if (len < 4) {
			break;
		}

		// realloc segment data if needed
		if (segment.size() < len) {
			segment.resize(len);
		}

		// read rest of segment, store back in header writer
		if (str_in->read(segment, len - 4, 4) != static_cast<std::size_t>(len - 4)) {
			break;
		}
		hdrw->write_n(segment.data(), len);
	}
	// JPEG reader loop end

	// check if everything went OK
	if (segments.empty() || huffdata.empty()) {
		throw std::runtime_error("unexpected end of data encountered");
	}

	// store garbage after EOI if needed
	std::uint8_t tmp;
	bool garbage_avail = str_in->read_byte(&tmp);
	if (garbage_avail) {

		auto grbgw = std::make_unique<abytewriter>(1024);
		grbgw->write(tmp);
		while (true) {
			std::size_t len = str_in->read(segment, segment.capacity());
			if (len == 0) {
				break;
			}
			grbgw->write_n(segment.data(), len);
		}
		grbgdata = grbgw->get_data();
	}

	// get filesize
	jpgfilesize = str_in->getsize();

	// parse header for image info
	try {
		jpg::setup_imginfo();
	} catch (const std::exception&) {
		throw;
	}
}

void JpgEncoder::merge() {
	int rpos = 0; // current restart marker position
	int scan = 1; // number of current scan	

	// write SOI
	constexpr std::array<std::uint8_t, 2> SOI{0xFF, 0xD8};
	str_out->write(SOI.data(), 2);

	// JPEG writing loop
	for (const auto& segment : segments) {
		// write header data to file
		str_out->write(segment.get_data().data(), segment.get_data().size());

		// get out if last marker segment type was not SOS
		if (segment.get_type() != Marker::kSOS) {
			continue;
		}

		// (re)set corrected rst pos
		std::uint32_t cpos = 0; // in scan corrected rst marker position

		// write & expand huffman coded image data
		// ipos is the current position in image data.
		for (std::uint32_t ipos = scnp[scan - 1]; ipos < scnp[scan]; ipos++) {
			// write current byte
			str_out->write_byte(huffdata[ipos]);
			// check current byte, stuff if needed
			if (huffdata[ipos] == 0xFF) {
				str_out->write_byte(std::uint8_t(0)); // 0xFF stuff value
			}
			// insert restart markers if needed
			if (!rstp.empty()) {
				if (ipos == rstp[rpos]) {
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
	constexpr std::array<std::uint8_t, 2> EOI{0xFF, 0xD9}; // EOI segment
	str_out->write(EOI.data(), 2);

	// write garbage if needed
	if (!grbgdata.empty()) {
		str_out->write(grbgdata.data(), grbgdata.size());
	}

	// errormessage if write error
	if (str_out->chkerr()) {
		throw std::runtime_error("write error, possibly drive is full");
	}

	// get filesize
	jpgfilesize = str_out->getsize();
}

void JpgDecoder::decode(JpegType jpegtype, const std::vector<Segment>& segments, std::vector<Component>& cmpts, const std::vector<std::uint8_t>& huffdata)
{		
	short block[64]; // store block for coeffs
	int scan_count = 0; // Count of scans.
	// open huffman coded image data for input in abitreader
	huffr = std::make_unique<abitreader>(huffdata); // bitwise reader for image data
	
	// JPEG decompression loop
	int rsti = 0; // restart interval
	std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2> hcodes; // huffman codes
	std::array<std::array<std::unique_ptr<HuffTree>, 4>, 2> htrees; // huffman decoding trees
	for (const auto& segment : segments) {
		// seek till start-of-scan, parse only DHT, DRI and SOS
		ScanInfo scan_info;
		const Marker type = segment.get_type();
		if (type == Marker::kDHT) {
			try {
				jfif::parse_dht(segment.get_data(), hcodes);
			} catch (const std::range_error&) {
				throw;
			}
			build_trees(hcodes, htrees);
		} else if (type == Marker::kDRI) {
			rsti = jfif::parse_dri(segment.get_data());
		} else if (type == Marker::kSOS) {
			try {
				scan_info = jfif::parse_sos(segment.get_data());
			} catch (std::runtime_error&) {
				throw;
			}
		} else {
			continue;
		}
		
		// get out if last marker segment type was not SOS
		if (type != Marker::kSOS) {
			continue;
		}
		
		// check if huffman tables are available
		for (int csc = 0; csc < scan_info.cmpc; csc++) {
			auto& cmpt = cmpts[scan_info.cmp[csc]];
			if ( ( scan_info.sal == 0 && !htrees[0][cmpt.huffdc] ) ||
				 (scan_info.sah >  0 && !htrees[1][cmpt.huffac] ) ) {
				throw std::runtime_error("huffman table missing in scan%i" + std::to_string(scan_count));
			}
		}
		
		
		// intial variables set for decoding
		int cmp  = scan_info.cmp[ 0 ];
		int csc  = 0;
		int mcu  = 0;
		int sub  = 0;
		int dpos = 0;
		
		// JPEG imagedata decoding routines
		while ( true )
		{			
			// (re)set last DCs for diff coding
			std::array<int, 4> lastdc{}; // last dc for each component
			
			// (re)set status
			int eob = 0;
			CodingStatus status = CodingStatus::OKAY;
			
			// (re)set eobrun
			int eobrun  = 0; // run of eobs
			int peobrun = 0; // previous eobrun
			
			// (re)set rst wait counter
			int rstw = rsti; // restart wait counter
			
			// decoding for interleaved data
			if ( scan_info.cmpc > 1 )
			{				
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential interleaved decoding <---
					while ( status == CodingStatus::OKAY ) {
						// decode block
						eob = this->block_seq(*htrees[0][cmpts[cmp].huffdc],
						                      *htrees[1][cmpts[cmp].huffdc],
						                      block);
						
						// check for non optimal coding
						if ( ( eob > 1 ) && ( block[ eob - 1 ] == 0 ) ) {
							throw std::runtime_error("reconstruction of inefficient coding not supported");
						}
						
						// fix dc
						block[ 0 ] += lastdc[ cmp ];
						lastdc[ cmp ] = block[ 0 ];
						
						// copy to colldata
						for (int bpos = 0; bpos < eob; bpos++ )
							cmpts[cmp].colldata[ bpos ][ dpos ] = block[ bpos ];
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = CodingStatus::ERROR;
						else status = jpg::next_mcupos(scan_info, rsti, &mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
				else if ( scan_info.sah == 0 ) {
					// ---> progressive interleaved DC decoding <---
					// ---> succesive approximation first stage <---
					while ( status == CodingStatus::OKAY ) {
						status = this->dc_prg_fs(*htrees[0][cmpts[cmp].huffdc],
						                         block);
						
						// fix dc for diff coding
						cmpts[cmp].colldata[0][dpos] = block[0] + lastdc[ cmp ];
						lastdc[ cmp ] = cmpts[cmp].colldata[0][dpos];
						
						// bitshift for succesive approximation
						cmpts[cmp].colldata[0][dpos] <<= scan_info.sal;
						
						// next mcupos if no error happened
						if ( status != CodingStatus::ERROR )
							status = jpg::next_mcupos(scan_info, rsti, &mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
				else {
					// ---> progressive interleaved DC decoding <---
					// ---> succesive approximation later stage <---					
					while ( status == CodingStatus::OKAY ) {
						// decode next bit
						this->dc_prg_sa(block);
						
						// shift in next bit
						cmpts[cmp].colldata[0][dpos] += block[0] << scan_info.sal;
						
						status = jpg::next_mcupos(scan_info, rsti, &mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
			}
			else // decoding for non interleaved data
			{
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential non interleaved decoding <---
					while ( status == CodingStatus::OKAY ) {
						// decode block
						eob = this->block_seq(*htrees[0][cmpts[cmp].huffdc],
						                      *htrees[1][cmpts[cmp].huffdc],
						                      block);
						
						// check for non optimal coding
						if ( ( eob > 1 ) && ( block[ eob - 1 ] == 0 ) ) {
							throw std::runtime_error("reconstruction of inefficient coding not supported");
						}
						
						// fix dc
						block[ 0 ] += lastdc[ cmp ];
						lastdc[ cmp ] = block[ 0 ];
						
						// copy to colldata
						for (int bpos = 0; bpos < eob; bpos++ )
							cmpts[cmp].colldata[ bpos ][ dpos ] = block[ bpos ];
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = CodingStatus::ERROR;
						else status = jpg::next_mcuposn(cmpts[cmp], rsti, &dpos, &rstw);
					}
				}
				else if ( scan_info.to == 0 ) {					
					if ( scan_info.sah == 0 ) {
						// ---> progressive non interleaved DC decoding <---
						// ---> succesive approximation first stage <---
						while ( status == CodingStatus::OKAY ) {
							status = this->dc_prg_fs(*htrees[0][cmpts[cmp].huffdc],
							                         block);
								
							// fix dc for diff coding
							cmpts[cmp].colldata[0][dpos] = block[0] + lastdc[ cmp ];
							lastdc[ cmp ] = cmpts[cmp].colldata[0][dpos];
							
							// bitshift for succesive approximation
							cmpts[cmp].colldata[0][dpos] <<= scan_info.sal;
							
							// check for errors, increment dpos otherwise
							if ( status != CodingStatus::ERROR )
								status = jpg::next_mcuposn(cmpts[cmp], rsti, &dpos, &rstw);
						}
					}
					else {
						// ---> progressive non interleaved DC decoding <---
						// ---> succesive approximation later stage <---
						while( status == CodingStatus::OKAY ) {
							// decode next bit
							this->dc_prg_sa(block);
							
							// shift in next bit
							cmpts[cmp].colldata[0][dpos] += block[0] << scan_info.sal;
							
							// increment dpos
							status = jpg::next_mcuposn(cmpts[cmp], rsti, &dpos, &rstw);
						}
					}
				}
				else {
					if ( scan_info.sah == 0 ) {
						// ---> progressive non interleaved AC decoding <---
						// ---> succesive approximation first stage <---
						while ( status == CodingStatus::OKAY ) {
							if ( eobrun == 0 ) {
								// decode block
								eob = this->ac_prg_fs(*htrees[1][cmpts[cmp].huffac],
								                      block, &eobrun, scan_info.from, scan_info.to);
								
								if ( eobrun > 0 ) {
									// check for non optimal coding
									if ( ( eob == scan_info.from )  && ( peobrun > 0 ) &&
										( peobrun <	hcodes[ 1 ][cmpts[cmp].huffac ]->max_eobrun - 1 ) ) {
										throw std::runtime_error("reconstruction of inefficient coding not supported");
									}
									peobrun = eobrun;
									eobrun--;
								} else peobrun = 0;
							
								// copy to colldata
								for (int bpos = scan_info.from; bpos < eob; bpos++)
									cmpts[cmp].colldata[ bpos ][ dpos ] = block[ bpos ] << scan_info.sal;
							} else eobrun--;
							
							// check for errors
							if ( eob < 0 ) status = CodingStatus::ERROR;
							else status = this->skip_eobrun(cmpts[cmp], rsti, &dpos, &rstw, &eobrun);
							
							// proceed only if no error encountered
							if ( status == CodingStatus::OKAY )
								status = jpg::next_mcuposn(cmpts[cmp], rsti, &dpos, &rstw);
						}
					}
					else {
						// ---> progressive non interleaved AC decoding <---
						// ---> succesive approximation later stage <---
						while ( status == CodingStatus::OKAY ) {
							// copy from colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								block[ bpos ] = cmpts[cmp].colldata[ bpos ][ dpos ];
							
							if ( eobrun == 0 ) {
								// decode block (long routine)
								eob = this->ac_prg_sa(*htrees[1][cmpts[cmp].huffac],
								                      block, &eobrun, scan_info.from, scan_info.to);
								
								if ( eobrun > 0 ) {
									// check for non optimal coding
									if ( ( eob == scan_info.from ) && ( peobrun > 0 ) &&
										( peobrun < hcodes[ 1 ][cmpts[cmp].huffac ]->max_eobrun - 1 ) ) {
										throw std::runtime_error("reconstruction of inefficient coding not supported");
									}
									
									// store eobrun
									peobrun = eobrun;
									eobrun--;
								} else peobrun = 0;
							}
							else {
								// decode block (short routine)
								this->eobrun_sa(block, scan_info.from, scan_info.to);
								eob = 0;
								eobrun--;
							}
								
							// copy back to colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								cmpts[cmp].colldata[ bpos ][ dpos ] += block[ bpos ] << scan_info.sal;
							
							// proceed only if no error encountered
							if ( eob < 0 ) status = CodingStatus::ERROR;
							else status = jpg::next_mcuposn(cmpts[cmp], rsti, &dpos, &rstw);
						}
					}
				}
			}			
			
			// unpad huffman reader / check padbit
			if ( jpg::padbit != -1 ) {
				if ( jpg::padbit != huffr->unpad( jpg::padbit ) ) {
					throw std::runtime_error("inconsistent use of padbits" );
					//jpg::padbit = 1;
				}
			}
			else {
				jpg::padbit = huffr->unpad( jpg::padbit );
			}
			
			// evaluate status
			if ( status == CodingStatus::ERROR ) {
				throw std::runtime_error("decode error in scan" + std::to_string(scan_count)
					+ " / mcu" + std::to_string(( scan_info.cmpc > 1 ) ? mcu : dpos));
			}
			else if ( status == CodingStatus::DONE ) {
				scan_count++; // increment scan counter
				break; // leave decoding loop, everything is done here
			}
		}
	}
	
	// check for missing data
	if (huffr->overread()) {
		throw std::runtime_error("coded image data truncated / too short" );
	}
	
	// check for surplus data
	if ( !huffr->eof()) {
		throw std::runtime_error("surplus data found after coded image data" );
	}

	huffr = nullptr;
}

void JpgEncoder::recode()
{		
	std::array<std::int16_t, 64> block; // store block for coeffs
	
	// open huffman coded image data in abitwriter
	auto huffw = std::make_unique<abitwriter>(0); // bitwise writer for image data
	huffw->set_fillbit( jpg::padbit );
	
	// init storage writer
	auto storw = std::make_unique<abytewriter>(0); // bytewise writer for storage of correction bits
	
	// preset count of scans and restarts
	int scan_count = 0;
	int rstc = 0; // count of restart markers
	
	// JPEG decompression loop
	int rsti = 0; // Restart interval.
	std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2> hcodes; // huffman codes
	for (const auto& segment : segments) {
		// seek till start-of-scan, parse only DHT, DRI and SOS
		ScanInfo scan_info;
		const Marker type = segment.get_type();
		if (type == Marker::kDHT) {
			try {
				jfif::parse_dht(segment.get_data(), hcodes);
			} catch (const std::range_error&) {
				throw;
			}
		} else if (type == Marker::kDRI) {
			rsti = jfif::parse_dri(segment.get_data());
		} else if (type == Marker::kSOS) {
			try {
				scan_info = jfif::parse_sos(segment.get_data());
			} catch (std::runtime_error&) {
				throw;
			}
		} else {
			continue;
		}

		// get out if last marker segment type was not SOS
		if (type != Marker::kSOS) {
			continue;
		}
		
		// (re)alloc scan positons array
		scnp.resize(scan_count + 2);
		
		// (re)alloc restart marker positons array if needed
		if ( rsti > 0 ) {
			int tmp = rstc + ( ( scan_info.cmpc > 1 ) ?
				( image::mcuc / rsti ) : ( cmpnfo[ scan_info.cmp[ 0 ] ].bc / rsti ) );
			rstp.resize(tmp + 1);
		}		
		
		// intial variables set for encoding
		int cmp  = scan_info.cmp[ 0 ];
		int csc  = 0;
		int mcu  = 0;
		int sub  = 0;
		int dpos = 0;
		
		// store scan position
		scnp[ scan_count ] = huffw->getpos();
		
		// JPEG imagedata encoding routines
		while ( true )
		{
			// (re)set last DCs for diff coding
			std::array<int, 4> lastdc{}; // last dc for each component
			
			// (re)set status
			CodingStatus status = CodingStatus::OKAY;
			
			// (re)set eobrun
			int eobrun = 0; // run of eobs
			
			// (re)set rst wait counter
			int rstw = rsti; // restart wait counter
			
			// encoding for interleaved data
			if ( scan_info.cmpc > 1 )
			{				
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential interleaved encoding <---
					while ( status == CodingStatus::OKAY ) {
						// copy from colldata
						for (int bpos = 0; bpos < 64; bpos++)
							block[ bpos ] = cmpnfo[cmp].colldata[ bpos ][ dpos ];
						
						// diff coding for dc
						block[ 0 ] -= lastdc[ cmp ];
						lastdc[ cmp ] = cmpnfo[cmp].colldata[ 0 ][ dpos ];
						
						// encode block
						int eob = this->block_seq( huffw,
						                              *hcodes[0][cmpnfo[cmp].huffac],
						                              *hcodes[1][cmpnfo[cmp].huffac],
						                              block );
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = CodingStatus::ERROR;
						else status = jpg::next_mcupos(scan_info, rsti, &mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
				else if ( scan_info.sah == 0 ) {
					// ---> progressive interleaved DC encoding <---
					// ---> succesive approximation first stage <---
					while ( status == CodingStatus::OKAY ) {
						// diff coding & bitshifting for dc 
						int tmp = cmpnfo[cmp].colldata[ 0 ][ dpos ] >> scan_info.sal;
						block[ 0 ] = tmp - lastdc[ cmp ];
						lastdc[ cmp ] = tmp;
						
						// encode dc
						this->dc_prg_fs(huffw,
						                       *hcodes[0][cmpnfo[cmp].huffdc],
						                       block);
						
						// next mcupos
						status = jpg::next_mcupos(scan_info, rsti, &mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
				else {
					// ---> progressive interleaved DC encoding <---
					// ---> succesive approximation later stage <---
					while ( status == CodingStatus::OKAY ) {
						// fetch bit from current bitplane
						block[ 0 ] = bitops::BITN(cmpnfo[cmp].colldata[ 0 ][ dpos ], scan_info.sal );
						
						// encode dc correction bit
						this->dc_prg_sa(huffw, block);
						
						status = jpg::next_mcupos(scan_info, rsti, &mcu, &cmp, &csc, &sub, &dpos, &rstw);
					}
				}
			}
			else // encoding for non interleaved data
			{
				if ( jpegtype == JpegType::SEQUENTIAL ) {
					// ---> sequential non interleaved encoding <---
					while ( status == CodingStatus::OKAY ) {
						// copy from colldata
						for (int bpos = 0; bpos < 64; bpos++)
							block[ bpos ] = cmpnfo[cmp].colldata[ bpos ][ dpos ];
						
						// diff coding for dc
						block[ 0 ] -= lastdc[ cmp ];
						lastdc[ cmp ] = cmpnfo[cmp].colldata[ 0 ][ dpos ];
						
						// encode block
						int eob = this->block_seq( huffw,
						                              *hcodes[0][cmpnfo[cmp].huffac],
						                              *hcodes[1][cmpnfo[cmp].huffac],
						                              block );
						
						// check for errors, proceed if no error encountered
						if ( eob < 0 ) status = CodingStatus::ERROR;
						else status = jpg::next_mcuposn(cmpnfo[cmp], rsti, &dpos, &rstw);
					}
				}
				else if ( scan_info.to == 0 ) {
					if ( scan_info.sah == 0 ) {
						// ---> progressive non interleaved DC encoding <---
						// ---> succesive approximation first stage <---
						while ( status == CodingStatus::OKAY ) {
							// diff coding & bitshifting for dc 
							int tmp = cmpnfo[cmp].colldata[ 0 ][ dpos ] >> scan_info.sal;
							block[ 0 ] = tmp - lastdc[ cmp ];
							lastdc[ cmp ] = tmp;
							
							// encode dc
							this->dc_prg_fs(huffw,
							                       *hcodes[0][cmpnfo[cmp].huffdc],
							                       block);							
							
							// check for errors, increment dpos otherwise
							status = jpg::next_mcuposn(cmpnfo[cmp], rsti, &dpos, &rstw);
						}
					}
					else {
						// ---> progressive non interleaved DC encoding <---
						// ---> succesive approximation later stage <---
						while ( status == CodingStatus::OKAY ) {
							// fetch bit from current bitplane
							block[ 0 ] = bitops::BITN(cmpnfo[cmp].colldata[ 0 ][ dpos ], scan_info.sal );
							
							// encode dc correction bit
							this->dc_prg_sa(huffw, block);
							
							// next mcupos if no error happened
							status = jpg::next_mcuposn(cmpnfo[cmp], rsti, &dpos, &rstw);
						}
					}
				}
				else {
					if ( scan_info.sah == 0 ) {
						// ---> progressive non interleaved AC encoding <---
						// ---> succesive approximation first stage <---
						while ( status == CodingStatus::OKAY ) {
							// copy from colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								block[ bpos ] =
									fdiv2(cmpnfo[cmp].colldata[ bpos ][ dpos ], scan_info.sal );
							
							// encode block
							int eob = this->ac_prg_fs( huffw,
							                              *hcodes[1][cmpnfo[cmp].huffac],
							                              block, &eobrun, scan_info.from, scan_info.to );
							
							// check for errors, proceed if no error encountered
							if ( eob < 0 ) status = CodingStatus::ERROR;
							else status = jpg::next_mcuposn(cmpnfo[cmp], rsti, &dpos, &rstw);
						}						
						
						// encode remaining eobrun
						this->eobrun(huffw, *hcodes[1][cmpnfo[cmp].huffac], &eobrun);
					}
					else {
						// ---> progressive non interleaved AC encoding <---
						// ---> succesive approximation later stage <---
						while ( status == CodingStatus::OKAY ) {
							// copy from colldata
							for (int bpos = scan_info.from; bpos <= scan_info.to; bpos++)
								block[ bpos ] =
									fdiv2(cmpnfo[cmp].colldata[ bpos ][ dpos ], scan_info.sal );
							
							// encode block
							int eob = this->ac_prg_sa( huffw, storw,
							                              *hcodes[1][cmpnfo[cmp].huffac],
							                              block, &eobrun, scan_info.from, scan_info.to );
							
							// check for errors, proceed if no error encountered
							if ( eob < 0 ) status = CodingStatus::ERROR;
							else status = jpg::next_mcuposn(cmpnfo[cmp], rsti, &dpos, &rstw);
						}						
						
						// encode remaining eobrun
						this->eobrun(huffw, *hcodes[1][cmpnfo[cmp].huffac], &eobrun);
							
						// encode remaining correction bits
						this->crbits( huffw, storw );
					}
				}
			}
			
			// pad huffman writer
			huffw->pad();
			
			// evaluate status
			if ( status == CodingStatus::ERROR ) {
				throw std::runtime_error("encode error in scan" + std::to_string(scan_count) 
					+ " / mcu" + std::to_string(( scan_info.cmpc > 1 ) ? mcu : dpos));
			}
			else if ( status == CodingStatus::DONE ) {
				scan_count++; // increment scan counter
				break; // leave decoding loop, everything is done here
			}
			else if ( status == CodingStatus::RESTART ) {
				if ( rsti > 0 ) // store rstp & stay in the loop
					rstp[ rstc++ ] = huffw->getpos() - 1;
			}
		}
	}
	
	// get data into huffdata
	huffdata = huffw->get_data();
	
	// store last scan & restart positions
	scnp[ scan_count ] = huffdata.size();
	if (!rstp.empty()) {
		rstp[rstc] = huffdata.size();
	}
}

PjgEncoder::PjgEncoder(const std::unique_ptr<iostream>& encoding_output) {
#if defined(DEV_INFOS)
	int dev_size = 0;
#endif

	// PJG-Header
	encoding_output->write(program_info::pjg_magic.data(), 2);

	// store settings if not auto
	if (!auto_set) {
		std::uint8_t hcode = 0x00;
		encoding_output->write_byte(hcode);
		for (std::size_t cmpt = 0; cmpt < cmpnfo.size(); cmpt++) {
			encoding_output->write_byte(cmpnfo[cmpt].nois_trs);
		}

		for (std::size_t cmpt = 0; cmpt < cmpnfo.size(); cmpt++) {
			encoding_output->write_byte(cmpnfo[cmpt].segm_cnt);
		}
	}

	// store version number
	const auto hcode = program_info::appversion;
	encoding_output->write_byte(hcode);

	// init arithmetic compression
	encoder_ = std::make_unique<ArithmeticEncoder>(encoding_output.get());
}

void PjgEncoder::encode() {	
	// optimize header for compression
	this->optimize_header(segments);
	// set padbit to 1 if previously unset
	if (jpg::padbit == -1 )	jpg::padbit = 1;
	
	// encode JPG header
	this->generic(segments);
	// store padbit (padbit can't be retrieved from the header)
	this->bit(jpg::padbit);
	// also encode one bit to signal false/correct use of RST markers
	this->bit(jpg::rst_err.empty() ? 0 : 1);
	// encode # of false set RST markers per scan
	if (!jpg::rst_err.empty()) {
		this->generic(jpg::rst_err);
	}
	
	// encode actual components data
	for (int cmp = 0; cmp < cmpnfo.size(); cmp++ ) {
		auto& cmpt = cmpnfo[cmp];
		#if !defined(DEV_INFOS)
		// encode frequency scan ('zero-sort-scan')
		cmpt.freqscan = this->zstscan(cmpt); // set zero sort scan as freqscan
		// encode zero-distribution-lists for higher (7x7) ACs
		this->zdst_high(cmpt);
		// encode coefficients for higher (7x7) ACs
		this->ac_high(cmpt);
		// encode zero-distribution-lists for lower ACs
		this->zdst_low(cmpt);
		// encode coefficients for first row / collumn ACs
		this->ac_low(cmpt);
		// encode coefficients for DC
		this->dc(cmpt);
		#else
		dev_size = str_out->getpos();
		// encode frequency scan ('zero-sort-scan')
		cmpt.freqscan = this->zstscan(cmpt); // set zero sort scan as freqscan
		dev_size_zsr[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode zero-distribution-lists for higher (7x7) ACs
		this->zdst_high(cmpt);
		dev_size_zdh[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode coefficients for higher (7x7) ACs
		this->ac_high(cmpt);
		dev_size_ach[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode zero-distribution-lists for lower ACs
		this->zdst_low(cmpt);
		dev_size_zdl[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode coefficients for first row / collumn ACs
		this->ac_low(cmpt);
		dev_size_acl[ cmp ] += str_out->getpos() - dev_size;
		dev_size = str_out->getpos();
		// encode coefficients for DC
		this->dc(cmpt);
		dev_size_dc[ cmp ] += str_out->getpos() - dev_size;
		dev_size_cmp[ cmp ] = 
			dev_size_zsr[ cmp ] + dev_size_zdh[ cmp ] +	dev_size_zdl[ cmp ] +
			dev_size_ach[ cmp ] + dev_size_acl[ cmp ] +	dev_size_dc[ cmp ];
		#endif
	}
	
	// encode checkbit for garbage (0 if no garbage, 1 if garbage has to be coded)
	this->bit(!grbgdata.empty() ? 1 : 0);
	// encode garbage data only if needed
	if (!grbgdata.empty()) {
		this->generic(grbgdata);
	}
	
	// errormessage if write error
	if ( str_out->chkerr() ) {
		throw std::runtime_error("write error, possibly drive is full");
	}
	
	// get filesize
	pjgfilesize = str_out->getsize();
}

PjgDecoder::PjgDecoder(const std::unique_ptr<iostream>& decoding_stream) {
	// check header codes ( maybe position in other function ? )
	while (true) {
		std::uint8_t hcode;
		try {
			hcode = decoding_stream->read_byte();
		} catch (const std::runtime_error&) {
			throw;
		}
		if (hcode == 0x00) {
			// retrieve compression settings from file
			for (auto& cmpt : cmpnfo) {
				try {
					cmpt.nois_trs = decoding_stream->read_byte();
				} catch (std::runtime_error&) {
					throw;
				}
			}
			for (auto& cmpt : cmpnfo) {
				try {
					cmpt.segm_cnt = decoding_stream->read_byte();
				} catch (std::runtime_error&) {
					throw;
				}
			}
			auto_set = false;
		} else if (hcode >= 0x14) {
			// compare version number
			if (hcode != program_info::appversion) {
				throw std::runtime_error("incompatible file, use " + program_info::appname
					+ " v" + std::to_string(hcode / 10) + "." + std::to_string(hcode % 10));
			} else {
				break;
			}
		} else {
			throw std::runtime_error("unknown header code, use newer version of " + program_info::appname);
		}
	}


	// init arithmetic compression
	decoder_ = std::make_unique<ArithmeticDecoder>(decoding_stream.get());

	// get filesize
	pjgfilesize = decoding_stream->getsize();
}

void PjgDecoder::decode() {
	// decode JPG header
	segments = Segment::parse_segments(this->generic());
	// retrieve padbit from stream
	jpg::padbit = this->bit();
	// decode one bit that signals false /correct use of RST markers
	auto cb = this->bit();
	// decode # of false set RST markers per scan only if available
	if ( cb == 1 ) {
		jpg::rst_err = this->generic();
	}
	
	// undo header optimizations
	this->deoptimize_header(segments);
	// parse header for image-info
	try {
		jpg::setup_imginfo();
	} catch (const std::exception&) {
		throw;
	}
	
	// decode actual components data
	for (auto& cmpt : cmpnfo) {
		// decode frequency scan ('zero-sort-scan')
		cmpt.freqscan = this->zstscan(); // set zero sort scan as freqscan
		// decode zero-distribution-lists for higher (7x7) ACs
		this->zdst_high(cmpt);
		// decode coefficients for higher (7x7) ACs
		this->ac_high(cmpt);
		// decode zero-distribution-lists for lower ACs
		this->zdst_low(cmpt);
		// decode coefficients for first row / collumn ACs
		this->ac_low(cmpt);
		// decode coefficients for DC
		this->dc(cmpt);
	}
	
	// retrieve checkbit for garbage (0 if no garbage, 1 if garbage has to be coded)
	auto garbage_exists = this->bit() == 1;
	
	// decode garbage data only if available
	if (garbage_exists) {
		grbgdata = this->generic();
	}
}

/* ----------------------- End of main functions -------------------------- */

/* ----------------------- Begin of JPEG specific functions -------------------------- */

void jpg::setup_imginfo()
{
		
	// header parser loop
	for (const auto& segment : segments) {
		const Marker type = segment.get_type();
		if (type != Marker::kDHT
			&& type != Marker::kDRI
			&& type != Marker::kSOS) {
			try {
				jfif::parse_jfif(segment);
			} catch (const std::runtime_error&) {
				throw;
			}
		}
	}

	// check if information is complete
	if (cmpnfo.empty()) {
		throw std::runtime_error("header contains incomplete information");
	}
	for (const auto& cmpt : cmpnfo) {
		if (cmpt.sfv == 0
			|| cmpt.sfh == 0
			|| cmpt.qtable[0] == 0
			|| jpegtype == JpegType::UNKNOWN) {
			throw std::runtime_error("header information is incomplete");
		}
	}

	// do all remaining component info calculations
	int sfhm = -1; // max horizontal sample factor
	int sfvm = -1; // max verical sample factor
	for (const auto& cmpt : cmpnfo) {
		sfhm = std::max(cmpt.sfh, sfhm);
		sfvm = std::max(cmpt.sfv, sfvm);
	}
	const int mcuv = static_cast<int>(ceil(static_cast<float>(image::imgheight) / static_cast<float>(8 * sfhm))); // MCUs per line.
	image::mcuh = static_cast<int>(ceil(static_cast<float>(image::imgwidth) / static_cast<float>(8 * sfvm)));
	image::mcuc = mcuv * image::mcuh;
	for (auto& cmpt : cmpnfo) {
		cmpt.mbs = cmpt.sfv * cmpt.sfh;
		cmpt.bcv = mcuv * cmpt.sfh;
		cmpt.bch = image::mcuh * cmpt.sfv;
		cmpt.bc = cmpt.bcv * cmpt.bch;
		cmpt.ncv = static_cast<int>(ceil(static_cast<float>(image::imgheight) *
			(static_cast<float>(cmpt.sfh) / (8.0 * sfhm))));
		cmpt.nch = static_cast<int>(ceil(static_cast<float>(image::imgwidth) *
			(static_cast<float>(cmpt.sfv) / (8.0 * sfvm))));

		for (auto& coeffs : cmpt.colldata) {
			coeffs.resize(cmpt.bc);
		}

		cmpt.zdstdata = std::vector<std::uint8_t>(cmpt.bc);
		cmpt.eobxhigh = std::vector<std::uint8_t>(cmpt.bc);
		cmpt.eobyhigh = std::vector<std::uint8_t>(cmpt.bc);
		cmpt.zdstxlow = std::vector<std::uint8_t>(cmpt.bc);
		cmpt.zdstylow = std::vector<std::uint8_t>(cmpt.bc);

	}

	// decide components' statistical ids
	if (cmpnfo.size() <= 3) {
		for (std::size_t cmpt = 0; cmpt < cmpnfo.size(); cmpt++) {
			cmpnfo[cmpt].sid = cmpt;
		}
	} else {
		for (auto& cmpt : cmpnfo) {
			cmpt.sid = 0;
		}
	}

	// also decide automatic settings here
	if (auto_set) {
		for (auto& cmpt : cmpnfo) {
			int i;
			for (i = 0;
			     pjg::conf_sets[i][cmpt.sid] > static_cast<std::uint32_t>(cmpt.bc);
			     i++);
			cmpt.segm_cnt = pjg::conf_segm;
			cmpt.nois_trs = pjg::conf_ntrs[i][cmpt.sid];
		}
	}
}

CodingStatus jpg::next_mcupos(const ScanInfo& scan_info, int rsti, int* mcu, int* cmp, int* csc, int* sub, int* dpos, int* rstw)
{
	CodingStatus sta = CodingStatus::OKAY;
	
	
	// increment all counts where needed
	if ( ( ++(*sub) ) >= cmpnfo[(*cmp)].mbs ) {
		(*sub) = 0;
		
		if ( ( ++(*csc) ) >= scan_info.cmpc ) {
			(*csc) = 0;
			(*cmp) = scan_info.cmp[ 0 ];
			(*mcu)++;
			if ( (*mcu) >= image::mcuc ) sta = CodingStatus::DONE;
			else if ( rsti > 0 )
				if ( --(*rstw) == 0 ) sta = CodingStatus::RESTART;
		}
		else {
			(*cmp) = scan_info.cmp[(*csc)];
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

CodingStatus jpg::next_mcuposn(const Component& cmpt, int rsti, int* dpos, int* rstw)
{
	// increment position
	(*dpos)++;
	
	// fix for non interleaved mcu - horizontal
	if ( cmpt.bch != cmpt.nch ) {
		if ( (*dpos) % cmpt.bch == cmpt.nch )
			(*dpos) += ( cmpt.bch - cmpt.nch );
	}
	
	// fix for non interleaved mcu - vertical
	if ( cmpt.bcv != cmpt.ncv ) {
		if ( (*dpos) / cmpt.bch == cmpt.ncv )
			(*dpos) = cmpt.bc;
	}
	
	// check position
	if ( (*dpos) >= cmpt.bc ) return CodingStatus::DONE;
	else if ( rsti > 0 )
		if ( --(*rstw) == 0 ) return CodingStatus::RESTART;
	

	return CodingStatus::OKAY;
}

/* ----------------------- End of JPEG specific functions -------------------------- */

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
static bool dump_hdr() {
	const std::string ext = "hdr";
	const auto basename = filelist[file_no];

	if (!dump_file(basename, ext, hdrdata.data(), 1, hdrdata.size())) {
		return false;
	}

	return true;
}

/* -----------------------------------------------
	Writes huffman coded file
	----------------------------------------------- */
static bool dump_huf() {
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
static bool dump_coll()
{
	const std::array<std::string, 4> ext{ "coll0", "coll1", "coll2", "coll3" };
	const auto& base = filelist[file_no];

	for (int cmp = 0; cmp < cmpnfo.size(); cmp++) {
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
				fwrite(cmpnfo[cmp].colldata[bpos].data(), sizeof(short), cmpnfo[cmp].bc, fp);
			}
			break;

		case CollectionMode::DHF:
			for (dpos = 0; dpos < cmpnfo[cmp].bc; dpos++) {
				for (int bpos = 0; bpos < 64; bpos++) {
					fwrite(&(cmpnfo[cmp].colldata[bpos][dpos]), sizeof(short), 1, fp);
				}
			}
			break;

		case CollectionMode::SQU:
			dpos = 0;
			for (int i = 0; i < 64; ) {
				const int bpos = pjg::zigzag[i++];
				fwrite(&(cmpnfo[cmp].colldata[bpos][dpos]), sizeof(short),
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
					const int bpos = pjg::zigzag[((i % 8) * 8) + (j % 8)];
					dpos = ((i / 8) * cmpnfo[cmp].bch) + (j / 8);
					fwrite(&(cmpnfo[cmp].colldata[bpos][dpos]), sizeof(short), 1, fp);
				}
			}
			break;

		case CollectionMode::SQU_ALT:
			dpos = 0;
			for (int i = 0; i < 64; ) {
				int bpos = pjg::even_zigzag[i++];
				fwrite(&(cmpnfo[cmp].colldata[bpos][dpos]), sizeof(short),
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
					const int bpos = pjg::even_zigzag[((i % 8) * 8) + (j % 8)];
					dpos = ((i / 8) * cmpnfo[cmp].bch) + (j / 8);
					fwrite(&(cmpnfo[cmp].colldata[bpos][dpos]), sizeof(short), 1, fp);
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
static bool dump_zdst() {
	const std::array<std::string, 4> ext{ "zdst0", "zdst1", "zdst2", "zdst3" };
	const auto basename = filelist[file_no];

	for (int cmp = 0; cmp < cmpnfo.size(); cmp++) {
		if (!dump_file(basename, ext[cmp], cmpnfo[cmp].zdstdata.data(), 1, cmpnfo[cmp].bc)) {
			return false;
		}
	}

	return true;
}

/* -----------------------------------------------
	Writes to file
	----------------------------------------------- */
static bool dump_file(const std::string& base, const std::string& ext, void* data, int bpv, int size) {
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
static bool dump_errfile() {
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
static bool dump_info() {
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
	fprintf( fp, "imageheight: %i / imagewidth: %i\n", image::imgheight, image::imgwidth );
	fprintf( fp, "component count: %u\n", cmpnfo.size());
	fprintf( fp, "mcu count: %i/%i/%i (all/v/h)\n\n", image::mcuc, image::mcuv, image::mcuh );
	
	// info about header
	fprintf(fp, "\nfile header structure:\n");
	fprintf(fp, " type  length   hpos\n");
	// header parser loop
	int hpos; // Position in the header.
	int len = 0; // Length of current marker segment.
	for (hpos = 0; hpos < hdrdata.size(); hpos += len) {
		std::uint8_t type = hdrdata[hpos + 1]; // Type of current marker segment.
		len = 2 + jfif::pack(hdrdata[hpos + 2], hdrdata[hpos + 3]);
		fprintf(fp, " FF%2X  %6i %6i\n", (int)type, len, hpos);
	}
	fprintf(fp, " _END       0 %6i\n", hpos);
	fprintf(fp, "\n");

	// info about compression settings	
	fprintf(fp, "\ncompression settings:\n");
	fprintf(fp, " no of segments    ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
		cmpnfo[0].segm_cnt, cmpnfo[1].segm_cnt, cmpnfo[2].segm_cnt, cmpnfo[3].segm_cnt);
	fprintf(fp, " noise threshold   ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
		cmpnfo[0].nois_trs, cmpnfo[1].nois_trs, cmpnfo[2].nois_trs, cmpnfo[3].nois_trs);
	fprintf(fp, "\n");

	// info about components
	for (int cmp = 0; cmp < cmpnfo.size(); cmp++) {
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
			int bpos = pjg::zigzag[i];
			if ((i % 8) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%4i, ", cmpnfo[cmp].quant(bpos));
		}
		fprintf(fp, "\n");
		fprintf(fp, "maximum values ->");
		for (int i = 0; i < 64; i++) {
			int bpos = pjg::zigzag[i];
			if ((i % 8) == 0) {
				fprintf(fp, "\n");
			}
			fprintf(fp, "%4i, ", cmpnfo[cmp].max_v(bpos));
		}
		fprintf(fp, "\n\n");
	}

	fclose(fp);

	return true;
}

/* -----------------------------------------------
	Writes distribution for use in valdist.h
	----------------------------------------------- */
static bool dump_dist() {
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
	for (int cmp = 0; cmp < cmpnfo.size(); cmp++) {
		for (int bpos = 0; bpos < 64; bpos++) {
			std::array<int, 1024 + 1> dist{};
			// get distribution
			for (int dpos = 0; dpos < cmpnfo[cmp].bc; dpos++) {
				dist[std::abs(cmpnfo[cmp].colldata[bpos][dpos])]++;
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
static bool dump_pgm() {
	const std::array<std::string, 4> ext{ "cmp0.pgm", "cmp1.pgm", "cmp2.pgm", "cmp3.pgm" };

	for (int cmp = 0; cmp < cmpnfo.size(); cmp++) {
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
					int pix_v = cmpnfo[cmp].idct_2d_fst_8x8(dpos, x, y);
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

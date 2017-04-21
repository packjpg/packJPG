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
#include <experimental/filesystem>
#include <map>
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
#include "frameinfo.h"
#include "huffcodes.h"
#include "hufftree.h"
#include "jpg.h"
#include "jfifparse.h"
#include "jpgdecoder.h"
#include "jpgencoder.h"
#include "jpgreader.h"
#include "pjgdecoder.h"
#include "pjgencoder.h"
#include "pjpgtbl.h"
#include "programinfo.h"
#include "reader.h"
#include "scaninfo.h"
#include "segment.h"
#include "streamtype.h"
#include "writer.h"

const std::string FWR_ERRMSG("could not write file / file write-protected: ");

/* -----------------------------------------------
global variables: messages
----------------------------------------------- */

static std::string errormessage;
static bool(*errorfunction)();
static bool  error;

/* -----------------------------------------------
global variables: data storage
----------------------------------------------- */

static std::vector<std::uint8_t> garbage_data; // garbage data
static std::vector<Segment> segments; // Header segments.
static std::vector<std::uint8_t> huffman_data; // huffman coded data

/* -----------------------------------------------
global variables: info about image
----------------------------------------------- */

static std::unique_ptr<FrameInfo> frame_info;

/* -----------------------------------------------
global variables: info about files
----------------------------------------------- */

static std::string destination_file = "";

static int    jpgfilesize;			// size of JPEG file
static int    pjgfilesize;			// size of PJG file
static FileType filetype;				// type of current file
static std::unique_ptr<Reader> str_in;	// input stream
static std::unique_ptr<Writer> str_out;	// output stream
static std::unique_ptr<Reader> str_str;	// storage stream


/* -----------------------------------------------
	function declarations: main interface
	----------------------------------------------- */
static void initialize_options( int argc, char** argv );
static void process_ui();
static std::string get_status( bool (*function)() );
static void show_help();
static void process_file();
static void execute( bool (*function)() );


/* -----------------------------------------------
	function declarations: main functions
	----------------------------------------------- */
static bool check_file();
static bool swap_streams();
static bool compare_output();
static bool reset_buffers();

/* -----------------------------------------------
filter DC coefficients
----------------------------------------------- */
static bool predict_dc() {
	// apply prediction, store prediction error instead of DC
	for (auto& cmpt : frame_info->components) {
		cmpt.predict_dc();
	}
	return true;
}

/* -----------------------------------------------
unpredict DC coefficients
----------------------------------------------- */
static bool unpredict_dc() {
	// remove prediction, store DC instead of prediction error
	for (auto& cmpt : frame_info->components) {
		cmpt.unpredict_dc();
	}
	return true;
}

/* -----------------------------------------------
calculate zero distribution lists
----------------------------------------------- */
static bool calc_zdst_lists() {
	for (auto& cmpt : frame_info->components) {
		cmpt.calc_zdst_lists();
	}

	return true;
}

namespace jpg {

char padbit = -1; // padbit (for huffman coding)

std::vector<std::uint8_t> rst_err; // number of wrong-set RST markers per scan

namespace encode {
	std::unique_ptr<JpgEncoder> jpeg_encoder = std::make_unique<JpgEncoder>();
	// JPEG encoding routine.
	bool recode() {
		try {
			jpeg_encoder->recode(segments, frame_info, jpg::padbit);
		}
		catch (const std::exception& e) {
			errormessage = e.what();
			error = true;
			return false;
		}
		return true;
	}
	// Merges header & image data to jpeg.
	bool merge() {
		try {
			jpeg_encoder->merge(str_out, segments, garbage_data, jpg::rst_err);
		} catch (const std::exception& e) {
			errormessage = e.what();
			error = true;
			return false;
		}

		// get filesize
		jpgfilesize = str_out->num_bytes_written();
		return true;
	}
}

namespace decode {
	std::unique_ptr<JpgDecoder> jpeg_decoder = std::make_unique<JpgDecoder>();

	// Read in header and image data.
	bool read() {
		auto reader = std::make_unique<JpgReader>();
		try {
			reader->read(str_in);

			frame_info = reader->get_frame_info();
			segments = reader->get_segments();
			huffman_data = reader->get_huffman_data();
			jpg::rst_err = reader->get_rst_err();
			garbage_data = reader->get_garbage_data();
		} catch (const std::exception& e) {
			errormessage = e.what();
			error = true;
			return false;
		}
		// get filesize
		jpgfilesize = str_in->get_size();
		return true;
	}
	// JPEG decoding routine.
	bool decode() {
		try {
			jpeg_decoder->decode(frame_info->coding_process, frame_info, segments, frame_info->components, huffman_data);
			jpg::padbit = jpeg_decoder->get_padbit();
		} catch (const std::exception& e) {
			errormessage = e.what();
			error = true;
			return false;
		}
		return true;
	}
	// Checks range of values, error if out of bounds.
	bool check_value_range() {
		try {
			jpeg_decoder->check_value_range(frame_info->components);
		} catch (const std::exception& e) {
			errormessage = e.what();
			error = true;
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
				pjg_encoder->encode(jpg::padbit, frame_info->components, segments, jpg::rst_err, garbage_data);
			} catch (const std::exception& e) {
				errormessage = e.what();
				error = true;
				return false;
			}
			pjgfilesize = str_out->num_bytes_written();
			return true;
		}
	}

	namespace decode {
		bool decode() {
			// get filesize
			pjgfilesize = str_in->get_size();
			try {
				auto pjg_decoder = std::make_unique<PjgDecoder>(str_in);
				pjg_decoder->decode();

				frame_info = pjg_decoder->get_frame_info();
				segments = pjg_decoder->get_segments();
				jpg::padbit = pjg_decoder->get_padbit();
				jpg::rst_err = pjg_decoder->get_rst_err();
				garbage_data = pjg_decoder->get_garbage_data();
			} catch (const std::exception& e) {
				errormessage = e.what();
				error = true;
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
		for (auto& cmpt : frame_info->components) {
			cmpt.adapt_icos();
		}
		return true;
	}
}


/* -----------------------------------------------
	function declarations: miscelaneous helpers
	----------------------------------------------- */
static void progress_bar(int current, int last);
static std::string create_filename(const std::string& oldname, const std::string& new_extension);
static std::string unique_filename(const std::string& oldname, const std::string& new_extension);

static std::vector<std::string> filelist; // list of files to process 
static int    file_no  = 0;			// number of current file


/* -----------------------------------------------
	global variables: settings
	----------------------------------------------- */

static bool verbose = false;	// level of verbosity
static bool overwrite  = false;	// overwrite files yes / no
static bool wait_on_finish  = false;	// pause after finished yes / no
static bool verify = false;		// verification level ( none (0), simple (1), detailed output (2) )

static FILE*  msgout   = stdout;// stream for output of messages
static bool   pipe_on  = false;	// use stdin/stdout instead of filelist


/* -----------------------------------------------
	main-function
	----------------------------------------------- */

int main( int argc, char** argv )
{	
	errormessage = "no errormessage specified";
		
	int error_cnt = 0;
	
	double acc_jpgsize = 0;
	double acc_pjgsize = 0;
	
	
	// read options from command line
	initialize_options( argc, argv );
	
	// write program info to screen
	fprintf( msgout,  "\n--> %s v%i.%i%s (%s) by %s <--\n",
	         program_info::apptitle.c_str(), program_info::appversion / 10, program_info::appversion % 10, program_info::subversion.c_str(), program_info::versiondate.c_str(), program_info::author.c_str());
	fprintf( msgout, "Copyright %s\nAll rights reserved\n\n", program_info::copyright.c_str() );
	
	// check if user input is wrong, show help screen if it is
	if (filelist.empty()) {
		show_help();
		return -1;
	}
	
	// (re)set program has to be done first
	reset_buffers();
	
	// process file(s) - this is the main function routine
	static std::vector<std::string> err_list(filelist.size()); // list of error messages 
	static std::vector<bool> err_tp(filelist.size()); // list of error types

	auto begin = std::chrono::steady_clock::now();
	for ( file_no = 0; file_no < filelist.size(); file_no++ ) {
		// process current file
		process_ui();
		// store error message and type if any
		if (error) {
			err_tp[file_no] = true;
			err_list[file_no] = errormessage;
		}
		// count errors / warnings / file sizes
		if (error) error_cnt++;
		else {
			acc_jpgsize += jpgfilesize;
			acc_pjgsize += pjgfilesize;
		}
	}
	auto end = std::chrono::steady_clock::now();
	
	// errors summary: only needed for -v2 or progress bar
	if (verbose) {
		// print summary of errors to screen
		if ( error_cnt > 0 ) {
			fprintf( stderr, "\n\nfiles with errors:\n" );
			fprintf( stderr, "------------------\n" );
			for ( file_no = 0; file_no < filelist.size(); file_no++ ) {
				if (err_tp[ file_no ]) {
					fprintf( stderr, "%s (%s)\n", filelist[ file_no ].c_str(), err_list[ file_no ].c_str());
				}
			}
		}
	}
	
	// show statistics
	fprintf( msgout,  "\n\n-> %i file(s) processed, %i error(s)\n",
		filelist.size(), error_cnt);
	if ( (filelist.size() > error_cnt ) && verbose) {
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
	}
	
	// pause before exit
	if ( wait_on_finish && ( msgout != stderr ) ) {
		fprintf( msgout, "\n\n< press ENTER >\n" );
		fgetc( stdin );
	}
	
	
	return 0;
}

/* ----------------------- Begin of main interface functions -------------------------- */


/* -----------------------------------------------
	reads in commandline arguments
	----------------------------------------------- */
static void initialize_options(int argc, char** argv) {
	// read in arguments
	while (--argc > 0) {
		argv++;
		std::string arg = *argv;
		// switches begin with '-'
		if (arg == "-verify" || arg == "-ver") {
			verify = true;
		} else if (arg == "-verbose" || arg == "-v2" || arg == "-v1") {
			verbose = true;
		} else if (arg == "-w") {
			wait_on_finish = true;
		} else if (arg == "-o") {
			overwrite = true;
		} else if (arg == "-") {
			// switch standard message out stream
			msgout = stderr;
			// use "-" as placeholder for stdin
			filelist.push_back("-");
		} else {
			// if argument is not switch, it's a filename
			filelist.push_back(arg);
		}
	}
}

/* -----------------------------------------------
	UI for processing one file
	----------------------------------------------- */
static void process_ui() {

	errorfunction = nullptr;
	error = false;
	jpgfilesize = 0;
	pjgfilesize = 0;

	// compare file name, set pipe if needed
	if (filelist[file_no] == "-") {
		pipe_on = true;
		filelist[file_no] = "STDIN";
	} else {
		pipe_on = false;
	}

	std::string actionmsg;
	fprintf(msgout, "\nProcessing file %i of %u \"%s\" -> ",
	        file_no + 1, filelist.size(), filelist[file_no].c_str());

	if (verbose) {
		fprintf(msgout, "\n----------------------------------------");
	}

	// check input file and determine filetype
	execute(check_file);

	// get specific action message
	if (filetype == FileType::F_UNK) {
		actionmsg = "unknown filetype";
	} else {
		actionmsg = filetype == FileType::F_JPG ? "Compressing" : "Decompressing";
	}

	if (!verbose) {
		fprintf(msgout, "%s -> ", actionmsg.c_str());
	}
	fflush(msgout);


	// main function routine
	auto begin = std::chrono::steady_clock::now();

	// streams are initiated, start processing file
	process_file();

	// close iostreams
	str_in.reset(nullptr);
	str_out.reset(nullptr);
	str_str.reset(nullptr);
	// delete if broken or if output not needed
	if (!pipe_on && error) {
		if (std::experimental::filesystem::exists(destination_file)) {
			std::experimental::filesystem::remove(destination_file);
		}
	}

	auto end = std::chrono::steady_clock::now();

	// speed and compression ratio calculation
	float cr = (jpgfilesize > 0) ? (100.0 * pjgfilesize / jpgfilesize) : 0;
	if (verbose)
		fprintf(msgout, "\n----------------------------------------");

	// display success/failure message
	std::string errtypemsg;
	if (!verbose) {
		if (!error) {
			fprintf(msgout, "%.2f%%", cr);
		} else {
			fprintf(msgout, "ERROR\n");
		}
	} else {
		if (!error) {
			fprintf(msgout, "\n-> %s OK\n", actionmsg.c_str());
		} else {
			fprintf(msgout, "\n-> %s ERROR\n", actionmsg.c_str());
		}
	}

	// set type of error message
	if (error) {
		errtypemsg = "fatal error";
	} else {
		errtypemsg = "none";
	}

	// error/ warning message
	if (error) {
		fprintf(msgout, " %s -> %s:\n", get_status(errorfunction).c_str(), errtypemsg.c_str());
		fprintf(msgout, " %s\n", errormessage.c_str());
	}
	if (verbose && !error) {
		auto duration = end - begin;
		auto total = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
		if (total >= 0) {
			fprintf(msgout, " time taken  : %7lld msec\n", total);
			int bpms = (total > 0) ? (jpgfilesize / total) : jpgfilesize;
			fprintf(msgout, " byte per ms : %7i byte\n", bpms);
		} else {
			fprintf(msgout, " time taken  : %7s msec\n", "N/A");
			fprintf(msgout, " byte per ms : %7s byte\n", "N/A");
		}
		fprintf(msgout, " comp. ratio : %7.2f %%\n", cr);
	}
	if (verbose)
		fprintf(msgout, "\n");
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
	} else {
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
	fprintf( msgout, "\n" );
	fprintf( msgout, "Examples: \"%s -v1 -o baboon.%s\"\n", program_info::appname.c_str(), program_info::pjg_ext.c_str() );
	fprintf( msgout, "          \"%s -p *.%s\"\n", program_info::appname.c_str(), program_info::jpg_ext.c_str() );
}

/* -----------------------------------------------
	processes one file
	----------------------------------------------- */
static void process_file() {
	if (filetype == FileType::F_JPG) {
		execute(jpg::decode::read);
		execute(jpg::decode::decode);
		execute(jpg::decode::check_value_range);
		execute(dct::adapt_icos);
		execute(predict_dc);
		execute(calc_zdst_lists);
		execute(pjg::encode::encode);
		if (verify) {
			execute(reset_buffers);
			execute(swap_streams);
			execute(pjg::decode::decode);
			execute(dct::adapt_icos);
			execute(unpredict_dc);
			execute(jpg::encode::recode);
			execute(jpg::encode::merge);
			execute(compare_output);
		}
	} else if (filetype == FileType::F_PJG) {
		execute(pjg::decode::decode);
		execute(dct::adapt_icos);
		execute(unpredict_dc);
		execute(jpg::encode::recode);
		execute(jpg::encode::merge);
		if (verify) {
			execute(reset_buffers);
			execute(swap_streams);
			execute(jpg::decode::read);
			execute(jpg::decode::decode);
			execute(jpg::decode::check_value_range);
			execute(dct::adapt_icos);
			execute(predict_dc);
			execute(calc_zdst_lists);
			execute(pjg::encode::encode);
			execute(compare_output);
		}
	}
	// reset buffers
	reset_buffers();
}


/* -----------------------------------------------
	main-function execution routine
	----------------------------------------------- */
static void execute( bool (*function)() )
{
	if (!error) {		
		// write statusmessage
		if (verbose) {
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
		
		if (error && ( errorfunction == nullptr ) )
			errorfunction = function;
		
		// write time or failure notice
		if ( success ) {
			auto duration = end - begin;
			auto total = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
			if (verbose) fprintf( msgout,  "%7lldms", ( total >= 0 ) ? total : -1 );
		}
		else {
			errorfunction = function;
			if (verbose) fprintf( msgout,  "%8s", "ERROR" );
		}
	}
}

/* ----------------------- End of main interface functions -------------------------- */

/* ----------------------- Begin of main functions -------------------------- */


/* -----------------------------------------------
	check file and determine filetype
	----------------------------------------------- */
static bool check_file() {
	std::uint8_t fileid[ 2 ]{};
	const std::string& filename = filelist[file_no];


	// open input stream, check for errors
	if (pipe_on) {
		str_in = std::make_unique<StreamReader>();
	} else {
		try {
			str_in = std::make_unique<FileReader>(filename);
		} catch (const std::runtime_error& e) {
			errormessage = e.what();
			error = true;
			return false;
		}
	}

	// free memory from filenames if needed
	destination_file = "";

	// immediately return error if 2 bytes can't be read
	if (str_in->read(fileid, 2) != 2) {
		filetype = FileType::F_UNK;
		errormessage = "file doesn't contain enough data";
		error = true;
		return false;
	}

	// check file id, determine filetype
	if ((fileid[0] == 0xFF) && (fileid[1] == 0xD8)) {
		// file is JPEG
		filetype = FileType::F_JPG;
		// create filenames
		if (!pipe_on) {
			destination_file = (overwrite) ?
				              create_filename(filename, program_info::pjg_ext) :
				              unique_filename(filename, program_info::pjg_ext);
		}
	} else if ((fileid[0] == program_info::pjg_magic[0]) && (fileid[1] == program_info::pjg_magic[1])) {
		// file is PJG
		filetype = FileType::F_PJG;
		// create filenames
		if (!pipe_on) {
			destination_file = (overwrite) ?
				              create_filename(filename, program_info::jpg_ext) :
				              unique_filename(filename, program_info::jpg_ext);
		}
	} else {
		// file is neither
		filetype = FileType::F_UNK;
		errormessage = "file type of " + filename + " is unknown";
		error = true;
		return false;
	}

	if (pipe_on) {
		str_out = std::make_unique<StreamWriter>();
	} else {
		try {
			str_out = std::make_unique<FileWriter>(destination_file);
		} catch (const std::runtime_error& e) {
			errormessage = e.what();
			error = true;
			return false;
		}
	}

	return true;
}

/* -----------------------------------------------
	swap streams / init verification
	----------------------------------------------- */
static bool swap_streams() {
	std::array<std::uint8_t, 2> magic_bytes;
	
	// store input stream
	str_str = std::move(str_in);
	str_str->rewind();
	
	// replace input stream by output stream / switch mode for reading / read first bytes
	const auto pjg_bytes = str_out->get_data();
	str_in = std::make_unique<MemoryReader>(pjg_bytes);
	str_in->read(magic_bytes.data(), 2);
	
	// open new stream for output / check for errors
	str_out = std::make_unique<MemoryWriter>();
	
	return true;
}

/* -----------------------------------------------
	comparison between input & output
	----------------------------------------------- */
static bool compare_output() {
	const auto& input_data = str_str->get_data();
	const auto& verif_data = str_out->get_data();
	if (input_data.size() != verif_data.size()) {
		errormessage = "Expected size: " + std::to_string(input_data.size()) + ", Verification size: " + std::to_string(verif_data.size());
		error = true;
		return false;
	}

	const auto result = std::mismatch(std::begin(input_data),
	                                  std::end(input_data),
	                                  std::begin(verif_data),
	                                  std::end(verif_data));
	if (result.first != std::end(input_data)
		|| result.second != std::end(verif_data)) {
		const auto first_diff = std::distance(std::begin(input_data), result.first);
		errormessage = "Difference found at byte position " + std::to_string(first_diff);
		error = true;
		return false;
	}

	return true;
}

/* -----------------------------------------------
	set each variable to its initial value
	----------------------------------------------- */

static bool reset_buffers() {
	// free buffers & set pointers nullptr
	segments.clear();
	huffman_data.clear();
	garbage_data.clear();
	jpg::rst_err.clear();

	jpg::encode::jpeg_encoder = std::make_unique<JpgEncoder>();
	
	frame_info.reset(nullptr);

	// reset padbit
	jpg::padbit = -1;

	return true;
}

/* ----------------------- End of main functions -------------------------- */

/* ----------------------- Begin of miscellaneous helper functions -------------------------- */


/* -----------------------------------------------
	displays progress bar on screen
	----------------------------------------------- */
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
	while (std::experimental::filesystem::exists(filename)) {
		filename_base += "_";
		filename = filename_base + "." + new_extension;
	}
	return filename;
}

/* ----------------------- End of miscellaneous helper functions -------------------------- */

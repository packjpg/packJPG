/*
packJPG v2.5m (11/21/2026)
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

v2.5l (11/21/26) (public)
 - fixed memory safety, Huffman/RST, and malformed-stream bugs
 - modular C++17 reentrant codec & thread-safe library API
 - ~1.6x-1.7x codec & streaming I/O speedup
 - added -out, -r, wildcard, and -j parallel batch CLI options
 - cross-platform Linux, macOS, and Windows builds & test suite

v2.5m (11/21/26) (public)
 - fixed CLI file-safety, C/C++ library ABI, and codec round-trip/UB bugs
 - added Windows IrfanView & Susie .pjg plug-in (contrib/irfanview)


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

#include "codec_pipeline.h"
#include "path_utils.h"

#if defined(DEV_BUILD)
#include "dev_tools.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {

int default_worker_count() noexcept
{
	const unsigned int hc = std::thread::hardware_concurrency();
	return ( hc > 0 ) ? clamped( 1, 64, static_cast<int>( hc ) ) : 1;
}

bool parse_strict_nonneg_int( const char* s, int max_clamp, int& out ) noexcept
{
	if ( ( s == nullptr ) || !std::isdigit( static_cast<unsigned char>( s[ 0 ] ) ) ) {
		return false;
	}
	errno = 0;
	char* endp = nullptr;
	const long val = std::strtol( s, &endp, 10 );
	if ( ( endp == nullptr ) || ( *endp != '\0' ) ) {
		return false;
	}
	if ( ( errno == ERANGE ) || ( val > max_clamp ) ) {
		out = max_clamp;
	}
	else if ( val < 0 ) {
		out = 0;
	}
	else {
		out = static_cast<int>( val );
	}
	return true;
}

#if defined(DEV_BUILD)
bool parse_strict_pair_int( const char* s, int max_a, int max_b, int& out_a, int& out_b ) noexcept
{
	if ( ( s == nullptr ) || !std::isdigit( static_cast<unsigned char>( s[ 0 ] ) ) ) {
		return false;
	}
	errno = 0;
	char* comma = nullptr;
	const long va = std::strtol( s, &comma, 10 );
	if ( ( comma == nullptr ) || ( *comma != ',' ) || !std::isdigit( static_cast<unsigned char>( comma[ 1 ] ) ) ) {
		return false;
	}
	const bool overflow_a = ( errno == ERANGE );
	errno = 0;
	char* endp = nullptr;
	const long vb = std::strtol( comma + 1, &endp, 10 );
	if ( ( endp == nullptr ) || ( *endp != '\0' ) ) {
		return false;
	}
	const bool overflow_b = ( errno == ERANGE );
	out_a = ( overflow_a || ( va > max_a ) ) ? max_a : static_cast<int>( std::max( 0L, va ) );
	out_b = ( overflow_b || ( vb > max_b ) ) ? max_b : static_cast<int>( std::max( 0L, vb ) );
	return true;
}
#endif

struct CliSettings {
	int   verbosity      = -1;
	bool  overwrite      = false;
	bool  wait_exit      = true;
	int   verify_lv      = 0;
	bool  developer      = false;
	bool  recursive      = false;
	int   jobs           = 1;
	bool  orig_auto_set  = true;
	int   collmode       = 0;
	FILE* msgout         = stdout;
	bool  had_expanded   = false;
	std::string out_dir;
	std::string arg_error;
	std::array<unsigned char, 8> orig_set{};
	std::vector<CliInputItem> inputs;
	std::vector<std::string>  err_list;
	std::vector<int>          err_tp;
};

struct BatchPathGuard {
	std::unordered_set<std::string> input_keys;
	std::unordered_set<std::string> claimed_output_keys;
	std::mutex                      mtx;

	explicit BatchPathGuard( const std::vector<CliInputItem>& items )
	{
		input_keys.reserve( items.size() );
		claimed_output_keys.reserve( items.size() );
		for ( const auto& it : items ) {
			if ( ( it.path != "-" ) && ( it.path != "STDIN" ) && file_exists( it.path ) ) {
				const std::string k = normalize_path_key( it.path );
				if ( !k.empty() ) {
					input_keys.insert( k );
				}
			}
		}
	}
};

struct FileOutcome {
	int          errorlevel  = 0;
	StageId      error_stage = StageId::None;
	std::string  errormessage;
	int          filetype    = F_UNK;
	std::int64_t jpgfilesize = 0;
	std::int64_t pjgfilesize = 0;
	int          elapsed_ms  = 0;
};

void progress_bar( FILE* msgout, int current, int last )
{
	const int safe_last = ( last > 0 ) ? last : 1;
	const int barpos = ( ( current * kBarLen ) + ( safe_last / 2 ) ) / safe_last;

	std::fprintf( msgout, "[" );
#if defined(_WIN32)
	int i = 0;
	for ( ; i < barpos; i++ )
		std::fprintf( msgout, "\xFE" );
#else
	int i = 0;
	for ( ; i < barpos; i++ )
		std::fprintf( msgout, "X" );
#endif
	for ( ; i < kBarLen; i++ )
		std::fprintf( msgout, " " );
	std::fprintf( msgout, "]" );
}

void show_help( const CliSettings& cli )
{
	std::fprintf( cli.msgout, "\n" );
	std::fprintf( cli.msgout, "Website: %s\n", kWebsite );
	std::fprintf( cli.msgout, "Email  : %s\n", kEmail );
	std::fprintf( cli.msgout, "\n" );
	std::fprintf( cli.msgout, "Usage: %s [switches] [filename(s) / dir(s)]\n\n", kAppName );
	std::fprintf( cli.msgout, " [-ver]      verify files after processing\n" );
	std::fprintf( cli.msgout, " [-v?]       set level of verbosity (max: 2) (def: 0)\n" );
	std::fprintf( cli.msgout, " [-np]       no pause after processing files\n" );
	std::fprintf( cli.msgout, " [-o]        overwrite existing files\n" );
	std::fprintf( cli.msgout, " [-out dir]  write output files to directory\n" );
	std::fprintf( cli.msgout, " [-r]        recursively process directories / wildcards\n" );
	std::fprintf( cli.msgout, " [-j?]       parallel worker threads (-j or -j0 = auto)\n" );
	std::fprintf( cli.msgout, " [-p]        proceed on warnings\n" );
	std::fprintf( cli.msgout, " [-d]        discard meta-info\n" );
#if defined(DEV_BUILD)
	if ( cli.developer ) {
		std::fprintf( cli.msgout, "\n" );
		std::fprintf( cli.msgout, " [-s?]    set global number of segments (1<=s<=49)\n" );
		std::fprintf( cli.msgout, " [-t?]    set global noise threshold (0<=t<=10)\n\n" );
		std::fprintf( cli.msgout, " [-s?,?]  set number of segments for component\n" );
		std::fprintf( cli.msgout, " [-t?,?]  set noise threshold for component\n\n" );
		std::fprintf( cli.msgout, " [-test]  test algorithms, alert if error\n" );
		std::fprintf( cli.msgout, " [-split] split jpeg (to header & image data)\n" );
		std::fprintf( cli.msgout, " [-coll?] write collections (0=std,1=dhf,2=squ,3=unc)\n" );
		std::fprintf( cli.msgout, " [-fcol?] write predicted collections (see above)\n" );
		std::fprintf( cli.msgout, " [-zdst]  write zero distribution lists\n" );
		std::fprintf( cli.msgout, " [-info]  write debug info to .nfo file\n" );
		std::fprintf( cli.msgout, " [-dist]  write distribution data to file\n" );
		std::fprintf( cli.msgout, " [-pgm]   convert and write to pgm files\n" );
	}
#endif
	std::fprintf( cli.msgout, "\n" );
	std::fprintf( cli.msgout, "Examples: \"%s -v1 -o baboon.%s\"\n", kAppName, kPjgExt );
	std::fprintf( cli.msgout, "          \"%s -r -j -out archive photos\"\n", kAppName );
	std::fprintf( cli.msgout, "          \"%s -p *.%s\"\n", kAppName, kJpgExt );
}

void initialize_options( int argc, char** argv, CliSettings& cli, CodecContext& ctx )
{
	int tmp_val = 0;
#if defined(DEV_BUILD)
	int idx     = 0;
#endif

	std::vector<std::string> raw_files;
	raw_files.reserve( static_cast<std::size_t>( argc > 1 ? argc - 1 : 0 ) );

	while ( --argc > 0 ) {
		argv++;
		if ( std::strcmp( *argv, "-p" ) == 0 ) {
			ctx.err_tol = 2;
		}
		else if ( std::strcmp( *argv, "-d" ) == 0 ) {
			ctx.disc_meta = true;
		}
		else if ( std::strcmp( *argv, "-ver" ) == 0 ) {
			if ( cli.verify_lv < 1 ) cli.verify_lv = 1;
		}
		else if ( std::strcmp( *argv, "-vp" ) == 0 ) {
			cli.verbosity = -1;
		}
		else if ( ( std::strncmp( *argv, "-v", 2 ) == 0 ) &&
		          parse_strict_nonneg_int( *argv + 2, 2, tmp_val ) ) {
			cli.verbosity = clamped( 0, 2, tmp_val );
		}
		else if ( std::strcmp( *argv, "-np" ) == 0 ) {
			cli.wait_exit = false;
		}
		else if ( std::strcmp( *argv, "-o" ) == 0 ) {
			cli.overwrite = true;
		}
		else if ( std::strcmp( *argv, "-r" ) == 0 ) {
			cli.recursive = true;
		}
		else if ( std::strcmp( *argv, "-out" ) == 0 ) {
			if ( argc > 1 ) {
				--argc;
				++argv;
				cli.out_dir = *argv;
				if ( cli.out_dir.empty() ) {
					cli.arg_error = "output directory path after -out must not be empty";
				}
			}
			else {
				cli.arg_error = "missing output directory path after -out";
			}
		}
		else if ( std::strncmp( *argv, "-out=", 5 ) == 0 ) {
			cli.out_dir = *argv + 5;
			if ( cli.out_dir.empty() ) {
				cli.arg_error = "output directory path in -out=... must not be empty";
			}
		}
		else if ( std::strcmp( *argv, "-j" ) == 0 ) {
			cli.jobs = default_worker_count();
		}
		else if ( ( ( *argv )[ 0 ] == '-' ) && ( ( *argv )[ 1 ] == 'j' ) &&
		          std::isdigit( static_cast<unsigned char>( ( *argv )[ 2 ] ) ) ) {
			int parsed_jobs = 0;
			if ( parse_strict_nonneg_int( *argv + 2, 64, parsed_jobs ) ) {
				cli.jobs = ( parsed_jobs == 0 ) ? default_worker_count() : clamped( 1, 64, parsed_jobs );
			}
			else {
				raw_files.emplace_back( *argv );
			}
		}
#if defined(DEV_BUILD)
		else if ( std::strcmp( *argv, "-dev" ) == 0 ) {
			cli.developer = true;
		}
		else if ( std::strcmp( *argv, "-test" ) == 0 ) {
			cli.verify_lv = 2;
		}
		else if ( ( std::strncmp( *argv, "-t", 2 ) == 0 ) &&
		          parse_strict_pair_int( *argv + 2, 3, 10, idx, tmp_val ) ) {
			ctx.nois_trs[ idx ] = static_cast<unsigned char>( clamped( 0, 10, tmp_val ) );
			ctx.auto_set = false;
		}
		else if ( ( std::strncmp( *argv, "-s", 2 ) == 0 ) &&
		          parse_strict_pair_int( *argv + 2, 3, 49, idx, tmp_val ) ) {
			ctx.segm_cnt[ idx ] = static_cast<unsigned char>( clamped( 1, 49, tmp_val ) );
			ctx.auto_set = false;
		}
		else if ( ( std::strncmp( *argv, "-t", 2 ) == 0 ) &&
		          parse_strict_nonneg_int( *argv + 2, 10, tmp_val ) ) {
			ctx.nois_trs.fill( static_cast<unsigned char>( clamped( 0, 10, tmp_val ) ) );
			ctx.auto_set = false;
		}
		else if ( ( std::strncmp( *argv, "-s", 2 ) == 0 ) &&
		          parse_strict_nonneg_int( *argv + 2, 49, tmp_val ) ) {
			ctx.segm_cnt.fill( static_cast<unsigned char>( clamped( 1, 49, tmp_val ) ) );
			ctx.auto_set = false;
		}
		else if ( ( std::strncmp( *argv, "-coll", 5 ) == 0 ) &&
		          parse_strict_nonneg_int( *argv + 5, 5, tmp_val ) ) {
			cli.collmode = clamped( 0, 5, tmp_val );
			ctx.action   = A_COLL_DUMP;
		}
		else if ( ( std::strncmp( *argv, "-fcol", 5 ) == 0 ) &&
		          parse_strict_nonneg_int( *argv + 5, 5, tmp_val ) ) {
			cli.collmode = clamped( 0, 5, tmp_val );
			ctx.action   = A_FCOLL_DUMP;
		}
		else if ( std::strcmp( *argv, "-split" ) == 0 ) {
			ctx.action = A_SPLIT_DUMP;
		}
		else if ( std::strcmp( *argv, "-zdst" ) == 0 ) {
			ctx.action = A_ZDST_DUMP;
		}
		else if ( std::strcmp( *argv, "-info" ) == 0 ) {
			ctx.action = A_TXT_INFO;
		}
		else if ( std::strcmp( *argv, "-dist" ) == 0 ) {
			ctx.action = A_DIST_INFO;
		}
		else if ( std::strcmp( *argv, "-pgm" ) == 0 ) {
			ctx.action = A_PGM_DUMP;
		}
		else if ( std::strcmp( *argv, "-comp" ) == 0 ) {
			ctx.action = A_COMPRESS;
		}
#endif
		else if ( std::strcmp( *argv, "-" ) == 0 ) {
			cli.msgout = stderr;
			raw_files.emplace_back( "-" );
		}
		else {
			raw_files.emplace_back( *argv );
		}
	}

	cli.inputs = expand_cli_inputs( raw_files, cli.recursive, cli.out_dir, cli.had_expanded );
	cli.err_list.assign( cli.inputs.size(), std::string() );
	cli.err_tp.assign( cli.inputs.size(), 0 );

	cli.orig_auto_set = ctx.auto_set;
	if ( !ctx.auto_set ) {
		for ( int i = 0; i < 4; i++ ) {
			cli.orig_set[ i ]     = ctx.nois_trs[ i ];
			cli.orig_set[ i + 4 ] = ctx.segm_cnt[ i ];
		}
	}
	else {
		cli.orig_set.fill( 0 );
	}
}

bool check_file(
	const CliSettings& cli,
	CodecContext& ctx,
	const CliInputItem& item,
	bool pipe_on,
	bool& opened_output_file,
	BatchPathGuard& path_guard,
	bool lock_guard )
{
	opened_output_file = false;
	unsigned char fileid[ 2 ] = { 0, 0 };

	try {
		if ( pipe_on ) {
			ctx.str_in = std::make_unique<StreamReader>();
		}
		else {
			ctx.str_in = std::make_unique<FileReader>( item.path );
		}
	} catch ( const std::runtime_error& e ) {
		ctx.errormessage = e.what();
		ctx.errorlevel   = 2;
		return false;
	}

	ctx.jpgfilename.clear();
	ctx.pjgfilename.clear();

	if ( ctx.str_in->read( fileid, 2 ) != 2 ) {
		ctx.filetype     = F_UNK;
		ctx.errormessage = "file doesn't contain enough data";
		ctx.errorlevel   = 2;
		return false;
	}

	auto open_file_output = [&]( const char* ext, std::string& out_name ) {
		const std::string out_base = make_output_base( item.path, cli.out_dir, item.rel_root );
		if ( ctx.action != A_COMPRESS ) {
			out_name = cli.overwrite
				? make_filename( out_base, ext )
				: make_unique_filename( out_base, ext );
			return;
		}

		auto reserve_and_open = [&]() {
			std::string candidate;
			std::string out_key;

			if ( cli.overwrite ) {
				candidate = make_filename( out_base, ext );
				if ( !ensure_parent_directory( candidate ) ) {
					throw std::runtime_error( "could not create output directory for: " + candidate );
				}
				if ( paths_refer_to_same_existing_file( item.path, candidate ) ) {
					throw std::runtime_error( "refusing to overwrite input file: " + candidate );
				}
				out_key = normalize_path_key( candidate );
				if ( !out_key.empty() && ( path_guard.input_keys.count( out_key ) > 0 ) ) {
					throw std::runtime_error( "refusing to overwrite batch input file: " + candidate );
				}
				if ( !out_key.empty() && !path_guard.claimed_output_keys.insert( out_key ).second ) {
					throw std::runtime_error( "output path collision in batch: " + candidate );
				}
			}
			else {
				const std::filesystem::path p( out_base );
				const std::filesystem::path dir = p.parent_path();
				std::string stem = p.stem().string();
				const std::string suffix = ( ext != nullptr ) ? ( std::string( "." ) + ext ) : std::string();
				auto build = [&]() {
					const std::string leaf = stem + suffix;
					return ( dir.empty() ? std::filesystem::path( leaf ) : ( dir / leaf ) ).string();
				};

				candidate = build();
				if ( !ensure_parent_directory( candidate ) ) {
					throw std::runtime_error( "could not create output directory for: " + candidate );
				}
				while ( true ) {
					out_key = normalize_path_key( candidate );
					const bool clash = file_exists( candidate ) ||
					                   paths_refer_to_same_existing_file( item.path, candidate ) ||
					                   ( !out_key.empty() && ( path_guard.input_keys.count( out_key ) > 0 ) ) ||
					                   ( !out_key.empty() && ( path_guard.claimed_output_keys.count( out_key ) > 0 ) );
					if ( !clash ) {
						break;
					}
					stem.push_back( '_' );
					candidate = build();
				}
				if ( !out_key.empty() ) {
					path_guard.claimed_output_keys.insert( out_key );
				}
			}

			try {
				ctx.str_out        = std::make_unique<FileWriter>( candidate );
				out_name           = candidate;
				opened_output_file = true;
			} catch ( ... ) {
				if ( !out_key.empty() ) {
					path_guard.claimed_output_keys.erase( out_key );
				}
				throw;
			}
		};

		if ( lock_guard ) {
			std::lock_guard<std::mutex> lock( path_guard.mtx );
			reserve_and_open();
		}
		else {
			reserve_and_open();
		}
	};

	if ( ( fileid[ 0 ] == 0xFF ) && ( fileid[ 1 ] == 0xD8 ) ) {
		ctx.filetype = F_JPG;
		try {
			if ( pipe_on ) {
				ctx.jpgfilename = "STDIN";
				ctx.pjgfilename = "STDOUT";
				ctx.str_out     = std::make_unique<StreamWriter>();
			}
			else {
				ctx.jpgfilename = item.path;
				open_file_output( kPjgExt, ctx.pjgfilename );
			}
		} catch ( const std::runtime_error& e ) {
			ctx.errormessage = e.what();
			ctx.errorlevel   = 2;
			return false;
		}

		if ( cli.orig_auto_set ) {
			ctx.auto_set = true;
		}
		else {
			for ( int i = 0; i < 4; i++ ) {
				ctx.nois_trs[ i ] = cli.orig_set[ i ];
				ctx.segm_cnt[ i ] = cli.orig_set[ i + 4 ];
			}
			ctx.auto_set = false;
		}
	}
	else if ( ( fileid[ 0 ] == kPjgMagic[ 0 ] ) && ( fileid[ 1 ] == kPjgMagic[ 1 ] ) ) {
		ctx.filetype = F_PJG;
		try {
			if ( pipe_on ) {
				ctx.jpgfilename = "STDOUT";
				ctx.pjgfilename = "STDIN";
				ctx.str_out     = std::make_unique<StreamWriter>();
			}
			else {
				ctx.pjgfilename = item.path;
				open_file_output( kJpgExt, ctx.jpgfilename );
			}
		} catch ( const std::runtime_error& e ) {
			ctx.errormessage = e.what();
			ctx.errorlevel   = 2;
			return false;
		}

		ctx.auto_set = true;
	}
	else {
		ctx.filetype = F_UNK;
		ctx.set_errormessage( "filetype of file \"%s\" is unknown", item.path.c_str() );
		ctx.errorlevel = 2;
		return false;
	}

	return true;
}

bool swap_streams( CodecContext& ctx, std::unique_ptr<Reader>& str_str, std::unique_ptr<Writer>& str_primary_out )
{
	unsigned char dmp[ 2 ];

	str_str = std::move( ctx.str_in );
	str_str->rewind();

	str_primary_out = std::move( ctx.str_out );

	ctx.str_in = std::make_unique<MemoryReader>( str_primary_out->get_data() );
	ctx.str_in->read( dmp, 2 );

	ctx.str_out = std::make_unique<MemoryWriter>();
	return true;
}

bool compare_output( CodecContext& ctx, Reader* str_str )
{
	if ( ctx.str_out->error() ) {
		ctx.errormessage = "error in comparison stream";
		ctx.errorlevel   = 2;
		return false;
	}
	if ( ctx.str_in->error() ) {
		ctx.errormessage = "error in output stream";
		ctx.errorlevel   = 2;
		return false;
	}
	if ( str_str->error() ) {
		ctx.errormessage = "error in input stream";
		ctx.errorlevel   = 2;
		return false;
	}

	const auto verif_data = ctx.str_out->get_data();
	const auto orig_data  = str_str->get_data();

	if ( verif_data.size() != orig_data.size() ) {
		ctx.errormessage = "file sizes do not match";
		ctx.errorlevel   = 2;
		return false;
	}

	const auto result = std::mismatch( orig_data.begin(), orig_data.end(), verif_data.begin(), verif_data.end() );
	if ( ( result.first != orig_data.end() ) || ( result.second != verif_data.end() ) ) {
		const auto first_diff = std::distance( orig_data.begin(), result.first );
		ctx.set_errormessage( "difference found at 0x%ld", static_cast<long>( first_diff ) );
		ctx.errorlevel = 2;
		return false;
	}

	return true;
}

void process_cli_file(
	const CliSettings& cli,
	CodecContext& ctx,
	const std::string& output_base,
	std::unique_ptr<Reader>& str_str,
	std::unique_ptr<Writer>& str_primary_out,
	const StageReporter* reporter )
{
	if ( ctx.action == A_COMPRESS ) {
		if ( ctx.filetype == F_JPG ) {
			run_compress_pipeline( ctx, reporter );
			if ( cli.verify_lv > 0 ) {
				execute_stage( ctx, StageId::ResetBuffers,  [&]() { return ctx.reset_buffers(); },                            reporter );
				execute_stage( ctx, StageId::SwapStreams,   [&]() { return swap_streams( ctx, str_str, str_primary_out ); },  reporter );
				run_decompress_pipeline( ctx, reporter );
				execute_stage( ctx, StageId::CompareOutput, [&]() { return compare_output( ctx, str_str.get() ); },           reporter );
			}
		}
		else if ( ctx.filetype == F_PJG ) {
			run_decompress_pipeline( ctx, reporter );
			if ( cli.verify_lv > 0 ) {
				execute_stage( ctx, StageId::ResetBuffers,  [&]() { return ctx.reset_buffers(); },                            reporter );
				execute_stage( ctx, StageId::SwapStreams,   [&]() { return swap_streams( ctx, str_str, str_primary_out ); },  reporter );
				run_compress_pipeline( ctx, reporter );
				execute_stage( ctx, StageId::CompareOutput, [&]() { return compare_output( ctx, str_str.get() ); },           reporter );
			}
		}
	}
#if defined(DEV_BUILD)
	else {
		run_dev_action( ctx, output_base, cli.collmode, cli.overwrite, reporter );
	}
	if ( ( cli.verify_lv > 1 ) && ( ctx.errorlevel >= ctx.err_tol ) ) {
		dump_errfile( ctx, output_base, cli.overwrite );
	}
#else
	(void) output_base;
#endif
	ctx.reset_buffers();
}

void finalize_and_cleanup_streams(
	bool pipe_on,
	bool opened_output_file,
	CodecContext& ctx,
	std::unique_ptr<Reader>& str_str,
	std::unique_ptr<Writer>& str_primary_out )
{
	Writer* final_out = str_primary_out ? str_primary_out.get() : ctx.str_out.get();
	if ( final_out != nullptr ) {
		if ( ( ctx.errorlevel < ctx.err_tol ) && ( ctx.action == A_COMPRESS ) ) {
			if ( !final_out->flush() ) {
				ctx.errormessage = "write error, possibly drive is full";
				ctx.errorlevel   = 2;
			}
		}
		else {
			final_out->reset();
		}
	}

	ctx.str_in.reset();
	ctx.str_out.reset();
	str_primary_out.reset();
	str_str.reset();

	if ( ( !pipe_on ) && opened_output_file && ( ctx.errorlevel >= ctx.err_tol ) ) {
		if ( ctx.filetype == F_JPG ) {
			if ( file_exists( ctx.pjgfilename ) ) std::remove( ctx.pjgfilename.c_str() );
		}
		else if ( ctx.filetype == F_PJG ) {
			if ( file_exists( ctx.jpgfilename ) ) std::remove( ctx.jpgfilename.c_str() );
		}
	}
}

void process_ui( CliSettings& cli, CodecContext& ctx, int file_no, BatchPathGuard& path_guard )
{
	const int file_cnt = static_cast<int>( cli.inputs.size() );
	const char* actionmsg  = nullptr;
	const char* errtypemsg = nullptr;

	ctx.error_stage = StageId::None;
	ctx.errorlevel  = 0;
	ctx.jpgfilesize = 0;
	ctx.pjgfilesize = 0;
#if !defined(DEV_BUILD)
	ctx.action = A_COMPRESS;
#endif

	bool pipe_on = false;
	if ( ( cli.inputs[ file_no ].path == "-" ) && ( ctx.action == A_COMPRESS ) ) {
		pipe_on = true;
		cli.inputs[ file_no ].path = "STDIN";
	}

	const CliInputItem& item = cli.inputs[ file_no ];
	const StageReporter reporter{ cli.msgout, cli.verbosity };
	bool opened_output_file = false;

	if ( cli.verbosity >= 0 ) {
		std::fprintf( cli.msgout, "\nProcessing file %i of %i \"%s\" -> ",
			file_no + 1, file_cnt, item.path.c_str() );

		if ( cli.verbosity > 1 )
			std::fprintf( cli.msgout, "\n----------------------------------------" );

		execute_stage( ctx, StageId::CheckFile, [&]() {
			return check_file( cli, ctx, item, pipe_on, opened_output_file, path_guard, false );
		}, &reporter );

		if ( ctx.filetype == F_UNK ) {
			actionmsg = "unknown filetype";
		}
		else {
			switch ( ctx.action ) {
				case A_COMPRESS:   actionmsg = ( ctx.filetype == F_JPG ) ? "Compressing" : "Decompressing"; break;
				case A_SPLIT_DUMP: actionmsg = "Splitting"; break;
				case A_COLL_DUMP:  actionmsg = "Extracting Colls"; break;
				case A_FCOLL_DUMP: actionmsg = "Extracting FColls"; break;
				case A_ZDST_DUMP:  actionmsg = "Extracting ZDST lists"; break;
				case A_TXT_INFO:   actionmsg = "Extracting info"; break;
				case A_DIST_INFO:  actionmsg = "Extracting distributions"; break;
				case A_PGM_DUMP:   actionmsg = "Converting"; break;
				default:           actionmsg = "Processing"; break;
			}
		}

		if ( cli.verbosity < 2 )
			std::fprintf( cli.msgout, "%s -> ", actionmsg );
	}
	else {
		std::fprintf( cli.msgout, "Processing file %2i of %2i ", file_no + 1, file_cnt );
		progress_bar( cli.msgout, file_no, file_cnt );
		std::fprintf( cli.msgout, "\r" );
		execute_stage( ctx, StageId::CheckFile, [&]() {
			return check_file( cli, ctx, item, pipe_on, opened_output_file, path_guard, false );
		}, &reporter );
	}
	std::fflush( cli.msgout );

	const auto t_begin = std::chrono::steady_clock::now();

	std::unique_ptr<Reader> str_str;
	std::unique_ptr<Writer> str_primary_out;
	const std::string output_base = pipe_on
		? item.path
		: make_output_base( item.path, cli.out_dir, item.rel_root );
	process_cli_file( cli, ctx, output_base, str_str, str_primary_out, &reporter );
	finalize_and_cleanup_streams( pipe_on, opened_output_file, ctx, str_str, str_primary_out );

	const auto t_end = std::chrono::steady_clock::now();
	const auto ms    = std::chrono::duration_cast<std::chrono::milliseconds>( t_end - t_begin ).count();
	const int total  = ( ms > std::numeric_limits<int>::max() ) ? std::numeric_limits<int>::max() : static_cast<int>( ms );
	const std::int64_t bpms64 = ( total > 0 ) ? ( ctx.jpgfilesize / total ) : ctx.jpgfilesize;
	const int bpms   = ( bpms64 > std::numeric_limits<int>::max() ) ? std::numeric_limits<int>::max() : static_cast<int>( bpms64 );
	const float cr   = ( ctx.jpgfilesize > 0 ) ? static_cast<float>( 100.0 * static_cast<double>( ctx.pjgfilesize ) / static_cast<double>( ctx.jpgfilesize ) ) : 0.0f;

	if ( cli.verbosity >= 0 ) {
		if ( cli.verbosity > 1 )
			std::fprintf( cli.msgout, "\n----------------------------------------" );

		switch ( cli.verbosity ) {
			case 0:
				if ( ctx.errorlevel < ctx.err_tol ) {
					if ( ctx.action == A_COMPRESS ) std::fprintf( cli.msgout, "%.2f%%", cr );
					else std::fprintf( cli.msgout, "DONE" );
				}
				else {
					std::fprintf( cli.msgout, "ERROR" );
				}
				if ( ctx.errorlevel > 0 ) std::fprintf( cli.msgout, "\n" );
				break;

			case 1:
				std::fprintf( cli.msgout, "%s\n", ( ctx.errorlevel < ctx.err_tol ) ? "DONE" : "ERROR" );
				break;

			case 2:
				if ( ctx.errorlevel < ctx.err_tol ) std::fprintf( cli.msgout, "\n-> %s OK\n", actionmsg );
				else std::fprintf( cli.msgout, "\n-> %s ERROR\n", actionmsg );
				break;
		}

		switch ( ctx.errorlevel ) {
			case 0:  errtypemsg = "none"; break;
			case 1:  errtypemsg = ( ctx.err_tol > 1 ) ? "warning (ignored)" : "warning (skipped file)"; break;
			case 2:
			default: errtypemsg = "fatal error"; break;
		}

		if ( ctx.errorlevel > 0 ) {
			std::fprintf( cli.msgout, " %s -> %s:\n", stage_status_text( ctx.error_stage ), errtypemsg );
			std::fprintf( cli.msgout, " %s\n", ctx.errormessage.c_str() );
		}
		if ( ( cli.verbosity > 0 ) && ( ctx.errorlevel < ctx.err_tol ) && ( ctx.action == A_COMPRESS ) ) {
			if ( total >= 0 ) {
				std::fprintf( cli.msgout, " time taken  : %7i msec\n", total );
				std::fprintf( cli.msgout, " byte per ms : %7i byte\n", bpms );
			}
			else {
				std::fprintf( cli.msgout, " time taken  : %7s msec\n", "N/A" );
				std::fprintf( cli.msgout, " byte per ms : %7s byte\n", "N/A" );
			}
			std::fprintf( cli.msgout, " comp. ratio : %7.2f %%\n", cr );
		}
		if ( ( cli.verbosity > 1 ) && ( ctx.action == A_COMPRESS ) )
			std::fprintf( cli.msgout, "\n" );
	}
	else {
		if ( file_no + 1 == file_cnt ) {
			std::fprintf( cli.msgout, "Processed %2i of %2i files ", file_no + 1, file_cnt );
			progress_bar( cli.msgout, 1, 1 );
			std::fprintf( cli.msgout, "\r" );
		}
	}
}

void run_parallel_batch(
	CliSettings& cli,
	const CodecContext& template_ctx,
	BatchPathGuard& path_guard,
	int& error_cnt,
	int& warn_cnt,
	double& acc_jpgsize,
	double& acc_pjgsize )
{
	const int file_cnt   = static_cast<int>( cli.inputs.size() );
	const int worker_cnt = std::min( cli.jobs, file_cnt );

	std::vector<FileOutcome> outcomes( static_cast<std::size_t>( file_cnt ) );
	std::atomic<int> next_idx{ 0 };
	int completed_cnt = 0;
	std::mutex io_mtx;

	if ( cli.verbosity == -1 ) {
		std::fprintf( cli.msgout, "Processing file %2i of %2i ", 1, file_cnt );
		progress_bar( cli.msgout, 0, file_cnt );
		std::fprintf( cli.msgout, "\r" );
		std::fflush( cli.msgout );
	}

	auto worker_fn = [&]() {
		CodecContext wctx;
		while ( true ) {
			const int idx = next_idx.fetch_add( 1, std::memory_order_relaxed );
			if ( idx >= file_cnt ) {
				break;
			}

			wctx.reset_buffers();
			wctx.err_tol      = template_ctx.err_tol;
			wctx.disc_meta    = template_ctx.disc_meta;
			wctx.auto_set     = template_ctx.auto_set;
			wctx.nois_trs     = template_ctx.nois_trs;
			wctx.segm_cnt     = template_ctx.segm_cnt;
			wctx.action       = A_COMPRESS;
			wctx.error_stage  = StageId::None;
			wctx.errorlevel   = 0;
			wctx.jpgfilesize  = 0;
			wctx.pjgfilesize  = 0;
			wctx.errormessage = "no errormessage specified";

			const CliInputItem& item = cli.inputs[ static_cast<std::size_t>( idx ) ];
			const auto t_begin = std::chrono::steady_clock::now();
			bool opened_output_file = false;

			execute_stage( wctx, StageId::CheckFile, [&]() {
				return check_file( cli, wctx, item, false, opened_output_file, path_guard, true );
			} );

			std::unique_ptr<Reader> str_str;
			std::unique_ptr<Writer> str_primary_out;
			const std::string output_base = make_output_base( item.path, cli.out_dir, item.rel_root );
			process_cli_file( cli, wctx, output_base, str_str, str_primary_out, nullptr );
			finalize_and_cleanup_streams( false, opened_output_file, wctx, str_str, str_primary_out );

			const auto t_end = std::chrono::steady_clock::now();
			const auto ms    = std::chrono::duration_cast<std::chrono::milliseconds>( t_end - t_begin ).count();
			const int elapsed = ( ms > std::numeric_limits<int>::max() ) ? std::numeric_limits<int>::max() : static_cast<int>( ms );

			FileOutcome& out = outcomes[ static_cast<std::size_t>( idx ) ];
			out.errorlevel   = wctx.errorlevel;
			out.error_stage  = wctx.error_stage;
			out.errormessage = wctx.errormessage;
			out.filetype     = wctx.filetype;
			out.jpgfilesize  = wctx.jpgfilesize;
			out.pjgfilesize  = wctx.pjgfilesize;
			out.elapsed_ms   = elapsed;

			{
				std::lock_guard<std::mutex> lock( io_mtx );
				completed_cnt++;
				if ( cli.verbosity == -1 ) {
					if ( completed_cnt < file_cnt ) {
						std::fprintf( cli.msgout, "Processing file %2i of %2i ", completed_cnt + 1, file_cnt );
						progress_bar( cli.msgout, completed_cnt, file_cnt );
					}
					else {
						std::fprintf( cli.msgout, "Processed %2i of %2i files ", file_cnt, file_cnt );
						progress_bar( cli.msgout, 1, 1 );
					}
					std::fprintf( cli.msgout, "\r" );
				}
				else {
					const char* act = ( out.filetype == F_UNK )
						? "unknown filetype"
						: ( ( out.filetype == F_JPG ) ? "Compressing" : "Decompressing" );
					const float cr = ( out.jpgfilesize > 0 )
						? static_cast<float>( 100.0 * static_cast<double>( out.pjgfilesize ) / static_cast<double>( out.jpgfilesize ) )
						: 0.0f;
					std::fprintf( cli.msgout, "\nProcessing file %i of %i \"%s\" -> %s -> ",
						idx + 1, file_cnt, item.path.c_str(), act );
					if ( out.errorlevel < wctx.err_tol ) {
						if ( cli.verbosity == 0 ) {
							std::fprintf( cli.msgout, "%.2f%%", cr );
							if ( out.errorlevel > 0 ) std::fprintf( cli.msgout, "\n" );
						}
						else {
							std::fprintf( cli.msgout, "DONE\n" );
						}
					}
					else {
						std::fprintf( cli.msgout, "ERROR\n" );
					}
					if ( out.errorlevel > 0 ) {
						const char* errtypemsg = ( out.errorlevel == 1 )
							? ( ( wctx.err_tol > 1 ) ? "warning (ignored)" : "warning (skipped file)" )
							: "fatal error";
						std::fprintf( cli.msgout, " %s -> %s:\n %s\n",
							stage_status_text( out.error_stage ), errtypemsg, out.errormessage.c_str() );
					}
					if ( ( cli.verbosity > 0 ) && ( out.errorlevel < wctx.err_tol ) ) {
						const std::int64_t bpms64 = ( out.elapsed_ms > 0 ) ? ( out.jpgfilesize / out.elapsed_ms ) : out.jpgfilesize;
						const int bpms = ( bpms64 > std::numeric_limits<int>::max() ) ? std::numeric_limits<int>::max() : static_cast<int>( bpms64 );
						std::fprintf( cli.msgout, " time taken  : %7i msec\n", out.elapsed_ms );
						std::fprintf( cli.msgout, " byte per ms : %7i byte\n", bpms );
						std::fprintf( cli.msgout, " comp. ratio : %7.2f %%\n", cr );
					}
				}
				std::fflush( cli.msgout );
			}
		}
	};

	std::vector<std::thread> workers;
	workers.reserve( static_cast<std::size_t>( worker_cnt ) );
	for ( int t = 0; t < worker_cnt; t++ ) {
		workers.emplace_back( worker_fn );
	}
	for ( auto& th : workers ) {
		th.join();
	}

	for ( int i = 0; i < file_cnt; i++ ) {
		const FileOutcome& out = outcomes[ static_cast<std::size_t>( i ) ];
		if ( out.errorlevel > 0 ) {
			cli.err_tp[ static_cast<std::size_t>( i ) ]   = out.errorlevel;
			cli.err_list[ static_cast<std::size_t>( i ) ] = out.errormessage;
		}
		if ( out.errorlevel >= template_ctx.err_tol ) {
			error_cnt++;
		}
		else {
			if ( out.errorlevel == 1 ) warn_cnt++;
			acc_jpgsize += static_cast<double>( out.jpgfilesize );
			acc_pjgsize += static_cast<double>( out.pjgfilesize );
		}
	}
}

} // namespace


int main( int argc, char** argv )
{
	CliSettings  cli;
	CodecContext ctx;

	int error_cnt = 0;
	int warn_cnt  = 0;

	double acc_jpgsize = 0;
	double acc_pjgsize = 0;

	initialize_options( argc, argv, cli, ctx );
	const int file_cnt = static_cast<int>( cli.inputs.size() );

	std::fprintf( cli.msgout, "\n--> %s v%i.%i%s (%s) by %s <--\n",
		kAppTitle, kAppVersion / 10, kAppVersion % 10, kSubVersion, kVersionDate, kAuthor );
	std::fprintf( cli.msgout, "Copyright %s\nAll rights reserved\n\n", kCopyright );

	if ( !cli.arg_error.empty() ) {
		std::fprintf( stderr, "Error: %s\n", cli.arg_error.c_str() );
		show_help( cli );
		return 1;
	}

	if ( !cli.out_dir.empty() ) {
		std::error_code ec;
		for ( std::filesystem::path probe( cli.out_dir ); !probe.empty(); ) {
			if ( std::filesystem::exists( probe, ec ) ) {
				if ( !std::filesystem::is_directory( probe, ec ) ) {
					std::fprintf( stderr, "Error: output path \"%s\" exists and is not a directory\n", cli.out_dir.c_str() );
					return 1;
				}
				break;
			}
			const std::filesystem::path parent = probe.parent_path();
			if ( parent == probe ) {
				break;
			}
			probe = parent;
		}
	}

	if ( ( ( file_cnt == 0 ) && !cli.had_expanded ) ||
		 ( ( !cli.developer ) && ( ( ctx.action != A_COMPRESS ) || ( !ctx.auto_set ) || ( cli.verify_lv > 1 ) ) ) ) {
		show_help( cli );
		return 1;
	}

	if ( !ctx.auto_set ) {
		std::fprintf( cli.msgout, " custom compression settings: \n" );
		std::fprintf( cli.msgout, " -------------------------------------------------\n" );
		std::fprintf( cli.msgout, " no of segments    ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
			ctx.segm_cnt[ 0 ], ctx.segm_cnt[ 1 ], ctx.segm_cnt[ 2 ], ctx.segm_cnt[ 3 ] );
		std::fprintf( cli.msgout, " noise threshold   ->  %3i[0] %3i[1] %3i[2] %3i[3]\n",
			ctx.nois_trs[ 0 ], ctx.nois_trs[ 1 ], ctx.nois_trs[ 2 ], ctx.nois_trs[ 3 ] );
		std::fprintf( cli.msgout, " -------------------------------------------------\n\n" );
	}

	ctx.reset_buffers();

	bool has_stdin_pipe = false;
	for ( const auto& item : cli.inputs ) {
		if ( item.path == "-" ) {
			has_stdin_pipe = true;
			break;
		}
	}

	BatchPathGuard path_guard( cli.inputs );

	const auto batch_begin = std::chrono::steady_clock::now();
	if ( ( cli.jobs > 1 ) && ( file_cnt > 1 ) && ( ctx.action == A_COMPRESS ) && !has_stdin_pipe && !cli.developer ) {
		run_parallel_batch( cli, ctx, path_guard, error_cnt, warn_cnt, acc_jpgsize, acc_pjgsize );
	}
	else {
		for ( int file_no = 0; file_no < file_cnt; file_no++ ) {
			process_ui( cli, ctx, file_no, path_guard );
			if ( ctx.errorlevel > 0 ) {
				cli.err_tp[ file_no ]   = ctx.errorlevel;
				cli.err_list[ file_no ] = ctx.errormessage;
			}
			if ( ctx.errorlevel >= ctx.err_tol ) {
				error_cnt++;
			}
			else {
				if ( ctx.errorlevel == 1 ) warn_cnt++;
				acc_jpgsize += static_cast<double>( ctx.jpgfilesize );
				acc_pjgsize += static_cast<double>( ctx.pjgfilesize );
			}
		}
	}

	const auto batch_end = std::chrono::steady_clock::now();

	if ( ( cli.verbosity == -1 ) || ( cli.verbosity == 2 ) ) {
		if ( error_cnt > 0 ) {
			std::fprintf( stderr, "\n\nfiles with errors:\n" );
			std::fprintf( stderr, "------------------\n" );
			for ( int file_no = 0; file_no < file_cnt; file_no++ ) {
				if ( cli.err_tp[ file_no ] >= ctx.err_tol ) {
					std::fprintf( stderr, "%s (%s)\n", cli.inputs[ file_no ].path.c_str(), cli.err_list[ file_no ].c_str() );
				}
			}
		}
		if ( warn_cnt > 0 ) {
			std::fprintf( stderr, "\n\nfiles with warnings:\n" );
			std::fprintf( stderr, "------------------\n" );
			for ( int file_no = 0; file_no < file_cnt; file_no++ ) {
				if ( cli.err_tp[ file_no ] == 1 ) {
					std::fprintf( stderr, "%s (%s)\n", cli.inputs[ file_no ].path.c_str(), cli.err_list[ file_no ].c_str() );
				}
			}
		}
	}

	std::fprintf( cli.msgout, "\n\n-> %i file(s) processed, %i error(s), %i warning(s)\n",
		file_cnt, error_cnt, warn_cnt );
	if ( ( file_cnt > error_cnt ) && ( cli.verbosity != 0 ) && ( ctx.action == A_COMPRESS ) ) {
		acc_jpgsize /= 1024.0;
		acc_pjgsize /= 1024.0;
		const double total = std::chrono::duration<double>( batch_end - batch_begin ).count();
		const int    kbps  = static_cast<int>( ( total > 0.0 ) ? ( acc_jpgsize / total ) : acc_jpgsize );
		const double cr    = ( acc_jpgsize > 0.0 ) ? ( 100.0 * acc_pjgsize / acc_jpgsize ) : 0.0;

		std::fprintf( cli.msgout, " --------------------------------- \n" );
		if ( total >= 0.0 ) {
			std::fprintf( cli.msgout, " total time        : %8.2f sec\n", total );
			std::fprintf( cli.msgout, " avrg. kbyte per s : %8i byte\n", kbps );
		}
		else {
			std::fprintf( cli.msgout, " total time        : %8s sec\n", "N/A" );
			std::fprintf( cli.msgout, " avrg. kbyte per s : %8s byte\n", "N/A" );
		}
		std::fprintf( cli.msgout, " avrg. comp. ratio : %8.2f %%\n", cr );
		std::fprintf( cli.msgout, " --------------------------------- \n" );
	}

	if ( cli.wait_exit && ( cli.msgout != stderr ) ) {
		std::fprintf( cli.msgout, "\n\n< press ENTER >\n" );
		std::fgetc( stdin );
	}

	return ( error_cnt > 0 ) ? 1 : 0;
}

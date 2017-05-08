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

#include <chrono>
#include <cstdio>
#include <experimental/filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "programinfo.h"
#include "fileprocessor.h"

struct ProgramOptions {
	FILE* info_stream = stdout; // stream for output of messages
	bool verbose = false; // Print high-level progress in program?
	bool overwrite = false; // Overwrite existing file(s) with output?
	bool wait_once_finished = false; // Wait for user input after finishing.
	bool verify_reversible = false; // Check to make sure this program can losslessly reverse its own output.
	std::vector<std::string> filelist; // The files to process.

	ProgramOptions(int argc, char** argv) {
		// read in arguments
		while (--argc > 0) {
			argv++;
			std::string arg = *argv;
			if (arg == "-verify" || arg == "-ver") {
				verify_reversible = true;
			} else if (arg == "-verbose" || arg == "-v2" || arg == "-v1") {
				verbose = true;
			} else if (arg == "-w") {
				wait_once_finished = true;
			} else if (arg == "-o") {
				overwrite = true;
			} else if (arg == "-") {
				info_stream = stderr;
				filelist.push_back("-"); // use "-" as a placeholder for stdin
			} else if (std::experimental::filesystem::exists(arg)) {
				filelist.push_back(arg);
			} else {
				fprintf(stderr, "Invalid option/file: %s", arg.c_str());
			}
		}
	}
	
	void show_help() const {
		fprintf(info_stream, "\n");
		fprintf(info_stream, "Website: %s\n", program_info::website.c_str());
		fprintf(info_stream, "Email  : %s\n", program_info::email.c_str());
		fprintf(info_stream, "\n");
		fprintf(info_stream, "Usage: %s [switches] [filename(s)]", program_info::appname.c_str());
		fprintf(info_stream, "\n");
		fprintf(info_stream, "\n");
		fprintf(info_stream, " [-ver]   verify files after processing\n");
		fprintf(info_stream, " [-v?]    set level of verbosity (max: 2) (def: 0)\n");
		fprintf(info_stream, " [-w]		wait after processing files\n");
		fprintf(info_stream, " [-o]     overwrite existing files\n");
		fprintf(info_stream, "\n");
		fprintf(info_stream, "Examples: \"%s -v1 -o baboon.%s\"\n", program_info::appname.c_str(), program_info::pjg_ext.c_str());
		fprintf(info_stream, "          \"%s -p *.%s\"\n", program_info::appname.c_str(), program_info::jpg_ext.c_str());
	}
};

int main(int argc, char** argv) {
	const auto options = std::make_unique<ProgramOptions>(argc, argv);

	program_info::display_program_info(options->info_stream);

	if (options->filelist.empty()) {
		options->show_help();
		return -1;
	}

	const auto begin = std::chrono::steady_clock::now();
	std::size_t cumulative_jpg_size = 0;
	std::size_t cumulative_pjg_size = 0;
	std::size_t errors = 0;
	for (const auto& filename : options->filelist) {
		std::unique_ptr<FileProcessor> processor;
		try {
			if (filename == "-") {
				processor = std::make_unique<FileProcessor>(options->verify_reversible, options->verbose);
			} else {
				processor = std::make_unique<FileProcessor>(filename, options->overwrite, options->verify_reversible, options->verbose);
			}
			processor->execute();
			cumulative_jpg_size += processor->get_jpg_size();
			cumulative_pjg_size += processor->get_pjg_size();
		} catch (const std::runtime_error& e) {
			const auto name = filename == "-" ? "stdin" : filename;
			fprintf(options->info_stream, "\nError processing %s: %s\n", name.c_str(), e.what());
			errors++;
		}
	}
	const auto end = std::chrono::steady_clock::now();

	// Summary statistics:
	const auto processed_sum_message = "\n\n-> " + std::to_string(options->filelist.size()) + " file(s) processed, " + std::to_string(errors) + " error(s)\n";
	fprintf(options->info_stream, processed_sum_message.c_str());
	if (cumulative_jpg_size > 0 && options->verbose) {
		std::chrono::duration<double> duration = end - begin;
		const auto total_time = duration.count();

		fprintf(options->info_stream, "\nSummary statistics: \n");
		if (total_time > 0) {
			fprintf(options->info_stream, "Total time       : %8.2f s\n", total_time);
			const auto kbps = (cumulative_jpg_size / 1024) / total_time;
			fprintf(options->info_stream, "Avg. kbyte per s : %8.0f KBps\n", kbps);
		} else {
			fprintf(options->info_stream, "Total time       : N/A s\n");
			fprintf(options->info_stream, "Avg. kbyte per s : N/A KBps\n");
		}
		const auto compression_ratio = 100.0 * cumulative_pjg_size / cumulative_jpg_size;
		fprintf(options->info_stream, "Avg. comp. ratio : %8.2f %%\n", compression_ratio);
	}

	if (options->wait_once_finished && options->info_stream != stderr) {
		fprintf(options->info_stream, "\n\n<Press ENTER to exit>\n");
		fgetc(stdin);
	}

	return 0;
}

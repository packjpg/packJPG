#include "programoptions.h"

#include <filesystem>

#include "programinfo.h"

std::pair<std::vector<std::string>, ProgramOptions> ProgramOptions::parse_input(int argc, char** argv) {
	ProgramOptions options;
	std::vector<std::string> files;

	// read in arguments
	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "-verify" || arg == "-ver") {
			options.verify_reversible = true;
		} else if (arg == "-verbose" || arg == "-v2" || arg == "-v1") {
			options.verbose = true;
		} else if (arg == "-w") {
			options.wait_once_finished = true;
		} else if (arg == "-o") {
			options.overwrite_existing_output = true;
		} else if (arg == "-") {
			options.info_stream = stderr;
			files.push_back("-"); // use "-" as a placeholder for stdin
		} else if (arg == "-split") {
			options.debug_options.split_dump = true;
		} else if (arg.find("-coll") == 0 && arg.size() == 6) {
			const auto digit = arg.substr(5);
			const auto digit_pos = digit.find_first_of("0123456789");
			if (digit_pos != std::string::npos) {
				options.debug_options.collmode = CollectionMode(std::clamp(std::stoi(digit), 0, 5));
				options.debug_options.coll_dump = true;
			} else {
				fprintf(stderr, "Invalid -coll argument: %s\n", arg.c_str());
			}
		} else if (arg.find("-fcol") == 0 && arg.size() == 6) {
			const auto digit = arg.substr(5);
			const auto digit_pos = digit.find_first_of("0123456789");
			if (digit_pos != std::string::npos) {
				options.debug_options.collmode = CollectionMode(std::clamp(std::stoi(digit), 0, 5));
				options.debug_options.fcoll_dump = true;
			} else {
				fprintf(stderr, "Invalid -fcol argument: %s\n", arg.c_str());
			}
		} else if (arg == "-zdst") {
			options.debug_options.zdst_dump = true;
		} else if (arg == "-info") {
			options.debug_options.txt_info = true;
		} else if (arg == "-dist") {
			options.debug_options.dist_info = true;
		} else if (arg == "-pgm") {
			options.debug_options.pgm_dump = true;
		} else if (std::filesystem::exists(arg)) {
			files.push_back(arg);
		} else {
			fprintf(stderr, "Invalid option/file: %s\n", arg.c_str());
		}
	}
	return std::make_pair(files, options);
}

void ProgramOptions::show_help() const {
	fprintf(info_stream, "\n");
	fprintf(info_stream, "Website: %s\n", program_info::website.c_str());
	fprintf(info_stream, "Email  : %s\n", program_info::email.c_str());
	fprintf(info_stream, "\n");
	fprintf(info_stream, "Usage: %s [options] [filepath(s)]", program_info::appname.c_str());
	fprintf(info_stream, "\n");
	fprintf(info_stream, "\n");
	fprintf(info_stream, " [-ver]   verify files after processing\n");
	fprintf(info_stream, " [-v?]    set level of verbosity (max: 2) (def: 0)\n");
	fprintf(info_stream, " [-w]		wait after processing files\n");
	fprintf(info_stream, " [-o]     overwrite existing files\n");
}

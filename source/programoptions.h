#ifndef PROGRAMOPTIONS_H
#define PROGRAM_OPTIONS_H

#include <cstdio>
#include <vector>

#include "debugoptions.h"

class ProgramOptions {
public:
	FILE* info_stream = stdout; // stream for output of messages
	bool verbose = false; // Print high-level progress in program?
	bool overwrite_existing_output = false; // Overwrite existing file(s) with output?
	bool wait_once_finished = false; // Wait for user input after finishing.
	bool verify_reversible = false; // Check to make sure this program can losslessly reverse its own output.
	DebugOptions debug_options;

	ProgramOptions() {}

	static std::pair<std::vector<std::string>, ProgramOptions> parse_input(int argc, char** argv);

	void show_help() const;
};

#endif
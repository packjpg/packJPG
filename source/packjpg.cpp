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
	bool overwrite_existing_output = false; // Overwrite existing file(s) with output?
	bool wait_once_finished = false; // Wait for user input after finishing.
	bool verify_reversible = false; // Check to make sure this program can losslessly reverse its own output.
	std::vector<std::string> files; // Paths to the file(s) to process.

	ProgramOptions(int argc, char** argv) {
		// read in arguments
		for (int i = 1; i < argc; i++) {
			std::string arg = argv[i];
			if (arg == "-verify" || arg == "-ver") {
				verify_reversible = true;
			} else if (arg == "-verbose" || arg == "-v2" || arg == "-v1") {
				verbose = true;
			} else if (arg == "-w") {
				wait_once_finished = true;
			} else if (arg == "-o") {
				overwrite_existing_output = true;
			} else if (arg == "-") {
				info_stream = stderr;
				files.push_back("-"); // use "-" as a placeholder for stdin
			} else if (std::experimental::filesystem::exists(arg)) {
				files.push_back(arg);
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
		fprintf(info_stream, "Usage: %s [options] [filepath(s)]", program_info::appname.c_str());
		fprintf(info_stream, "\n");
		fprintf(info_stream, "\n");
		fprintf(info_stream, " [-ver]   verify files after processing\n");
		fprintf(info_stream, " [-v?]    set level of verbosity (max: 2) (def: 0)\n");
		fprintf(info_stream, " [-w]		wait after processing files\n");
		fprintf(info_stream, " [-o]     overwrite existing files\n");
	}
};

int main(int argc, char** argv) {
	const auto options = std::make_unique<ProgramOptions>(argc, argv);

	program_info::display_program_info(options->info_stream);

	if (options->files.empty()) {
		options->show_help();
		return -1;
	}

	const auto begin = std::chrono::steady_clock::now();
	std::size_t cumulative_jpg_size = 0;
	std::size_t cumulative_pjg_size = 0;
	std::size_t errors = 0;
	for (const auto& file_path : options->files) {
		std::unique_ptr<FileProcessor> processor;
		try {
			if (file_path == "-") {
				processor = std::make_unique<FileProcessor>(options->verify_reversible, options->verbose);
			} else {
				processor = std::make_unique<FileProcessor>(file_path, options->overwrite_existing_output, options->verify_reversible, options->verbose);
			}
			processor->execute();
			cumulative_jpg_size += processor->get_jpg_size();
			cumulative_pjg_size += processor->get_pjg_size();
		} catch (const std::runtime_error& e) {
			const auto name = file_path == "-" ? "stdin" : file_path;
			fprintf(options->info_stream, "\nError processing %s: %s\n", name.c_str(), e.what());
			bool successfully_deleted = processor->delete_output();
			if (successfully_deleted) {
				fprintf(options->info_stream, "Unable to delete the output of processing %s\n", name.c_str());
			}
			errors++;
		}
	}
	const auto end = std::chrono::steady_clock::now();

	// Summary statistics:
	const auto processed_sum_message = "\n\n-> " + std::to_string(options->files.size()) + " file(s) processed, " + std::to_string(errors) + " error(s)\n";
	fprintf(options->info_stream, processed_sum_message.c_str());
	if (cumulative_jpg_size > 0 && options->verbose) {
		std::chrono::duration<double> program_duration = end - begin;
		const auto total_time = program_duration.count();

		fprintf(options->info_stream, "\nSummary statistics: \n");
		if (total_time > 0) {
			fprintf(options->info_stream, "Total time       : %8.2f s\n", total_time);
			const auto kbps = (cumulative_jpg_size / 1024) / total_time;
			fprintf(options->info_stream, "Avg. kbyte per s : %8.0f KBps\n", kbps);
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

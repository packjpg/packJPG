#include <chrono>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "fileprocessor.h"
#include "programinfo.h"

int main(int argc, char** argv) {
	const auto parsed_input = ProgramOptions::parse_input(argc, argv);
	const auto files = parsed_input.first;
	const auto options = parsed_input.second;

	program_info::display_program_info(options.info_stream);

	if (files.empty()) {
		options.show_help();
		return -1;
	}

	const auto begin = std::chrono::steady_clock::now();
	std::size_t cumulative_jpg_size = 0;
	std::size_t cumulative_pjg_size = 0;
	std::size_t errors = 0;
	for (const auto& file_path : files) {
		std::unique_ptr<FileProcessor> processor;
		try {
			if (file_path == "-") {
				processor = std::make_unique<FileProcessor>(options);
			} else {
				processor = std::make_unique<FileProcessor>(file_path, options);
			}
			processor->execute();
			cumulative_jpg_size += processor->get_jpg_size();
			cumulative_pjg_size += processor->get_pjg_size();
		} catch (const std::runtime_error& e) {
			const auto name = file_path == "-" ? "stdin" : file_path;
			fprintf(options.info_stream, "\nError processing %s: %s\n", name.c_str(), e.what());
			bool deleted_output = processor->delete_output();
			if (!deleted_output) {
				fprintf(options.info_stream, "Unable to delete the output of processing %s\n", name.c_str());
			}
			errors++;
		}
	}
	const auto end = std::chrono::steady_clock::now();

	// Summary statistics:
	const auto processed_sum_message = "\n\n-> " + std::to_string(files.size()) + " file(s) processed, " + std::to_string(errors) + " error(s)\n";
	fprintf(options.info_stream, processed_sum_message.c_str());
	if (cumulative_jpg_size > 0 && options.verbose) {
		std::chrono::duration<double> program_duration = end - begin;
		const auto total_time = program_duration.count();

		fprintf(options.info_stream, "\nSummary statistics: \n");
		if (total_time > 0) {
			fprintf(options.info_stream, "Total time       : %8.2f s\n", total_time);
			const auto kbps = (static_cast<double>(cumulative_jpg_size) / 1024) / total_time;
			fprintf(options.info_stream, "Avg. kbyte per s : %8.0f KBps\n", kbps);
		}
		const auto compression_ratio = 100.0 * cumulative_pjg_size / cumulative_jpg_size;
		fprintf(options.info_stream, "Avg. comp. ratio : %8.2f %%\n", compression_ratio);
	}

	if (options.wait_once_finished && options.info_stream != stderr) {
		fprintf(options.info_stream, "\n\n<Press ENTER to exit>\n");
		fgetc(stdin);
	}

	return 0;
}

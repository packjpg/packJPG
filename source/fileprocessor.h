#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <string>
#include "reader.h"
#include "writer.h"
#include "filetype.h"

class FileProcessor {
public:
	/**
	 * Process the given file.
	 */
	FileProcessor(const std::string& input_file, bool verify, bool verbose);
	/*
	 * Process from standard input (stdin). Writes to stdout for data and stderr for logs.
	 */
	FileProcessor(bool verify, bool verbose);

	void execute();

private:
	FileType get_file_type();
	void verify_output(Writer& verification_output);

	bool verify_;
	bool verbose_;

	std::unique_ptr<Reader> input_;
	std::unique_ptr<Writer> output_;

	FileType file_type;
};

#endif
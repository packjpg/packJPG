#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include <string>

#include "controller.h"
#include "filetype.h"
#include "reader.h"
#include "writer.h"

class FileProcessor {
public:
	/**
	 * Process the given file.
	 */
	FileProcessor(const std::string& input_file, bool overwrite, bool verify, bool verbose);
	/*
	 * Process from standard input (stdin). Writes to stdout for data and stderr for logs.
	 */
	FileProcessor(bool verify, bool verbose);

	void execute();

private:
	/*
	 * Returns the file type, either JPG or PJG. Throws a runtime error if
	 * the file type cannot be determined or is neither JPG nor PJG.
	 */
	FileType get_file_type();
	void verify_reversible(Writer& verification_output) const;
	std::string determine_output_destination(const std::string& input_file, const std::string& new_extension) const;

	bool overwrite_ = false;
	bool verify_reversible_ = true;
	bool verbose_ = false;

	std::unique_ptr<Controller> controller_;

	std::unique_ptr<Reader> input_;
	std::unique_ptr<Writer> output_;

	FileType file_type_;
};

#endif
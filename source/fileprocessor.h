#ifndef FILEPROCESSOR_H
#define FILEPROCESSOR_H

#include "controller.h"
#include "filetype.h"
#include "imagedebug.h"
#include "programoptions.h"
#include "reader.h"
#include "writer.h"

class FileProcessor {
public:
	/**
	 * Process the given file.
	 */
	FileProcessor(const std::string& input_file, ProgramOptions options);
	/*
	 * Process from standard input (stdin). Writes to stdout for data and stderr for logs.
	 */
	FileProcessor(ProgramOptions options);

	void execute();

	std::size_t get_jpg_size() const;
	std::size_t get_pjg_size() const;

	/*
	 * Attempts to delete the file created for output, returning whether the file was successfully removed from the filesystem.
	 * Does nothing and returns true if the output destination was stdout or execute() has not been run.
	 */
	bool delete_output();

private:
	/*
	 * Returns the file type, either JPG or PJG. Throws a runtime error if
	 * the file type cannot be determined or is neither JPG nor PJG.
	 */
	FileType get_file_type();
	void verify_reversible(Writer& verification_output) const;
	std::string output_destination(const std::string& input_file) const;

	bool executed_ = false; // Has execute() been run?

	const ProgramOptions options_;
	const ImageDebug debug_;

	std::unique_ptr<Controller> controller_;

	std::unique_ptr<Reader> input_reader_;
	std::unique_ptr<Writer> output_writer_;

	FileType file_type_;
	std::string output_destination_;
	bool output_dest_is_stdout_ = false;
};

#endif
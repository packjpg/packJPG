#include "fileprocessor.h"

#include <experimental/filesystem>
#include <string>

#include "programinfo.h"
#include "jpgtopjgontroller.h"
#include "pjgtojpgcontroller.h"

FileProcessor::FileProcessor(const std::string& input_file, bool overwrite, bool verify, bool verbose) : overwrite_(overwrite), verify_reversible_(verify), verbose_(verbose) {
	input_reader_ = std::make_unique<MemoryFileReader>(input_file);
	file_type_ = get_file_type();
	output_destination_ = output_destination(input_file);
	output_writer_ = std::make_unique<FileWriter>(output_destination_);

	if (file_type_ == FileType::JPG) {
		controller_ = std::make_unique<JpgToPjgController>(*input_reader_, *output_writer_);
	} else {
		controller_ = std::make_unique<PjgToJpgController>(*input_reader_, *output_writer_);
	}
}

FileProcessor::FileProcessor(bool verify, bool verbose) : verify_reversible_(verify), verbose_(verbose) {
	input_reader_ = std::make_unique<StreamReader>();
	file_type_ = get_file_type();
	output_writer_ = std::make_unique<StreamWriter>();
	output_dest_is_stdout_ = true;

	if (file_type_ == FileType::JPG) {
		controller_ = std::make_unique<JpgToPjgController>(*input_reader_, *output_writer_);
	} else {
		controller_ = std::make_unique<PjgToJpgController>(*input_reader_, *output_writer_);
	}
}

void FileProcessor::execute() {
	if (executed_) {
		throw std::runtime_error("Tried to execute the file processor again.");
	} else {
		executed_ = true;
	}

	controller_->execute();

	if (!verify_reversible_) {
		return;
	}

	auto output_as_input = std::make_unique<MemoryReader>(output_writer_->get_data());
	std::array<std::uint8_t, 2> magic_bytes{};
	output_as_input->read(magic_bytes.data(), 2);
	auto verification_output = std::make_unique<MemoryWriter>();
	std::unique_ptr<Controller> reversed_controller;
	if (file_type_ == FileType::JPG) {
		reversed_controller = std::make_unique<PjgToJpgController>(*output_as_input, *verification_output);
	} else {
		reversed_controller = std::make_unique<JpgToPjgController>(*output_as_input, *verification_output);
	}

	reversed_controller->execute();
	verify_reversible(*verification_output);
}

std::size_t FileProcessor::get_jpg_size() const {
	if (file_type_ == FileType::JPG) {
		return input_reader_->get_size();
	} else {
		return output_writer_->num_bytes_written();
	}
}

std::size_t FileProcessor::get_pjg_size() const {
	if (file_type_ == FileType::JPG) {
		return output_writer_->num_bytes_written();
	} else {
		return input_reader_->get_size();
	}
}

bool FileProcessor::delete_output() {
	if (!output_dest_is_stdout_) {
		executed_ = true;
		output_writer_.reset(nullptr); // Close stream associated with destination file.
		try {
			return std::experimental::filesystem::remove(output_destination_);
		} catch (const std::exception&) {
			return false;
		}
	} else {
		return true;
	}
}

FileType FileProcessor::get_file_type() {
	std::array<std::uint8_t, 2> magic_bytes{};
	if (input_reader_->read(magic_bytes.data(), 2) != 2) {
		throw std::runtime_error("Not enough data to determine file type");
	}

	constexpr std::array<std::uint8_t, 2> jpg_magic_bytes{0xFF, 0xD8};
	auto is_jpg = std::equal(std::begin(magic_bytes),
	                         std::end(magic_bytes),
	                         std::begin(jpg_magic_bytes),
	                         std::end(jpg_magic_bytes));

	auto is_pjg = std::equal(std::begin(magic_bytes),
	                         std::end(magic_bytes),
	                         std::begin(program_info::pjg_magic),
	                         std::end(program_info::pjg_magic));

	if (is_jpg) {
		return FileType::JPG;
	} else if (is_pjg) {
		return FileType::PJG;
	} else {
		throw std::runtime_error("Unknown file type.");
	}
}

void FileProcessor::verify_reversible(Writer& verification_output) const {
	const auto& input_data = input_reader_->get_data();
	const auto& verification_data = verification_output.get_data();
	if (input_data.size() != verification_data.size()) {
		throw std::runtime_error("Expected (input) file size: " + std::to_string(input_data.size())
			+ ", Verification file size: " + std::to_string(verification_data.size()));
	}

	const auto result = std::mismatch(std::begin(input_data),
	                                  std::end(input_data),
	                                  std::begin(verification_data),
	                                  std::end(verification_data));
	if (result.first != std::end(input_data) || result.second != std::end(verification_data)) {
		const auto first_diff = std::distance(std::begin(input_data), result.first);
		throw std::runtime_error("First difference between expected (input) and verification file found at byte position "
			+ std::to_string(first_diff));
	}
}

std::string FileProcessor::output_destination(const std::string& input_file) const {
	const auto new_extension = file_type_ == FileType::JPG ? program_info::pjg_ext : program_info::jpg_ext;
	auto filename_base = input_file.substr(0, input_file.find_last_of("."));
	auto filename = filename_base + "." + new_extension;
	while (std::experimental::filesystem::exists(filename) && !overwrite_) {
		filename_base += "_";
		filename = filename_base + "." + new_extension;
	}
	return filename;
}

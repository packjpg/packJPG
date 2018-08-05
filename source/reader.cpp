#include "reader.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

MemoryReader::MemoryReader(const std::vector<std::uint8_t>& bytes) :
	data_(bytes),
	cbyte_(std::begin(data_)) {
}

std::size_t MemoryReader::read(std::uint8_t* to, std::size_t num_to_read) {
	if (num_to_read == 0 || to == nullptr) {
		return 0;
	}
	auto numAvailable = std::distance(cbyte_, std::end(data_));
	auto numRead = std::min(static_cast<std::size_t>(numAvailable), num_to_read);
	auto end = std::next(cbyte_, numRead);
	std::copy(cbyte_, end, to);
	cbyte_ = end;
	return numRead;
}

std::size_t MemoryReader::read(std::vector<std::uint8_t>& into, std::size_t n, std::size_t offset) {
	const std::size_t num_available = get_size() - num_bytes_read(); // The number of bytes in the reader not yet read.
	const std::size_t num_to_read = std::min(n, num_available); // How many bytes will be read.
	if (into.size() < num_to_read + offset) {
		into.resize(num_to_read + offset);
	}

	const auto end = std::next(cbyte_, num_to_read);
	const auto write_start = std::next(std::begin(into), offset);
	std::copy(cbyte_, end, write_start);
	cbyte_ = end;
	return num_to_read;
}

std::uint8_t MemoryReader::read_byte() {
	if (end_of_reader()) {
		throw std::runtime_error("No bytes left to read");
	} else {
		std::uint8_t the_byte = *cbyte_;
		++cbyte_;
		return the_byte;
	}
}

bool MemoryReader::read_byte(std::uint8_t* byte) {
	if (end_of_reader()) {
		return false;
	} else {
		*byte = *cbyte_;
		++cbyte_;
		return true;
	}
}

void MemoryReader::skip(std::size_t n) {
	auto num_to_skip = std::min(n, std::size_t(std::distance(cbyte_, std::end(data_))));
	cbyte_ += num_to_skip;
}

void MemoryReader::rewind_bytes(std::size_t n) {
	auto num_to_rewind = std::min(n, std::size_t(std::distance(std::begin(data_), cbyte_)));
	auto new_pos = std::distance(std::begin(data_), cbyte_) - num_to_rewind;
	cbyte_ = std::next(std::begin(data_), new_pos);
}

void MemoryReader::rewind() {
	cbyte_ = std::begin(data_);
}

std::size_t MemoryReader::num_bytes_read() {
	return std::distance(std::begin(data_), cbyte_);
}

std::size_t MemoryReader::get_size() {
	return data_.size();
}

std::vector<std::uint8_t> MemoryReader::get_data() {
	return data_;
}

bool MemoryReader::error() {
	return false;
}

bool MemoryReader::end_of_reader() {
	return cbyte_ == std::end(data_);
}

FileReader::FileReader(const std::string& file_path) {
	if (std::ifstream is{ file_path, std::ios::binary | std::ios::ate }) {
		const auto size = is.tellg();
		std::vector<std::uint8_t> data(size);
		is.seekg(0);
		if (is.read(reinterpret_cast<char*>(data.data()), size)) {
			reader_ = std::make_unique<MemoryReader>(data);
		} else {
			throw std::runtime_error("MemoryFileReader: unable to read bytes from file.");
		}
	} else {
		throw std::runtime_error("MemoryFileReadera: unable to open read stream for file.");
	}
}

FileReader::~FileReader() {}

std::size_t FileReader::read(std::uint8_t* to, std::size_t num_to_read) {
	return reader_->read(to, num_to_read);
}

std::size_t FileReader::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return reader_->read(into, num_to_read, offset);
}

std::uint8_t FileReader::read_byte() {
	return reader_->read_byte();
}

bool FileReader::read_byte(std::uint8_t* to) {
	return reader_->read_byte(to);
}

void FileReader::skip(std::size_t n) {
	return reader_->skip(n);
}

void FileReader::rewind_bytes(std::size_t n) {
	return reader_->rewind_bytes(n);
}

void FileReader::rewind() {
	reader_->rewind();
}

std::size_t FileReader::num_bytes_read() {
	return reader_->num_bytes_read();
}

std::size_t FileReader::get_size() {
	return reader_->get_size();
}

std::vector<std::uint8_t> FileReader::get_data() {
	return reader_->get_data();
}

bool FileReader::error() {
	return reader_->error();
}

bool FileReader::end_of_reader() {
	return reader_->end_of_reader();
}

StreamReader::StreamReader() {
#if defined(_WIN32) || defined(WIN32)
	const int result = _setmode(_fileno(stdin), _O_BINARY);
	if (result == -1) {
		throw std::runtime_error("Unable to set mode for stdin");
	}
#endif
	// read whole stream into memory buffer
	std::vector<std::uint8_t> stream_data;
	constexpr auto buffer_capacity = 1024 * 1024;
	std::vector<std::uint8_t> buffer(buffer_capacity);

	auto bytes_read = std::fread(buffer.data(), sizeof buffer[0], buffer_capacity, stdin);
	while (bytes_read > 0) {
		stream_data.insert(std::end(stream_data), std::begin(buffer), std::begin(buffer) + bytes_read);
		bytes_read = std::fread(buffer.data(), sizeof buffer[0], buffer_capacity, stdin);
	}

	reader_ = std::make_unique<MemoryReader>(stream_data);
}

std::size_t StreamReader::read(std::uint8_t* to, std::size_t num_to_read) {
	return reader_->read(to, num_to_read);
}

std::size_t StreamReader::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return reader_->read(into, num_to_read, offset);
}

std::uint8_t StreamReader::read_byte() {
	return reader_->read_byte();
}

bool StreamReader::read_byte(std::uint8_t* to) {
	return reader_->read_byte(to);
}

void StreamReader::skip(std::size_t n) {
	reader_->skip(n);
}

void StreamReader::rewind_bytes(std::size_t n) {
	reader_->rewind_bytes(n);
}

void StreamReader::rewind() {
	reader_->rewind();
}

std::size_t StreamReader::num_bytes_read() {
	return reader_->num_bytes_read();
}

std::size_t StreamReader::get_size() {
	return reader_->get_size();
}

std::vector<std::uint8_t> StreamReader::get_data() {
	return reader_->get_data();
}

bool StreamReader::error() {
	return reader_->error();
}

bool StreamReader::end_of_reader() {
	return reader_->end_of_reader();
}

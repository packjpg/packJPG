#include "reader.h"

#include <algorithm>
#include <cstdio>
#include <experimental/filesystem>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

#include "writer.h"

FileReader::FileReader(const std::string& file_path) : file_path_(file_path) {
	fptr_ = fopen(file_path.c_str(), "rb");
	if (fptr_ != nullptr) {
		file_buffer_.reserve(32768);
		std::setvbuf(fptr_, file_buffer_.data(), _IOFBF, file_buffer_.capacity());
	} else {
		throw std::runtime_error("Unable to open " + file_path);
	}
}

FileReader::~FileReader() {
	if (fptr_ != nullptr) {
		fclose(fptr_);
	}
}

std::size_t FileReader::read(std::uint8_t* to, std::size_t num_to_read) {
	return fread(to, sizeof to[0], num_to_read, fptr_);
}

std::size_t FileReader::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return read(into.data() + offset, num_to_read);
}

std::uint8_t FileReader::read_byte() {
	const int val = fgetc(fptr_);
	if (val != EOF) {
		return static_cast<std::uint8_t>(val);
	} else {
		throw std::runtime_error("No bytes left in " + file_path_ + " to read!");
	}
}

bool FileReader::read_byte(std::uint8_t* to) {
	const int val = fgetc(fptr_);
	*to = val;
	return val != EOF;
}

void FileReader::rewind() {
	fseek(fptr_, 0, SEEK_SET);
}

std::size_t FileReader::num_bytes_read() {
	return ftell(fptr_);
}

std::size_t FileReader::get_size() {
	return std::experimental::filesystem::file_size(file_path_);
}

std::vector<std::uint8_t> FileReader::get_data() {
	auto position = num_bytes_read();
	std::vector<std::uint8_t> data_copy(get_size());
	fseek(fptr_, 0, SEEK_SET);
	fread(data_copy.data(), sizeof data_copy[0], data_copy.capacity(), fptr_);
	fseek(fptr_, position, SEEK_SET);
	return data_copy;
}

bool FileReader::error() {
	return fptr_ == nullptr || ferror(fptr_) != 0;
}

bool FileReader::end_of_reader() {
	return feof(fptr_) != 0;
}

MemoryReader::MemoryReader(const std::vector<std::uint8_t>& bytes) :
	data_(bytes),
	cbyte_(std::begin(data_)),
	eof_(bytes.empty()) {
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
	eof_ = cbyte_ == std::end(data_);
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
	eof_ = cbyte_ == std::end(data_);
	return num_to_read;
}

std::uint8_t MemoryReader::read_byte() {
	if (cbyte_ == std::end(data_)) {
		throw std::runtime_error("No bytes left to read");
	} else {
		std::uint8_t the_byte = *cbyte_;
		++cbyte_;
		eof_ = cbyte_ == std::end(data_);
		return the_byte;
	}
}

bool MemoryReader::read_byte(std::uint8_t* byte) {
	if (cbyte_ == std::end(data_)) {
		eof_ = true;
		return false;
	} else {
		*byte = *cbyte_;
		++cbyte_;
		eof_ = cbyte_ == std::end(data_);
		return true;
	}
}

void MemoryReader::rewind() {
	cbyte_ = std::begin(data_);
	eof_ = cbyte_ == std::end(data_);
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
	return eof_;
}

StreamReader::StreamReader() {
#if defined(_WIN32) || defined(WIN32)
	_setmode(_fileno(stdin), _O_BINARY);
#endif
	// read whole stream into memory buffer
	auto writer = std::make_unique<MemoryWriter>();
	constexpr auto buffer_capacity = 1024 * 1024;
	std::vector<std::uint8_t> buffer(buffer_capacity);

	auto bytes_read = fread(buffer.data(), sizeof buffer[0], buffer_capacity, stdin);
	while (bytes_read > 0) {
		writer->write(buffer.data(), bytes_read);
		bytes_read = fread(buffer.data(), sizeof buffer[0], buffer_capacity, stdin);
	}
	const auto bytes = writer->get_data();

	reader_ = std::make_unique<MemoryReader>(bytes);
}

std::size_t StreamReader::read(std::uint8_t* to, std::size_t num_to_read) {
	return reader_->read(to, num_to_read);
}

std::size_t StreamReader::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return reader_->read(into, num_to_read, offset);
}

std::uint8_t StreamReader::read_byte() {
	try {
		return reader_->read_byte();
	} catch (const std::exception&) {
		throw;
	}
}

bool StreamReader::read_byte(std::uint8_t* to) {
	return reader_->read_byte(to);
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

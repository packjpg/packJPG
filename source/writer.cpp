#include "writer.h"

#include <experimental/filesystem>
#include <fstream>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

std::size_t Writer::write_str(const std::string& s) {
	std::size_t count = 0;
	for (const auto c : s) {
		bool char_write_successful = this->write_byte(c);
		if (char_write_successful) {
			count++;
		}
	}
	return count;
	//return this->write(reinterpret_cast<const std::uint8_t*>(s.c_str()), s.size());
}

FileWriter::FileWriter(const std::string& file_path) : file_path_(file_path) {
	fptr_ = std::fopen(file_path.c_str(), "wb");
	if (fptr_ != nullptr) {
		file_buffer_.reserve(32768);
		std::setvbuf(fptr_, file_buffer_.data(), _IOFBF, file_buffer_.capacity());
	} else {
		throw std::runtime_error("Unable to open " + file_path_ + " for writing.");
	}

}

FileWriter::~FileWriter() {
	if (fptr_ != nullptr) {
		std::fflush(fptr_);
		std::fclose(fptr_);
	}
}

std::size_t FileWriter::write(const std::uint8_t* from, std::size_t n) {
	return std::fwrite(from, sizeof from[0], n, fptr_);
}

std::size_t FileWriter::write(const std::vector<std::uint8_t>& bytes) {
	return write(bytes.data(), bytes.size());
}

std::size_t FileWriter::write(const std::array<std::uint8_t, 2>& bytes) {
	return write(bytes.data(), 2);
}

bool FileWriter::write_byte(std::uint8_t byte) {
	return std::fputc(byte, fptr_) == byte;
}

std::vector<std::uint8_t> FileWriter::get_data() {
	std::fflush(fptr_);
	if (std::ifstream is{ file_path_, std::ios::binary | std::ios::ate }) {
		const auto size = is.tellg();
		std::vector<std::uint8_t> data_copy(size);
		is.seekg(0);
		if (is.read(reinterpret_cast<char*>(data_copy.data()), size)) {
			return data_copy;
		} else {
			throw std::runtime_error("FileWriter::get_data: unable to read bytes from file.");
		}
	} else {
		throw std::runtime_error("FileWriter::get_data: unable to open read stream for file.");
	}
}

void FileWriter::rewind() {
	std::fseek(fptr_, 0, SEEK_SET);
}

std::size_t FileWriter::num_bytes_written() {
	std::fflush(fptr_);
	return std::experimental::filesystem::file_size(file_path_);
}

bool FileWriter::error() {
	return fptr_ == nullptr || std::ferror(fptr_);
}

MemoryWriter::MemoryWriter() {}

std::size_t MemoryWriter::write(const std::uint8_t* from, std::size_t n) {
	data_.insert(std::end(data_), from, from + n);
	return n;
}

std::size_t MemoryWriter::write(const std::vector<std::uint8_t>& bytes) {
	data_.insert(std::end(data_), std::begin(bytes), std::end(bytes));
	return bytes.size();
}

std::size_t MemoryWriter::write(const std::array<std::uint8_t, 2>& bytes) {
	data_.insert(std::end(data_), std::begin(bytes), std::end(bytes));
	return bytes.size();
}

bool MemoryWriter::write_byte(std::uint8_t byte) {
	data_.emplace_back(byte);
	return true;
}

std::vector<std::uint8_t> MemoryWriter::get_data() {
	return data_;
}

void MemoryWriter::rewind() {
	data_.resize(0);
}

std::size_t MemoryWriter::num_bytes_written() {
	return data_.size();
}

bool MemoryWriter::error() {
	return false;
}

StreamWriter::StreamWriter() {
	writer_ = std::make_unique<MemoryWriter>();
}

StreamWriter::~StreamWriter() {
#if defined(_WIN32) || defined(WIN32)
	const int result = _setmode(_fileno(stdout), _O_BINARY);
	if (result == -1) {
		return;
	}
#endif
	const auto& data = writer_->get_data();
	fwrite(data.data(), sizeof data[0], data.size(), stdout);
}

std::size_t StreamWriter::write(const std::uint8_t* from, std::size_t n) {
	return writer_->write(from, n);
}

std::size_t StreamWriter::write(const std::vector<std::uint8_t>& bytes) {
	return writer_->write(bytes);
}

std::size_t StreamWriter::write(const std::array<std::uint8_t, 2>& bytes) {
	return writer_->write(bytes);
}

bool StreamWriter::write_byte(std::uint8_t byte) {
	return writer_->write_byte(byte);
}

std::vector<std::uint8_t> StreamWriter::get_data() {
	return writer_->get_data();
}

void StreamWriter::rewind() {
	writer_->rewind();
}

std::size_t StreamWriter::num_bytes_written() {
	return writer_->num_bytes_written();
}

bool StreamWriter::error() {
	return writer_->error();
}

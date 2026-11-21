/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <utility>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif


/* -----------------------------------------------
	reads n bits from BitReader (boundary/long fallback)
	----------------------------------------------- */

unsigned int BitReader::read_slow( int nbits ) {
	unsigned int retval = 0;

	if ( eof() ) {
		peof_ += nbits;
		return 0;
	}

	while ( nbits >= cbit ) {
		nbits -= cbit;
		retval |= ( rbits_u8( data[ cbyte ], cbit ) << nbits );
		update_curr_byte();
		if ( eof() ) {
			peof_ = nbits;
			return retval;
		}
	}

	if ( nbits > 0 ) {
		retval |= mbits_u8( data[ cbyte ], cbit, cbit - nbits );
		cbit -= nbits;
	}

	return retval;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

int BitReader::unpad( int fillbit ) {
	if ( ( cbit == 8 ) || eof() ) {
		return fillbit;
	} else {
		fillbit = static_cast<int>( read_bit() );
		while ( cbit != 8 ) {
			if ( static_cast<int>( read_bit() ) != fillbit ) {
				fillbit = 2;
			}
		}
	}

	return fillbit;
}

/* -----------------------------------------------
	set byte and bit position
	----------------------------------------------- */

void BitReader::setpos( int pbyte, int pbit ) {
	if ( pbyte < lbyte ) {
		eof_ = false;
		cbyte = pbyte;
		cbit = pbit;
	} else {
		eof_ = true;
		cbyte = lbyte;
		cbit = 8;
		peof_ = ( ( pbyte - lbyte ) * 8 ) + 8 - pbit;
	}
}

/* -----------------------------------------------
	rewind n bits
	----------------------------------------------- */

void BitReader::rewind_bits( int nbits ) {
	if ( eof() ) {
		if ( nbits > peof_ ) {
			nbits -= peof_;
			peof_ = 0;
		} else {
			peof_ -= nbits;
			return;
		}
		eof_ = false;
	}

	cbit += nbits;
	cbyte -= cbit / 8;
	cbit = cbit % 8;
	if ( cbyte < 0 ) {
		cbyte = 0;
		cbit = 8;
	}
}

void BitWriter::pad() {
	while (curr_bit_ < 8) {
		write_bit(padbit_);
	}
}

std::vector<std::uint8_t> BitWriter::get_bytes() {
	pad();
	return bytes_;
}

std::vector<std::uint8_t> BitWriter::take_bytes() {
	pad();
	return std::move(bytes_);
}

unsigned char* BitWriter::get_c_bytes() {
	pad();
	auto c_bytes = static_cast<unsigned char*>(std::malloc(bytes_.size() * sizeof bytes_[0]));
	if (c_bytes == nullptr) {
		return nullptr;
	}
	std::copy(std::begin(bytes_), std::end(bytes_), c_bytes);
	return c_bytes;
}

unsigned char* Reader::get_c_data() {
	const auto data = this->get_data();
	auto c_data_copy = static_cast<unsigned char*>(std::malloc(data.size() * sizeof data[0]));
	if (c_data_copy == nullptr) {
		return nullptr;
	}

	std::copy(std::begin(data), std::end(data), c_data_copy);
	return c_data_copy;
}

MemoryReader::MemoryReader(const std::vector<std::uint8_t>& bytes) :
	data_(bytes),
	cbyte_(std::begin(data_)) {
}

MemoryReader::MemoryReader(std::vector<std::uint8_t>&& bytes) :
	data_(std::move(bytes)),
	cbyte_(std::begin(data_)) {
}

MemoryReader::MemoryReader(const std::uint8_t* bytes, std::size_t size) :
	data_((bytes != nullptr && size > 0) ? std::vector<std::uint8_t>(bytes, bytes + size) : std::vector<std::uint8_t>{}),
	cbyte_(std::begin(data_)) {
	if (bytes == nullptr && size > 0) {
		throw std::runtime_error("MemoryReader: null pointer with non-zero size.");
	}
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
	const std::size_t num_available = get_size() - num_bytes_read();
	const std::size_t num_to_read = std::min(n, num_available);
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

const std::uint8_t* MemoryReader::peek_contiguous(std::size_t& avail) const {
	const auto rem = static_cast<std::size_t>(std::distance(cbyte_, std::end(data_)));
	avail = rem;
	if (rem == 0) {
		return nullptr;
	}
	const auto off = static_cast<std::size_t>(std::distance(std::begin(data_), cbyte_));
	return data_.data() + off;
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

unsigned char* Writer::get_c_data() {
	try {
		const auto data = this->get_data();
		auto c_data_copy = static_cast<unsigned char*>(std::malloc(data.size() * sizeof data[0]));
		if (c_data_copy == nullptr) {
			return nullptr;
		}

		std::copy(std::begin(data), std::end(data), c_data_copy);
		return c_data_copy;
	} catch (const std::exception&) {
		return nullptr;
	}
}

std::vector<std::uint8_t> MemoryWriter::get_data() {
	return data_;
}

std::vector<std::uint8_t> MemoryWriter::take_data() {
	return std::move(data_);
}

void MemoryWriter::reset() {
	data_.resize(0);
}

std::size_t MemoryWriter::num_bytes_written() {
	return data_.size();
}

bool MemoryWriter::error() {
	return false;
}

FileWriter::FileWriter(const std::string& file_path) : file_path_(file_path) {
	fptr_ = std::fopen(file_path.c_str(), "wb");
	if (fptr_ != nullptr) {
		file_buffer_.resize(32768);
		std::setvbuf(fptr_, file_buffer_.data(), _IOFBF, file_buffer_.size());
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
		if (size < 0) {
			throw std::runtime_error("FileWriter::get_data: invalid file size.");
		}
		std::vector<std::uint8_t> data_copy(static_cast<std::size_t>(size));
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

void FileWriter::reset() {
	if (fptr_ != nullptr) {
		std::fseek(fptr_, 0, SEEK_SET);
	}
}

bool FileWriter::flush() {
	if (fptr_ == nullptr) {
		return false;
	}
	return std::fflush(fptr_) == 0 && !std::ferror(fptr_);
}

std::size_t FileWriter::num_bytes_written() {
	if (fptr_ != nullptr) {
		std::fflush(fptr_);
	}
	if (std::ifstream is{ file_path_, std::ios::binary | std::ios::ate }) {
		const auto size = is.tellg();
		if (size >= 0) {
			return static_cast<std::size_t>(size);
		}
	}
	const long pos = (fptr_ != nullptr) ? std::ftell(fptr_) : -1L;
	return (pos >= 0) ? static_cast<std::size_t>(pos) : 0;
}

bool FileWriter::error() {
	return fptr_ == nullptr || std::ferror(fptr_);
}

StreamWriter::StreamWriter(FILE* stream) : stream_(stream) {
	if (stream_ == nullptr) {
		throw std::runtime_error("StreamWriter: null FILE* stream.");
	}
	writer_ = std::make_unique<MemoryWriter>();
}

StreamWriter::~StreamWriter() {
	if (!flushed_) {
		flush();
	}
}

std::size_t StreamWriter::write(const std::uint8_t* from, std::size_t n) {
	flushed_ = false;
	return writer_->write(from, n);
}

std::size_t StreamWriter::write(const std::vector<std::uint8_t>& bytes) {
	flushed_ = false;
	return writer_->write(bytes);
}

std::size_t StreamWriter::write(const std::array<std::uint8_t, 2>& bytes) {
	flushed_ = false;
	return writer_->write(bytes);
}

bool StreamWriter::write_byte(std::uint8_t byte) {
	flushed_ = false;
	return writer_->write_byte(byte);
}

std::vector<std::uint8_t> StreamWriter::get_data() {
	return writer_->get_data();
}

void StreamWriter::reset() {
	writer_->reset();
	flushed_ = true;
}

bool StreamWriter::flush() {
	if (flushed_) {
		return !error_;
	}
	if (stream_ == nullptr) {
		error_ = true;
		return false;
	}
#if defined(_WIN32) || defined(WIN32)
	if (stream_ == stdout) {
		const int result = _setmode(_fileno(stdout), _O_BINARY);
		if (result == -1) {
			error_ = true;
			return false;
		}
	}
#endif
	const auto data = writer_->get_data();
	if (!data.empty()) {
		const std::size_t written = std::fwrite(data.data(), sizeof data[0], data.size(), stream_);
		if (written != data.size()) {
			error_ = true;
		}
	}
	if (std::fflush(stream_) != 0 || std::ferror(stream_)) {
		error_ = true;
	}
	flushed_ = true;
	return !error_;
}

std::size_t StreamWriter::num_bytes_written() {
	return writer_->num_bytes_written();
}

bool StreamWriter::error() {
	return error_ || writer_->error() || (stream_ == nullptr) || std::ferror(stream_);
}

FileReader::FileReader(const std::string& file_path) {
	std::error_code ec;
	if (std::filesystem::is_directory(file_path, ec)) {
		throw std::runtime_error("FileReader: path is a directory: " + file_path);
	}
	if (std::ifstream is{ file_path, std::ios::binary | std::ios::ate }) {
		const auto size = is.tellg();
		if (size < 0) {
			throw std::runtime_error("FileReader: unable to determine size of " + file_path);
		}
		std::vector<std::uint8_t> data(static_cast<std::size_t>(size));
		is.seekg(0);
		if (size == 0 || is.read(reinterpret_cast<char*>(data.data()), size)) {
			reader_ = std::make_unique<MemoryReader>(std::move(data));
		} else {
			throw std::runtime_error("FileReader: unable to read bytes from " + file_path);
		}
	} else {
		throw std::runtime_error("FileReader: unable to open read stream for " + file_path);
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

const std::uint8_t* FileReader::peek_contiguous(std::size_t& avail) const {
	return reader_->peek_contiguous(avail);
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

StreamReader::StreamReader(FILE* stream) {
	if (stream == nullptr) {
		throw std::runtime_error("StreamReader: null FILE* stream.");
	}
#if defined(_WIN32) || defined(WIN32)
	if (stream == stdin) {
		const int result = _setmode(_fileno(stdin), _O_BINARY);
		if (result == -1) {
			throw std::runtime_error("Unable to set mode for stdin");
		}
	}
#endif
	std::vector<std::uint8_t> stream_data;
	constexpr auto buffer_capacity = 1024 * 1024;
	std::vector<std::uint8_t> buffer(buffer_capacity);

	auto bytes_read = std::fread(buffer.data(), sizeof buffer[0], buffer_capacity, stream);
	while (bytes_read > 0) {
		stream_data.insert(std::end(stream_data), std::begin(buffer), std::begin(buffer) + bytes_read);
		bytes_read = std::fread(buffer.data(), sizeof buffer[0], buffer_capacity, stream);
	}
	if (std::ferror(stream)) {
		throw std::runtime_error("StreamReader: error reading from stream.");
	}

	reader_ = std::make_unique<MemoryReader>(std::move(stream_data));
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

const std::uint8_t* StreamReader::peek_contiguous(std::size_t& avail) const {
	return reader_->peek_contiguous(avail);
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

/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <experimental/filesystem>
#include <fstream>
#include <stdexcept>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif


/* -----------------------------------------------
	constructor for BitReader class
	----------------------------------------------- */	

BitReader::BitReader( unsigned char* array, int size ) {
	data = array;
	lbyte = size;	
}

/* -----------------------------------------------
	destructor for BitReader class
	----------------------------------------------- */	

BitReader::~BitReader() {}

/* -----------------------------------------------
	reads n bits from BitReader
	----------------------------------------------- */	

unsigned int BitReader::read( int nbits ) {
	unsigned int retval = 0;
	
	if ( eof()) {
		peof_ += nbits;
		return 0;
	}
	
	while ( nbits >= cbit ) {
		nbits -= cbit;
		retval |= ( RBITS( data[cbyte], cbit ) << nbits );		
        update_curr_byte();
        if (eof()) {
            peof_ = nbits;
            return retval;
        }
	}
	
	if ( nbits > 0 ) {		
		retval |= ( MBITS( data[cbyte], cbit, (cbit-nbits) ) );
		cbit -= nbits;		
	}
	
	return retval;
}

/* -----------------------------------------------
	reads one bit from BitReader
	----------------------------------------------- */	
	
unsigned char BitReader::read_bit() {
	if (eof()) {
		peof_++;
		return 0;
	}
	
	// read one bit
	unsigned char bit = BITN( data[cbyte], --cbit );
	if ( cbit == 0 ) {
        update_curr_byte();
	} 
	
	return bit;
}

void BitReader::update_curr_byte() {
    cbyte++;
    eof_ = cbyte == lbyte;
    cbit = 8;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

unsigned char BitReader::unpad( unsigned char fillbit ) {
	if ( ( cbit == 8 ) || eof()) {
        return fillbit;
    } else {
		fillbit = read( 1 );
		while ( cbit != 8 ) {
            read( 1 );
        }
	}
	
	return fillbit;
}

/* -----------------------------------------------
	get current position in array
	----------------------------------------------- */	

int BitReader::getpos() {
	return cbyte;
}

/* -----------------------------------------------
	get current bit position
	----------------------------------------------- */
	
int BitReader::getbitp() {
	return cbit;
}

/* -----------------------------------------------
	set byte and bit position
	----------------------------------------------- */
	
void BitReader::setpos( int pbyte, int pbit ) {
	if ( pbyte < lbyte ) {
		// reset eof
		eof_ = false;
		// set positions
		cbyte = pbyte;
		cbit = pbit;
	} else {
		// set eof
		eof_ = true;
		// set positions
		cbyte = lbyte;
		cbit = 8;
		peof_ = ( ( pbyte - lbyte ) * 8 ) + 8 - pbit;
	}	
}

/* -----------------------------------------------
	rewind n bits
	----------------------------------------------- */
	
void BitReader::rewind_bits( int nbits ) {
	if (eof()) {
		if (nbits > peof_) {
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

bool BitReader::eof() {
	return eof_;
}

int BitReader::peof() {
	return peof_;
}

BitWriter::BitWriter(std::uint8_t padbit) : padbit_(padbit) {}

BitWriter::~BitWriter() {}

std::uint32_t rbits32(std::uint32_t val, std::size_t n) {
    return val & (0xFFFFFFFF >> (32 - n));
}

std::uint32_t mbits32(std::uint32_t val, std::size_t l, std::size_t r) {
    return rbits32(val, l) >> r;
}

void BitWriter::write_u16(std::uint16_t val, std::size_t num_bits) {
	while (num_bits >= curr_bit_) {
		curr_byte_ |= mbits32(val, num_bits, num_bits - curr_bit_);
		num_bits -= curr_bit_;
		write_curr_byte();
	}

	if (num_bits > 0) {
		curr_byte_ |= rbits32(val, num_bits) << (curr_bit_ - num_bits);
		curr_bit_ -= num_bits;
	}
}

void BitWriter::write_bit(std::uint8_t bit) {
	curr_bit_--;
	curr_byte_ |= bit << curr_bit_;
	if (curr_bit_ == 0) {
		write_curr_byte();
	}
}

void BitWriter::write_curr_byte() {
	bytes_.emplace_back(curr_byte_);
	curr_byte_ = 0;
	curr_bit_ = 8;
}

void BitWriter::pad() {
	while (curr_bit_ < 8) {
		write_bit(padbit_);
	}
}

std::vector<std::uint8_t> BitWriter::get_bytes() {
	pad(); // Pad the last bits of the current byte before returning the written bytes.
	return bytes_;
}

unsigned char* BitWriter::get_c_bytes() {
    pad(); // Pad the last bits of the current byte before returning the written bytes.
    unsigned char* c_bytes = new unsigned char[bytes_.size()];
    std::copy(std::begin(bytes_), std::end(bytes_), c_bytes);
    return c_bytes;
}

std::size_t BitWriter::num_bytes_written() const {
	return bytes_.size();
}

unsigned char* Reader::get_c_data() {
    const auto data = this->get_data();
    auto c_data_copy = (unsigned char*)std::malloc(data.size() * sizeof data[0]);
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

MemoryReader::MemoryReader(const std::uint8_t* bytes, std::size_t size) :
    data_(bytes, bytes + size),
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

unsigned char* Writer::get_c_data() {
    try {
        const auto data = this->get_data();
        auto c_data_copy = (unsigned char*)std::malloc(data.size() * sizeof data[0]);
        if (c_data_copy == nullptr) {
            return nullptr;
        }

        std::copy(std::begin(data), std::end(data), c_data_copy);
        return c_data_copy;
    } catch (const std::exception&) {
        return nullptr;
    }
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

void FileWriter::reset() {
	std::fseek(fptr_, 0, SEEK_SET);
}

std::size_t FileWriter::num_bytes_written() {
	std::fflush(fptr_);
	return std::experimental::filesystem::file_size(file_path_);
}

bool FileWriter::error() {
	return fptr_ == nullptr || std::ferror(fptr_);
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

void StreamWriter::reset() {
	writer_->reset();
}

std::size_t StreamWriter::num_bytes_written() {
	return writer_->num_bytes_written();
}

bool StreamWriter::error() {
	return writer_->error();
}



FileReader::FileReader(const std::string& file_path) {
	if (std::ifstream is{ file_path, std::ios::binary | std::ios::ate }) {
		const auto size = is.tellg();
		std::vector<std::uint8_t> data(size);
		is.seekg(0);
		if (is.read(reinterpret_cast<char*>(data.data()), size)) {
			reader_ = std::make_unique<MemoryReader>(data);
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

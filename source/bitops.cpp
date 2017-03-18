/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <vector>

#if defined(_WIN32) || defined(WIN32)
#include <fcntl.h>
#include <io.h>
#endif

/* -----------------------------------------------
	constructor for abitreader class
	----------------------------------------------- */

abitreader::abitreader(const std::vector<std::uint8_t>& bits) : data(bits) {
	eof_ = data.empty();
}

/* -----------------------------------------------
	destructor for abitreader class
	----------------------------------------------- */

abitreader::~abitreader() {}

/* -----------------------------------------------
	reads n bits from abitreader
	----------------------------------------------- */

unsigned int abitreader::read(int nbits) {
	unsigned int retval = 0;

	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	while (nbits >= cbit) {
		nbits -= cbit;
		retval |= (RBITS( data[cbyte], cbit ) << nbits);
		cbit = 8;
		cbyte++;
		if (cbyte >= data.size()) {
			overread_ = cbyte > data.size();
			eof_ = true;
			return retval;
		}
	}

	if (nbits > 0) {
		retval |= (MBITS( data[cbyte], cbit, (cbit-nbits) ));
		cbit -= nbits;
	}

	return retval;
}

/* -----------------------------------------------
	reads one bit from abitreader
	----------------------------------------------- */

unsigned char abitreader::read_bit() {
	unsigned char bit;

	// safety check for eof
	if (eof()) {
		overread_ = true;
		return 0;
	}

	// read one bit
	bit = BITN( data[cbyte], --cbit );
	if (cbit == 0) {
		cbyte++;
		if (cbyte == data.size()) {
			eof_ = true;
		}
		cbit = 8;
	}

	return bit;
}

/* -----------------------------------------------
	to skip padding from current byte
	----------------------------------------------- */

unsigned char abitreader::unpad(unsigned char fillbit) {
	if ((cbit == 8) || eof()) {
		return fillbit;
	} else {
		fillbit = read(1);
		while (cbit != 8)
			read(1);
	}

	return fillbit;
}

bool abitreader::eof() {
	return eof_;
}

bool abitreader::overread() {
	return overread_;
}


/* -----------------------------------------------
	constructor for abitwriter class
	----------------------------------------------- */

abitwriter::abitwriter(int size) : data(std::max(size, 65536)) {}

/* -----------------------------------------------
	destructor for abitwriter class
	----------------------------------------------- */

abitwriter::~abitwriter() {}

/* -----------------------------------------------
	writes n bits to abitwriter
	----------------------------------------------- */

void abitwriter::write(unsigned int val, int nbits) {
	// safety check for error
	if (nbits < 0) {
		return;
	}

	// test if pointer beyond flush treshold
	if (cbyte > (data.size() - 5)) {
		data.resize(data.size() * 2);
	}

	// write data
	while (nbits >= cbit) {
		data[cbyte] |= (MBITS32(val, nbits, (nbits-cbit)));
		nbits -= cbit;
		cbyte++;
		cbit = 8;
	}

	if (nbits > 0) {
		data[cbyte] |= ((RBITS32(val, nbits)) << (cbit - nbits));
		cbit -= nbits;
	}
}

/* -----------------------------------------------
	writes one bit to abitwriter
	----------------------------------------------- */

void abitwriter::write_bit(unsigned char bit) {

	// write data
	if (bit) {
		data[cbyte] |= 0x1 << (--cbit);
	} else {
		--cbit;
	}
	if (cbit == 0) {
		// test if pointer beyond flush treshold
		cbyte++;
		if (cbyte > (data.size() - 5)) {
			data.resize(data.size() * 2);
		}
		cbit = 8;
	}
}

/* -----------------------------------------------
	Sets the fillbit for padding data.
   ----------------------------------------------- */
void abitwriter::set_fillbit(unsigned char fillbit) {
	fillbit_ = fillbit;
}


/* -----------------------------------------------
	pads data using fillbit
	----------------------------------------------- */

void abitwriter::pad() {
	while (cbit < 8) {
		write(fillbit_, 1);
	}
}

/* -----------------------------------------------
	gets data array from abitwriter
	----------------------------------------------- */

std::vector<std::uint8_t> abitwriter::get_data() {
	pad(); // Pad the last bits of the data before returning it.
	data.resize(cbyte);
	return data;
}

/* -----------------------------------------------
	gets size of data array from abitwriter
	----------------------------------------------- */

int abitwriter::getpos() {
	return cbyte;
}


/* -----------------------------------------------
	constructor for abytewriter class
	----------------------------------------------- */

abytereader::abytereader(const std::vector<std::uint8_t>& bytes) : data(bytes) {
	cbyte = 0;
	_eof = bytes.empty();
}

/* -----------------------------------------------
	destructor for abytewriter class
	----------------------------------------------- */

abytereader::~abytereader() {}

/* -----------------------------------------------
	reads 1 byte from abytereader
	----------------------------------------------- */

int abytereader::read(unsigned char* byte) {
	if (cbyte >= data.size()) {
		cbyte = data.size();
		_eof = true;
		return 0;
	} else {
		*byte = data[cbyte];
		cbyte++;
		_eof = cbyte >= data.size();
		return 1;
	}
}

/* -----------------------------------------------
	reads n bytes from abytereader
	----------------------------------------------- */
	
int abytereader::read_n( unsigned char* byte, int n )
{
	if (n <= 0 || byte == nullptr) {
		return 0;
	}
	int numAvailable = data.size() - cbyte;
	int numRead = std::min(numAvailable, n);
	auto start = std::next(std::begin(data), cbyte);
	auto end = std::next(std::begin(data), cbyte + numRead);
	std::copy(start, end, byte);
	cbyte += numRead;
	_eof = cbyte >= data.size();
	return numRead;
}

std::size_t abytereader::read(std::vector<std::uint8_t>& into, std::size_t n, std::size_t offset) {
	const size_t num_available = getsize() - getpos(); // The number of bytes in the reader not yet read.
	const size_t num_to_read = std::min(n, num_available); // How many bytes will be read.
	if (into.size() < num_to_read + offset) {
		into.resize(num_to_read + offset);
	}

	const auto start = std::next(std::begin(data), getpos());
	const auto end = std::next(std::begin(data), getpos() + num_to_read);

	const auto write_start = std::next(std::begin(into), offset);

	std::copy(start, end, write_start);
	cbyte += num_to_read;
	return num_to_read;
}

/* -----------------------------------------------
	go to position in data
	----------------------------------------------- */
	
void abytereader::seek( int pos )
{
	int newPos = std::max(pos, 0);
	cbyte = std::min(newPos, int(data.size()));
	_eof = cbyte >= data.size();
}

/* -----------------------------------------------
	gets size of current data
	----------------------------------------------- */
	
int abytereader::getsize()
{
	return data.size();
}

/* -----------------------------------------------
	gets current position from abytereader
	----------------------------------------------- */	

int abytereader::getpos()
{
	return cbyte;
}

bool abytereader::eof()
{
	return _eof;
}

std::vector<std::uint8_t> abytereader::get_data() {
	return data;
}


/* -----------------------------------------------
	constructor for abytewriter class
	----------------------------------------------- */	

abytewriter::abytewriter(int size) : data(std::max(size, 65536)) {}

/* -----------------------------------------------
	destructor for abytewriter class
	----------------------------------------------- */	

abytewriter::~abytewriter() {}

/* -----------------------------------------------
	writes 1 byte to abytewriter
	----------------------------------------------- */

void abytewriter::write(unsigned char byte) {
	// test if pointer beyond flush threshold
	if (cbyte == data.size()) {
		data.resize(data.size() * 2);
	}

	// write data
	data[cbyte] = byte;
	cbyte++;
}

/* -----------------------------------------------
	writes n byte to abytewriter
	----------------------------------------------- */

void abytewriter::write_n(const unsigned char* byte, int n) {
	// safety check for error
	if (n <= 0) {
		return;
	}

	// make sure that pointer doesn't get beyond flush threshold
	while (cbyte + n >= data.size()) {
		data.resize(data.size() * 2);
	}

	std::copy(byte, byte + n, std::next(std::begin(data), cbyte));
	cbyte += n;
}

/* -----------------------------------------------
	gets data array from abytewriter
	----------------------------------------------- */

std::vector<std::uint8_t> abytewriter::get_data()
{
	// realloc data
	data.resize(cbyte);
	
	return data;
}

/* -----------------------------------------------
	gets size of data array from abytewriter
	----------------------------------------------- */	

int abytewriter::getpos()
{
	return cbyte;
}

/* -----------------------------------------------
	reset without realloc
	----------------------------------------------- */	
	
void abytewriter::reset()
{
	// set position of current byte
	cbyte = 0;
}

MemStream::MemStream(const std::vector<std::uint8_t>& bytes, StreamMode mode) {
	is_stream = false;
	io_mode = mode;
	if (mode == StreamMode::kRead)
		mrdr = std::make_unique<abytereader>(bytes);
	else
		mwrt = std::make_unique<abytewriter>(bytes.size());
}

MemStream::MemStream(StreamMode mode) {
#if defined(_WIN32) || defined(WIN32)
	_setmode(_fileno(stdin), _O_BINARY);
	_setmode(_fileno(stdout), _O_BINARY);
#endif

	std::vector<std::uint8_t> bytes;
	if (mode == StreamMode::kRead) {
		// read whole stream into memory buffer
		auto strwrt = std::make_unique<abytewriter>(0);
		constexpr int buffer_capacity = 1024 * 1024;
		std::vector<unsigned char> buffer(buffer_capacity);

		int bytesRead = fread(buffer.data(), sizeof(buffer[0]), buffer_capacity, stdin);
		while (bytesRead > 0) {
			strwrt->write_n(buffer.data(), bytesRead);
			bytesRead = fread(buffer.data(), sizeof(buffer[0]), buffer_capacity, stdin);
		}
		bytes = strwrt->get_data();
	}

	MemStream(bytes, mode);
	is_stream = true;
}

/* -----------------------------------------------
	destructor for iostream class
	----------------------------------------------- */

MemStream::~MemStream() {
	// if needed, write memory to stream or free memory from buffered stream
	if (is_stream) {
		if (io_mode == StreamMode::kWrite) {
			const auto& data = mwrt->get_data();
			fwrite(data.data(), sizeof(std::uint8_t), data.size(), stdout);
		}
	}
}

/* -----------------------------------------------
	switches mode from reading to writing and vice versa
	----------------------------------------------- */
	
void MemStream::switch_mode()
{	
	// return immediately if there's an error
	if ( chkerr() ) return;
	
	
	if ( io_mode == StreamMode::kRead) {
		// WARNING: when switching from reading to writing, information might be lost forever
		mwrt = std::make_unique<abytewriter>(mrdr->getsize());
		mrdr.reset();
		io_mode = StreamMode::kWrite;
	}
	else {
		mrdr = std::make_unique<abytereader>(mwrt->get_data());
		mwrt.reset();
		io_mode = StreamMode::kRead;
	}
}


/* -----------------------------------------------
	rewind to beginning of stream
	----------------------------------------------- */
int MemStream::rewind() {
	if (io_mode == StreamMode::kRead)
		mrdr->seek(0);
	else
		mwrt->reset();

	return getpos();
}

/* -----------------------------------------------
	get current position in stream
	----------------------------------------------- */
int MemStream::getpos()
{
	int pos;
	
	if ( io_mode == StreamMode::kRead )
		pos = mrdr->getpos();
	else
		pos = mwrt->getpos();

	return pos;
}

/* -----------------------------------------------
	get size of file
	----------------------------------------------- */
int MemStream::getsize()
{
	int siz;

	if (io_mode == StreamMode::kRead) {
		siz = mrdr->getsize();
	} else {
		siz = getpos();
	}

	return siz;
}

std::vector<std::uint8_t> MemStream::get_data()
{
	return (io_mode == StreamMode::kRead) ? mrdr->get_data() : mwrt->get_data();
}

/* -----------------------------------------------
	check for errors
	----------------------------------------------- */	
bool MemStream::chkerr() {
	return false;
}

/* -----------------------------------------------
	check for eof (read only)
	----------------------------------------------- */	
bool MemStream::chkeof()
{
	return false;
}

/* -----------------------------------------------
	write function for memory
	----------------------------------------------- */
int MemStream::write(const unsigned char* from, int dtsize )
{	
	mwrt->write_n(from, dtsize);
	
	return dtsize;
}

int MemStream::write_byte(unsigned char byte) {
	mwrt->write(byte);
	return 1;
}

/* -----------------------------------------------
	read function for memory
	----------------------------------------------- */
int MemStream::read(unsigned char* to, int dtsize)
{	
	return mrdr->read_n(to, dtsize);
}

std::size_t MemStream::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return mrdr->read(into, num_to_read, offset);
}

bool MemStream::read_byte(unsigned char* to) {
	return mrdr->read(to) == 1;
}

FileStream::FileStream(const std::string& file, StreamMode iomode) : file_path(file), io_mode(iomode) {
	// open file for reading / writing
	fptr = fopen(file_path.c_str(), (io_mode == StreamMode::kRead) ? "rb" : "wb");
	if (fptr != nullptr) {
		file_buffer.reserve(32768);
		std::setvbuf(fptr, file_buffer.data(), _IOFBF, file_buffer.capacity());
	}
}

FileStream::~FileStream() {
	if (fptr != nullptr) {
		if (io_mode == StreamMode::kWrite) {
			fflush(fptr);
		}
		fclose(fptr);
	}
}

void FileStream::switch_mode() {
	if (io_mode == StreamMode::kRead) {
		fclose(fptr);
		fptr = fopen(file_path.c_str(), "wb");
		io_mode = StreamMode::kWrite;
	} else {
		fflush(fptr);
		fclose(fptr);
		fptr = fopen(file_path.c_str(), "rb");
		io_mode = StreamMode::kRead;
	}
}

int FileStream::read(unsigned char* to, int dtsize) {
	return fread(to, sizeof(unsigned char), dtsize, fptr);
}

std::size_t FileStream::read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset) {
	return read(into.data() + offset, num_to_read);
}

bool FileStream::read_byte(unsigned char* to) {
	const int val = fgetc(fptr);
	*to = val;
	return val != EOF;
}

int FileStream::write(const unsigned char* from, int dtsize) {
	return fwrite(from, sizeof(unsigned char), dtsize, fptr);
}

int FileStream::write_byte(unsigned char byte) {
	return fputc(byte, fptr) == byte;
}

int FileStream::rewind() {
	return fseek(fptr, 0, SEEK_SET);
}

int FileStream::getpos() {
	return ftell(fptr);
}

int FileStream::getsize() {
	const int pos = ftell(fptr);
	fseek(fptr, 0, SEEK_END);
	const int size = ftell(fptr);
	fseek(fptr, pos, SEEK_SET);
	return size;
}

bool FileStream::chkerr() {
	if (fptr == nullptr) {
		return true;
	} else if (ferror(fptr)) {
		return true;
	}
	return false;
}

bool FileStream::chkeof() {
	return  feof(fptr) != 0;
}

std::vector<std::uint8_t> FileStream::get_data() {
	std::vector<uint8_t> buffer(getsize());
	read(buffer.data(), buffer.size());
	return buffer;
}

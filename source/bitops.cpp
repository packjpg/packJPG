/*
This file contains special classes for bitwise
reading and writing of arrays
*/

#include "bitops.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <vector>

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

void BitWriter::write_u16(std::uint16_t val, std::size_t num_bits) {
	while (num_bits >= curr_bit_) {
		curr_byte_ |= MBITS32(val, num_bits, num_bits - curr_bit_);
		num_bits -= curr_bit_;
		write_curr_byte();
	}

	if (num_bits > 0) {
		curr_byte_ |= RBITS32(val, num_bits) << (curr_bit_ - num_bits);
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

unsigned char* MemoryWriter::get_c_data() {
    auto c_data_copy = (unsigned char*)std::malloc(data_.size() * sizeof data_[0]);
    if (c_data_copy == nullptr) {
        return nullptr;
    }
    std::copy(std::begin(data_), std::end(data_), c_data_copy);
    return c_data_copy;
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

/* -----------------------------------------------
	constructor for iostream class
	----------------------------------------------- */

iostream::iostream( void* src, StreamType srctype, int srcsize, StreamMode iomode )
{
	// locally copy source, source type # and io mode #
	source = src;
	srct   = srctype;
	srcs   = srcsize;
	mode   = iomode;
	
	// don't free memory when reading - this will be useful if switching occurs
	free_mem_sw = false;
	
	// set binary mode for streams
	#if defined(_WIN32) || defined(WIN32)
		_setmode( _fileno( stdin ), _O_BINARY);
		_setmode( _fileno( stdout ), _O_BINARY);
	#endif
	
	// open file/mem/stream
	switch ( srct )
	{
		case StreamType::kFile:
			open_file();
			break;
		
		case StreamType::kMemory:
			open_mem();
			break;
		
		case StreamType::kStream:
			open_stream();
			break;
		
		default:			
			break;
	}
}

/* -----------------------------------------------
	destructor for iostream class
	----------------------------------------------- */

iostream::~iostream()
{
	// if needed, write memory to stream or free memory from buffered stream
	if ( srct == StreamType::kStream) {
		if ( mode == StreamMode::kWrite ) {
			if ( !(mwrt->error()) ) {
				srcs   = mwrt->num_bytes_written();
				source = mwrt->get_c_data();
				fwrite( source, sizeof( char ), srcs, stdout );
			}
		}
	}
	
	// free all buffers
	if (srct == StreamType::kFile) {
		if (fptr != nullptr) {
			if (mode == StreamMode::kWrite) fflush(fptr);
			fclose(fptr);
		}
	}
	else if (mode == StreamMode::kRead) {
		if (free_mem_sw)
			free(source);
	}
}

/* -----------------------------------------------
	switches mode from reading to writing and vice versa
	----------------------------------------------- */
	
void iostream::switch_mode()
{	
	// return immediately if there's an error
	if ( chkerr() ) return;
	
	
	if ( mode == StreamMode::kRead) {
		// WARNING: when switching from reading to writing, information might be lost forever
		switch ( srct ) {
			case StreamType::kFile:
				fclose( fptr );
				fptr = fopen( ( char* ) source, "wb" );
				break;
			case StreamType::kMemory:
			case StreamType::kStream:
				mrdr.reset();
				if ( free_mem_sw )
					free( source );
				mwrt = std::make_unique<MemoryWriter>();
				break;
			default:
				break;
		}
		mode = StreamMode::kWrite;
	}
	else {
		// switching from writing to reading is a bit more complicated
		switch ( srct ) {
			case StreamType::kFile:
				fflush( fptr );
				fclose( fptr );
				fptr = fopen( ( char* ) source, "rb" );
				break;
			case StreamType::kMemory:
			case StreamType::kStream:
				source = mwrt->get_c_data();
				srcs   = mwrt->num_bytes_written();
				mwrt.reset();
				mrdr = std::make_unique<MemoryReader>( ( unsigned char* ) source, srcs );
				free_mem_sw = true;
				break;
			default:
				break;
		}
		mode = StreamMode::kRead;
	}
}

/* -----------------------------------------------
	generic read function
	----------------------------------------------- */
	
int iostream::read(unsigned char* to, int dtsize)
{
	return ( srct == StreamType::kFile) ? read_file( to, dtsize ) : read_mem( to, dtsize );
}

bool iostream::read_byte(unsigned char* to) {
	return  srct == StreamType::kFile ? read_file_byte(to) : read_mem_byte(to);
}

/* -----------------------------------------------
	generic write function
	----------------------------------------------- */

int iostream::write(const unsigned char* from, int dtsize )
{
	return ( srct == StreamType::kFile) ? write_file( from, dtsize ) : write_mem( from, dtsize );
}

int iostream::write_byte(unsigned char byte) {
	return srct == StreamType::kFile ? write_file_byte(byte) : write_mem_byte(byte);
}

/* -----------------------------------------------
	flush function 
	----------------------------------------------- */

int iostream::flush()
{
	if ( srct == StreamType::kFile)
		fflush( fptr );
	
	return getpos();
}

/* -----------------------------------------------
	rewind to beginning of stream
	----------------------------------------------- */

int iostream::rewind()
{
	// WARNING: when writing, rewind might lose all your data
	if ( srct == StreamType::kFile)
		fseek( fptr, 0, SEEK_SET );
	else if ( mode == StreamMode::kRead )
		mrdr->rewind();
	else
		mwrt->reset();
	
	return getpos();
}

/* -----------------------------------------------
	get current position in stream
	----------------------------------------------- */

int iostream::getpos()
{
	int pos;
	
	if ( srct == StreamType::kFile)
		pos = ftell( fptr );
	else if ( mode == StreamMode::kRead )
		pos = mrdr->num_bytes_read();
	else
		pos = mwrt->num_bytes_written();

	return pos;
}

/* -----------------------------------------------
	get size of file
	----------------------------------------------- */

int iostream::getsize()
{
	int siz;
	
	if ( mode == StreamMode::kRead ) {
		if ( srct == StreamType::kFile) {
			int pos = ftell( fptr );
			fseek( fptr, 0, SEEK_END );
			siz = ftell( fptr );
			fseek( fptr, pos, SEEK_SET );
		}
		else {
			siz = mrdr->get_size();
		}
	}
	else {
		siz = getpos();
	}

	return siz;
}

/* -----------------------------------------------
	get data pointer (for mem io only)
	----------------------------------------------- */

unsigned char* iostream::getptr()
{
	if ( srct == StreamType::kMemory)
		return ( mode == StreamMode::kRead ) ? ( unsigned char* ) source : mwrt->get_c_data();
	else
		return nullptr;
}

/* -----------------------------------------------
	check for errors
	----------------------------------------------- */
	
bool iostream::chkerr()
{
	bool error = false;
	
	// check for io errors
	if ( srct == StreamType::kFile) {
		if ( fptr == nullptr )
			error = true;
		else if ( ferror( fptr ) )
			error = true;
	}
	else if ( mode == StreamMode::kRead ) {
		if ( mrdr == nullptr )			
			error = true;
	}
	else {		
		if ( mwrt == nullptr )
			error = true;
		else if ( mwrt->error() )
			error = true;
	}
	
	return error;
}

/* -----------------------------------------------
	check for eof (read only)
	----------------------------------------------- */
	
bool iostream::chkeof()
{
	if ( mode == StreamMode::kRead )
		return ( srct == StreamType::kFile) ? feof( fptr ) != 0 : mrdr->end_of_reader();
	else
		return false;
}

/* -----------------------------------------------
	open function for files
	----------------------------------------------- */

void iostream::open_file()
{
	char* fn = (char*) source;
	
	// open file for reading / writing
	fptr = fopen( fn, ( mode == StreamMode::kRead ) ? "rb" : "wb" );
	if (fptr != nullptr) {
		file_buffer.reserve(32768);
		std::setvbuf(fptr, file_buffer.data(), _IOFBF, file_buffer.capacity());
	}
}

/* -----------------------------------------------
	open function for memory
	----------------------------------------------- */

void iostream::open_mem()
{
	if ( mode == StreamMode::kRead )
		mrdr = std::make_unique<MemoryReader>( ( unsigned char* ) source, srcs );
	else
		mwrt = std::make_unique<MemoryWriter>();
}

/* -----------------------------------------------
	open function for streams
	----------------------------------------------- */

void iostream::open_stream()
{	
	
	if ( mode == StreamMode::kRead ) {
		// read whole stream into memory buffer
		auto strwrt = std::make_unique<MemoryWriter>();
		constexpr int buffer_capacity = 1024 * 1024;
        std::vector<unsigned char> buffer(buffer_capacity);

		int bytesRead = fread(buffer.data(), sizeof(buffer[0]), buffer_capacity, stdin);
		while (bytesRead > 0) {
			strwrt->write(buffer.data(), bytesRead);
			bytesRead = fread(buffer.data(), sizeof(buffer[0]), buffer_capacity, stdin);
		}
		if ( strwrt->error() ) {
			source = nullptr;
			srcs   = 0;
		}
		else {
			source = strwrt->get_c_data();
			srcs   = strwrt->num_bytes_written();
		}
		// free memory after done
		free_mem_sw = true;
	}
	
	// for writing: simply open new stream in mem writer
	// writing to stream will be done later
	open_mem();
}

/* -----------------------------------------------
	write function for files
	----------------------------------------------- */

int iostream::write_file(const unsigned char* from, int dtsize )
{
	return fwrite( from, sizeof(unsigned char), dtsize, fptr );
}

int iostream::write_file_byte(unsigned char byte) {
	return fputc(byte, fptr) == byte;
}

/* -----------------------------------------------
	read function for files
	----------------------------------------------- */

int iostream::read_file(unsigned char* to, int dtsize )
{
	return fread( to, sizeof(unsigned char), dtsize, fptr );
}

bool iostream::read_file_byte(unsigned char* to) {
	int val = fgetc(fptr);
	*to = val;
	return val != EOF;
}

/* -----------------------------------------------
	write function for memory
	----------------------------------------------- */
	
int iostream::write_mem(const unsigned char* from, int dtsize )
{	
	mwrt->write(from, dtsize);
	
	return ( mwrt->error()) ? 0 : dtsize;
}

int iostream::write_mem_byte(unsigned char byte) {
	mwrt->write_byte(byte);
	return mwrt->error() ? 0 : 1;
}

/* -----------------------------------------------
	read function for memory
	----------------------------------------------- */

int iostream::read_mem(unsigned char* to, int dtsize)
{	
	return mrdr->read(to, dtsize);
}

bool iostream::read_mem_byte(unsigned char* to) {
	return mrdr->read_byte(to);
}

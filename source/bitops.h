#ifndef BITOPS_H
#define BITOPS_H

#define RBITS( c, n )		( c & ( 0xFF >> (8 - n) ) )
#define LBITS( c, n )		( c >> (8 - n) )
#define MBITS( c, l, r )	( RBITS( c,l ) >> r )
#define BITN( c, n )		( (c >> n) & 0x1 )
#define BITLEN( l, v )		for ( l = 0; ( v >> l ) > 0; l++ )
#define FDIV2( v, p )		( ( v < 0 ) ? -( (-v) >> p ) : ( v >> p ) )

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

	
/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */
class BitReader {
public:
	BitReader( unsigned char* array, int size );
	~BitReader();	
	unsigned int read( int nbits );
	unsigned char read_bit();
	unsigned char unpad( unsigned char fillbit );
	int getpos();
	int getbitp();
	void setpos( int pbyte, int pbit );
	void rewind_bits( int nbits );
	bool eof();
	int peof();
	
private:
    void update_curr_byte();

	unsigned char* data = nullptr;
	int lbyte = 0;
	int cbyte = 0;
	int cbit = 8;
	int peof_ = 0;
	bool eof_ = false;
};


class BitWriter {
public:
	BitWriter(std::uint8_t padbit);
	~BitWriter();
	/*
	* Writes the first (lowest) n bits from val (n is assumed to be less than or equal to 16).
	*/
	void write_u16(std::uint16_t val, std::size_t num_bits);

	/*
	* Writes the bit (assumed to be 0 or 1).
	*/
	void write_bit(std::uint8_t bit);
	/**
	* Fills in the remainder of the current byte using the padbit.
	*/
	void pad();
    /**
    * Returns a copy of the bytes written to the writer, padding the current byte if necessary.
    */
	std::vector<std::uint8_t> get_bytes();
    /**
    * Allocates and returns a copy of the bytes written to the writer, padding the current byte if necessary.
    */
    unsigned char* get_c_bytes();
	/*
	* Returns the number of bytes written. If a byte has not been fully written (e.g., only 5 bits have been written),
	* then it is not counted in the number of bytes written.
	*/
	std::size_t num_bytes_written() const;
private:
	void write_curr_byte();

	std::vector<std::uint8_t> bytes_;
	const std::uint8_t padbit_; // A bit, either 0 or 1, to fill the unwritten bits of the current byte with.
	std::uint8_t curr_byte_ = 0; // The current byte being written.
	std::size_t curr_bit_ = 8; // The position of the next bit in the current byte.
};

class Reader {
public:
	Reader() {}

	virtual ~Reader() {}

	/*
	* Reads the minimum of n and the number of unread bytes to the pointer.
	*/
	virtual std::size_t read(std::uint8_t* to, std::size_t n) = 0;

	/*
	* Reads the minimum of n and the number of unread bytes to the vector, starting at the given offset in the
	* vector. If the destination vector is too small, it is resized. Returns the number of bytes read.
	*/
	virtual std::size_t read(std::vector<std::uint8_t>& into, std::size_t n, std::size_t offset = 0) = 0;

	/*
	* Returns one byte from the reader, throwing a std::runtime_error exception if there are none left to read.
	*/
	virtual std::uint8_t read_byte() = 0;

	/*
	* Reads one byte to the pointer, returning whether there was a byte available to writer.
	*/
	virtual bool read_byte(std::uint8_t* to) = 0;

	/*
	 * Skips the minimum of the n and the number of unread bytes left in the reader.
	 */
	virtual void skip(std::size_t n) = 0;

	/*
	 * Moves the reader back by the minimum of n and the number of bytes already read in the reader.
	 */
	virtual void rewind_bytes(std::size_t n) = 0;

	/*
	* Resets the number of bytes read to zero.
	*/
	virtual void rewind() = 0;

	/*
	* Returns the number of bytes read.
	*/
	virtual std::size_t num_bytes_read() = 0;

	/*
	* Returns the number of bytes in the reader.
	*/
	virtual std::size_t get_size() = 0;

	/*
	* Returns a copy of the data backing the reader.
	*/
	virtual std::vector<std::uint8_t> get_data() = 0;

    unsigned char* get_c_data();

	virtual bool error() = 0;
	/*
	* Returns whether all bytes in the reader have been read.
	*/
	virtual bool end_of_reader() = 0;
};

class MemoryReader : public Reader {
public:
	MemoryReader(const std::vector<std::uint8_t>& bytes);
    MemoryReader(const std::uint8_t* bytes, std::size_t size);

	~MemoryReader() {}

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
	void rewind() override;

	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	const std::vector<std::uint8_t> data_;
	std::vector<std::uint8_t>::const_iterator cbyte_; // The position in the data of the byte being read.
};

class FileReader : public Reader {
public:
	FileReader(const std::string& file_path);
	~FileReader();

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
	void rewind() override;

	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	std::unique_ptr<MemoryReader> reader_;
};

class StreamReader : public Reader {
public:
	StreamReader();

	~StreamReader() {}

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
	void rewind() override;

	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	std::unique_ptr<MemoryReader> reader_;
};

class Writer {
public:
	Writer() {}

	virtual ~Writer() {}

	virtual std::size_t write(const std::uint8_t* from, std::size_t n) = 0;
	virtual std::size_t write(const std::vector<std::uint8_t>& bytes) = 0;
	virtual std::size_t write(const std::array<std::uint8_t, 2>& bytes) = 0;

	virtual bool write_byte(std::uint8_t byte) = 0;

	virtual std::vector<std::uint8_t> get_data() = 0;
    unsigned char* get_c_data();

	virtual void reset() = 0;
	virtual std::size_t num_bytes_written() = 0;
	virtual bool error() = 0;
};

class MemoryWriter : public Writer {
public:
	MemoryWriter();

	~MemoryWriter() {}

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void reset() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::vector<std::uint8_t> data_;
};

class FileWriter : public Writer {
public:
	FileWriter(const std::string& file_path);
	~FileWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void reset() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	FILE* fptr_ = nullptr;
	std::vector<char> file_buffer_; // Used to replace the default file buffer for reads/writes to improve performance.
	const std::string file_path_;
};

class StreamWriter : public Writer {
public:
	StreamWriter();
	~StreamWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void reset() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::unique_ptr<MemoryWriter> writer_;
};

#endif

#ifndef BITOPS_H
#define BITOPS_H

#include <cstdint>
#include <memory>
#include <vector>

namespace bitops {
	template <class T>
	constexpr T RBITS(T val, int n) {
		return val & (0xFF >> (8 - n));
	}

	template <class T>
	constexpr T LBITS(T val, int n) {
		return val >> (8 - n);
	}

	template <class T>
	constexpr int BITN(T val, int n) {
		return (val >> n) & 0x1;
	}
}

enum class StreamType {
	kFile = 0,
	kMemory = 1,
	kStream = 2
};

enum class StreamMode {
	kRead = 0,
	kWrite = 1
};

	
/* -----------------------------------------------
	class to read arrays bitwise
	----------------------------------------------- */

class abitreader {
public:
	abitreader(const std::vector<std::uint8_t>& bits);
	~abitreader();
	unsigned int read(int nbits);
	unsigned char read_bit();
	unsigned char unpad(unsigned char fillbit);
	bool eof() const;
	bool overread() const;

private:
	static constexpr std::uint8_t MBITS(std::uint8_t val, int l, int r) {
		return bitops::RBITS(val, l) >> r;
	}

	const std::vector<std::uint8_t> data;
	std::vector<std::uint8_t>::const_iterator cbyte; // The position in the data of the byte being read.
	int cbit = 8; // The position of the next bit in the current byte.
	bool overread_ = false; // Tried to read more bits than available in the reader.
	bool eof_ = false; // Read all the bits in the reader.
};


/* -----------------------------------------------
	class to write arrays bitwise
	----------------------------------------------- */

class abitwriter {
public:
	abitwriter(int size);
	~abitwriter();
	void write(unsigned int val, int nbits);
	void write_bit(unsigned char bit);
	void set_fillbit(unsigned char fillbit);
	void pad();
	std::vector<std::uint8_t> get_data();
	int getpos() const;

private:
	static constexpr std::uint32_t RBITS32(std::uint32_t val, int n) {
		return val & (0xFFFFFFFF >> (32 - n));
	}

	static constexpr std::uint32_t MBITS32(std::uint32_t val, int l, int r) {
		return RBITS32(val, l) >> r;
	}

	unsigned char fillbit_ = 1;
	std::vector<std::uint8_t> data;
	int cbyte = 0; // The position in the data of the byte being written.
	int cbit = 8; // The position of the next bit in the current byte.
};


/* -----------------------------------------------
	class to read arrays bytewise
	----------------------------------------------- */

class abytereader {
public:
	abytereader(const std::vector<std::uint8_t>& bytes);
	~abytereader();
	/*
	 * Reads one byte to the pointer, returning whether there was a byte available to writer.
	 */
	bool read(unsigned char* byte);
	/*
	 * Reads the minimum of n and the numbers of unread bytes to the pointer, returning the number of bytes read.
	 */
	int read_n(unsigned char* byte, int n);
	/*
	 * Reads the minimum of n and the number of unread bytes to the vector, starting at the given offset in the
	 * vector. If the destination vector is too small, it is resized. Returns the number of bytes read.
	 */
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0);
	/*
	 * Resets the number of bytes read to zero.
	 */
	void reset();
	/*
	 * Returns the number of bytes in the reader.
	 */
	int num_bytes() const;
	/*
	 * Returns the number of bytes read.
	 */
	int num_bytes_read() const;
	/*
	 * Returns whether all bytes in the reader have been read.
	 */
	bool all_bytes_read() const;
	/*
	 * Returns a copy of the data backing the reader.
	 */
	std::vector<std::uint8_t> get_data() const;

private:
	const std::vector<std::uint8_t> data;
	std::vector<std::uint8_t>::const_iterator cbyte; // The position in the data of the byte being read.
	bool _eof = false;
};


/* -----------------------------------------------
	class to write arrays bytewise
	----------------------------------------------- */

class abytewriter
{
public:
	abytewriter( int size );
	~abytewriter();	
	/*
	 * Writes the byte.
	 */
	void write( unsigned char byte );
	/*
	 * Writes n bytes from the pointer.
	 */
	void write_n(const unsigned char* bytes, int n );
	std::vector<std::uint8_t> get_data();
	int getpos() const;
	void reset();
	
private:
	std::vector<std::uint8_t> data;
	int cbyte = 0;
};


/* -----------------------------------------------
	class for input and output from file or memory
	----------------------------------------------- */

class iostream
{
public:
	iostream() {}
	virtual ~iostream() {}
	virtual void switch_mode() = 0;
	virtual int read(unsigned char* to, int dtsize) = 0;
	virtual std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) = 0;
	virtual bool read_byte(unsigned char* to) = 0;
	virtual int write(const unsigned char* from, int dtsize) = 0;
	virtual int write_byte(unsigned char byte) = 0;
	virtual int rewind() = 0;
	virtual int getpos() = 0;
	virtual int getsize() = 0;
	virtual std::vector<std::uint8_t> get_data() = 0;
	virtual bool chkerr() = 0;
	virtual bool chkeof() = 0;
};

class MemStream : public iostream {
public:
	MemStream(StreamMode mode);
	MemStream(const std::vector<std::uint8_t>& bytes, StreamMode mode);
	~MemStream();
	void switch_mode() override;
	int read(unsigned char* to, int dtsize) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	bool read_byte(unsigned char* to) override;
	int write(const unsigned char* from, int dtsize) override;
	int write_byte(unsigned char byte) override;
	int rewind() override;
	int getpos() override;
	int getsize() override;
	bool chkerr() override;
	bool chkeof() override;
	std::vector<std::uint8_t> get_data() override;

private:
	std::unique_ptr<abytewriter> mwrt;
	std::unique_ptr<abytereader> mrdr;
	StreamMode io_mode = StreamMode::kRead;
	bool is_stream;
};

class FileStream : public iostream {
public:
	FileStream(const std::string& file_path, StreamMode iomode);
	~FileStream();
	void switch_mode() override;
	int read(unsigned char* to, int dtsize) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	bool read_byte(unsigned char* to) override;
	int write(const unsigned char* from, int dtsize) override;
	int write_byte(unsigned char byte) override;
	int rewind() override;
	int getpos() override;
	int getsize() override;
	bool chkerr() override;
	bool chkeof() override;
	std::vector<std::uint8_t> get_data() override;

private:
	FILE* fptr = nullptr;
	std::vector<char> file_buffer; // Used to replace the default file buffer for reads/writes to improve performance.
	const std::string file_path;
	StreamMode io_mode;
};

#endif
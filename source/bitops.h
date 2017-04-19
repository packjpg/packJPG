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
	std::uint8_t read_bit();
	std::uint8_t unpad(std::uint8_t fillbit);
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
	void write_bit(std::uint8_t bit);
	void set_fillbit(std::uint8_t fillbit);
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

	std::uint8_t fillbit_ = 1;
	std::vector<std::uint8_t> data;
	int cbyte = 0; // The position in the data of the byte being written.
	int cbit = 8; // The position of the next bit in the current byte.
};

/* -----------------------------------------------
	class to write arrays bytewise
	----------------------------------------------- */

class abytewriter {
public:
	abytewriter(int size);
	~abytewriter();
	/*
	 * Writes the byte.
	 */
	void write(std::uint8_t byte);
	/*
	 * Writes n bytes from the pointer.
	 */
	void write_n(const std::uint8_t* bytes, int n);
	std::vector<std::uint8_t> get_data();
	int getpos() const;
	void reset();

private:
	std::vector<std::uint8_t> data;
	int cbyte = 0;
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

	virtual bool error() = 0;
	/*
	* Returns whether all bytes in the reader have been read.
	*/
	virtual bool end_of_reader() = 0;
};

class FileReader : public Reader {
public:
	FileReader(const std::string& file_path);
	~FileReader();

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;

	void rewind() override;
	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	FILE* fptr_ = nullptr;
	std::vector<char> file_buffer_; // Used to replace the default file buffer for reads/writes to improve performance.
	const std::string file_path_;
};

class MemoryReader : public Reader {
public:
	MemoryReader(const std::vector<std::uint8_t>& bytes);
	~MemoryReader() {}

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;

	void rewind() override;
	std::size_t num_bytes_read() override;
	std::size_t get_size() override;
	std::vector<std::uint8_t> get_data() override;
	bool error() override;
	bool end_of_reader() override;

private:
	const std::vector<std::uint8_t> data_;
	std::vector<std::uint8_t>::const_iterator cbyte_; // The position in the data of the byte being read.
	bool eof_ = false;
};

class StreamReader : public Reader {
public:
	StreamReader();
	~StreamReader() {}

	std::size_t read(std::uint8_t* to, std::size_t num_to_read) override;
	std::size_t read(std::vector<std::uint8_t>& into, std::size_t num_to_read, std::size_t offset = 0) override;
	std::uint8_t read_byte() override;
	bool read_byte(std::uint8_t* to) override;

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
	virtual bool write_byte(std::uint8_t byte) = 0;

	virtual std::vector<std::uint8_t> get_data() = 0;

	virtual void rewind() = 0;
	virtual std::size_t num_bytes_written() = 0;
	virtual bool error() = 0;
};

class FileWriter : public Writer {
public:
	FileWriter(const std::string& file_path);
	~FileWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	FILE* fptr_ = nullptr;
	std::vector<char> file_buffer_; // Used to replace the default file buffer for reads/writes to improve performance.
	const std::string file_path_;
};

class MemoryWriter : public Writer {
public:
	MemoryWriter();
	MemoryWriter(std::size_t initial_capacity);
	~MemoryWriter() {}

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::vector<std::uint8_t> data_;
	std::size_t curr_byte_ = 0;

};

class StreamWriter : public Writer {
public:
	StreamWriter();
	~StreamWriter();

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::unique_ptr<MemoryWriter> writer_;
};

#endif
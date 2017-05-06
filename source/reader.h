#ifndef READER_H
#define READER_H

#include <memory>
#include <vector>

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

	void skip(std::size_t n) override;
	void rewind_bytes(std::size_t n) override;
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
	bool eof_ = false;
};

class MemoryFileReader : public Reader {
public:
	MemoryFileReader(const std::string& file_path);
	~MemoryFileReader();

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

#endif

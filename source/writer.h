#ifndef WRITER_H
#define WRITER_H

#include <array>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <type_traits>
#include <vector>

class Writer {
public:
	Writer() {}

	virtual ~Writer() {}

	virtual std::size_t write(const std::uint8_t* bytes, std::size_t num_bytes) = 0;
	virtual std::size_t write(const std::vector<std::uint8_t>& bytes) = 0;
	virtual std::size_t write(const std::array<std::uint8_t, 2>& bytes) = 0;
	virtual std::size_t write_str(const std::string& s);

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
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	template<typename Integer, class = typename std::enable_if<std::is_integral<Integer>::value>::type>
	std::size_t write(Integer val) {
		std::size_t num_bytes = sizeof(val);
		for (int i = val - 1; i >= 0; i--) {
			std::uint8_t byte = static_cast<std::uint8_t>((val >> (8 * val)) & std::numeric_limits<std::uint8_t>::max());
			this->write_byte(byte);
		}
		return num_bytes;
	}


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

	~MemoryWriter() {}

	std::size_t write(const std::uint8_t* from, std::size_t n) override;
	std::size_t write(const std::vector<std::uint8_t>& bytes) override;
	std::size_t write(const std::array<std::uint8_t, 2>& bytes) override;
	bool write_byte(std::uint8_t byte) override;

	std::vector<std::uint8_t> get_data() override;

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::vector<std::uint8_t> data_;
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

	void rewind() override;
	std::size_t num_bytes_written() override;
	bool error() override;

private:
	std::unique_ptr<MemoryWriter> writer_;
};

#endif

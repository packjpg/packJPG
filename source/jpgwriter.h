#ifndef JPGWRITER_H
#define JPGWRITER_H

#include <cstdint>
#include <vector>

#include "segment.h"
#include "writer.h"

class JpgWriter {
public:
	JpgWriter(Writer& writer,
		const std::vector<Segment>& segments,
		const std::vector<std::uint8_t>& huffman_data,
		const std::vector<std::uint8_t>& garbage_data,
		const std::vector<std::size_t>& restart_marker_pos,
		const std::vector<std::uint8_t>& rst_err,
		const std::vector<std::size_t>& scan_pos);

	void write();

private:
	void write_scan_huffman_data(int& restart_pos, int scan);
		
	Writer& writer_;
	const std::vector<Segment>& segments_;
	const std::vector<std::uint8_t> huffman_data_;
	const std::vector<std::uint8_t> garbage_data_;
	const std::vector<std::size_t> restart_marker_pos_;
	const std::vector<std::size_t> scan_pos_;
	std::vector<std::uint8_t> rst_err_;
};

#endif
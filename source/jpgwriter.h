#ifndef JPGWRITER_H
#define JPGWRITER_H

#include <cstdint>
#include <vector>

#include "segment.h"
#include "writer.h"

class JpgWriter {
public:
	JpgWriter(Writer& writer);

	void write(const std::vector<Segment>& segments,
		const std::vector<std::uint8_t>& huffman_data,
		const std::vector<std::uint8_t>& garbage_data,
		const std::vector<std::size_t>& restart_marker_pos,
		std::vector<std::uint8_t> rst_err,
		const std::vector<std::size_t>& scan_pos);

private:
	void write_scan_huffman_data(int& restart_pos, int scans_processed,
		const std::vector<std::uint8_t>& huffman_data,
		const std::vector<std::size_t>& restart_marker_pos,
		std::vector<std::uint8_t> rst_err,
		const std::vector<std::size_t>& scan_pos);
		
	Writer& writer_;
};

#endif
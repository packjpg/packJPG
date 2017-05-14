#include "segment.h"

#include <array>
#include <numeric>

#include "bitops.h"
#include "pjpgtbl.h"

Segment::Segment(Reader& reader) {
	std::array<std::uint8_t, 2> segment_marker{};
	auto num_read_marker = reader.read(segment_marker.data(), 2);
	if(num_read_marker != 2) {
		throw std::runtime_error("Unable to read segment marker.");
	}

	if (std::get<0>(segment_marker) != 0xFF) {
		throw std::runtime_error("First byte of segment is not 0xFF");
	}
	type_ = Marker(std::get<1>(segment_marker)); // TODO: invalid value check.

	data_.resize(2);
	std::copy(std::begin(segment_marker), std::end(segment_marker), std::begin(data_));
	if (!has_length(type_)) {
		return;
	}

	std::array<std::uint8_t, 2> segment_length{};
	auto num_read_length = reader.read(segment_length.data(), 2);
	if (num_read_length != 2) {
		throw std::runtime_error("Unable to read segment length.");
	}

	std::size_t segment_size = 2 + bitops::pack(std::get<0>(segment_length), std::get<1>(segment_length));
	data_.resize(segment_size);
	std::copy(std::begin(segment_length), std::end(segment_length), std::begin(data_) + 2);
	// read rest of segment, store back in header writer
	const auto payload_size = segment_size - 4;
	if (reader.read(data_, payload_size, 4) != payload_size) {
		throw std::runtime_error("Unable to read entire segment payload.");
	}
}

Segment::Segment(const std::vector<std::uint8_t>& headerData, std::size_t offset) {
	if (offset >= headerData.size() - 1) {
		return; // If there aren't at least two bytes to read the segment type, nothing to read.
	}
	type_ = Marker(headerData[offset + 1]); // TODO: invalid value check.

	// Read in the length of the data if the marker has a length field:
	int size; // The number of bytes in the segment.
	if (has_length(type_)) {
		size = 2 + (int(headerData[offset + 2]) << 8) + int(headerData[offset + 3]);
	} else {
		size = 2;
	}
	const auto start = std::next(std::begin(headerData), offset);
	const auto end = std::next(start, size);
	data_ = std::vector<std::uint8_t>(start, end);

}

std::vector<Segment> Segment::parse_segments(const std::vector<std::uint8_t>& header_data, std::size_t offset) {
	if (offset > header_data.size()) {
		return std::vector<Segment>(); // Don't try to read in invalid memory.
	}

	std::vector<Segment> segments; // The segments in the header.
	auto header_pos = std::next(std::begin(header_data), offset); // Position in the header.

																  // Parse the segments:
	while (std::distance(header_pos, std::end(header_data)) > 0) {
		Segment segment(header_data, std::distance(std::begin(header_data), header_pos));
		if (segment.get_type() == Marker::EOI) {
			break; // Last segment encountered, don't read any more.
		}
		segments.push_back(segment);
		std::advance(header_pos, segment.get_size());
	}

	return segments;
}

Marker Segment::get_type() const {
	return type_;
}

std::size_t Segment::get_size() const {
	return data_.size();
}

std::vector<std::uint8_t> Segment::get_data() const {
	return data_;
}

bool Segment::has_length(Marker type) {
	switch (type) {
	case Marker::RST0:
	case Marker::RST1:
	case Marker::RST2:
	case Marker::RST3:
	case Marker::RST4:
	case Marker::RST5:
	case Marker::RST6:
	case Marker::RST7:
	case Marker::SOI:
	case Marker::EOI:
	case Marker::TEM:
	case Marker::INVALID:
		return false;
	default:
		return true;

	}
}

void Segment::optimize_dqt() {
	std::size_t segment_pos = 4; // Skip marker and segment length data.
	while (segment_pos < data_.size()) {
		const int precision = bitops::left_nibble(data_[segment_pos]);
		segment_pos++;
		if (precision == 1) {
			// Skip 16-bit precision tables.
			segment_pos += 128;
			continue;
		}
		// Difference code 8-bit precision tables (for better packjpg compression):
		auto start_table = std::next(std::begin(data_), segment_pos);
		auto end_table = std::next(start_table, 64);
		std::adjacent_difference(start_table, end_table, start_table);

		segment_pos += 64;
	}
}

void Segment::optimize_dht() {
	std::size_t segment_pos = 4; // Skip marker and segment length data.
	while (segment_pos < data_.size()) {
		segment_pos++;
		 // Replace a standard jpg table with zeroes if one is found:
		for (std::size_t table_id = 0; table_id < pjg::standard_huffman_tables.size(); table_id++) {
			const auto& standard_table = pjg::standard_huffman_tables[table_id];
			bool tables_match = std::equal(std::begin(standard_table),
			                               std::end(standard_table),
			                               std::begin(data_) + segment_pos);
			if (!tables_match) {
				continue;
			}

			data_[segment_pos] = standard_table.size() - 16 - table_id;
			data_[segment_pos + 1] = table_id;

			auto start_table_data = std::next(std::begin(data_), segment_pos + 2);
			auto end_table_data = std::next(std::begin(data_), segment_pos + standard_table.size());
			std::fill(start_table_data, end_table_data, std::uint8_t(0));
			break;
		}

		auto start_table_counts = std::next(std::begin(data_), segment_pos);
		auto end_table_counts = std::next(start_table_counts, 16);
		std::size_t table_size = std::accumulate(start_table_counts,
		                                         end_table_counts,
		                                         std::size_t(16));
		segment_pos += table_size;
	}
}

void Segment::optimize() {
	if (type_ == Marker::DHT) {
		this->optimize_dht();
	} else if (type_ == Marker::DQT) {
		this->optimize_dqt();
	}
}

void Segment::undo_dqt_optimization() {
	std::size_t segment_pos = 4; // Skip marker and segment length data.
	while (segment_pos < data_.size()) {
		const int precision = bitops::left_nibble(data_[segment_pos]);
		segment_pos++;
		if (precision == 1) {
			// Skip 16-bit precision tables, since they aren't optimized.
			segment_pos += 128;
			continue;
		}
		//  Undo difference coding of 8-bit precision tables:
		auto start_table = std::next(std::begin(data_), segment_pos);
		auto end_table = std::next(start_table, 64);
		std::partial_sum(start_table, end_table, start_table);

		segment_pos += 64;
	}
}

void Segment::undo_dht_optimization() {
	size_t segment_pos = 4; // Skip marker and segment length data.
	while (segment_pos < data_.size()) {
		segment_pos++;
		// table found - check if modified
		if (data_[segment_pos] > 2) {
			// reinsert the standard table
			const int i = data_[segment_pos + 1];
			const auto& standard_table = pjg::standard_huffman_tables[i];
			std::copy(std::begin(standard_table), std::end(standard_table), std::begin(data_) + segment_pos);
		}

		auto start_table_counts = std::next(std::begin(data_), segment_pos);
		auto end_table_counts = std::next(start_table_counts, 16);
		std::size_t table_size = std::accumulate(start_table_counts,
		                                         end_table_counts,
		                                         std::size_t(16));
		segment_pos += table_size;
	}
}

void Segment::undo_optimize() {
	if (type_ == Marker::DHT) {
		this->undo_dht_optimization();
	} else if (type_ == Marker::DQT) {
		this->undo_dqt_optimization();
	}
}
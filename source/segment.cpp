#include "segment.h"

#include <array>
#include <numeric>

#include "bitops.h"
#include "pjpgtbl.h"
#include "jfifparse.h"

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

	std::size_t segment_size = 2 + jfif::pack(std::get<0>(segment_length), std::get<1>(segment_length));
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
	std::size_t hpos = 4; // Skip marker and segment length data.
	while (hpos < data_.size()) {
		const int precision = bitops::left_nibble(data_[hpos]);
		hpos++;
		if (precision == 1) {
			// Skip 16-bit precision tables.
			hpos += 128;
			continue;
		}
		// Difference code 8-bit precision tables (for better packjpg compression):
		auto start_table = std::next(std::begin(data_), hpos);
		auto end_table = std::next(start_table, 64);
		std::adjacent_difference(start_table, end_table, start_table);

		hpos += 64;
	}
}

void Segment::optimize_dht() {
	std::size_t hpos = 4; // Skip marker and segment length data.
	while (hpos < data_.size()) {
		hpos++;
		// table found - compare with each of the four standard tables		
		for (int i = 0; i < 4; i++) {
			int sub_pos;
			for (sub_pos = 0; sub_pos < pjg::std_huff_lengths[i]; sub_pos++) {
				if (data_[hpos + sub_pos] != pjg::std_huff_tables[i][sub_pos]) {
					break;
				}
			}
			// check if comparison ok
			if (sub_pos != pjg::std_huff_lengths[i]) {
				continue;
			}

			// if we get here, the table matches the standard table
			// number 'i', so it can be replaced
			data_[hpos] = pjg::std_huff_lengths[i] - 16 - i;
			data_[hpos + 1] = i;
			auto start_table_data = std::next(std::begin(data_), hpos + 2);
			auto end_table_data = std::next(std::begin(data_), hpos + pjg::std_huff_lengths[i]);
			std::fill(start_table_data, end_table_data, std::uint8_t(0));
			// everything done here, so leave
			break;
		}

		auto start_table_counts = std::next(std::begin(data_), hpos);
		auto end_table_counts = std::next(start_table_counts, 16);
		std::size_t table_size = std::accumulate(start_table_counts,
		                                         end_table_counts,
		                                         std::size_t(16));
		hpos += table_size;
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
	std::size_t hpos = 4; // Skip marker and segment length data.
	while (hpos < data_.size()) {
		const int precision = bitops::left_nibble(data_[hpos]);
		hpos++;
		if (precision == 1) {
			// Skip 16-bit precision tables, since they aren't optimized.
			hpos += 128;
			continue;
		}
		//  Undo difference coding of 8-bit precision tables:
		auto start_table = std::next(std::begin(data_), hpos);
		auto end_table = std::next(start_table, 64);
		std::partial_sum(start_table, end_table, start_table);

		hpos += 64;
	}
}

void Segment::undo_dht_optimization() {
	size_t hpos = 4; // Skip marker and segment length data.
	while (hpos < data_.size()) {
		hpos++;
		// table found - check if modified
		if (data_[hpos] > 2) {
			// reinsert the standard table
			const int i = data_[hpos + 1];
			std::copy(pjg::std_huff_tables[i], pjg::std_huff_tables[i] + pjg::std_huff_lengths[i], std::begin(data_) + hpos);
		}

		auto start_table_counts = std::next(std::begin(data_), hpos);
		auto end_table_counts = std::next(start_table_counts, 16);
		std::size_t table_size = std::accumulate(start_table_counts,
		                                         end_table_counts,
		                                         std::size_t(16));
		hpos += table_size;
	}
}

void Segment::undo_optimize() {
	if (type_ == Marker::DHT) {
		this->undo_dht_optimization();
	} else if (type_ == Marker::DQT) {
		this->undo_dqt_optimization();
	}
}
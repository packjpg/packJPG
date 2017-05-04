#include "segment.h"

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
	auto type = Marker(std::get<1>(segment_marker));
	set_type(type);

	data_.resize(2);
	std::copy(std::begin(segment_marker), std::end(segment_marker), std::begin(data_));
	if (!has_length(type)) {
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
	auto type = Marker(headerData[offset + 1]);
	set_type(type);

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

void Segment::set_type(Marker type) {
	// Handle invalid enums:
	switch (type) {
	case Marker::kSOF0:
		type_ = Marker::kSOF0;
		break;
	case Marker::kSOF1:
		type_ = Marker::kSOF1;
		break;
	case Marker::kSOF2:
		type_ = Marker::kSOF2;
		break;
	case Marker::kSOF3:
		type_ = Marker::kSOF3;
		break;
	case Marker::kDHT:
		type_ = Marker::kDHT;
		break;
	case Marker::kSOF5:
		type_ = Marker::kSOF5;
		break;
	case Marker::kSOF6:
		type_ = Marker::kSOF6;
		break;
	case Marker::kSOF7:
		type_ = Marker::kSOF7;
		break;
	case Marker::kJPG:
		type_ = Marker::kJPG;
		break;
	case Marker::kSOF9:
		type_ = Marker::kSOF9;
		break;
	case Marker::kSOF10:
		type_ = Marker::kSOF10;
		break;
	case Marker::kSOF11:
		type_ = Marker::kSOF11;
		break;
	case Marker::kDAC:
		type_ = Marker::kDAC;
		break;
	case Marker::kSOF13:
		type_ = Marker::kSOF13;
		break;
	case Marker::kSOF14:
		type_ = Marker::kSOF14;
		break;
	case Marker::kSOF15:
		type_ = Marker::kSOF15;
		break;
	case Marker::kRST0:
		type_ = Marker::kRST0;
		break;
	case Marker::kRST1:
		type_ = Marker::kRST1;
		break;
	case Marker::kRST2:
		type_ = Marker::kRST2;
		break;
	case Marker::kRST3:
		type_ = Marker::kRST3;
		break;
	case Marker::kRST4:
		type_ = Marker::kRST4;
		break;
	case Marker::kRST5:
		type_ = Marker::kRST5;
		break;
	case Marker::kRST6:
		type_ = Marker::kRST6;
		break;
	case Marker::kRST7:
		type_ = Marker::kRST7;
		break;
	case Marker::kSOI:
		type_ = Marker::kSOI;
		break;
	case Marker::kEOI:
		type_ = Marker::kEOI;
		break;
	case Marker::kSOS:
		type_ = Marker::kSOS;
		break;
	case Marker::kDQT:
		type_ = Marker::kDQT;
		break;
	case Marker::kDNL:
		type_ = Marker::kDNL;
		break;
	case Marker::kDRI:
		type_ = Marker::kDRI;
		break;
	case Marker::kDHP:
		type_ = Marker::kDHP;
		break;
	case Marker::kEXP:
		type_ = Marker::kEXP;
		break;
	case Marker::kAPP0:
		type_ = Marker::kAPP0;
		break;
	case Marker::kAPP1:
		type_ = Marker::kAPP1;
		break;
	case Marker::kAPP2:
		type_ = Marker::kAPP2;
		break;
	case Marker::kAPP3:
		type_ = Marker::kAPP3;
		break;
	case Marker::kAPP4:
		type_ = Marker::kAPP4;
		break;
	case Marker::kAPP5:
		type_ = Marker::kAPP5;
		break;
	case Marker::kAPP6:
		type_ = Marker::kAPP6;
		break;
	case Marker::kAPP7:
		type_ = Marker::kAPP7;
		break;
	case Marker::kAPP8:
		type_ = Marker::kAPP8;
		break;
	case Marker::kAPP9:
		type_ = Marker::kAPP9;
		break;
	case Marker::kAPP10:
		type_ = Marker::kAPP10;
		break;
	case Marker::kAPP11:
		type_ = Marker::kAPP11;
		break;
	case Marker::kAPP12:
		type_ = Marker::kAPP12;
		break;
	case Marker::kAPP13:
		type_ = Marker::kAPP13;
		break;
	case Marker::kAPP14:
		type_ = Marker::kAPP14;
		break;
	case Marker::kAPP15:
		type_ = Marker::kAPP15;
		break;
	case Marker::kJPG0:
		type_ = Marker::kJPG0;
		break;
	case Marker::kJPG13:
		type_ = Marker::kJPG13;
		break;
	case Marker::kCOM:
		type_ = Marker::kCOM;
		break;
	case Marker::kTEM:
		type_ = Marker::kTEM;
		break;
	default:
		type_ = Marker::kINVALID;
	}
}

bool Segment::has_length(Marker type) {
	switch (type) {
	case Marker::kRST0:
	case Marker::kRST1:
	case Marker::kRST2:
	case Marker::kRST3:
	case Marker::kRST4:
	case Marker::kRST5:
	case Marker::kRST6:
	case Marker::kRST7:
	case Marker::kSOI:
	case Marker::kEOI:
	case Marker::kTEM:
	case Marker::kINVALID:
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
	if (type_ == Marker::kDHT) {
		this->optimize_dht();
	} else if (type_ == Marker::kDQT) {
		this->optimize_dqt();
	}
}

void Segment::undo_dqt_optimization() {
	int hpos = 4; // Skip marker and segment length data.
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
	if (type_ == Marker::kDHT) {
		this->undo_dht_optimization();
	} else if (type_ == Marker::kDQT) {
		this->undo_dqt_optimization();
	}
}
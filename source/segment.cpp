#include "segment.h"

Segment::Segment(const std::vector<uint8_t>& headerData, size_t offset) {
	if (offset >= headerData.size() - 1) {
		return; // If there aren't at least two bytes to read the segment type, nothing to read.
	}
	header_pos_ = offset;
	auto type = Marker(headerData[header_pos_ + 1]);

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

	// Read in the length of the data if the marker has a length field:
	int size; // The number of bytes in the segment.
	if (has_length(type_)) {
		size = 2 + (int(headerData[header_pos_ + 2]) << 8) + int(headerData[header_pos_ + 3]);
	} else {
		size = 2;
	}
	const auto start = std::next(std::begin(headerData), header_pos_);
	const auto end = std::next(start, size);
	data_ = std::vector<uint8_t>(start, end);

}

Marker Segment::get_type() const {
	return type_;
}

size_t Segment::get_size() const {
	return data_.size();
}

size_t Segment::get_header_offset() const {
	return header_pos_;
}

std::vector<uint8_t> Segment::get_data() const {
	return data_;
}

void Segment::set_data(std::vector<uint8_t>& data) {
	if (data.size() == data_.size()
		&& data[0] == data_[0]
		&& data[1] == data_[1]) {
		data_ = data;
	}
}

bool Segment::has_length(Marker type) const {
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

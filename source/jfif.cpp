#include "jfif.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <stdexcept>

#include "bitops.h"
#include "jpegtype.h"
#include "marker.h"
#include "pjpgtbl.h"

void jfif::parse_dht(const Segment& segment, std::map<int, std::unique_ptr<HuffCodes>>& dc_tables, std::map<int, std::unique_ptr<HuffCodes>>& ac_tables) {
	if (segment.get_type() != Marker::DHT) {
		throw std::runtime_error("Tried to parse a " + std::to_string(int(segment.get_type())) + " segment in the DHT parser.");
	}
	auto reader = std::make_unique<MemoryReader>(segment.get_data());
	reader->skip(4); // Skip the segment header.
	// build huffman trees & codes
	while (!reader->end_of_reader()) {
		const auto byte = reader->read_byte();
		int table_class = bitops::left_nibble(byte);
		if (table_class != 0 && table_class != 1) {
			throw std::runtime_error("Invalid table class " + std::to_string(table_class) + " in DHT.");
		}

		int table_index = bitops::right_nibble(byte);

		if (table_index > 3) {
			throw std::runtime_error("Invalid table destination identifier " + std::to_string(table_index) + " in DHT.");
		}

		std::vector<std::uint8_t> counts(16);
		const auto counts_num_read = reader->read(counts, counts.size());
		if (counts_num_read != 16) {
			throw std::runtime_error("Insufficient bytes to read counts in DHT segment.");
		}

		const auto size = std::accumulate(std::begin(counts), std::end(counts), std::size_t(0));
		if (size == 0) {
			throw std::runtime_error("Table of zero length in DHT segment.");
		} else if (size > 256) {
			throw std::runtime_error("Overly long table in DHT segment.");
		}

		std::vector<std::uint8_t> values(size);
		const auto length_read = reader->read(values, values.size());
		if (length_read != size) {
			throw std::runtime_error("Insufficient bytes to read table values in DHT segment.");
		}

		auto table = std::make_unique<HuffCodes>(counts, values);
		if (table_class == 0) {
			dc_tables[table_index] = std::move(table);
		} else {
			ac_tables[table_index] = std::move(table);
		}
	}
}

void jfif::parse_dht(const Segment& segment, std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes) {
	if (segment.get_type() != Marker::DHT) {
		throw std::runtime_error("Tried to parse a " + std::to_string(int(segment.get_type())) + " segment in the DHT parser.");
	}
	auto reader = std::make_unique<MemoryReader>(segment.get_data());
	reader->skip(4); // Skip the segment header.
	// build huffman trees & codes
	while (!reader->end_of_reader()) {
		const auto byte = reader->read_byte();
		int table_class = bitops::left_nibble(byte);
		if (table_class != 0 && table_class != 1) {
			throw std::runtime_error("Invalid table class " + std::to_string(table_class) + " in DHT.");
		}

		int table_index = bitops::right_nibble(byte);

		if (table_index > 3) {
			throw std::runtime_error("Invalid table destination identifier " + std::to_string(table_index) + " in DHT.");
		}

		std::vector<std::uint8_t> counts(16);
		const auto counts_num_read = reader->read(counts, counts.size());
		if (counts_num_read != 16) {
			throw std::runtime_error("Insufficient bytes to read counts in DHT segment.");
		}

		const auto size = std::accumulate(std::begin(counts), std::end(counts), std::size_t(0));
		if (size == 0) {
			throw std::runtime_error("Table of zero length in DHT segment.");
		} else if (size > 256) {
			throw std::runtime_error("Overly long table in DHT segment.");
		}

		std::vector<std::uint8_t> values(size);
		const auto length_read = reader->read(values, values.size());
		if (length_read != size) {
			throw std::runtime_error("Insufficient bytes to read table values in DHT segment.");
		}

		hcodes[table_class][table_index] = std::make_unique<HuffCodes>(counts, values);
	}
}

void jfif::parse_dqt(std::map<int, std::array<std::uint16_t, 64>>& qtables, const Segment& segment) {
	if (segment.get_type() != Marker::DQT) {
		throw std::runtime_error("Tried to parse a " + std::to_string(int(segment.get_type())) + " segment in the DQT parser.");
	}
	auto reader = std::make_unique<MemoryReader>(segment.get_data());
	reader->skip(4); // Skip the segment header.
	while (!reader->end_of_reader()) {
		const auto byte = reader->read_byte();
		const int precision = bitops::left_nibble(byte);
		if (precision != 0 && precision != 1) {
			throw std::runtime_error("Invalid quantization table element precision: " + std::to_string(precision));
		}

		const int index = bitops::right_nibble(byte);
		if (index < 0 || index > 3) {
			throw std::runtime_error("Invalid quantization table destination identifier: " + std::to_string(index));
		}

		std::array<std::uint16_t, 64> qtable{};
		if (precision == 0) { // 8-bit quantization table element precision.
			for (std::size_t i = 0; i < qtable.size(); i++) {
				qtable[i] = reader->read_byte();
				if (qtable[i] == 0) {
					throw std::runtime_error("Quantization table contains an element with a zero value.");
				}
			}
		} else { // 16-bit quantization table element precision.
			for (std::size_t i = 0; i < qtable.size(); i++) {
				const auto first = reader->read_byte();
				const auto second = reader->read_byte();

				qtable[i] = bitops::pack(first, second);
				if (qtable[i] == 0) {
					throw std::runtime_error("Quantization table contains an element with a zero value.");
				}
			}
		}
		qtables[index] = qtable;
	}
}

int jfif::parse_dri(const Segment& segment) {
	if (segment.get_type() != Marker::DRI) {
		throw std::runtime_error("Tried to parse a " + std::to_string(int(segment.get_type())) + " segment in the DRI parser.");
	}
	auto reader = std::make_unique<MemoryReader>(segment.get_data());
	if (reader->get_size() < 6) {
		throw std::runtime_error("Insufficient bytes in DRI segment.");
	}
	reader->skip(4); // Skip the segment header.
	const auto first = reader->read_byte();
	const auto second = reader->read_byte();
	return bitops::pack(first, second);
}

void jfif::parse_sof_component_info(std::map<int, std::array<std::uint16_t, 64>>& qtables, Reader& reader, std::vector<Component>& components) {
	for (auto& component : components) {
		component.jid = reader.read_byte();

		const auto byte = reader.read_byte();

		component.sfv = bitops::left_nibble(byte);
		if (component.sfv == 0 || component.sfv > 4) {
			throw std::runtime_error("Invalid vertical sampling factor: " + std::to_string(component.sfv));
		}

		component.sfh = bitops::right_nibble(byte);
		if (component.sfh == 0 || component.sfh > 4) {
			throw std::runtime_error("Invalid horizontal sampling factor: " + std::to_string(component.sfh));
		}

		int quantization_table_index = reader.read_byte();
		if (quantization_table_index > 3) {
			throw std::runtime_error("Invalid quantization table index: " + std::to_string(quantization_table_index));
		}

		component.qtable = qtables[quantization_table_index];

		component.mbs = component.sfv * component.sfh;

	}
}

std::unique_ptr<FrameInfo> jfif::parse_sof(Marker type, const Segment& segment, std::map<int, std::array<std::uint16_t, 64>> qtables) {
	if (segment.get_type() != Marker::SOF0 && segment.get_type() != Marker::SOF1 && segment.get_type() != Marker::SOF2) {
		throw std::runtime_error("Tried to parse a " + std::to_string(int(segment.get_type())) + " segment in the SOF parser.");
	}
	auto reader = std::make_unique<MemoryReader>(segment.get_data());
	reader->skip(4); // Skip the segment header.
	auto frame_info = std::make_unique<FrameInfo>();
	// set JPEG coding type
	if (type == Marker::SOF2) {
		frame_info->coding_process = JpegType::PROGRESSIVE;
	} else if (type == Marker::SOF0 || type == Marker::SOF1) {
		frame_info->coding_process = JpegType::SEQUENTIAL;
	} else {
		throw std::runtime_error("Unsupported JPG coding type.");
	}

	// check data precision, only 8 bit is allowed
	const int precision = reader->read_byte();
	if (precision != 8) {
		throw std::runtime_error(std::to_string(precision) + " bit data precision is not supported");
	}

	auto first = reader->read_byte();
	auto second = reader->read_byte();
	frame_info->image_height = bitops::pack(first, second);
	if (frame_info->image_height == 0) {
		throw std::runtime_error("Image height is zero in the frame header.");
	}

	first = reader->read_byte();
	second = reader->read_byte();
	frame_info->image_width = bitops::pack(first, second);
	if (frame_info->image_width == 0) {
		throw std::runtime_error("Image width is zero in the frame header.");
	}

	const int component_count = reader->read_byte();
	if (component_count == 0) {
		throw std::runtime_error("Zero component count in the frame header.");
	} else if (component_count > 4) {
		throw std::runtime_error("Image has " + std::to_string(component_count) + " components, max 4 are supported");
	}

	frame_info->components.resize(component_count);

	parse_sof_component_info(qtables, *reader, frame_info->components);

	if (!reader->end_of_reader()) {
		throw std::runtime_error("Too many bytes in SOF segment.");
	}

	int h_max = -1; // max horizontal sample factor
	int v_max = -1; // max verical sample factor
	for (const auto& component : frame_info->components) {
		h_max = std::max(component.sfh, h_max);
		v_max = std::max(component.sfv, v_max);
	}
	frame_info->mcu_height = static_cast<int>(std::ceil(static_cast<float>(frame_info->image_height) / static_cast<float>(8 * h_max))); // MCUs per line.
	frame_info->mcu_width = static_cast<int>(std::ceil(static_cast<float>(frame_info->image_width) / static_cast<float>(8 * v_max)));
	frame_info->mcu_count = frame_info->mcu_height * frame_info->mcu_width;
	for (auto& component : frame_info->components) {
		component.bcv = frame_info->mcu_height * component.sfh;
		component.bch = frame_info->mcu_width * component.sfv;
		component.bc = component.bcv * component.bch;
		component.ncv = static_cast<int>(std::ceil(static_cast<float>(frame_info->image_height) *
			(static_cast<float>(component.sfh) / (8.0 * h_max))));
		component.nch = static_cast<int>(std::ceil(static_cast<float>(frame_info->image_width) *
			(static_cast<float>(component.sfv) / (8.0 * v_max))));

		for (auto& coeffs : component.colldata) {
			coeffs.resize(component.bc);
		}
	}

	// decide components' statistical ids
	if (frame_info->components.size() <= 3) {
		for (std::size_t component = 0; component < frame_info->components.size(); component++) {
			frame_info->components[component].sid = component;
		}
	} else {
		for (auto& component : frame_info->components) {
			component.sid = 0;
		}
	}

	// also decide automatic settings here
	for (auto& component : frame_info->components) {
		int set;
		for (set = 0; pjg::conf_sets[set][component.sid] > static_cast<std::uint32_t>(component.bc); set++) {
			if (pjg::conf_sets[set][component.sid] <= static_cast<std::uint32_t>(component.bc)) {
				break; // This is guaranteed to happen, since the last array of conf_sets is filled with zeroes.
			}
		}
		if (set >= pjg::conf_sets.size()) {
			throw std::runtime_error("Failed to decide valid automatic setting.");
		}
		component.segm_cnt = pjg::conf_segm;
		component.nois_trs = pjg::conf_ntrs[set][component.sid];
	}
	return frame_info;
}

std::unique_ptr<FrameInfo> jfif::get_frame_info(const std::vector<Segment>& segments) {
	// Get the quantization tables:
	std::map<int, std::array<std::uint16_t, 64>> qtables;
	for (auto& segment : segments) {
		if (segment.get_type() == Marker::DQT) {
			parse_dqt(qtables, segment);
		}
	}

	// Find and parse the SOF segment:
	for (auto& segment : segments) {
		switch (segment.get_type()) {
		case Marker::SOF0:
			// coding process: baseline DCT
		case Marker::SOF1:
			// coding process: extended sequential DCT
		case Marker::SOF2:
			// coding process: progressive DCT
			return parse_sof(segment.get_type(), segment, qtables);
		case Marker::SOF3:
			// coding process: lossless sequential
			throw std::runtime_error("sof3 marker found, image is coded lossless");
		case Marker::SOF5:
			// coding process: differential sequential DCT
			throw std::runtime_error("sof5 marker found, image is coded diff. sequential");
		case Marker::SOF6:
			// coding process: differential progressive DCT
			throw std::runtime_error("sof6 marker found, image is coded diff. progressive");
		case Marker::SOF7:
			// coding process: differential lossless
			throw std::runtime_error("sof7 marker found, image is coded diff. lossless");
		case Marker::SOF9:
			// coding process: arithmetic extended sequential DCT
			throw std::runtime_error("sof9 marker found, image is coded arithm. sequential");
		case Marker::SOF10:
			// coding process: arithmetic extended sequential DCT
			throw std::runtime_error("sof10 marker found, image is coded arithm. progressive");
		case Marker::SOF11:
			// coding process: arithmetic extended sequential DCT
			throw std::runtime_error("sof11 marker found, image is coded arithm. lossless");
		case Marker::SOF13:
			// coding process: arithmetic differntial sequential DCT
			throw std::runtime_error("sof13 marker found, image is coded arithm. diff. sequential");
		case Marker::SOF14:
			// coding process: arithmetic differential progressive DCT
			throw std::runtime_error("sof14 marker found, image is coded arithm. diff. progressive");
		case Marker::SOF15:
			// coding process: arithmetic differntial lossless
			throw std::runtime_error("sof15 marker found, image is coded arithm. diff. lossless");
		default:
			break; // Ignore other segments.
		}
	}

	throw std::runtime_error("No SOF segment found.");
}

ScanInfo jfif::get_scan_info(FrameInfo& frame_info, const Segment& segment) {
	if (segment.get_type() != Marker::SOS) {
		throw std::runtime_error("Tried to parse a " + std::to_string(int(segment.get_type())) + " segment in the SOS parser.");
	}
	auto reader = std::make_unique<MemoryReader>(segment.get_data());
	reader->skip(4); // Skip the segment header.
	ScanInfo scan_info;
	const int component_count = reader->read_byte();
	if (component_count > frame_info.components.size()) {
		throw std::range_error(std::to_string(component_count) + " components in scan, only " + std::to_string(frame_info.components.size()) + " are allowed");
	}
	scan_info.cmp.resize(component_count);
	for (int i = 0; i < component_count; i++) {
		const int jpg_id = reader->read_byte();
		int cmp;
		for (cmp = 0; cmp < frame_info.components.size(); cmp++) {
			if (jpg_id == frame_info.components[cmp].jid) {
				break;
			}
		}
		if (cmp == frame_info.components.size()) {
			throw std::range_error("component id mismatch in start-of-scan");
		}
		auto& component = frame_info.components[cmp];
		scan_info.cmp[i] = cmp;
		const auto byte = reader->read_byte();
		component.huffdc = bitops::left_nibble(byte);
		component.huffac = bitops::right_nibble(byte);
		if (component.huffdc > 3 || component.huffac > 3) {
			throw std::range_error("huffman table number mismatch");
		}
	}
	scan_info.from = reader->read_byte();
	scan_info.to = reader->read_byte();
	if (scan_info.from > scan_info.to || scan_info.from > 63 || scan_info.to > 63) {
		throw std::range_error("spectral selection parameter out of range");
	}

	const auto byte = reader->read_byte();
	scan_info.sah = bitops::left_nibble(byte);
	scan_info.sal = bitops::right_nibble(byte);
	if (scan_info.sah >= 12 || scan_info.sal >= 12) {
		throw std::range_error("successive approximation parameter out of range");
	}

	return scan_info;
}

CodingStatus jfif::increment_counts(const FrameInfo& frame_info, const ScanInfo& scan_info, int rsti, int& mcu, int& component, int& csc, int& sub, int& rstw) {
	sub++;
	if (sub >= frame_info.components[component].mbs) {
		sub = 0;
		csc++;
		if (csc >= scan_info.cmp.size()) {
			csc = 0;
			component = scan_info.cmp[0];
			mcu++;
			if (mcu >= frame_info.mcu_count) {
				return CodingStatus::DONE;
			}
			else if (rsti > 0) {
				rstw--;
				if (rstw == 0) {
					return CodingStatus::RESTART;
				}
			}
		}
		else {
			component = scan_info.cmp[csc];
		}
	}
	return CodingStatus::OKAY;
}
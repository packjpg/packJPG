#ifndef JFIFPARSE_H
#define JFIFPARSE_H

#include <cmath>
#include <cstdint>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <stdexcept>
#include <vector>

#include "bitops.h"
#include "huffcodes.h"
#include "jpegtype.h"
#include "reader.h"
#include "scaninfo.h"
#include "segment.h"

namespace jfif {
	constexpr int pack(std::uint8_t left, std::uint8_t right) {
		return (int(left) << 8) + int(right);
	}

	// Builds Huffman trees and codes.
	inline void parse_dht(const std::vector<std::uint8_t>& segment, std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes) {
		auto reader = std::make_unique<MemoryReader>(segment);
		std::vector<std::uint8_t> skip(4); // Skip the segment header.
		reader->read(skip, 4);
		// build huffman trees & codes
		while (!reader->end_of_reader()) {
			std::uint8_t byte;
			try {
				byte = reader->read_byte();
			} catch (const std::runtime_error&) {
				throw;
			}
			int table_class = bitops::LBITS(byte, 4);
			if (table_class != 0 && table_class != 1) {
				throw std::runtime_error("Invalid table class " + std::to_string(table_class) + " in DHT.");
			}

			int table_index = bitops::RBITS(byte, 4);

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

	/*
	 * Reads quantization tables from a dqt segment into the supplied map. Any existing quantization table in the map with the same index
	 * as one of the new quantization tables is overwritten. Throws a runtime_error exception if there is a problem parsing the segment.
	 */
	inline void parse_dqt(std::map<int, std::array<std::uint16_t, 64>>& qtables, const std::vector<std::uint8_t>& segment) {
		auto reader = std::make_unique<MemoryReader>(segment);
		std::vector<std::uint8_t> skip(4); // Skip the segment header.
		reader->read(skip, 4);
		while (!reader->end_of_reader()) {
			std::uint8_t byte;
			try {
				byte = reader->read_byte();
			} catch (const std::runtime_error&) {
				throw;
			}
			const int precision = bitops::LBITS(byte, 4);
			if (precision != 0 && precision != 1) {
				throw std::runtime_error("Invalid quantization table element precision: " + std::to_string(precision));
			}

			const int index = bitops::RBITS(byte, 4);
			if (index < 0 || index > 3) {
				throw std::runtime_error("Invalid quantization table destination identifier: " + std::to_string(index));
			}

			std::array<std::uint16_t, 64> qtable{};
			if (precision == 0) { // 8-bit quantization table element precision.
				for (std::size_t i = 0; i < qtable.size(); i++) {
					try {
						qtable[i] = reader->read_byte();
					} catch (const std::runtime_error&) {
						throw;
					}
					if (qtable[i] == 0) {
						throw std::runtime_error("Quantization table contains an element with a zero value.");
					}
				}
			} else { // 16-bit quantization table element precision.
				for (std::size_t i = 0; i < qtable.size(); i++) {
					std::uint8_t first;
					std::uint8_t second;
					try {
						first = reader->read_byte();
						second = reader->read_byte();
					} catch (const std::runtime_error&) {
						throw;
					}

					qtable[i] = pack(first, second);
					if (qtable[i] == 0) {
						throw std::runtime_error("Quantization table contains an element with a zero value.");
					}
				}
			}
			qtables[index] = qtable;
		}
	}

	// Helper function that parses DRI segments.
	inline int parse_dri(const std::vector<std::uint8_t>& segment) {
		auto reader = std::make_unique<MemoryReader>(segment);
		if (reader->get_size() < 6) {
			throw std::runtime_error("Insufficient bytes in DRI segment.");
		}
		std::vector<std::uint8_t> skip(4); // Skip the segment header.
		reader->read(skip, 4);
		try {
			const auto first = reader->read_byte();
			const auto second = reader->read_byte();
			return pack(first, second);
		} catch (const std::runtime_error&) {
			throw;
		}
	}

	// Helper function that parses SOF0/SOF1/SOF2 segments.
	inline std::unique_ptr<FrameInfo> parse_sof(Marker type, const std::vector<std::uint8_t>& segment, std::map<int, std::array<std::uint16_t, 64>> qtables) {
		auto reader = std::make_unique<MemoryReader>(segment);
		std::vector<std::uint8_t> skip(4); // Skip the segment header.
		reader->read(skip, 4);
		auto frame_info = std::make_unique<FrameInfo>();
		// set JPEG coding type
		if (type == Marker::kSOF2) {
			frame_info->coding_process = JpegType::PROGRESSIVE;
		} else if (type == Marker::kSOF0 || type == Marker::kSOF1) {
			frame_info->coding_process = JpegType::SEQUENTIAL;
		} else {
			throw std::runtime_error("Unsupported JPG coding type.");
		}

		// check data precision, only 8 bit is allowed
		int precision;
		try {
			precision = reader->read_byte();
		} catch (std::runtime_error&) {
			throw;
		}
		if (precision != 8) {
			throw std::runtime_error(std::to_string(precision) + " bit data precision is not supported");
		}

		try {
			const auto first = reader->read_byte();
			const auto second = reader->read_byte();
			frame_info->image_height = jfif::pack(first, second);
			if (frame_info->image_height == 0) {
				throw std::runtime_error("Image height is zero in the frame header.");
			}
		} catch (const std::runtime_error&) {
			throw;
		}

		try {
			const auto first = reader->read_byte();
			const auto second = reader->read_byte();
			frame_info->image_width = jfif::pack(first, second);
			if (frame_info->image_width == 0) {
				throw std::runtime_error("Image width is zero in the frame header.");
			}
		} catch (const std::runtime_error&) {
			throw;
		}

		int component_count;
		try {
			component_count = reader->read_byte();
			if (component_count == 0) {
				throw std::runtime_error("Zero component count in the frame header.");
			} else if (component_count > 4) {
				throw std::runtime_error("Image has " + std::to_string(component_count) + " components, max 4 are supported");
			}
		} catch (const std::runtime_error&) {
			throw;
		}

		frame_info->components.resize(component_count);

		// components contained in image
		for (auto& component : frame_info->components) {
			try {
				component.jid = reader->read_byte();
			} catch (const std::runtime_error&) {
				throw;
			}

			std::uint8_t byte;
			try {
				byte = reader->read_byte();
			} catch (const std::runtime_error&) {
				throw;
			}

			component.sfv = bitops::LBITS(byte, 4);
			if (component.sfv == 0 || component.sfv > 4) {
				throw std::runtime_error("Invalid vertical sampling factor: " + std::to_string(component.sfv));
			}

			component.sfh = bitops::RBITS(byte, 4);
			if (component.sfh == 0 || component.sfh > 4) {
				throw std::runtime_error("Invalid horizontal sampling factor: " + std::to_string(component.sfh));
			}

			std::size_t quantization_table_index;
			try {
				quantization_table_index = reader->read_byte();
				if (quantization_table_index > 3) {
					throw std::runtime_error("Invalid quantization table index: " + std::to_string(quantization_table_index));
				}
			} catch (const std::runtime_error&) {
				throw;
			}

			component.qtable = qtables[quantization_table_index];
			
			component.mbs = component.sfv * component.sfh;

		}

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

			component.zdstdata.resize(component.bc);
			component.eobxhigh.resize(component.bc);
			component.eobyhigh.resize(component.bc);
			component.zdstxlow.resize(component.bc);
			component.zdstylow.resize(component.bc);

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
			int i;
			for (i = 0;
			     pjg::conf_sets[i][component.sid] > static_cast<std::size_t>(component.bc);
			     i++);
			component.segm_cnt = pjg::conf_segm;
			component.nois_trs = pjg::conf_ntrs[i][component.sid];
		}
		return frame_info;
	}

	/*
	* Gets and sets the frame info (components, etc.) by parsing the appropriate segments.
	* Throws an exception if there is an error parsing those segments or if a segment is invalid (e.g.,
	* an unsupported SOF type).
	*/
	inline std::unique_ptr<FrameInfo> get_frame_info(const std::vector<Segment>& segments) {
		// Get the quantization tables:
		std::map<int, std::array<std::uint16_t, 64>> qtables;
		for (auto& segment : segments) {
			if (segment.get_type() == Marker::kDQT) {
				try {
					parse_dqt(qtables, segment.get_data());
				} catch (std::runtime_error&) {
					throw;
				}
			}
		}

		// Find and parse the SOF segment:
		for (auto& segment : segments) {
			switch (segment.get_type()) {
			case Marker::kSOF0:
				// coding process: baseline DCT
			case Marker::kSOF1:
				// coding process: extended sequential DCT
			case Marker::kSOF2:
				// coding process: progressive DCT
				try {
					return parse_sof(segment.get_type(), segment.get_data(), qtables);
				} catch (const std::runtime_error&) {
					throw;
				}
			case Marker::kSOF3:
				// coding process: lossless sequential
				throw std::runtime_error("sof3 marker found, image is coded lossless");
			case Marker::kSOF5:
				// coding process: differential sequential DCT
				throw std::runtime_error("sof5 marker found, image is coded diff. sequential");
			case Marker::kSOF6:
				// coding process: differential progressive DCT
				throw std::runtime_error("sof6 marker found, image is coded diff. progressive");
			case Marker::kSOF7:
				// coding process: differential lossless
				throw std::runtime_error("sof7 marker found, image is coded diff. lossless");
			case Marker::kSOF9:
				// coding process: arithmetic extended sequential DCT
				throw std::runtime_error("sof9 marker found, image is coded arithm. sequential");
			case Marker::kSOF10:
				// coding process: arithmetic extended sequential DCT
				throw std::runtime_error("sof10 marker found, image is coded arithm. progressive");
			case Marker::kSOF11:
				// coding process: arithmetic extended sequential DCT
				throw std::runtime_error("sof11 marker found, image is coded arithm. lossless");
			case Marker::kSOF13:
				// coding process: arithmetic differntial sequential DCT
				throw std::runtime_error("sof13 marker found, image is coded arithm. diff. sequential");
			case Marker::kSOF14:
				// coding process: arithmetic differential progressive DCT
				throw std::runtime_error("sof14 marker found, image is coded arithm. diff. progressive");
			case Marker::kSOF15:
				// coding process: arithmetic differntial lossless
				throw std::runtime_error("sof15 marker found, image is coded arithm. diff. lossless");
			default:
				break; // Ignore other segments.
			}
		}

		throw std::runtime_error("No SOF segment found.");
	}

	// Helper function that parses SOS segments.
	inline ScanInfo get_scan_info(FrameInfo& frame_info, const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		ScanInfo scan_info;
		scan_info.cmpc = segment[hpos];
		if (scan_info.cmpc > frame_info.components.size()) {
			throw std::range_error(std::to_string(scan_info.cmpc) + " components in scan, only " + std::to_string(frame_info.components.size()) + " are allowed");
		}
		hpos++;
		for (int i = 0; i < scan_info.cmpc; i++) {
			int cmp;
			for (cmp = 0; (segment[hpos] != frame_info.components[cmp].jid) && (cmp < frame_info.components.size()); cmp++);
			if (cmp == frame_info.components.size()) {
				throw std::range_error("component id mismatch in start-of-scan");
			}
			auto& cmpt = frame_info.components[cmp];
			scan_info.cmp[i] = cmp;
			cmpt.huffdc = bitops::LBITS(segment[hpos + 1], 4);
			cmpt.huffac = bitops::RBITS(segment[hpos + 1], 4);
			if ((cmpt.huffdc < 0) || (cmpt.huffdc >= 4) ||
				(cmpt.huffac < 0) || (cmpt.huffac >= 4)) {
				throw std::range_error("huffman table number mismatch");
			}
			hpos += 2;
		}
		scan_info.from = segment[hpos + 0];
		scan_info.to = segment[hpos + 1];
		scan_info.sah = bitops::LBITS(segment[hpos + 2], 4);
		scan_info.sal = bitops::RBITS(segment[hpos + 2], 4);
		// check for errors
		if ((scan_info.from > scan_info.to) || (scan_info.from > 63) || (scan_info.to > 63)) {
			throw std::range_error("spectral selection parameter out of range");
		}
		if ((scan_info.sah >= 12) || (scan_info.sal >= 12)) {
			throw std::range_error("successive approximation parameter out of range");
		}
		return scan_info;
	}
}

#endif
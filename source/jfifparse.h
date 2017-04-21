#ifndef JFIFPARSE_H
#define JFIFPARSE_H

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

#include "bitops.h"
#include "huffcodes.h"
#include "jpegtype.h"
#include "scaninfo.h"
#include "segment.h"

namespace jfif {
	constexpr int pack(std::uint8_t left, std::uint8_t right) {
		return (int(left) << 8) + int(right);
	}

	// Builds Huffman trees and codes.
	inline void parse_dht(const std::vector<std::uint8_t>& segment, std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes) {
		int hpos = 4; // current position in segment, start after segment header
					  // build huffman trees & codes
		while (hpos < segment.size()) {
			int lval = bitops::LBITS(segment[hpos], 4);
			int rval = bitops::RBITS(segment[hpos], 4);
			if (lval < 0 || lval >= 2 || rval < 0 || rval >= 4) {
				break;
			}

			hpos++;
			// build huffman codes & trees
			hcodes[lval][rval] = std::make_unique<HuffCodes>(&(segment[hpos + 0]), &(segment[hpos + 16]));

			int skip = 16;
			for (int i = 0; i < 16; i++) {
				skip += static_cast<int>(segment[hpos + i]);
			}
			hpos += skip;
		}

		if (hpos != segment.size()) {
			// if we get here, something went wrong
			throw std::range_error("size mismatch in dht marker");
		}
	}

	/*
	 * Reads quantization tables from a dqt segment into the supplied map. Any existing quantization table in the map with the same index
	 * as one of the new quantization tables is overwritten. Throws a runtime_error exception if there is a problem parsing the segment.
	 */
	inline void parse_dqt(std::map<int, std::array<std::uint16_t, 64>>& qtables, const std::vector<std::uint8_t>& segment) {
		std::size_t segment_pos = 4; // current position in segment, start after segment header
		while (segment_pos < segment.size()) {
			const std::uint8_t byte = segment[segment_pos];
			const int precision = bitops::LBITS(byte, 4);
			if (precision < 0 || precision > 1) {
				throw std::runtime_error("Invalid quantization table element precision: " + std::to_string(precision));
			}

			const int index = bitops::RBITS(byte, 4);
			if (index < 0 || index > 3) {
				throw std::runtime_error("Invalid quantization table destination identifier: " + std::to_string(index));
			}
			segment_pos++;

			std::array<std::uint16_t, 64> qtable{};
			if (precision == 0) { // 8-bit quantization table element precision.
				for (std::size_t i = 0; i < qtable.size(); i++) {
					qtable[i] = static_cast<std::uint16_t>(segment[segment_pos + i]);
					if (qtable[i] == 0) {
						throw std::runtime_error("Quantization table contains an element with a zero value.");
					}
				}
				segment_pos += 64;
			}
			else { // 16-bit quantization table element precision.
				for (std::size_t i = 0; i < qtable.size(); i++) {
					qtable[i] = pack(segment[segment_pos + (2 * i)], segment[segment_pos + (2 * i) + 1]);
					if (qtable[i] == 0) {
						throw std::runtime_error("Quantization table contains an element with a zero value.");
					}
				}
				segment_pos += 128;
			}
			qtables[index] = qtable;
		}

		if (segment_pos != segment.size()) {
			// if we get here, something went wrong
			throw std::runtime_error("Invalid length in DQT segment.");
		}
	}

	// Helper function that parses DRI segments.
	inline int parse_dri(const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		return pack(segment[hpos], segment[hpos + 1]);
	}

	// Helper function that parses SOF0/SOF1/SOF2 segments.
	inline std::unique_ptr<FrameInfo> parse_sof(Marker type, const std::vector<std::uint8_t>& segment, std::map<int, std::array<std::uint16_t, 64>> qtables) {
		int hpos = 4; // current position in segment, start after segment header
		auto frame_info = std::make_unique<FrameInfo>();
		// set JPEG coding type
		if (type == Marker::kSOF2) {
			frame_info->coding_process = JpegType::PROGRESSIVE;
		} else if (type == Marker::kSOF0 || type == Marker::kSOF1) {
			frame_info->coding_process = JpegType::SEQUENTIAL;
		} else {
			throw std::runtime_error("Unsupported JPG coding type."); // TODO: switch case for each SOF type.
		}

		// check data precision, only 8 bit is allowed
		int precision = segment[hpos];
		if (precision != 8) {
			throw std::runtime_error(std::to_string(precision) + " bit data precision is not supported");
		}

		// image size, height & component count
		frame_info->image_height = jfif::pack(segment[hpos + 1], segment[hpos + 2]);
		if (frame_info->image_height == 0) {
			throw std::runtime_error("Image height is zero in the frame header.");
		}

		frame_info->image_width = jfif::pack(segment[hpos + 3], segment[hpos + 4]);
		if (frame_info->image_width == 0) {
			throw std::runtime_error("Image width is zero in the frame header.");
		}

		int component_count = segment[hpos + 5];
		if (component_count == 0) {
			throw std::runtime_error("Zero component count in the frame header.");
		} else if (component_count > 4) {
			throw std::runtime_error("image has " + std::to_string(component_count) + " components, max 4 are supported");
		}

		frame_info->components.resize(component_count);

		hpos += 6;
		// components contained in image
		for (auto& component : frame_info->components) {
			component.jid = segment[hpos];

			std::uint8_t byte = segment[hpos + 1];
			component.sfv = bitops::LBITS(byte, 4);
			if (component.sfv == 0 || component.sfv > 4) {
				throw std::runtime_error("Invalid vertical sampling factor: " + std::to_string(component.sfv));
			}

			component.sfh = bitops::RBITS(byte, 4);
			if (component.sfh == 0 || component.sfh > 4) {
				throw std::runtime_error("Invalid horizontal sampling factor: " + std::to_string(component.sfh));
			}

			const int quantization_table_index = segment[hpos + 2];
			if (quantization_table_index < 0 || quantization_table_index > 3) {
				throw std::runtime_error("Invalid quantization table index: " + std::to_string(quantization_table_index));
			}

			component.qtable = qtables[quantization_table_index];
			hpos += 3;

			component.mbs = component.sfv * component.sfh;

		}

		int h_max = -1; // max horizontal sample factor
		int v_max = -1; // max verical sample factor
		for (const auto& component : frame_info->components) {
			h_max = std::max(component.sfh, h_max);
			v_max = std::max(component.sfv, v_max);
		}
		frame_info->mcu_height = static_cast<int>(ceil(static_cast<float>(frame_info->image_height) / static_cast<float>(8 * h_max))); // MCUs per line.
		frame_info->mcu_width = static_cast<int>(ceil(static_cast<float>(frame_info->image_width) / static_cast<float>(8 * v_max)));
		frame_info->mcu_count = frame_info->mcu_height * frame_info->mcu_width;
		for (auto& component : frame_info->components) {
			component.bcv = frame_info->mcu_height * component.sfh;
			component.bch = frame_info->mcu_width * component.sfv;
			component.bc = component.bcv * component.bch;
			component.ncv = static_cast<int>(ceil(static_cast<float>(frame_info->image_height) *
				(static_cast<float>(component.sfh) / (8.0 * h_max))));
			component.nch = static_cast<int>(ceil(static_cast<float>(frame_info->image_width) *
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
			     pjg::conf_sets[i][component.sid] > static_cast<std::uint32_t>(component.bc);
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
	inline ScanInfo get_scan_info(const std::unique_ptr<FrameInfo>& frame_info, const std::vector<std::uint8_t>& segment) {
		int hpos = 4; // current position in segment, start after segment header
		ScanInfo scan_info;
		scan_info.cmpc = segment[hpos];
		if (scan_info.cmpc > frame_info->components.size()) {
			throw std::range_error(std::to_string(scan_info.cmpc) + " components in scan, only " + std::to_string(frame_info->components.size()) + " are allowed");
		}
		hpos++;
		for (int i = 0; i < scan_info.cmpc; i++) {
			int cmp;
			for (cmp = 0; (segment[hpos] != frame_info->components[cmp].jid) && (cmp < frame_info->components.size()); cmp++);
			if (cmp == frame_info->components.size()) {
				throw std::range_error("component id mismatch in start-of-scan");
			}
			auto& cmpt = frame_info->components[cmp];
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
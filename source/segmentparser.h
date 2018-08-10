#ifndef SEGMENTPARSER_H
#define SEGMENTPARSER_H

#include <map>

#include "component.h"
#include "frameinfo.h"
#include "huffcodes.h"
#include "scaninfo.h"
#include "segment.h"

namespace SegmentParser {
	/*
	Returns an in-order vector of the segments contained in the header data, starting at the given offset in the data.
	*/
	std::vector<Segment> parse_segments(const std::vector<std::uint8_t>& header_data, std::size_t offset = 0);

	// Builds Huffman trees and codes. Throws a runtime_error exception if there is a problem parsing the segment.
	void parse_dht(const Segment& segment, std::map<int, std::unique_ptr<HuffCodes>>& dc_tables, std::map<int, std::unique_ptr<HuffCodes>>& ac_tables);

	// Builds Huffman trees and codes. Throws a runtime_error exception if there is a problem parsing the segment.
	void parse_dht(const Segment& segment, std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes);

	/*
	* Reads quantization tables from a dqt segment into the supplied map. Any existing quantization table in the map with the same index
	* as one of the new quantization tables is overwritten. Throws a runtime_error exception if there is a problem parsing the segment.
	*/
	void parse_dqt(const Segment& segment, std::map<int, std::array<std::uint16_t, 64>>& qtables);

	// Parses DRI segments. Throws a runtime_error exception if there is a problem parsing the segment.
	int parse_dri(const Segment& segment);

	/*
	* Gets and sets the frame info (components, etc.) by parsing the appropriate segments. Throws an exception if there is an error parsing
	* those segments or if a segment is invalid (e.g., an unsupported SOF type).
	*/
	std::tuple<FrameInfo, std::vector<Component>> get_frame_info(const std::vector<Segment>& segments);

	// Parses SOS segments. Throws a runtime_error exception if there is a problem parsing the segment.
	ScanInfo get_scan_info(const Segment& segment, std::vector<Component>& components);
};

#endif

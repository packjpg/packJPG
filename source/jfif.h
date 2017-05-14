#ifndef JFIF_H
#define JFIF_H

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

#include "component.h"
#include "frameinfo.h"
#include "huffcodes.h"
#include "marker.h"
#include "reader.h"
#include "scaninfo.h"
#include "segment.h"

namespace jfif {
	// Builds Huffman trees and codes.
	void parse_dht(const Segment& segment, std::map<int, std::unique_ptr<HuffCodes>>& dc_tables, std::map<int, std::unique_ptr<HuffCodes>>& ac_tables);

	// Builds Huffman trees and codes.
	void parse_dht(const Segment& segment, std::array<std::array<std::unique_ptr<HuffCodes>, 4>, 2>& hcodes);

	/*
	 * Reads quantization tables from a dqt segment into the supplied map. Any existing quantization table in the map with the same index
	 * as one of the new quantization tables is overwritten. Throws a runtime_error exception if there is a problem parsing the segment.
	 */
	void parse_dqt(std::map<int, std::array<std::uint16_t, 64>>& qtables, const Segment& segment);

	// Helper function that parses DRI segments.
	int parse_dri(const Segment& segment);

	void parse_sof_component_info(std::map<int, std::array<std::uint16_t, 64>>& qtables, Reader& reader, std::vector<Component>& components);

	// Helper function that parses SOF0/SOF1/SOF2 segments.
	std::unique_ptr<FrameInfo> parse_sof(Marker type, const Segment& segment, std::map<int, std::array<std::uint16_t, 64>> qtables);

	/*
	* Gets and sets the frame info (components, etc.) by parsing the appropriate segments.
	* Throws an exception if there is an error parsing those segments or if a segment is invalid (e.g.,
	* an unsupported SOF type).
	*/
	std::unique_ptr<FrameInfo> get_frame_info(const std::vector<Segment>& segments);

	// Helper function that parses SOS segments.
	ScanInfo get_scan_info(FrameInfo& frame_info, const Segment& segment);

	// increment all counts where needed
	CodingStatus increment_counts(const FrameInfo& frame_info, const ScanInfo& scan_info, int rsti, int& mcu, int& component, int& csc, int& sub, int& rstw);
}

#endif
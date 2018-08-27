#ifndef IMAGEDEBUG_H
#define IMAGEDEBUG_H

#include <cstdint>
#include <string>
#include <vector>

#include "component.h"
#include "debugoptions.h"
#include "frameinfo.h"
#include "segment.h"

class ImageDebug {
private:
	const std::string base_file_;

public:
	ImageDebug() = default;
	ImageDebug(const std::string& base_filename, DebugOptions options);
	void dump_coll(std::vector<Component>& components, CollectionMode collmode) const;
	void dump_data_to_file(const std::vector<std::uint8_t>& data, const std::string& file) const;
	void dump_dist(const std::vector<Component>& components) const;
	void dump_header(const std::vector<Segment>& segments) const;
	void dump_huffman(const std::vector<std::uint8_t>& huffman_data) const;
	void dump_info(const FrameInfo& frame_info, const std::vector<Component>& components, const std::vector<Segment>& segments) const;
	void dump_pgm(const std::vector<Component>& components) const;
	void dump_zdst(const std::vector<Component>& components) const;

	const DebugOptions options_;
};

#endif
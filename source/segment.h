#ifndef SEGMENT_H
#define SEGMENT_H

#include <cstdint>
#include <vector>

#include "marker.h"
#include "reader.h"

class Segment {
public:
	/*
	 * Reads the next segment in from the reader. If the next bytes in the reader aren't a segment,
	 * there are insufficient bytes in the reader for the segment, or the segment is otherwise invalid,
	 * a runtime_error exception is thrown.
	 */
	Segment(Reader& reader);

	/* Reads the information for the segment in the provided header data that starts at
	the given index.
	*/
	Segment(const std::vector<std::uint8_t>& header, std::size_t offset);
	/* Returns the jpeg marker of the segment. Returns an INVALID  type if the segment is invalid.
	*/
	Marker get_type() const;
	/* Returns the number of bytes in the segment, defined as 2 plus the payload size.
	Returns -1 if the segment is invalid.
	*/
	std::size_t get_size() const;

	/*
	Returns a copy of the segment's data, including the marker (and length field, where applicable).
	*/
	std::vector<std::uint8_t> get_data() const;

	/*
	 * Optimizes the segment (if it is a DHT or DQT segment) for PJG compression.
	 */
	void optimize();
	/*
	 * Un-optimizes the segment (if it is a DHT or DQT segment), such that the segment has data == unoptimize(optimize(data)).
	 * Running undo_optimize on a non-optimized segment has undefined (but likely not good) effects.
	 */
	void undo_optimize();

	/*
	Returns an in-order vector of the segments contained in the header data,
	starting at the given offset in the data.
	*/
	static std::vector<Segment> parse_segments(const std::vector<std::uint8_t>& header_data, std::size_t offset = 0);

private:
	/*
	Returns whether the given marker type has a length field associated with it.
	*/
	static bool has_length(Marker type);

	void optimize_dqt();
	void optimize_dht();

	// Undoes DHT segment optimizations.
	void undo_dht_optimization();

	// Undoes DQT segment optimizations.
	void undo_dqt_optimization();

	Marker type_ = Marker::INVALID; // The type of the segment.
	std::vector<std::uint8_t> data_; // The bytes in the segment, including the segment type and length fields where applicable.
};

#endif

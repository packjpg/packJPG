#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include <vector>

#include "component.h"
#include "jpegtype.h"

class FrameInfo {
public:
	JpegType coding_process;

	int image_height = 0;
	int image_width = 0;

	int mcu_height = 0;
	int mcu_width = 0;
	int mcu_count = 0;

	std::vector<Component> components;

	// Calculates next position for MCU.
	int next_mcupos(int mcu, int cmp, int sub) const;
};

#endif
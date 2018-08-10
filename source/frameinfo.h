#ifndef FRAMEINFO_H
#define FRAMEINFO_H

#include "jpegtype.h"

struct FrameInfo {
	JpegType coding_process = JpegType::SEQUENTIAL;

	int image_height = 0;
	int image_width = 0;

	int mcu_height = 0;
	int mcu_width = 0;
	int mcu_count = 0;
};

#endif
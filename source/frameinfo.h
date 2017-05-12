#ifndef FRAMEINFO_H
#define FRAMEINFO_H

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
	inline int next_mcupos(int mcu, int cmp, int sub) const {
		// get correct position in image ( x & y )
		int dpos;
		if (components[cmp].sfh > 1) { // to fix mcu order
			dpos = (mcu / mcu_width) * components[cmp].sfh + (sub / components[cmp].sfv);
			dpos *= components[cmp].bch;
			dpos += (mcu % mcu_width) * components[cmp].sfv + (sub % components[cmp].sfv);
		} else if (components[cmp].sfv > 1) {
			// simple calculation to speed up things if simple fixing is enough
			dpos = (mcu * components[cmp].mbs) + sub;
		} else {
			// no calculations needed without subsampling
			dpos = mcu;
		}
		return dpos;
	}
};

#endif
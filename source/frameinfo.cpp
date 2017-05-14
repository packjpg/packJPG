#include "frameinfo.h"

int FrameInfo::next_mcupos(int mcu, int cmp, int sub) const {
	// get correct position in image ( x & y )
	int dpos;
	if (components[cmp].sfh > 1) { // to fix mcu order
		dpos = (mcu / mcu_width) * components[cmp].sfh + (sub / components[cmp].sfv);
		dpos *= components[cmp].bch;
		dpos += (mcu % mcu_width) * components[cmp].sfv + (sub % components[cmp].sfv);
	}
	else if (components[cmp].sfv > 1) {
		// simple calculation to speed up things if simple fixing is enough
		dpos = (mcu * components[cmp].mbs) + sub;
	}
	else {
		// no calculations needed without subsampling
		dpos = mcu;
	}
	return dpos;
}
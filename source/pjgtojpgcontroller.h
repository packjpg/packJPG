#ifndef PJGTOJPGCONTROLLER_H
#define PJGTOJPGCONTROLLER_H

#include "reader.h"
#include "writer.h"

class PjgToJpgController {
public:
	PjgToJpgController(Reader& pjg_input, Writer& jpg_output);
	~PjgToJpgController();

	void execute();

private:
	Reader& pjg_input_;
	Writer& jpg_output_;
};

#endif
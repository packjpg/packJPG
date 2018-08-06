#ifndef PJGTOJPGCONTROLLER_H
#define PJGTOJPGCONTROLLER_H

#include "controller.h"

#include "reader.h"
#include "writer.h"
#include "imagedebug.h"

class PjgToJpgController : public Controller {
public:
	PjgToJpgController(Reader& pjg_input, Writer& jpg_output);
	PjgToJpgController(Reader& pjg_input, Writer& jpg_output, ImageDebug debug);
	~PjgToJpgController() = default;

	void execute() override;

private:
	Reader& pjg_input_;
	Writer& jpg_output_;
	const ImageDebug debug_;
};

#endif
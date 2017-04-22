#ifndef JPGTOPJGCONTROLLER
#define JPGTOPJGCONTROLLER

#include "controller.h"

#include "reader.h"
#include "writer.h"

class JpgToPjgController : public Controller {
public:
	JpgToPjgController(Reader& jpg_input, Writer& pjg_output);
	~JpgToPjgController();

	void execute() override;

private:
	Reader& jpg_input_;
	Writer& pjg_output_;
};

#endif
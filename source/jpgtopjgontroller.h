#ifndef JPGTOPJGCONTROLLER
#define JPGTOPJGCONTROLLER

#include "controller.h"

#include <vector>

#include "component.h"
#include "imagedebug.h"
#include "reader.h"
#include "writer.h"

class JpgToPjgController : public Controller {
public:
	JpgToPjgController(Reader& jpg_input, Writer& pjg_output);
	JpgToPjgController(Reader& jpg_input, Writer& pjg_output, ImageDebug debug);
	~JpgToPjgController();

	void execute() override;

private:
	/*
	 * Checks the range of values, throwing a std::runtime_error exception if a value is out of range.
	 * Out of range should never happen for unmodified JPEG files.
	 */
	void check_value_range(const std::vector<Component>& components) const;

	Reader& jpg_input_;
	Writer& pjg_output_;
	const ImageDebug debug_;
};

#endif
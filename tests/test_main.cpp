#include "test_framework.h"

#include <cstdio>
#include <cstring>

int main(int argc, char** argv) {
	const char* filter = (argc > 1) ? argv[1] : nullptr;
	int passed = 0;
	int failed = 0;

	for (const auto& tc : pjg_test::registry()) {
		if (filter != nullptr && std::strstr(tc.name, filter) == nullptr) {
			continue;
		}
		try {
			tc.fn();
			std::printf("  [PASS] %s\n", tc.name);
			++passed;
		} catch (const std::exception& e) {
			std::fprintf(stderr, "  [FAIL] %s\n         %s\n", tc.name, e.what());
			++failed;
		} catch (...) {
			std::fprintf(stderr, "  [FAIL] %s (unknown exception)\n", tc.name);
			++failed;
		}
	}

	if (filter != nullptr && passed == 0 && failed == 0) {
		std::fprintf(stderr, "No test cases matched filter: \"%s\"\n", filter);
		return 1;
	}

	std::printf("\nTest summary: %d passed, %d failed (%d assertions)\n",
		passed, failed, pjg_test::check_count());
	return (failed == 0) ? 0 : 1;
}

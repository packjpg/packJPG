#ifndef PACKJPG_TEST_FRAMEWORK_H
#define PACKJPG_TEST_FRAMEWORK_H

#include <cstdio>
#include <exception>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace pjg_test {

struct TestCase {
	const char* name;
	std::function<void()> fn;
};

inline std::vector<TestCase>& registry() {
	static std::vector<TestCase> cases;
	return cases;
}

inline int& check_count() {
	static int count = 0;
	return count;
}

struct Registrar {
	Registrar(const char* name, std::function<void()> fn) {
		registry().push_back({name, std::move(fn)});
	}
};

struct AssertionFailure : std::runtime_error {
	using std::runtime_error::runtime_error;
};

inline void fail(const char* expr, const char* file, int line, const std::string& detail = {}) {
	std::ostringstream oss;
	oss << file << ":" << line << ": assertion failed: " << expr;
	if (!detail.empty()) {
		oss << " (" << detail << ")";
	}
	throw AssertionFailure(oss.str());
}

} // namespace pjg_test

#define PJG_CONCAT_INNER(a, b) a##b
#define PJG_CONCAT(a, b) PJG_CONCAT_INNER(a, b)

#define TEST_CASE(test_name)                                                              \
	static void PJG_CONCAT(pjg_test_fn_, __LINE__)();                                     \
	static ::pjg_test::Registrar PJG_CONCAT(pjg_test_reg_, __LINE__)(                     \
		test_name, PJG_CONCAT(pjg_test_fn_, __LINE__));                                   \
	static void PJG_CONCAT(pjg_test_fn_, __LINE__)()

#define REQUIRE(expr)                                                                     \
	do {                                                                                  \
		++::pjg_test::check_count();                                                      \
		if (!(expr)) {                                                                    \
			::pjg_test::fail(#expr, __FILE__, __LINE__);                                  \
		}                                                                                 \
	} while (false)

#define REQUIRE_EQ(lhs, rhs)                                                              \
	do {                                                                                  \
		++::pjg_test::check_count();                                                      \
		auto&& pjg_lhs_ = (lhs);                                                          \
		auto&& pjg_rhs_ = (rhs);                                                          \
		if (!(pjg_lhs_ == pjg_rhs_)) {                                                    \
			std::ostringstream pjg_oss_;                                                  \
			pjg_oss_ << pjg_lhs_ << " != " << pjg_rhs_;                                   \
			::pjg_test::fail(#lhs " == " #rhs, __FILE__, __LINE__, pjg_oss_.str());       \
		}                                                                                 \
	} while (false)

#define REQUIRE_THROWS(expr)                                                              \
	do {                                                                                  \
		++::pjg_test::check_count();                                                      \
		bool pjg_threw_ = false;                                                          \
		try {                                                                             \
			(void)(expr);                                                                 \
		} catch (const std::exception&) {                                                 \
			pjg_threw_ = true;                                                            \
		}                                                                                 \
		if (!pjg_threw_) {                                                                \
			::pjg_test::fail("expected exception: " #expr, __FILE__, __LINE__);           \
		}                                                                                 \
	} while (false)

#endif

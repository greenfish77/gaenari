#ifndef HEADER_SUPUL_TESTER_HPP
#define HEADER_SUPUL_TESTER_HPP

namespace supul::supul {
// supul_tester.
// access protected things.
class supul_tester {
public:
	supul_tester() = delete;
	inline supul_tester(supul_t& supul): supul{supul} {}
public:
	// return protected variables.
	inline auto& get_db(void) {return supul.model.get_db();}
public:
	supul_t& supul;
};
}
using supul_tester = supul::supul::supul_tester;

#endif // HEADER_SUPUL_TESTER_HPP

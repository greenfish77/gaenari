#ifndef HEADER_GAENARI_GAENARI_METHOD_STRINGFY_UTIL_HPP
#define HEADER_GAENARI_GAENARI_METHOD_STRINGFY_UTIL_HPP

namespace gaenari {
namespace method {
namespace stringfy {
namespace util {

inline stringfy::type get_mime_type(_in const std::string& mime_type) {
	if (mime_type == "text/plain")		return stringfy::type::text_plain;
	if (mime_type == "text/colortag")	return stringfy::type::text_colortag;
	return stringfy::type::unknown;
}

inline std::string get_op(_in const decision_tree::rule_t::rule_type& ruleype) {
	if (ruleype == decision_tree::rule_t::rule_type::cmp_equ) return "=";
	if (ruleype == decision_tree::rule_t::rule_type::cmp_lte) return "<=";
	if (ruleype == decision_tree::rule_t::rule_type::cmp_lt)  return "<";
	if (ruleype == decision_tree::rule_t::rule_type::cmp_gt)  return ">";
	if (ruleype == decision_tree::rule_t::rule_type::cmp_gte) return ">=";
	return "<err>";
}

} // util
} // stringfy
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_STRINGFY_UTIL_HPP

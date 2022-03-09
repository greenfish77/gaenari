#ifndef HEADER_GAENARI_GAENARI_COMMON_VARIANT_HPP
#define HEADER_GAENARI_GAENARI_COMMON_VARIANT_HPP

namespace gaenari {
namespace common {

// variant object to string
inline std::string to_string(_in const type::variant& v) {
	auto index = v.index();
	if (index == type::variant_int)    return std::to_string(std::get<type::variant_int>(v));		// int
	if (index == type::variant_size_t) return std::to_string(std::get<type::variant_size_t>(v));	// size_t
	if (index == type::variant_string) return                std::get<type::variant_string>(v);		// string
	if (index == type::variant_double) return  dbl_to_string(std::get<type::variant_double>(v));	// double
	if (index == type::variant_int64)  return std::to_string(std::get<type::variant_int64>(v));		// int64_t
	THROW_GAENARI_INVALID_PARAMETER("invalid variant instance.");
	return "";
}

inline int to_int(_in const type::variant& v) {
	auto index = v.index();
	if (index == type::variant_int)    return                  std::get<type::variant_int>(v);
	if (index == type::variant_size_t) return static_cast<int>(std::get<type::variant_size_t>(v));
	if (index == type::variant_string) return        std::stoi(std::get<type::variant_string>(v));
	if (index == type::variant_double) return static_cast<int>(std::get<type::variant_double>(v));
	if (index == type::variant_int64)  return static_cast<int>(std::get<type::variant_int64>(v));
	THROW_GAENARI_INVALID_PARAMETER("invalid variant instance.");
	return 0;
}

inline size_t to_size_t(_in const type::variant& v) {
	auto index = v.index();
	if (index == type::variant_int)    return static_cast<size_t>(std::get<type::variant_int>(v));
	if (index == type::variant_size_t) return                     std::get<type::variant_size_t>(v);
	if (index == type::variant_string) return static_cast<size_t>(std::stoll(std::get<type::variant_string>(v)));
	if (index == type::variant_double) return static_cast<size_t>(std::get<type::variant_double>(v));
	if (index == type::variant_int64)  return static_cast<size_t>(std::get<type::variant_int64>(v));
	THROW_GAENARI_INVALID_PARAMETER("invalid variant instance.");
	return 0;
}

inline double to_double(_in const type::variant& v) {
	auto index = v.index();
	if (index == type::variant_int)    return static_cast<double>(std::get<type::variant_int>(v));
	if (index == type::variant_size_t) return static_cast<double>(std::get<type::variant_size_t>(v));
	if (index == type::variant_string) return           std::stod(std::get<type::variant_string>(v));
	if (index == type::variant_double) return                     std::get<type::variant_double>(v);
	if (index == type::variant_int64)  return static_cast<double>(std::get<type::variant_int64>(v));
	THROW_GAENARI_INVALID_PARAMETER("invalid variant instance.");
	return 0;
}

inline int64_t to_int64(_in const type::variant& v) {
	auto index = v.index();
	if (index == type::variant_int)    return static_cast<int64_t>(std::get<type::variant_int>(v));
	if (index == type::variant_size_t) return static_cast<int64_t>(std::get<type::variant_size_t>(v));
	if (index == type::variant_string) return static_cast<int64_t>(std::stoll(std::get<type::variant_string>(v)));
	if (index == type::variant_double) return static_cast<int64_t>(std::get<type::variant_double>(v));
	if (index == type::variant_int64)  return static_cast<int64_t>(std::get<type::variant_int64>(v));
	THROW_GAENARI_INVALID_PARAMETER("invalid variant instance.");
	return 0;
}

inline int option_int(_in const std::map<std::string, type::variant>& options, _in const std::string& name) {
	auto it = options.find(name);
	if (it == options.end()) THROW_GAENARI_ERROR("not found name: " + name);
	if (it->second.index() != type::variant_int) THROW_GAENARI_ERROR("type check error.");
	return std::get<type::variant_int>(it->second);
}

inline size_t option_size(_in const std::map<std::string, type::variant>& options, _in const std::string& name) {
	auto it = options.find(name);
	if (it == options.end()) THROW_GAENARI_ERROR("not found name: " + name);
	if (it->second.index() != type::variant_size_t) THROW_GAENARI_ERROR("type check error.");
	return std::get<type::variant_size_t>(it->second);
}

inline std::string option_string(_in const std::map<std::string, type::variant>& options, _in const std::string& name) {
	auto it = options.find(name);
	if (it == options.end()) THROW_GAENARI_ERROR("not found name: " + name);
	if (it->second.index() != type::variant_string) THROW_GAENARI_ERROR("type check error.");
	return std::get<type::variant_string>(it->second);
}

inline double option_double(_in const std::map<std::string, type::variant>& options, _in const std::string& name) {
	auto it = options.find(name);
	if (it == options.end()) THROW_GAENARI_ERROR("not found name: " + name);
	if (it->second.index() != type::variant_double) THROW_GAENARI_ERROR("type check error.");
	return std::get<type::variant_double>(it->second);
}

inline int64_t option_int64(_in const std::map<std::string, type::variant>& options, _in const std::string& name) {
	auto it = options.find(name);
	if (it == options.end()) THROW_GAENARI_ERROR("not found name: " + name);
	if (it->second.index() != type::variant_int64) THROW_GAENARI_ERROR("type check error.");
	return std::get<type::variant_int64>(it->second);
}

} // namespace common
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_VARIANT_HPP

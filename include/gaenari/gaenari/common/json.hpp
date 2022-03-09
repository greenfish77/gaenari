#ifndef HEADER_JSON_HPP
#define HEADER_JSON_HPP

namespace gaenari {
namespace common {

// forward definition.
template <template <typename...> class object_map_t>
class json_impl;

// choose one of them.
// the default is std::unordered_map.
// if `json_insert_order_map` is used, the name order of the json object is kept.
// ex)
// auto obj  = json    ::parse(s);	// use std::unordered_map.
// auto obj1 = json_map::parse(s);	// use std::map.
using json_map              = json_impl<std::map>;
using json                  = json_map;
using json_insert_order_map = json_impl<common::insert_order_map>;

// use static_assert(false, ... -> static_assert(dependent_false_v<T>, ...
template<typename> inline constexpr bool dependent_false_v = false;

// customize json object(name=value) map.
// ex)
// using json_order = json_impl<my::insert_order_map>;
// auto obj = json_order::parse(s);	// use my::insert_order_map.

// just like namespace `json_impl`.
template <template <typename...> typename object_map_t=std::unordered_map>
class json_impl {
public:
	json_impl()  = default;
	~json_impl() = default;

	// public types.
public:
	// forward declarations.
	struct value_container;

	// json numeric : int, double, int64_t.
	// json object  : name with value.
	// json array   : values.
	// json block   : object, array.
	// json value   : null, string, numeric, object, array, bool.
	using object  = object_map_t<std::string, value_container>;
	using array   = std::vector<value_container>;
	using numeric = std::variant<int, double, int64_t>;
	using block   = std::variant<object, array>;
	using value   = std::variant<std::monostate, std::string, numeric, object, array, bool>;
	struct value_container {
		value v;
	};

	// path object : string name(object) or size_t index(array).
	using path_object = std::variant<std::string, size_t>;

	// set value path object : string name(object) or size_t index(array) or array_append(array append).
	enum class path_option {
		unknown = 0,
		array_append = 1,
	};
	using set_vale_path_object = std::variant<std::string, size_t, path_option>;

	// remark)
	// - do not support duplicatd keys in json object.
	//   to support that we can use multimap instead of map(default: unordered_map).
	//   however, even if it does, the method of accessing it by path(={"key"}) is not suitable.
	//   to reduce complexity, it is not supported.
	//   if you need to access duplicates, use SAX instead of DOM to receive callbacks for them in order.

	// internal types.
protected:
	enum class seq {
		seq_unknown       = 0,
		seq_object        = 100,
		seq_obj_pre       = 101,	// {"...
		seq_obj_in        = 102,	//  "...
		seq_obj_post      = 103,	//   ...":
		seq_value         = 200,
		seq_value_pre     = 201,	//        [... or {... or "...
		seq_value_in      = 202,	//                        "..."
		seq_value_set     = 203,	//
		seq_value_post    = 204,	//                             , or }	
		seq_value_obj     = 300,
	};

	enum block_type {
		block_type_unknown = 0,
		block_type_object  = 1,
		block_type_array   = 2,
	};

	// callback value (base)
	template<typename T=value>
	using callback_value_t = std::function<bool(const std::vector<std::variant<std::string,size_t>>& path, T& value)>;

	// callback value.
	// called when value is comming.

	// [](const auto& path, auto& value) -> bool
	//   - pass path with value.
	//   - return false to stop, return true to continue.
	//   - value is mutable(not const). so you can safely change it or std::move(value) in callback function.
	using callback_value = callback_value_t<typename json_impl<object_map_t>::value>;

	// [](const auto& path, const auto& value) -> bool
	//   - pass path with value.
	//   - return false to stop, return true to continue.
	//   - value is immutable. you can not change it in callback function.
	using callback_value_const = callback_value_t<const value>;

	// callback block.
	// called when block(object or array) is comming.

	// [](const auto& path, block_type type, const auto& pos) -> bool
	//   - pass path with pos.
	//   - return false to stop, return true to continue.
	using callback_block = std::function<bool(const std::vector<std::variant<std::string,size_t>>& path, block_type type, const char* pos)>;

	// stack for dom build.
	// if an object, the name is used.
	struct stack_object {
		std::string name;
		block value;
	};

	// sax object.
	struct sax_object {
		std::vector<path_object> path;
		std::vector<size_t> count;
		value_container value;
	};

	// traverse stack object.
	struct traverse_stack_object {
		size_t pos = 0;
		std::variant<typename object::iterator, typename array::iterator> it;
		std::variant<typename object::iterator, typename array::iterator> end;
	};

	struct traverse_stack_object_const {
		size_t pos = 0;
		std::variant<typename object::const_iterator, typename array::const_iterator> it;
		std::variant<typename object::const_iterator, typename array::const_iterator> end;
	};

	// member variable.
protected:
	block b; // parsed DOM block(start with object or array).

	// exceptions.
public:
	using runt = std::runtime_error;
	class base:						public runt	{ public: base(const std::string& msg): runt(msg) {} };
	class invalid_parameter:		public base	{ public: invalid_parameter(const std::string& msg): base(msg) {} };
	class internal_error:			public base	{ public: internal_error(): base("") {} };
	class index_overflow:			public base	{ public: index_overflow(): base("") {} };
	class its_not_array:			public base	{ public: its_not_array(): base("") {} };
	class invalid_type:				public base	{ public: invalid_type(const std::string& msg): base(msg) {} };
	class object_name_not_found:	public base	{ public: object_name_not_found(const std::string& msg): base(msg) {} };
	class parse_error:				public base	{ public: parse_error(const std::string& msg): base(msg) {} };
	class invalid_utf8:				public base { public: invalid_utf8(): base("") {} };
	class unescape_error:			public base { public: unescape_error(): base("") {} };
	class not_a_number:				public base { public: not_a_number(): base("") {} };
	class not_a_object:				public base { public: not_a_object(): base("") {} };
	class not_a_array:				public base { public: not_a_array(): base("") {} };
	class not_a_value:				public base { public: not_a_value(): base("") {} };

	// internal utility.
protected:
	static inline bool is_white_space(char c) {
		if (c == ' ')  return true;
		if (c == '\r') return true;
		if (c == '\n') return true;
		if (c == '\t') return true;
		return false;
	}

	static inline void pass_white_space(const char** c) {
		for (;;(*c)++) {
			if (*(*c) == '\0') break;
			if (is_white_space(*(*c))) continue;
			break;
		}
	}

	static inline bool to_numeric(const char* c, size_t& len, numeric& n) {
		char* e = nullptr;
		double d = strtod(c, &e);
		if (c == e) {
			len = 0;
			n = 0;
			return false;
		}
		len = e - c;
		int i = static_cast<int>(d);
		if (static_cast<double>(i) == d) {
			n = i;
		} else {
			int64_t i64 = static_cast<int64_t>(d);
			if (static_cast<double>(i64) == d) {
				n = i64;
			} else {
				n = d;
			}
		}
		return true;
	}

	static inline std::string to_hex(char c) {
		char h[4] = {0,};
		auto v = static_cast<unsigned char>(c);
		if (v < 0x10) snprintf(h, 3, "0%X", v);
		else snprintf(h, 3, "%X", v);
		return h;
	}

	static inline unsigned char from_hex(const char* s) {
		static const char* t0 = "0123456789abcdef";
		static const char* t1 = "0123456789ABCDEF";
		int f0 = -1;
		int f1 = -1;
		int i;
		if ((s == nullptr) or (s+1 == nullptr)) throw unescape_error();
		for (i=0; i<=15; i++) {
			if ((f0 < 0) and ((*(s+1) == t0[i]) or (*(s+1) == t1[i]))) f0 = i;
			if ((f1 < 0) and ((*s == t0[i])     or (*s == t1[i])))     f1 = i;
			if ((f0 >= 0) and (f1 >= 0)) break;
		}
		if ((f0 < 0) or (f1 < 0)) throw unescape_error();
		return f0 + 16 * f1;
	}

	template <bool ensure_ascii=false>
	static inline std::string escape(const std::string& utf8) {
		const unsigned char* c = (const unsigned char*)utf8.c_str();
		std::string ret;

		// reserve memory in advance by adding 16 characters.
		ret.reserve(utf8.length() + 16);

		for (;*c!='\0'; c++) {
			// escape.
			// order by frequerncy.
			if      (*c == '\n') { ret += "\\n";  continue; }
			else if (*c == '\r') { ret += "\\r";  continue; }
			else if (*c == '\"') { ret += "\\\""; continue; }
			else if (*c == '\t') { ret += "\\t";  continue; }
			else if (*c == '\\') { ret += "\\\\"; continue; }
			else if (*c == '/')  { ret += "\\/";  continue; }
			else if (*c == '\b') { ret += "\\b";  continue; }
			else if (*c == '\f') { ret += "\\f";  continue; }
			else if ((*c <= 0x1F) or (*c == 0x7F))  { // 0x1F => 31, 0x7F => 127(DEL)
				// control charecter to \u00xx.
				ret += "\\u00" + to_hex(*c);
				continue;
			}
			if constexpr(ensure_ascii) {
				// compiled when ensure ascii is set.
				// utf8 -> utf16 -> \uxxxx[\uxxxx]
				if (*c <= 127)	{
					// 0xxxxxxx
					ret.push_back(*c);
				} else if ((*c & 0b1110'0000) == 0b1100'0000) {
					// 110xxxxx
					auto& b0 = *c;
					auto& b1 = *(c+1);
					if ((b1 & 0b1100'0000) != 0b1000'0000) throw invalid_utf8();
					unsigned char x0 = b0 & 0b0001'1111;
					unsigned char x1 = b1 & 0b0011'1111;
					unsigned char u0 = x0 >> 2;
					unsigned char u1 = (x0 << 6) | x1;
					ret += "\\u" + to_hex(u0) + to_hex(u1);
					c += 1;
				} else if ((*c & 0b1111'0000) == 0b1110'0000) {
					// 1110xxxx
					auto& b0 = *c;
					auto& b1 = *(c+1);
					auto& b2 = *(c+2);
					if ((b1 & 0b1100'0000) != 0b1000'0000) throw invalid_utf8();
					if ((b2 & 0b1100'0000) != 0b1000'0000) throw invalid_utf8();
					// 1110xxxx 10xxxxxx 10xxxxxx => xxxxxxxx xxxxxxxx
					//    b0        b1       b2
					unsigned char x0 = b0 & 0b0000'1111;
					unsigned char x1 = b1 & 0b0011'1111;
					unsigned char x2 = b2 & 0b0011'1111;
					unsigned char u0 = (x0 << 4) | (x1 >> 2);
					unsigned char u1 = (x1 << 6) | x2;
					ret += "\\u" + to_hex(u0) + to_hex(u1);
					c += 2;
				} else if ((*c & 0b1111'1000) == 0b1111'0000) {
					// 11110zzz 10zzxxxx 10xxxxxx 10xxxxxx => 110110ZZ ZZxxxxxx 110111xx xxxxxxxx
					// (ZZZZ = zzzzz - 1)
					auto& b0 = *c;
					auto& b1 = *(c+1);
					auto& b2 = *(c+2);
					auto& b3 = *(c+3);
					if ((b1 & 0b1100'0000) != 0b1000'0000) throw invalid_utf8();
					if ((b2 & 0b1100'0000) != 0b1000'0000) throw invalid_utf8();
					if ((b3 & 0b1100'0000) != 0b1000'0000) throw invalid_utf8();
					unsigned char x0 = b0 & 0b0000'0111;
					unsigned char x1 = b1 & 0b0000'1111;
					unsigned char x2 = b2 & 0b0011'1111;
					unsigned char x3 = b3 & 0b0011'1111;
					unsigned char x1_= b1 & 0b0011'0000;
					unsigned char zzzzz = (x0 << 3) | (x1_ >> 4);
					unsigned char ZZZZ  = (zzzzz - 1) & 0b0000'1111;
					unsigned char u0 = 0b1101'1000 | (ZZZZ >> 2);
					unsigned char u1 = (ZZZZ << 6) | (x1 << 2) | (x2 >> 4);
					unsigned char u2 = 0b110'11100 | ((x2 & 0b0000'1100) >> 2);
					unsigned char u3 = (x2 << 6) | x3;
					ret += "\\u" + to_hex(u0) + to_hex(u1) + "\\u" + to_hex(u2) + to_hex(u3);
					c += 3;
				} else {
					// invalid utf-8 string.
					throw invalid_utf8();
				}
			} else {
				ret.push_back(*c);
			}
		}

		return ret;
	}

	// unescape up to '"'. returns the last position('"').
	// if only interested in the return value, pass leave utf8 as default value.
	static inline const char* unescape(const char* c, std::string* utf8 = nullptr) {
		// clear utf8.
		if (utf8) utf8->clear();

		for (;;c++) {
			if (*c == '\0') throw unescape_error();
			if (*c == '\\') {
				auto& n = *(c+1);
				if (n == 'n')		{ if (utf8) { utf8->push_back('\n'); } c++; continue; }
				else if (n == 'r')	{ if (utf8) { utf8->push_back('\r'); } c++; continue; }
				else if (n == '\"')	{ if (utf8) { utf8->push_back('\"'); } c++; continue; }
				else if (n == 't')	{ if (utf8) { utf8->push_back('\t'); } c++; continue; }
				else if (n == '\\')	{ if (utf8) { utf8->push_back('\\'); } c++; continue; }
				else if (n == '/')	{ if (utf8) { utf8->push_back('/');  } c++; continue; }
				else if (n == 'b')	{ if (utf8) { utf8->push_back('\b'); } c++; continue; }
				else if (n == 'f')	{ if (utf8) { utf8->push_back('\f'); } c++; continue; }
				else if (n != 'u') throw unescape_error();

				// process /uxxxx.
				// utf16 -> utf8
				unsigned char v0, v1, v2, v3;
				unsigned char u0, u1, u2, u3, ZZZZ, zzzzz;
				v0 = from_hex(c+2);
				v1 = from_hex(c+4);
				if ((v0 == 0) and (v1 <= 127)) {
					// 00000000 0xxxxxxx => 0xxxxxxx
					if (utf8) utf8->push_back(v1);
					c += 5;
				} else if (v0 <= 7) {
					// 00000xxx xxxxxxxx => 110xxxxx 10xxxxxx
					u0 = 0b1100'0000 | (v0 << 2) | (v1 >> 6);
					u1 = 0b1000'0000 | (v1 & 0b0011'1111);
					if (utf8) {
						utf8->push_back(u0);
						utf8->push_back(u1);
					}
					c += 5;
				} else if ((v0 & 0b1101'1000) == 0b1101'1000) {
					// 110110ZZ ZZxxxxxx 110111xx xxxxxxxx => 11110zzz 10zzxxxx 10xxxxxx 10xxxxxx (ZZZZ = zzzzz - 1)
					if (not (('\\' == *(c+6)) and ('u' == *(c+7)))) throw unescape_error();
					v2 = from_hex(c+8);
					v3 = from_hex(c+10);
					if ((v2 & 0b1101'1100) != 0b1101'1100) throw unescape_error();
					ZZZZ = ((v0 & 0b0000'0011) << 2) | (v1 >> 6);
					zzzzz = ZZZZ + 1;
					u0 = 0b1111'0000 | (zzzzz >> 2);
					u1 = 0b1000'0000 | ((zzzzz & 0b0000'0011) << 4) | ((v1 & 0b0011'1111) >> 2);
					u2 = 0b1000'0000 | ((v1    & 0b0000'0011) << 4) | ((v2 & 0b0000'0011) << 2) | (v3 >> 6);
					u3 = 0b1000'0000 | (v3 & 0b0011'1111);
					if (utf8) {
						utf8->push_back(u0); 
						utf8->push_back(u1); 
						utf8->push_back(u2); 
						utf8->push_back(u3);
					}
					c += 11;
				} else {
					// xxxxxxxx xxxxxxxx => 1110xxxx 10xxxxxx 10xxxxxx
					u0 = 0b1110'0000 | (v0 >> 4);
					u1 = 0b1000'0000 | ((v0 & 0b0000'1111) << 2) | (v1 >> 6);
					u2 = 0b1000'0000 | (v1 & 0b0011'1111);
					if (utf8) {
						utf8->push_back(u0);
						utf8->push_back(u1);
						utf8->push_back(u2);
					}
					c += 5;
				}
				continue;
			}

			// end check.
			if (*c == '\"') break;

			// copy.
			if (utf8) utf8->push_back(*c);
		}

		return c;
	}

	// public member method.
public:
	// parse json as DOM.
	// to use the sax method, use static_parse.
	inline void parse(const char* utf8_json_block, size_t* length=nullptr) {
		auto block = static_parse(utf8_json_block, length);
		this->b = std::move(block);
	}

	// parse without exceptions.
	inline bool parse_noexcept(const char* utf8_json_block, size_t* length=nullptr) noexcept try {
		auto block = static_parse(utf8_json_block, length);
		this->b = std::move(block);
		return true;
	} catch(...) {
		return false;
	}

	// clear.
	inline void clear(void) noexcept {
		// set empty object.
		this->b = object();
	}

	// return block.
	// auto& b = j.get_block();
	inline auto& get_block(void) {
		return this->b;
	}

	// return immutable block.
	// auto& b = j.get_block_const();
	inline const auto& get_block_const(void) const {
		return this->b;
	}

	// traverse block.
	// traverse with the callback that receives the json path and value.
	// the value can be modified.
	// return false to stop traverse.
	// ex)
	// j.traverse([&](const auto& path, auto& value) -> bool {
	//     return true;
	// });
	inline void traverse(callback_value cb) {
		static_traverse(this->b, cb);
	}

	// traverse block.
	// traverse with the callback that receives the json path and value.
	// the value can not be modified.
	// return false to stop traverse.
	// ex)
	// j.traverse_const([&](const auto& path, const auto& value) -> bool {
	//     return true;
	// });
	inline void traverse_const(callback_value_const cb) const {
		static_traverse_const(this->b, cb);
	}

	// immutable reference find with path.
	// exception raised if not found.
	// ex)
	// try {
	//     auto& value = j.find_value_const({"category", "name"});
	// ...
	inline auto find_value_const(const std::vector<path_object>& target_path) const {
		return static_find_value_const(this->b, target_path);
	}

	// immutable reference find with path.
	// return std::nullopt if not found.
	// ex)
	// auto& find = find_value_noexcept_const_static({"category", "name"});
	// if (!find) {...} // not found.
	// ...
	inline auto find_value_noexcept_const(const std::vector<path_object>& target_path) const noexcept {
		return static_find_value_noexcept_const(this->b, target_path);
	}

	// find value with path(no except, pointer).
	// return json::value* pointer.
	// ex)
	// json::value* v = nullptr;
	// v = j.find_value_ptr_const({"category", "name"});
	// if (!v) {...} // not found.
	// ...
	// remark)
	// the json::... of json::value must be the same as this instance class.
	// json j;
	// ...
	// json::value*     v1 = j.find_value_ptr_const(...); // good.
	// json_map::value* v2 = j.find_value_ptr_const(...); // error!
	inline auto find_value_ptr_const(const std::vector<path_object>& target_path) const noexcept {
		return static_find_value_ptr_const(this->b, target_path);
	}

	// set the value in the path.
	// if the path is not existed, a new one can be created.
	// you can call set_value_noexcept when no exception is needed.
	// ex)
	// j.set_value({"category", "married"},                                  false);
	// j.set_value({"category", "rate"},                                     3.14);
	// j.set_value({"category", "comment", json::path_option::array_append}, "hi, ther.");
	// j.set_value({"category", "comment", json::path_option::array_append}, "thanks.");
	// j.set_value({"category", "comment",                               0}, "hi, there.");
	inline void set_value(const std::vector<set_vale_path_object>& target_path, const char* val) {
		static_set_value(this->b, target_path, val);
	}
	template <typename T>
	inline void set_value(const std::vector<set_vale_path_object>& target_path, const T& val) {
		static_set_value(this->b, target_path, val);
	}

	// move set the string value in the path.
	// this is used when val is not needed after call.
	inline void set_value_move(const std::vector<set_vale_path_object>& target_path, std::string& val) {
		static_set_value_move(this->b, target_path, val);
	}

	// set empty object(={}) in the path.
	inline void set_value_empty_object(const std::vector<set_vale_path_object>& target_path) {
		static_set_value_empty_object(this->b, target_path);
	}

	// set empty array(=[]) in the path.
	inline void set_value_empty_array(const std::vector<set_vale_path_object>& target_path) {
		static_set_value_empty_array(this->b, target_path);
	}

	// set empty object(={}) in the path without exceptions.
	inline bool set_value_empty_object_noexcept(const std::vector<set_vale_path_object>& target_path) noexcept try {
		static_set_value_empty_object(this->b, target_path);
		return true;
	} catch(...) {
		return false;
	}

	// set empty array(=[]) in the path without exceptions.
	inline void set_value_empty_array_noexcept(const std::vector<set_vale_path_object>& target_path) noexcept try {
		static_set_value_empty_array(this->b, target_path);
	} catch(...) {
	}

	// set value without exceptions.
	inline bool set_value_noexcept(const std::vector<set_vale_path_object>& target_path, const char* val) noexcept try {
		static_set_value(this->b, target_path, val);
		return true;
	} catch (...) {
		return false;
	}

	// move set value without exceptions.
	inline bool set_value_move_noexcept(const std::vector<set_vale_path_object>& target_path, std::string& val) noexcept try {
		static_set_value_move(this->b, target_path, val);
		return true;
	} catch (...) {
		return false;
	}

	template <typename T>
	inline bool set_value_noexcept(const std::vector<set_vale_path_object>& target_path, const T& val) noexcept try {
		static_set_value(this->b, target_path, val);
		return true;
	} catch(...) {
		return false;
	}

	// get reference value from json.
	// the return type is determined by the type of `def`.
	// return value is const reference.
	// in case of hard-coded number, reference is not supported, so call get_value_numeric(...).
	// ex)
	// auto& v0 = get_value({"name"},     std::string(name));
	// auto& v1 = get_value({"marriage"}, false);
	// remark)
	// when def is const char*, string(def)& can not be returned, so compile error occurs.
	// call std::string(const char*) instead of const char*.
	// ex) auto& v2 = get_value({"tmp"}, "N/A"))				--> compile error.
	//     auto& v3 = get_value({"tmp"}, std::string("N/A"))	--> good.
	//     std::string empty;
	//     auto& v4 = get_value({"tmp"}, empty)					--> good.
	template <typename T>
	inline const T& get_value(const std::vector<path_object>& target_path, const T& def) const {
		try {
			if constexpr (std::is_same<std::string, T>::value) {
				auto ref_val = find_value_const(target_path);
				auto& v = ref_val.get();
				if (v.index() != 1) throw invalid_type("type is not string.");
				return std::get<1>(v);
			} else if constexpr (std::is_same<bool, T>::value) {
				auto ref_val = find_value_const(target_path);
				auto& v = ref_val.get();
				if (v.index() != 5) throw invalid_type("type is not bool.");
				return std::get<5>(v);
			} else {
				// you call def as const char* (or "...").
				// wrap def as std::string(def).
				static_assert(dependent_false_v<T>, "internal compile error!");
			}
		} catch (object_name_not_found& /*e*/) {
			// it is not an error if the object name is not found.
			return def;
		}
		return def;
	}

	// return the value converted to string.
	// it's a copy.
	// ex)
	// auto string_value = j.get_value_as_string({"name","age"});
	// ...
	inline const std::string get_value_as_string(const std::vector<path_object>& target_path) {
		std::string _;
		return get_value_as_string(target_path, _);
	}

	// returns the value converted to string.
	// it's a reference.
	// in order to return a string reference that is not an actual string, pass an unused temporary string additionally.
	// ex)
	// std::string _;
	// auto& string_value = j.get_value_as_string({"name","age"}, _);
	// ...
	inline const std::string& get_value_as_string(const std::vector<path_object>& target_path, std::string& _) const {
		auto& v = find_value_const(target_path).get();
		auto index = v.index();
		if (index == 0) {
			// monostate.
			_ = "null";
			return _;
		} else if (index == 1) {
			// string.
			return std::get<1>(v);
		} else if (index == 2) {
			// numeric.
			auto numeric_index = std::get<2>(v).index();
			if (numeric_index == 0) {
				// int.
				_ = std::to_string(std::get<0>(std::get<2>(v)));
				return _;
			} else if (numeric_index == 2) {
				// int64_t.
				_ = std::to_string(std::get<2>(std::get<2>(v)));
				return _;
			} else if (numeric_index == 1) {
				// double.
				_ = std::to_string(std::get<1>(std::get<2>(v)));
				_.erase(_.find_last_not_of("0") + 1);
				if (_.back() == '.') _.pop_back();
				return _;
			} else throw internal_error();
		} else if (index == 3) {
			// object.
			throw not_a_value();
		} else if (index == 4) {
			// array.
			throw not_a_value();
		} else if (index == 5) {
			// bool.
			if (std::get<5>(v)) _ = "true";
			else _ = "false";
			return _;
		} else throw internal_error();
	}

	// get number value from json.
	// the return type is determined by the type of `def`.
	// ex)
	// auto v0 = get_value({"age"},   15);
	// auto v1 = get_value({"ratio"}, 0.0);
	template <typename T>
	inline const T get_value_numeric(const std::vector<path_object>& target_path, const T def) const {
		try {
			if constexpr (std::is_arithmetic<T>::value) {
				auto ref_val = find_value_const(target_path);
				auto& v = ref_val.get();
				if (v.index() != 2) throw not_a_number();
				auto& n = std::get<2>(v);
				auto index = n.index();
				if (index == 0)			return static_cast<T>(std::get<0>(n));
				else if (index == 1)	return static_cast<T>(std::get<1>(n));
				else if (index == 2)	return static_cast<T>(std::get<2>(n));
				else throw internal_error();
			} else {
				static_assert(dependent_false_v<T>, "internal compile error!");
			}
		} catch (object_name_not_found& /*e*/) {
			// it is not an error if the object name is not found.
			return def;
		}
		return def;
	}

	// get object names list.
	// call object_names_noexcept for exception free.
	inline std::vector<std::reference_wrapper<const std::string>> object_names(const std::vector<path_object>& target_path) const {
		return static_object_names(this->b, target_path);
	}

	inline std::vector<std::reference_wrapper<const std::string>> object_names_noexcept(const std::vector<path_object>& target_path) const noexcept try {
		return static_object_names(this->b, target_path);
	} catch(...) {
		return {};
	}

	// get array size.
	// call array_size_noexcept for exception free.
	inline size_t array_size(const std::vector<path_object>& target_path) const {
		return static_array_size(this->b, target_path);
	}

	inline size_t array_size_noexcept(const std::vector<path_object>& target_path) const noexcept try {
		return static_array_size(this->b, target_path);
	} catch(...) {
		return 0;
	}

	// public static method.
	// you can call it without object.
	// ex)
	// auto block = json::parse_static("....");
	// json::traverse(block, ...);
	// auto find = json::find(block, ...);
public:
	// DOM like.
	// whole tree memory used.
	static inline block static_parse(const char* utf8_json_block, size_t* length=nullptr) {
		return parse_main(utf8_json_block, length);
	}

	// SAX like.
	// only path and it's value memory used.
	// ex) parse_static(j, [](const auto& path, const auto& value) -> bool {
	//         return true;
	//     });
	static inline void static_parse(const char* utf8_json_block, callback_value cb, size_t* length=nullptr) {
		parse_main(utf8_json_block, length, cb);
	}

	// find block(object, array) with path.
	// ex)
	// 1) path : {"a"}       -> {"b":..., block_type_object.
	// 2) path : {"a","c",2} -> {"q":..., block_type_array.
	// 3) path : {"a","d"}   -> [555,..., block_type_array.
	// 4) path : {"a","b"}   -> nullptr.
	// 
	// the return value becomes an area of interest, and can call parse_static(returned, ...) to improve performance and simplify.
	static inline const char* static_find_block(const char* utf8_json_block, const std::vector<path_object>& target_path, block_type* type=nullptr) {
		const char* ret = nullptr;

		if (target_path.empty()) throw invalid_parameter("empty path.");

		// string value can be skipped(<true>) because we do not need to get `string_value` here.
		parse_main<true>(utf8_json_block, nullptr, nullptr, [&ret, &target_path, &t=type](const auto& path, block_type type, const auto& pos) -> bool {
			if (target_path == path) {
				// found.
				ret = pos;
				if (t) *t = type;
				return false;
			}
			return true;
		});
		return ret;
	}

	// immutable traverse DOM.
	// can not change block and value.
	// traverse_const_static(block, [](const auto& path, const auto& value) -> bool {
	//     return true;
	// });
	static inline void static_traverse_const(const block& block, callback_value_const cb) {
		traverse_main<>(block, cb);
	}

	// mutable traverse DOM.
	// can change value or std::move(value).
	// that is, the `block` can be changed after the call.
	// traverse_static(block, [&some_value](const auto& path, auto& value) -> bool {
	//     some_value = std::move(value);
	//     return true;
	// });
	static inline void static_traverse(block& _block, callback_value cb) {
		traverse_main<_block, traverse_stack_object, callback_value>(_block, cb);
	}

	// find value with path.
	// return {}(=std::nullopt) if not found.
	// ex)
	// auto v = find_value_static(s, {"category1", "name1"});
	// if (!v) printf("not found.\n");
	// else {
	//     auto value& = v.value();
	//     ...
	// }
	// ex)
	// 1) path : {"a", "b"}    -> value() = 111.
	// 2) path : {"a", "d"}    -> {}
	// 3) path : {"a", "d", 1} -> value() = 777.
	// 4) path : {"a", "d", 2} -> {}
	static inline std::optional<value> static_find_value(const char* utf8_json_block, const std::vector<path_object>& target_path) {
		std::optional<value> ret;
		const char* block = nullptr;
		std::vector<path_object> target_value_path;

		// find parent block which has the value.
		if (target_path.empty()) throw invalid_parameter("empty path.");
		if (target_path.size() == 1) {
			target_value_path = target_path;
			block = utf8_json_block;
		} else {
			// assume that the cost of copy path is small.
			auto target_block_path = target_path;
			target_value_path = {target_block_path.back()};
			target_block_path.pop_back();

			// parent block find.
			block = static_find_block(utf8_json_block, target_block_path);
			if (not block) return false;
		}

		// find value while parsing.
		parse_main(block, nullptr, [&ret, &target_value_path](const auto& path, auto& value) -> bool {
			if (path == target_value_path) {
				// found.
				ret = value;
				return false;
			}
			return true;
		});

		return ret;
	}

	static inline std::reference_wrapper<const value> static_find_value_const(const block& _block, const std::vector<path_object>& target_path) {
		auto v = find_value_main<const block, const value, const object, const array>(_block, target_path);
		return *v;
	}

	static inline std::optional<std::reference_wrapper<const value>> static_find_value_noexcept_const(const block& _block, const std::vector<path_object>& target_path) noexcept {
		try {
			return static_find_value_const(_block, target_path);
		} catch (...) {
			return std::nullopt;
		}
	}

	static inline const value* static_find_value_ptr_const(const block& _block, const std::vector<path_object>& target_path) noexcept {
		try {
			return find_value_main<const block,
								   const value,
								   const object,
								   const array>(_block, target_path);
		} catch (...) {
			return nullptr;
		}
	}

	static inline void static_set_value(block& _block, const std::vector<set_vale_path_object>& target_path, const char* val) {
		static_set_value(_block, target_path, std::string(val));
	}

	static inline void static_set_value_move(block& _block, const std::vector<set_vale_path_object>& target_path, std::string& val) {
		auto v = find_value_for_set_value_main(_block, target_path);
		*v = std::move(val);
	}

	template <typename T>
	static inline void static_set_value(block& _block, const std::vector<set_vale_path_object>& target_path, const T& val) {
		auto v = find_value_for_set_value_main(_block, target_path);
		if constexpr (std::is_same<std::string, T>::value) {
			// do not escape.
			// escape on to_string.
			*v = val;
		} else if constexpr (std::is_same<std::nullptr_t, T>::value) {
			// nullptr to monostate(json null).
			// do not use NULL, it's numeric 0.
			*v = {};
		} else if constexpr (std::is_same<bool, T>::value) {
			// json boolean.
			*v = val;
		} else if constexpr (std::is_arithmetic<T>::value) {
			// json numeric.
			*v = numeric(val);
		} else {
			static_assert(dependent_false_v<T>, "internal compile error!");
		}
	}

	static inline void static_set_value_empty_object(block& _block, const std::vector<set_vale_path_object>& target_path) {
		auto v = find_value_for_set_value_main(_block, target_path);
		*v = object();
	}

	static inline void static_set_value_empty_array(block& _block, const std::vector<set_vale_path_object>& target_path) {
		auto v = find_value_for_set_value_main(_block, target_path);
		*v = array();
	}

	static inline std::vector<std::reference_wrapper<const std::string>> static_object_names(const block& _block, const std::vector<path_object>& target_path) {
		return object_names_main(_block, target_path);
	}

	static inline size_t static_array_size(const block& _block, const std::vector<path_object>& target_path) {
		return array_size_main(_block, target_path);
	}

	// to use style in to_string(...), you must define
	//   - newline, indent, comma, trim_right_comma, block_close, block_open, set_value.
	// see pretty's comments.
	//
	// you can customize your own style, and pass it to style template in static_to_string.
	// see minimize and pretty_space2 classes.
	// remark) if the new style has nothing to do with pretty, there is no need to inherit it.
	//         define at least only promised functions.
	struct pretty {
	public:
		// add a new line.
		inline void newline(std::string& out) {
			out += '\n';
		}
		// add indentation depth times.
		inline void indent(std::string& out, size_t depth) {
			for (size_t i=0; i<depth; i++) out += '\t';
		}
		// add ',' character.
		inline void comma(std::string& out) {
			out += ',';
		}
		// change the (comma + new line) at the right end to a (new line).
		// that is, the comma is removed.
		// ...<comma>+<newline> => ...<newline>
		inline void trim_right_comma(std::string& out) {
			if (out.length() <= 2) throw internal_error();
			auto c = &out[out.length()-2];
			if ((c[0] == ',') and (c[1] == '\n')) {
				out.pop_back();
				out.back() = '\n';
			}
		}
		// close an object or array block.
		inline void block_close(std::string& out, bool is_array) {
			if (is_array) out += ']';
			else          out += '}';
		}
		// open a object or array block with name.
		inline void block_open(std::string& out, bool is_array, const std::string* name = nullptr) {
			if (name) {
				if (is_array) out += '\"' + escape(*name) + "\": [";
				else          out += '\"' + escape(*name) + "\": {";
			} else {
				if (is_array) out += '[';
				else          out += '{';
			}
		}
		// stores the (name = value) pair.
		// you can call pretty::append_value_as_string(...) if neccessary.
		inline void set_value(std::string& out, const value& value, const std::string* name = nullptr) {
			if (name) out += '\"' + escape(*name) + "\": ";
			append_value_as_string(out, value, true);
		}

	public:
		// common utility
		inline void append_value_as_string(std::string& out, const value& value, bool quoto_on_string) {
			auto index = value.index();
			if (index == 0) {
				out += "null";
			} else if (index == 1) {
				if (quoto_on_string) out += '\"' + escape(std::get<1>(value)) + '\"';
				else                 out +=        escape(std::get<1>(value));
			} else if (index == 2) {
				auto numeric_index = std::get<2>(value).index();
				if (numeric_index == 0) {
					// int
					out += std::to_string(std::get<0>(std::get<2>(value)));
				} else if (numeric_index == 2) {
					// int64_t
					out += std::to_string(std::get<2>(std::get<2>(value)));
				} else if (numeric_index == 1) {
					// double
					out += std::to_string(std::get<1>(std::get<2>(value)));
					out.erase(out.find_last_not_of("0") + 1);
					if (out.back() == '.') out.pop_back();
				} else {
					throw internal_error();
				}
			} else if (index == 3) {
				if (std::get<3>(value).empty()) {
					// empty object is available.
					out += "{}";
				} else {
					// object is not called here.
					throw internal_error();
				}
			} else if (index == 4) {
				if (std::get<4>(value).empty()) {
					// empty array is available.
					out += "[]";
				} else {
					// array is not called here.
					throw internal_error();
				}
			} else if (index == 5) {
				if (std::get<5>(value) == true) {
					out += "true";
				} else {
					out += "false";
				}
			} else {
				throw internal_error();
			}
		}
	};

	struct minimize: public pretty {
	public:
		inline void newline(std::string& out)				{return;}
		inline void indent(std::string& out, size_t depth)	{return;}
		inline void comma(std::string& out)					{out += ',';}
		inline void trim_right_comma(std::string& out) {
			if (out.length() <= 1) throw internal_error();
			auto c = &out[out.length()-1];
			if (c[0] == ',') out.pop_back();
		}
		// inline void block_close(std::string& out, bool is_array) // call pretty.
		inline void block_open(std::string& out, bool is_array, const std::string* name = nullptr) {
			if (name) {
				if (is_array) out += '\"' + escape(*name) + "\":[";
				else          out += '\"' + escape(*name) + "\":{";
			} else {
				if (is_array) out += '[';
				else          out += '{';
			}
		}
		inline void set_value(std::string& out, const value& value, const std::string* name = nullptr) {
			if (name) out += '\"' + escape(*name) + "\":";
			pretty::append_value_as_string(out, value, true);
		}
	};

	struct pretty_space2: public pretty {
	public:
		// inline void newline(std::string& out)															// call pretty.
		inline void indent(std::string& out, size_t depth) {for (size_t i=0; i<depth; i++) out += "  ";}
		// inline void comma(std::string& out)																// call pretty.
		// inline void trim_right_comma(std::string& out)													// call pretty.
		// inline void block_close(std::string& out, bool is_array)											// call pretty.
		// inline void block_open(std::string& out, bool is_array, const std::string* name = nullptr)		// call pretty.
		// inline void set_value(std::string& out, const value& value, const std::string* name = nullptr)	// call pretty.
	};

	// json block to string.
	// choose pre-defined style.
	//  - pretty        : default pretty json(use a tab as ident).
	//  - pretty_space2 : pretty json(use two spaces as ident).
	//  - minimize      : miminize json. hard to read.
	template <typename style_t=pretty>
	static inline std::string static_to_string(const block& _block) {
		std::string ret;
		size_t depth = 0;
		std::stack<bool> block_histories;
		std::vector<path_object> last_path;
		std::vector<const path_object*> add, del;
		style_t style;

		// add root. { or [.
		if (_block.index() == 0) {
			style.block_open(ret, false);
			block_histories.push(false);
		} else {
			style.block_open(ret, true);
			block_histories.push(true);
		}
		style.newline(ret);
		depth = 1;

		// traverse (name=value) to stringify.
		// json block add and del are processed by path change check.
		static_traverse_const(_block, [&](const auto& path, const auto& value) -> bool {
			// check block.
			check_block_change(last_path, path, add, del);

			// check block del.
			for (size_t i=0; i<del.size(); i++) {
				depth--;
				style.trim_right_comma(ret);
				style.indent(ret, depth);
				style.block_close(ret, block_histories.top());
				style.comma(ret);
				style.newline(ret);
				block_histories.pop();
			}

			// check block add.
			for (size_t i=0; i<add.size(); i++) {
				style.indent(ret, depth);
				const std::string* name = nullptr;
				const path_object* next = nullptr;
				if (add[i]->index() == 0) name = &std::get<0>(*add[i]);
				if (i < add.size() - 1) {
					// the middle item depends on the next item in path.
					next = add[i+1];
				} else {
					// the last item depends on the last item in path.
					next = &path.back();
				}
				if (next->index() == 0) {
					style.block_open(ret, false, name);
					style.newline(ret);
					block_histories.push(false);
				} else {
					style.block_open(ret, true, name);
					style.newline(ret);
					block_histories.push(true);
				}
				depth++;
			}

			// (key = name) value set.
			style.indent(ret, depth);
			if (path.back().index() == 0) {
				style.set_value(ret, value, &std::get<0>(path.back()));
				style.comma(ret);
			} else {
				style.set_value(ret, value);
				style.comma(ret);
			}
			style.newline(ret);

			last_path = path;
			return true;
		});

		// clean block close.
		// stack iteration.
		while (not block_histories.empty()) {
			depth--;
			style.trim_right_comma(ret);
			style.indent(ret, depth);
			style.block_close(ret, block_histories.top());
			style.newline(ret);
			block_histories.pop();
		}

		// here, the depth is 0.
		if (depth != 0) throw internal_error();

		return ret;
	}

	// json block to string with style.
	// ex)
	// get pretty string.
	// auto p = j.to_string();
	// get miminized string.
	// auto m = j.to_string<json::minimized>();
	// get pretty with two spaces as tab.
	// auto s = j.to_string<json::pretty_space2>();
	template <typename style=pretty>
	inline std::string to_string(void) const {
		return static_to_string<style>(this->b);
	}
	
	// to_string without exceptions.
	// empty string on error.
	template <typename style=pretty>
	inline std::string to_string_noexcept(void) const noexcept try {
		return static_to_string<style>(this->b);
	} catch(...) {
		return "";
	}

	// etc public utilities.
	template <bool ensure_ascii=false>
	static inline std::optional<std::string> static_escape(const std::string& utf8) noexcept {
		try {
			return escape<ensure_ascii>(utf8);
		} catch(...) {
			return std::nullopt;
		}
	}

protected:

	template <typename T1, typename T2>
	static inline void check_block_change(const T1& last_path, const T1& cur_path, T2& add, T2& del) {
		size_t same_item_count = 0;
		size_t i = 0;
		size_t to = 0;

		// clear output.
		add.clear();
		del.clear();

		// root.
		if (last_path.empty()) {
			if (cur_path.empty()) throw invalid_parameter("");
			for (size_t i=0; i<cur_path.size()-1; i++) {
				add.push_back(&cur_path[i]);
			}
			return;
		}

		// get same item count.
		// if same_item_count is 0, it is an empty object or array.
		auto min_index = std::min(last_path.size(), cur_path.size());
		if (min_index == 0) throw invalid_parameter("");
		for (i=0; i<=min_index; i++) {
			if (last_path[i] != cur_path[i]) break;
		}
		same_item_count = i;

		// check add.
		to = cur_path.size() - 1;
		for (i=same_item_count; i<to; i++) {
			add.push_back(&cur_path[i]);
		}

		// check del.
		to = last_path.size() - 1;
		for (int n=static_cast<int>(to)-1; n>=static_cast<int>(same_item_count); n--) {
			del.push_back(&last_path[n]);
		}
	}

	// find a value for the set.
	// unlike find_value_main(...), if object or array does not exist, it is added.
	// appended_array_index
	//   - when an item is added to the array by path_option::array_append,
	//     the caller does not know the added index.
	//     if appended_array_index is passed,
	//     either monostate(not the appended array) or size_t(the index of the appended array) is passed like to the path vector.
	static inline value* find_value_for_set_value_main(block& _block, const std::vector<set_vale_path_object>& target_path, std::vector<std::variant<std::monostate, size_t>>* appended_array_index = nullptr) {
		value*  v = nullptr;
		object* o = nullptr;
		array*  a = nullptr;
		bool found_value = false;
		bool empty_block = false;
		bool inserted = false;
		std::variant<std::monostate, size_t> _appended_array_index;

		if (target_path.empty()) throw invalid_parameter("empty path.");
		if (appended_array_index) appended_array_index->clear();

		// check block is empty({} or []).
		// empty block can be converted to {} or [] by the first item of path.
		// ex)
		// ({0},   1.1) => [1.1]
		// ({"a"}, 1.1) => {"a":1.1}
		if      ((_block.index() == 0) and (std::get<0>(_block).empty())) empty_block = true;
		else if ((_block.index() == 1) and (std::get<1>(_block).empty())) empty_block = true;
		if (empty_block) {
			auto path_index = target_path.front().index();
			if (path_index == 0) {
				// convert to object.
				_block = object();
			} else if (path_index == 1) {
				// convert to array.
				_block = array();
			} else if ((path_index == 2) and (std::get<2>(target_path.front()) == path_option::array_append)) {
				// convert to array.
				_block = array();
			} else {
				throw internal_error();
			}
		}

		// get first pos.
		auto block_index = _block.index();
		if (block_index == 0) {
			// object.
			o = &std::get<0>(_block);
		} else if (block_index == 1) {
			// array.
			a = &std::get<1>(_block);
		} else throw internal_error();

		size_t i, count;
		count = target_path.size();
		for (i=0; i<count; i++) {
			const auto& current_path = target_path[i];
			auto path_index = current_path.index();	// 0(string) => object, 1(size_t) => array, 2(path_option)
			inserted = false;
			_appended_array_index = {};

			if ((path_index == 0) and o) {
				// object.
				const auto& name = std::get<0>(current_path);
				auto find = o->find(name);
				if (find == o->end()) {
					// add a value.
					auto [it, _] = o->insert({name, value_container()});
					v = &it->second.v;
					inserted = true;
				} else {
					// get a value.
					v = &find->second.v;
				}
			} else if ((path_index == 1) and a) {
				// array.
				auto index = std::get<1>(current_path);
				if (a->size() <= index) throw index_overflow();
				// get a value.
				v = &(*a)[index].v;
			} else if ((path_index == 2) and (std::get<2>(current_path) == path_option::array_append) and a) {
				// append to array.
				if (not a) throw its_not_array();
				a->emplace_back(value_container());
				_appended_array_index = a->size();
				v = &a->back().v;
				inserted = true;
			} else {
				// block is missing.
				// - block is array,  but path item is object.
				// - block is object, but path item is array index.
				throw invalid_parameter("block is missing.");
			}

			// array appended?
			if (appended_array_index) appended_array_index->emplace_back(_appended_array_index);

			// inserted?
			if ((inserted) and (i+1 != count)) {
				// when inserted in the middle of path, the object or array is determined by the next path item.
				const auto& next_path = target_path[i+1];
				auto path_index = next_path.index();	// 0(string) => object, 1(size_t) => array, 2(path_option)
				if (path_index == 0) {
					*v = object();
				} else if ((path_index == 2) and (std::get<2>(next_path) == path_option::array_append)) {
					*v = array();
				} else {
					throw internal_error();
				}
			}

			// check.
			auto index = v->index();
			if (index == 3) {
				// object.
				o = &std::get<3>(*v);
				a = nullptr;
				v = nullptr;
			} else if (index == 4) {
				// array.
				o = nullptr;
				a = &std::get<4>(*v);
				v = nullptr;
			} else if (index <= 5) {
				if (i+1 == count) {
					found_value = true;
					break;
				}
				throw invalid_type("one of path item is not object or array.");
			} else throw internal_error();
		}
		if (not found_value) throw internal_error();

		// return value for set.
		return v;
	}

	template <typename block_t=block, typename value_t=value, typename object_t=object, typename array_t=array>
	static inline value_t* find_value_main(block_t& block, const std::vector<path_object>& target_path) {
		auto [v,o,a] = find_main<block_t,value_t,object_t,array_t>(block, target_path);
		return v;
	}

	template <typename block_t=block, typename value_t=value, typename object_t=object, typename array_t=array>
	static inline object_t* find_object_main(block_t& block, const std::vector<path_object>& target_path) {
		auto [v,o,a] = find_main<block_t,value_t,object_t,array_t>(block, target_path);
		return o;
	}

	template <typename block_t=block, typename value_t=value, typename object_t=object, typename array_t=array>
	static inline array_t* find_array_main(block_t& block, const std::vector<path_object>& target_path) {
		auto [v,o,a] = find_main<block_t,value_t,object_t,array_t>(block, target_path);
		return a;
	}

	// find stuff(value or object or array) with path.
	// return : [value pointer, object pointer, array pointer] tuple.
	// exception : not found or error occured.
	template <typename block_t=block, typename value_t=value, typename object_t=object, typename array_t=array>
	static inline std::tuple<value_t*,object_t*,array_t*> find_main(block_t& block, const std::vector<path_object>& target_path) {
		value_t*  v = nullptr;
		object_t* o = nullptr;
		array_t*  a = nullptr;

		if (target_path.empty()) throw invalid_parameter("empty path.");

		// get first pos.
		auto block_index = block.index();
		if (block_index == 0) {
			// object.
			o = &std::get<0>(block);
		} else if (block_index == 1) {
			// array.
			a = &std::get<1>(block);
		} else throw internal_error();

		size_t i, count;
		count = target_path.size();
		for (i=0; i<count; i++) {
			const auto& current_path = target_path[i];
			auto path_index  = current_path.index();	// 0(string) => object, 1(size_t) => array.

			if ((path_index == 0) and o) {
				// object.
				auto find = o->find(std::get<0>(current_path));
				if (find == o->end()) throw object_name_not_found(std::get<0>(current_path) + " is not found.");
				// get a value.
				v = &find->second.v;
			} else if ((path_index == 1) and a) {
				// array.
				auto index = std::get<1>(current_path);
				if (a->size() <= index) throw index_overflow();
				// get a value.
				v = &(*a)[index].v;
			} else {
				throw internal_error();
			}

			// check.
			auto index = v->index();
			if (index == 3)	{
				// object.
				o = &std::get<3>(*v);
				a = nullptr;
				v = nullptr;
			} else if (index == 4) {
				// array.
				o = nullptr;
				a = &std::get<4>(*v);
				v = nullptr;
			} else if (index <= 5) {
				if (i+1 == count) return {v, nullptr, nullptr};
				throw its_not_array();
			} else throw internal_error();
		}

		return {v, o, a};
	}

	static inline std::vector<std::reference_wrapper<const std::string>> object_names_main(const block& _block, const std::vector<path_object>& target_path) {
		std::vector<std::reference_wrapper<const std::string>> ret;
		auto o = find_object_main<const block, const value, const object, const array>(_block, target_path);
		if (not o) throw not_a_object();
		for (const auto& it : *o) ret.push_back(it.first);
		
		return ret;
	}

	static inline size_t array_size_main(const block& _block, const std::vector<path_object>& target_path) {
		auto a = find_array_main<const block, const value, const object, const array>(_block, target_path);
		if (not a) throw not_a_array();
		return a->size();
	}

	// traverse DOM main function.
	// ex) traverse(block, [](const auto& path, const auto& value) -> bool {
	//         return true;
	//     });
	template<typename block_type_t            = const block, 
		     typename traverse_stack_object_t = traverse_stack_object_const, 
		     typename callback_t              = callback_value_const>
	static inline void traverse_main(block_type_t& block, callback_t cb) {
		traverse_stack_object_t root;
		traverse_stack_object_t child;
		const object empty_object;
		const array empty_array;
		const std::string empty_string;
		std::stack<traverse_stack_object_t> stack;
		std::vector<path_object> path;

		// push root to stack.
		if (block.index() == 0) {
			root.it  = std::get<0>(block).begin();
			root.end = std::get<0>(block).end();
			root.pos = 0;
			path.emplace_back(std::get<0>(root.it)->first);
		} else if (block.index() == 1) {
			root.it  = std::get<1>(block).begin();
			root.end = std::get<1>(block).end();
			root.pos = 0;
			path.push_back(0);
		} else {
			throw invalid_parameter("");
		}
		stack.emplace(root);

		for (;;) {
			// traverse till stack empty.
			if (stack.empty()) break;
		
			auto& top = stack.top();
			auto  top_index = top.it.index();
			if (top_index == 0) {
				// in object.
				auto& it  = std::get<0>(top.it);
				auto& end = std::get<0>(top.end);
				auto& pos = top.pos;

				if (it == end) {
					stack.pop();
					path.pop_back();
					continue;
				}

				// set index in path array from name.
				if (path.back().index() != 0) throw internal_error();
				path.back() = it->first;

				// get value index.
				auto value_index = it->second.v.index();
				if (value_index == 3) {
					// child : object.
					child.it  = std::get<3>(it->second.v).begin();
					child.end = std::get<3>(it->second.v).end();
					child.pos = 0;

					if (child.it == child.end) {
						// empty child.
						// call it.
						if (not (cb)(path, it->second.v)) break;
					} else {
						// push child.
						stack.emplace(child);
						path.emplace_back(std::get<0>(child.it)->first);
					}
				} else if (value_index == 4) {
					// child : array.
					child.it  = std::get<4>(it->second.v).begin();
					child.end = std::get<4>(it->second.v).end();
					child.pos = 0;

					if (child.it == child.end) {
						// empty child.
						// call it.
						if (not (cb)(path, it->second.v)) break;
					} else {
						// push child.
						stack.emplace(child);
						path.emplace_back(0);
					}
				} else {
					// call.
					// path, it.
					if (not (cb)(path, it->second.v)) break;
				}

				// goto next.
				it++;
				pos++;
			} else if (top_index == 1) {
				auto& it  = std::get<1>(top.it);
				auto& end = std::get<1>(top.end);
				auto& pos = top.pos;

				if (it == end) {
					stack.pop();
					path.pop_back();
					continue;
				}

				// set index in path array from pos.
				if (path.back().index() != 1) throw internal_error();
				path.back() = pos;

				// get value index.
				auto value_index = it->v.index();

				if (value_index == 3) {
					// child : object.
					child.it  = std::get<3>(it->v).begin();
					child.end = std::get<3>(it->v).end();
					child.pos = 0;

					if (child.it == child.end) {
						// empty child.
						// call it.
						if (not (cb)(path, (*it).v)) break;
					} else {
						// push child.
						stack.emplace(child);
						path.emplace_back(std::get<0>(child.it)->first);
					}
				} else if (value_index == 4) {
					// child : array.
					child.it  = std::get<4>(it->v).begin();
					child.end = std::get<4>(it->v).end();
					child.pos = 0;

					if (child.it == child.end) {
						// empty child.
						// call it.
						if (not (cb)(path, (*it).v)) break;
					} else {
						// push child.
						stack.emplace(child);
						path.emplace_back(0);
					}
				} else {
					// call.
					// path, it.
					if (not (cb)(path, (*it).v)) break;
				}

				// goto next.
				it++;
				pos++;
			} else {
				throw internal_error();
			}
		}
	}

	// internal parse main function.
	// [in]      j                 : utf-8 text.
	// [out]     length            : processed character count.
	// [in]      cb_value          : callback value. nullptr(return full DOM), cb pointer(minimize memory passing only path and value).
	//                               [](const auto& path, auto& value) -> bool
	// [in]      cb_block          : callback block.
	//                               [](const auto& path, block_type type, const auto& pos) -> bool
	// [compile] skip_value_string : save more memory and time when want to understand the doc structure without using value.
	//
	// remark)
	//   - any callback(cb_value, cb_block)     used, run like SAX parser. minimize memory passing only path and value.
	//   - all callback(cb_value, cb_block) not used, run like DOM parser. return full memory DOM object.
	template <bool skip_value_string=false>
	static inline block parse_main(const char* j, size_t* length=nullptr, callback_value cb_value=nullptr, callback_block cb_block=nullptr) {
		const auto* c = j;	// c(=current character)
		int depth = 0;
		bool dom = true;
		bool cancel = false;
		seq s = seq::seq_object;
		size_t len = 0;
		std::string name;
		std::string value_string;
		value_container value;
		stack_object so;
		numeric num;
		std::stack<stack_object> stack;
		sax_object sax;
		const object       empty_object;
		const array        empty_array;
		const std::string  empty_string;
		
		if (not j) throw invalid_parameter("");

		// when callback used, turn off dom.
		if (cb_value) dom = false;
		if (cb_block) dom = false;

		// pass preceeding white spaces.
		pass_white_space(&c);

		for (;;c++) {
			if (*c == '\0') break;

			if (s == seq::seq_obj_pre) {
				name.clear();
				pass_white_space(&c);
				if (*c == '\"') {
					s = seq::seq_obj_in;
					continue;
				}
				if (*c == '}') {
					// only empty object.

					if (dom) {
						// check empty.
						if (stack.empty()) throw parse_error("");
						if (0 != stack.top().value.index()) {
							// current is not object.
							throw parse_error("");
						}

						if (not std::get<0>(stack.top().value).empty()) {
							// not empty.
							// called here when ...{'a':1,}.
							//                           ^ error
							throw parse_error("");
						}
					} else {
						// check empty.
						if (sax.path.empty()) throw parse_error("");
						if (sax.path.size() != sax.count.size()) throw internal_error();
						if (0 != sax.path.back().index()) {
							// current is not object.
							throw parse_error("");
						}

						if (sax.count.back() != 0) {
							// not empty.
							// called here when ...{'a':1,}.
							//                           ^ error
							throw parse_error("");
						}
					}

					s = seq::seq_value_obj;
					// keep going.
				} else {
					throw parse_error("");
				}
			}

			if (s == seq::seq_obj_in) {
				c = unescape(c, &name);
				s = seq::seq_obj_post;
				if (s == seq::seq_obj_post) continue;
				throw parse_error("");
			}

			if (s == seq::seq_obj_post) {
				pass_white_space(&c);
				if (*c == ':') {
					s = seq::seq_value_pre;
					continue;
				} else {
					throw parse_error("");
				}
			}

			if (s == seq::seq_value_pre) {
				pass_white_space(&c);

				// check value type.
				if (*c == '\"') {
					s = seq::seq_value_in;
					value_string.clear();
					continue;
				} else if (*c == '{') {
					s = seq::seq_value_obj;
					// do not continue, keep going.
				} else if (*c == '[') {
					s = seq::seq_value_obj;
					// do not continue, keep going.
				} else if (*c == ']') {
					// only empty array.

					if (dom) {
						// check empty.
						if (stack.empty()) throw parse_error("");
						if (1 != stack.top().value.index()) {
							// current is not array.
							throw parse_error("");
						}

						if (not std::get<1>(stack.top().value).empty()) {
							// not empty.
							// called here when ...[1,2,].
							//                         ^ error
							throw parse_error("");
						}
					} else {
						// check empty.
						if (sax.path.empty()) throw parse_error("");
						if (sax.path.size() != sax.count.size()) throw internal_error();
						if (1 != sax.path.back().index()) {
							// current is not array.
							throw parse_error("");
						}

						if (sax.count.back() != 0) {
							// not empty.
							// called here when ...[1,2,].
							//                         ^ error
							throw parse_error("");
						}
					}

					s = seq::seq_value_obj;
					// keep going...
				} else {
					if (*c == 't') {
						if (0 == strncmp(c, "true", 4)) {
							value.v = true;
							c += 3;
							s = seq::seq_value_set;
							continue;
						}
					} else if (*c == 'n') {
						if (0 == strncmp(c, "null", 4)) {
							value.v = {};	// set to monospace.
							c += 3;
							s = seq::seq_value_set;
							continue;
						}
					} else if (*c == 'f') {
						if (0 == strncmp(c, "false", 5)) {
							value.v = false;
							c += 4;
							s = seq::seq_value_set;
							continue;
						}
					} else if (to_numeric(c, len, num)) {
						if (len == 0) throw parse_error("");
						value.v = num;
						c += len - 1;
						s = seq::seq_value_set;
						continue;
					}
					throw parse_error("");
				}
			}

			if (s == seq::seq_value_in) {
				if constexpr (skip_value_string) {
					c = unescape(c, nullptr);
					value.v = empty_string;
				} else {
					c = unescape(c, &value_string);
					value.v = std::move(value_string);
				}
				s = seq::seq_value_set;
				if (s == seq::seq_value_set) continue;
				throw parse_error("");
			}

			if (s == seq::seq_value_set) {
				// it's time to set value.
				if (dom) {
					if (stack.empty()) throw parse_error("");
					auto index = stack.top().value.index();
					if (index == 0) {
						// in object.
						// set (name,value).
						std::get<0>(stack.top().value)[name] = std::move(value);
					} else if (index == 1) {
						// in array.
						// append (value).
						std::get<1>(stack.top().value).emplace_back(std::move(value));
					} else {
						throw parse_error("");
					}
				} else {
					if (sax.path.empty()) throw parse_error("");
					if (sax.path.size() != sax.count.size()) throw internal_error();
					auto index = sax.path.back().index();
					// set (value).
					sax.value = std::move(value);
					if (index == 0) {
						// in object.
						sax.path.back() = std::move(name);
					} else if (index == 1) {
						// in array.
						sax.path[sax.path.size()-1] = sax.count.back();
					} else {
						throw internal_error();
					}

					// call cb.
					if ((cb_value) and (not (cb_value)(sax.path, sax.value.v))) {
						cancel = true;
						break;
					}

					sax.count.back()++;
				}

				// since `seq_value_set` is an event that does not assume current movement.
				// it is move back one for next loop c++.
				c--;
				s = seq::seq_value_post;
				continue;
			}

			if (s == seq::seq_value_post) {
				pass_white_space(&c);
				if (*c == ',') {
					if (dom) {
						if (stack.empty()) throw parse_error("");
						auto index = stack.top().value.index();
						if (index == 0) {
							// in object.
							s = seq::seq_obj_pre;
						} else if (index == 1) {
							// in array.
							s = seq::seq_value_pre;
						} else {
							throw internal_error();
						}
					} else {
						if (sax.path.empty()) throw parse_error("");
						auto index = sax.path.back().index();
						if (index == 0) {
							// in object.
							s = seq::seq_obj_pre;
						} else if (index == 1) {
							// in array.
							s = seq::seq_value_pre;
						} else {
							throw internal_error();
						}
					}
					continue;
				} else if (*c == '}') {
					// keep going.
					// do `depth processing`
				} else if (*c == ']') {
					// keep going.
					// do `depth processing`
				} else {
					throw parse_error("");
				}
			}

			// depth processing here.

			if (*c == '{') {
				depth++;
				if (dom) {
					so.value = empty_object;
					so.name = std::move(name);
					stack.emplace(std::move(so));
				} else {
					if (not sax.path.empty()) {
						auto index = sax.path.back().index();
						if (index == 0) {
							sax.path.back() = std::move(name);
						} else if (index == 1) {
							sax.path.back() = sax.count.back();
							sax.count.back()++;
						} else {
							throw parse_error("");
						}
					}

					// callback
					// new object block started.
					if ((cb_block) and (not sax.path.empty()) and (not (cb_block)(sax.path, block_type::block_type_object, c))) {
						cancel = true;
						break;
					}

					sax.path.emplace_back(empty_string);
					sax.count.push_back(0);
				}
				s = seq::seq_obj_pre;
				continue;
			}
			else if (*c == '[') {
				depth++;
				if (dom) {
					so.value = empty_array;
					so.name = std::move(name);
					stack.emplace(std::move(so));
				} else {
					if (not sax.path.empty()) {
						auto index = sax.path.back().index();
						if (index == 0) {
							// parent is object.
							sax.path.back() = std::move(name);
							sax.count.back()++;
						} else if (index == 1) {
							// parent is array.
							sax.path.back() = sax.count.back();
							sax.count.back()++;
						} else {
							throw parse_error("");
						}
					}

					// callback
					// new array block started.
					if ((cb_block) and (not sax.path.empty()) and (not (cb_block)(sax.path, block_type::block_type_array, c))) {
						cancel = true;
						break;
					}

					sax.value.v = {};
					sax.path.emplace_back(0);
					sax.count.push_back(0);
				}
				s = seq::seq_value_pre;

				continue;
			}
			else if (*c == '}') {
				depth--;

				if (dom) {
					// completed.
					if (stack.size() == 1) break;

					if (stack.empty()) throw parse_error("");
					auto current = std::move(stack.top());
					auto index = current.value.index();
					std::string current_name;

					if (index == 0) {
						// in object.
						// good.
					} else if (index == 1) {
						// current block is array(start with `[`), but end with `}`.
						throw parse_error("");
					} else {
						throw internal_error();
					}

					// get current object, and stack pop.
					auto& current_object = std::get<0>(current.value);
					current_name = std::move(current.name);
					stack.pop();

					// get parent.
					if (stack.empty()) throw parse_error("");
					auto& parent = stack.top();
					auto  parent_index = parent.value.index();
					if (parent_index == 0) {
						// parent is in object.
						// set (key, value).
						std::get<0>(parent.value)[current_name].v = std::move(current_object);
					} else if (parent_index == 1) {
						// parent is in array.
						// append (value).
						value.v = std::move(current_object);
						std::get<1>(parent.value).emplace_back(std::move(value));
					} else {
						throw internal_error();
					}

					// if the object is empty, seq has not been changed yet. so chage it.
					if (current_object.empty()) {
						s = seq::seq_value_post;
					}
				} else {
					// completed.
					if (sax.path.size() == 1) break;
					
					if (sax.path.empty()) throw parse_error("");
					if (sax.path.size() != sax.count.size()) throw internal_error();

					auto index = sax.path.back().index();
					if (index == 0) {
						// in object.
						// good.
					} else if (index == 1) {
						// current block is array(start with `[`), but end with `}`.
						throw parse_error("");
					} else {
						throw internal_error();
					}

					if (sax.count.back() == 0) {
						// empty object?
						
						// call cb.
						sax.value.v = empty_object;
						if ((cb_value) and (not (cb_value)(sax.path, sax.value.v))) {
							cancel = true;
							break;
						}

						// if the object is empty, seq has not been changed yet. so chage it.
						s = seq::seq_value_post;
					}

					sax.path.pop_back();
					sax.count.pop_back();
				}
			}
			else if (*c == ']') {
				depth--;

				if (dom) {
					// completed.
					if (stack.size() == 1) break;

					if (stack.empty()) throw parse_error("");
					auto current = std::move(stack.top());
					auto index = current.value.index();
					std::string current_name;

					if (index == 0) {
						// current block is object(start with `{`), but end with `]`.
						throw parse_error("");
					} else if (index == 1) {
						// in array.
						// good.
					} else {
						throw internal_error();
					}

					// get current array, and stack pop.
					auto& current_array = std::get<1>(current.value);
					current_name = std::move(current.name);
					stack.pop();

					// get parent.
					if (stack.empty()) throw parse_error("");
					auto& parent = stack.top();
					auto  parent_index = parent.value.index();
					if (parent_index == 0) {
						// parent is in object.
						// set (key, value).
						std::get<0>(parent.value)[current_name].v = std::move(current_array);
					} else if (parent_index == 1) {
						// parent is in array.
						// append (value).
						value.v = std::move(current_array);
						std::get<1>(parent.value).emplace_back(std::move(value));
					} else {
						throw internal_error();
					}

					// if the array is empty, seq has not been changed yet. so chage it.
					if (current_array.empty()) {
						s = seq::seq_value_post;
					}
				} else {
					// completed.
					if (sax.path.size() == 1) break;

					if (sax.path.empty()) throw parse_error("");
					if (sax.path.size() != sax.count.size()) throw internal_error();

					auto index = sax.path.back().index();
					if (index == 0) {
						// current block is object(start with `{`), but end with `]`.
						throw parse_error("");
					} else if (index == 1) {
						// in array.
						// good.
					} else {
						throw internal_error();
					}

					if (sax.count.back() == 0) {
						// empty object?

						// call cb.
						sax.value.v = array();
						if ((cb_value) and (not (cb_value)(sax.path, sax.value.v))) {
							cancel = true;
							break;
						}

						// if the array is empty, seq has not been changed yet. so chage it.
						s = seq::seq_value_post;
					}

					sax.path.pop_back();
					sax.count.pop_back();
				}
			} else {
				throw parse_error("");
			}
		}

		// cb canceled?
		if (cancel) {
			// return empty.
			return {};
		}

		// check depth.
		if (depth != 0) {
			throw parse_error("");
		}

		// get length.
		if (length) {
			*length = c - j + 1;
		}

		if (dom) {
			// return only one root item.
			// check final stack.
			if (stack.size() != 1) {
				throw internal_error();
			}

			// get root.
			if (stack.top().value.index() == 0) {
				auto& obj = std::get<0>(stack.top().value);
				return obj;
			}

			if (stack.top().value.index() == 1) {
				auto& arr = std::get<1>(stack.top().value);
				return arr;
			}
		} else {
			// check final stack.
			if ((sax.path.size() != 1) or (sax.count.size() != 1)) {
				throw internal_error();
			}

			// return empty.
			return {};
		}

		throw internal_error();
		return {};
	}
};

} // namespace common
} // namespace gaenari

#endif // HEADER_JSON_HPP

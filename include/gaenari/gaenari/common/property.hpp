#ifndef HEADER_GAENARI_GAENARI_COMMON_PROPERTY_HPP
#define HEADER_GAENARI_GAENARI_COMMON_PROPERTY_HPP

namespace gaenari {
namespace common {

// property file management.
//
// property file format
//
// # comment
// key1 = value1
// key2 = value2
// key3=value3
// ...
//
// * key is case-sensitive.
// * comment is start with '#'.
// * comment in front of the key is the comment of the corresponding key.
//   ex)
//   ...
//   money = 3
//   # min count is 1.      => (1)
//   # max count is 100.    => (2)
//   count = 30
//   `count` comment is (1) + \r\n + (2) + \r\n
// * multi-line is not supported.
// * supported data type is,
//     - string
//     - int
//     - int64_t
//     - double
//     - bool (true: 1, "true", "yes" / false: 0, "false", "no" / others)
//
// api
//
// void clear()
//   ; clear all data.
// void empty()
//   ; check empty.
// string path()
//   ; return path.
// bool read(path, set_comment[false])
//   ; read property file to cache.
//   ; set_comment - read comment contents to memory.
//                   set to false when read only mode.
//                   if you want to save while keeping comments, pass true.
//   ; return true(success), false(fail)
// value get(name, def)
//   ; get name's value.
//   ; the return type depends on the `def` type.
//   ; return value is the copy of cache.
// value& get(name, existed[nullptr])
//   ; get name's string value.
//   ; return value is the const reference of cache.
// 3-state get_safe_bool(name, def)
//   ; get bool with safety.
//   ; return 3-state value. (true, false, other).
//   ; if key name is not found, return def.
// bool reload(void)
//   ; call read(...) again.
//   ; return true(success), false(fail)
// bool all_keys_existed(names)
//   ; check if all keys are existed.
// void set_comment(name, comment)
//   ; set key's comment.
//   ; set empty comment to erase.
// string get_comment(name)
//   ; get key's comment.
//   ; if key is not found, return empty string.
// void set(name, value, comment[null])
//   ; add/update cache (name = value).
//   ; can pass with comment.
// void set_move(name, value:string, comment[null])
//   ; add/update cache (name = value).
//   ; value:string is moved to cache. the value will be empty.
//     it's used when the value is not needed after the call.
//   ; can pass with comment.
// void erase(name)
//   ; remove cache (name).
// void set_default(arr(name,value,comment))
//   ; multiple (name,value,comment).
//   ; set (name,value,comment) to cache if name is not found.
//   ; value type is const char*.
//     comment can nullptr.
//   ; ex) {{"money","1",nullptr}, {"married","false","is optional"}}
// bool save(path[nullopt])
//   ; save cache to file.
//   ; delete existing data.
//   ; save()     : save cache to opened file.
//     save(path) : save cache to new file.
//   ; return true(success), false(fail)

struct prop {
	prop()  = default;
	~prop() = default;

	struct value {
		// values.
		// they're not union.
		// you can choose one of them.
		int				    i   = 0;
		int64_t			    i64 = 0;
		double				d   = 0.0;
		std::optional<bool> b   = false; // "true", "yes", "1" => true / "false", "no", "0" => false / others => std::nullopt
		std::string s;

		// misc.
		bool arithmetic = false;

		// clear.
		void clear(void) {
			i   = 0;
			i64 = 0;
			d   = 0.0;
			b.reset();
			s.clear();
			arithmetic = false;
		}

		// set value.
		// v will be empty after call.
		void set_move(_in _out std::string& v) {
			const char* c = nullptr;
			char* e = nullptr;
			double d = 0;
			int i = 0;
			int64_t i64 = 0;

			// clear.
			clear();

			// check v is numeric?
			c = v.c_str();
			d = strtod(c, &e);
			if (c == e) {
				// it's string.
				if (common::strcmp_ignore_case(v,      "true"))  this->b = true;
				else if (common::strcmp_ignore_case(v, "yes"))   this->b = true;
				else if (common::strcmp_ignore_case(v, "false")) this->b = false;
				else if (common::strcmp_ignore_case(v, "no"))    this->b = false;
				else if (common::strcmp_ignore_case(v, ""))      this->b = false;
				this->s = std::move(v);
				return;
			}

			// it's numeric.
			this->arithmetic = true;
			i = static_cast<int>(d);
			if (static_cast<double>(i) == d) {
				// it's int.
				this->i = i;
				this->i64 = static_cast<int64_t>(i);
				this->d = static_cast<double>(i);
				if (i == 1) this->b = true;
				else if (i == 0) this->b = false;
				this->s = std::move(v);
				return;
			} 

			i64 = static_cast<int64_t>(d);
			if (static_cast<double>(i64) == d) {
				// it's int64.
				this->i64 = i64;
				this->i = static_cast<int>(i64);
				this->d = static_cast<double>(i64);
				this->b = static_cast<bool>(i64);
				this->s = std::move(v);
				return;
			} 

			// it's double.
			this->d = d;
			this->i = static_cast<int>(d);
			this->i64 = static_cast<int64_t>(d);
			this->b = static_cast<bool>(d);
			this->s = std::move(v);
			return;
		}

		void set(_in const std::string& v) {
			std::string s = v;
			set_move(s);
		}
	};

	struct set_default_type {
		const char* name    = nullptr;
		const char* value   = nullptr;
		const char* comment = nullptr;
	};

public:
	void clear() { 
		p.clear();
		_path.clear();
		comments.clear();
		footer_comment.clear();
	}
	bool empty() const { return p.empty(); }
	const std::string& path(void) const { return _path; }

	// read property file.
	//   - set_comments
	//     true if wants save comments to memory.
	//     set to false when only reading a value.
	//     in case of saving, comments in the existing file can be removed.
	//     if you want to save while keeping comments, pass true.
	//
	// <file>.properties format
	//
	// # comment
	// # (`key`,value)
	// key = value
	// 
	// # (`second key`,value)
	// second key = value
	//
	// remark)
	// - does not support inline comment.
	//   value = hello # world
	//   (`value`, `hello # world`)
	//
	// prop properties;
	// prop.read("C:/p.properites");
	// prop.reload();
	// auto& v = prop.get("key_int", 3);
	// auto& s = prop.get("key_str", "hello");
	// bool c = prop.key_existed("key_name");
	bool read(_in const std::string& path, _option _in bool set_comments=false) try {
		std::fstream file;
		std::string line;
		std::string k;
		std::string v;
		type::variant var;
		value val;
		std::string comment;

		// if the lock name is a property file name, it is more optimized.
		// however, in the case of linux, there is an issue that the lock file(/tmp/glock/*.lck) increases, so use the same name.
		common::lock_guard_named_mutex lock("property");

		// clear.
		clear();

		// set path.
		_path = path;

		// open.
		if (path.empty()) THROW_GAENARI_INTERNAL_ERROR0;
		file.open(path, std::fstream::in);
		if (not file) THROW_GAENARI_INTERNAL_ERROR0;
		if (file.fail()) THROW_GAENARI_INTERNAL_ERROR0;

		for (;;) {
			if (file.eof()) break;
			std::getline(file, line);
			if (not split(line, k, v)) {
				if (set_comments) comment += line + "\r\n";
				continue;
			}

			// build value from string v.
			// v is move to val, v will be empty.
			val.set(v);

			// set property.
			p[k] = std::move(val);

			// comments
			if (not comment.empty()) comments[k] = std::move(comment);
		}
		if (not comment.empty()) {
			// remove last \r\n.
			comment.pop_back(); comment.pop_back();
			footer_comment = std::move(comment);
		}
		return true;
	} catch(...) {
		return false;
	}

	// returns a value with the same `def` type.
	// T: int, bool, double, int64_t, std::string.
	// auto v = p.get("count", 0);
	//      - ==> v is integer.
	// remark)
	// numeric requests for string values returns 0, not def.
	// ex)
	// name=hello,world
	// auto f = p.get("name", 3.14159);
	// printf("%f", f) => 0.0
	template<typename T>
	const T get(_in const std::string& name, _in const T& def) const {
		auto find = p.find(name);
		if (find == p.end()) return def;

		if constexpr(std::is_integral<T>::value) {
			if constexpr(sizeof(T) == sizeof(int64_t)) {
				return find->second.i64;
			} else if constexpr(std::is_same<T, bool>::value) {
				return find->second.b;
			} else { 
				return find->second.i;
			}
		} else if constexpr(std::is_floating_point<T>::value) {
			return find->second.d;
		} else {
			static_assert(type::dependent_false_v<T>, "invalid type!");
		}

		return T();
	}

	// returns : true, false.
	// if the value type is not bool("hello", "world", 3.14, ...), returns def.
	bool get(_in const std::string& name, _in bool def) const {
		auto v = get_safe_bool(name, def);
		if (not v.has_value()) return def;
		return v.value();
	}

	// call get(name, existed) to get the reference string.
	// this function returns a copy.
	std::string get(_in const std::string& name, _in const char* def) const {
		std::string d{def};
		return get(name, d);
	}

	// call get(name, existed) to get the reference string.
	// this function returns a copy.
	const std::string get(_in const std::string& name, _in const std::string& def) const {
		auto find = p.find(name);
		if (find == p.end()) return def;
		return find->second.s;
	}

	// returns : true("true","yes",1), false("false","no",0), or nullopt.
	std::optional<bool> get_safe_bool(_in const std::string& name, _in bool def) const {
		auto find = p.find(name);
		if (find == p.end()) return def;
		return find->second.b;
	}

	// return reference string.
	//  - if name is not found, return empty string.
	//    existed is not null, *existed = false.
	//  - if name is found, return string.
	//    auto& v = get("name");
	const std::string& get(_in const std::string& name, _option _out bool* existed=nullptr) const {
		auto find = p.find(name);
		if (find == p.end()) {
			if (existed) *existed = false;
			return empty_string;
		}
		if (existed) *existed = true;
		return find->second.s;
	}

	bool reload(void) {
		std::string p = this->_path;
		return read(p, true);
	}

	bool all_keys_existed(_in const std::vector<std::string>& names) const {
		for (const auto& name: names) if (p.find(name) == p.end()) return false;
		return true;
	}

	// to delete comment, pass empty comment.
	void set_comment(_in const std::string& name, _in const std::string& comment) {
		std::string mark = "";
		auto find = p.find(name);
		if (find == p.end()) THROW_GAENARI_ERROR("invalid name: " + name);
		if (comment.empty()) {
			comments.erase(name);
			return;
		}
		if (comment[0] != '#') mark = "# ";
		comments[name] = mark + comment + "\r\n";
	}

	const std::string& get_comment(_in const std::string& name) const {
		auto find = comments.find(name);
		if (find == comments.end()) return empty_string;
		return find->second;
	}

	template <typename T>
	void set(_in const std::string& name, _in const T& v, _option_in const char* comment=nullptr) {
		value val;
		if constexpr(std::is_same<T, bool>::value) {
			if (v) val.set("true");
			else   val.set("false");
		} else {
			val.set(std::to_string(v));
		}
		p[name] = std::move(val);
		if (comment) set_comment(name, comment);
	}

	void set(_in const std::string& name, _in const std::string& v, _option_in const char* comment=nullptr) {
		value val;
		val.set(v);
		p[name] = std::move(val);
		if (comment) set_comment(name, comment);
	}

	void set(_in const std::string& name, _in const char* v, _option_in const char* comment=nullptr) {
		set(name, std::string(v), comment);
	}

	// v will be empty after call.
	// use this when v is not needed after call.
	void set_move(_in const std::string& name, _in _out std::string& v, _option_in const char* comment=nullptr) {
		value val;
		val.set_move(v);
		p[name] = std::move(val);
		if (comment) set_comment(name, comment);
	}

	void erase(_in const std::string& name) {
		p.erase(name);
		comments.erase(name);
	}

	// set defult (name=value, comment) sets.
	// set when name is not found.
	// ex)
	// p.set_default_type({{"money","1",nullptr}, {"married","false","is optional"}});
	// set(name,value,comment) when money or married not found.
	void set_default(_in const std::vector<set_default_type>& defaults) {
		for (const auto& def: defaults) {
			if ((not def.name) or (not def.value)) THROW_GAENARI_INTERNAL_ERROR0;
			auto find = p.find(def.name);
			if (find != p.end()) continue;
			set(def.name, def.value, def.comment);
		}
	}

	// save cache to file.
	// delete existing data.
	// path
	//   - nullopt     : save cache to opened file.
	//   - std::string : save cache to new file.
	bool save(_in const std::optional<std::string>& path=std::nullopt) const try {
		std::string read_path;
		std::fstream file;
		common::lock_guard_named_mutex lock("property");

		if (empty()) return false;
		if (path == std::nullopt) read_path = _path;
		else read_path = path.value();
		if (read_path.empty()) return false;
		file.open(read_path, std::ios::out | std::ios::trunc | std::ios::binary);
		for (const auto& it: p) {
			auto comment = comments.find(it.first);
			if (comment != comments.end()) file << comment->second;
			file << it.first << " = " << it.second.s << '\n';
		}
		if (not footer_comment.empty()) file << footer_comment;
		return true;
	} catch(...) {
		return false;
	}

protected:
	bool split(_in std::string& line, _out std::string& key, _out std::string& value) const {
		common::trim_both(line, " \t");
		if (line.empty()) return false;
		if (line[0] == '#') return false;
		auto find = line.find('=');
		if (std::string::npos == find) return false;
		value = &line[find+1];
		key = std::move(line);
		key.erase(find, key.length() - find);
		common::trim_right(key, " \t");
		common::trim_left(value, " \t");
		return true;
	}

protected:
	insert_order_map<std::string, value> p;
	std::string _path;
	const std::string empty_string;
	std::map<std::string, std::string> comments;
	std::string footer_comment;
};

} // namespace common
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_PROPERTY_HPP

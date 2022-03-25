#ifndef HEADER_GAENARI_GAENARI_COMMON_MISC_HPP
#define HEADER_GAENARI_GAENARI_COMMON_MISC_HPP

namespace gaenari {
namespace common {

// "17.000000"     -> "17"
// "171474.457100" -> "171474.4571"
inline std::string dbl_to_string(_in double v) {
	std::string ret = std::to_string(v);
	ret.erase(ret.find_last_not_of("0") + 1);
	if (ret.back() == '.') ret.pop_back();
	return ret;
}

inline int atoi_ex(_in const char* s, _out size_t& len) {
	int r = 0;
	for (len=0; s[len] != '\0'; len++) {
		if (('0' <= s[len]) and (s[len] <= '9')) {
			// number.
			r = r * 10 + s[len] - '0';
			continue;
		}
		break;
	}
	return r;
}

// format message.
// r = f("[%0][%1][%0][%%1][%9]", {1, "hello"}) => "[1][hello][1][%1][%9]"
inline std::string f(_in const std::string& fmt, _in const std::vector<std::variant<std::monostate,int,unsigned int,int64_t,size_t,double,std::string>>& params) {
	const char* text = nullptr;
	size_t i = 0;
	size_t len = 0;
	size_t variant_index = 0;
	size_t tag_length = 0;
	int tag_index = 0;
	std::string r;

	if (fmt.empty()) return "";

	text = fmt.c_str();
	len  = fmt.length();
	for (i=0; i<len; i++) {
		tag_index = -1;
		if ((text[i] == '%') and (text[i+1] != '%')) {
			tag_index = atoi_ex(&text[i+1], tag_length);
			if ((tag_index < 0) or (tag_index >= (int)params.size())) {
				// out of tag_index.
				tag_index = -1;
			}

			if (tag_length == 0) {
				// format error.
				tag_index = -1;
			}
		} else if ((text[i] == '%') and (text[i+1] == '%')) {
			r += "%";
			i += 1;
			continue;
		}

		if (tag_index >= 0) {
			// tag found.
			variant_index = params[tag_index].index();
			try {
				if      (1 == variant_index) r += std::to_string(std::get<1>(params[tag_index]));	// <int>
				else if (2 == variant_index) r += std::to_string(std::get<2>(params[tag_index]));	// <unsigned int>
				else if (3 == variant_index) r += std::to_string(std::get<3>(params[tag_index]));	// <int64_t>
				else if (4 == variant_index) r += std::to_string(std::get<4>(params[tag_index]));	// <size_t>
				else if (5 == variant_index) r += dbl_to_string	(std::get<5>(params[tag_index]));	// <float>   9.900000 -> 9.9
				else if (6 == variant_index) r +=				 std::get<6>(params[tag_index]);	// <string>
			} catch (const std::bad_variant_access&) {}

			// next.
			i += tag_length;
		} else {
			r += text[i];
		}
	}

	return r;
}

template<typename K, typename COUNTABLE>
inline COUNTABLE map_count(_in const std::map<K,COUNTABLE>& m) {
	COUNTABLE ret = 0;
	for (const auto& it: m) ret += it.second;
	return ret;
}

template<typename K,typename V>
inline const V& map_find(const std::map<K,V>& m, const K& k, const V& def) {
	const auto& f = m.find(k);
	if (m.end() == f) return def;
	return f->second;
}

template<typename M>
inline const typename M::mapped_type& map_find(const M& m, const typename M::key_type& k) {
	const auto& f = m.find(k);
	if (m.end() == f) THROW_GAENARI_INVALID_PARAMETER("key is not found.");
	return f->second;
}

template<typename M>
inline std::optional<std::reference_wrapper<const typename M::mapped_type>> map_find_noexcept(const M& m, const typename M::key_type& k) noexcept {
	try {
		const auto& f = m.find(k);
		if (m.end() == f) return std::nullopt;
		return f->second;
	} catch(...) {
		return std::nullopt;
	}
}

template<typename M>
inline const typename M::mapped_type& map_find_with_def(const M& m, const typename M::key_type& k, const typename M::mapped_type& def) {
	const auto& f = m.find(k);
	if (m.end() == f) return def;
	return f->second;
}

inline bool is_numeric(_in dataset::data_type_t t) {
	if (t == dataset::data_type_t::data_type_int)    return true;
	if (t == dataset::data_type_t::data_type_int64)  return true;
	if (t == dataset::data_type_t::data_type_double) return true;
	return false;
}

inline bool is_nominal(_in dataset::data_type_t t) {
	if (t == dataset::data_type_t::data_type_string) return true;
	if (t == dataset::data_type_t::data_type_string_table) return true;
	return false;
}

// build fixed length string
// - src          : source string
// - width        : string max width
// - padding      : default \0 : no padding. string.length() <= width.
//                  ' ', ...   : use padding. force to string.length() = width.
// - margin_right : right margin with padding characters. if padding = '\0', no margin applied.
// - margin_left  : left  margin with padding characters. if padding = '\0', no margin applied.
// - ellipsis     : default false : no  ellipsis. just cut string. abcdefg -> abcd
//                  true          : use ellipsis. cut with `...`. abcdefg -> abcd... (* mininum 4 character needed)
// - align_right  : horizental right align(default false, left align). if padding = '\0', right align is not supported.
inline std::string fixed_length(_in const std::string& str, _in size_t width, _option_in char padding='\0', _in size_t margin_right=0, _in size_t margin_left=0, _in bool ellipsis=false, _in bool align_right=false) {
	std::string r;

	if (padding == '\0') {
		// no padding char, no margin.
		margin_left  = 0;
		margin_right = 0;
		r = str;
	} else {
		std::string align_padding;
		if (align_right) {
			int spaces = static_cast<int>(width) - static_cast<int>(margin_right) - static_cast<int>(margin_left) - static_cast<int>(str.length());
			if (spaces > 0) align_padding = std::string(spaces, padding);
		}
		r = std::string(margin_left, padding) + align_padding + str;
	}

	if (width <= margin_left + margin_right) {
		return "";
	}

	if (r.length() + margin_right <= width) {
		if (padding == '\0') return r;
		return r + std::string(width - r.length(), padding);
	}

	r = r.substr(0, width - margin_right);
	if (ellipsis) {
		if (r.length() >= 6 + margin_right) {
			r = r.substr(0, r.length() - 3) + "...";
		}
	}

	if (padding != '\0') {
		r += std::string(margin_right, padding);
	}
	return r;
}

// random shuffle values.
inline std::vector<size_t> random_shuffle(_in size_t count, _in int seed = 0, _in size_t start = 0) {
	std::vector<size_t> v(count);
	std::iota(v.begin(), v.end(), start);
	std::mt19937_64 g(seed);
	std::shuffle(v.begin(), v.end(), g);
	return v;
}

// random shuffle values with split.
inline std::pair<std::vector<size_t>,std::vector<size_t>> random_shuffle_with_split(_in size_t count, _in double second_ratio, _in bool sort = true, _in int seed = 0, _in size_t start = 0) {
	size_t cut_count = 0;
	std::vector<size_t> first;
	std::vector<size_t> second;

	if ((second_ratio <= 0.0) or (second_ratio >= 1.0)) {
		THROW_GAENARI_INVALID_PARAMETER("invalid random_shuffle_with_split second_range: " + std::to_string(second_ratio));
		return {};
	}

	// get random shuffle
	second = random_shuffle(count, seed, start);

	// cut position
	cut_count = static_cast<size_t>(static_cast<double>(count) * second_ratio);
	if (cut_count == 0) cut_count = 1;

	// second
	std::copy(second.begin() + cut_count, second.end(), std::back_inserter(first));

	// remove
	second.erase(second.begin() + cut_count, second.end());

	// sort
	if (sort) {
		std::sort(first.begin(),  first.end());
		std::sort(second.begin(), second.end());
	}

	return std::make_pair(first,second);
}

template <typename T>
inline std::vector<T> remove_duplicated(_in const std::vector<T> s) {
	std::vector<T> r;
	std::set<T> d;

	for (const auto& i: s) {
		if (d.end() == d.find(i)) r.push_back(i);
		d.insert(i);
	}

	return r;
}

inline size_t max_element(const std::vector<double>& v, _out bool* error=nullptr) {
	size_t r = 0;
	size_t i = 0;
	bool _error = true;
	auto max = std::numeric_limits<double>::lowest();
	for (auto& val: v) {
		if ((not std::isnan(val)) and (val > max)) {
			max = val;
			r = i;
			_error = false;
		}
		i++;
	}
	if (error) *error = _error;
	return r;
}

// https://www.omnicalculator.com/statistics/confidence-interval
static inline double _get_z_score(_in int confidence_level_percent) {
	const static std::map<int,double> m={
		{0,0},{1,0.012534},{2,0.025069},{3,0.037609},{4,0.050153},{5,0.062707},{6,0.07527},{7,0.087844},{8,0.100434},{9,0.113039},{10,0.125661},
		{11,0.138304},{12,0.150969},{13,0.163659},{14,0.176374},{15,0.189118},{16,0.201893},{17,0.214702},{18,0.227545},{19,0.240426},{20,0.253347},
		{21,0.266311},{22,0.279319},{23,0.292375},{24,0.305481},{25,0.31864},{26,0.331853},{27,0.345126},{28,0.358459},{29,0.371856},{30,0.385321},
		{31,0.398855},{32,0.412463},{33,0.426148},{34,0.439913},{35,0.453762},{36,0.467699},{37,0.481727},{38,0.495851},{39,0.510074},{40,0.524401},
  	    {41,0.538836},{42,0.553384},{43,0.568051},{44,0.582842},{45,0.59776},{46,0.612813},{47,0.628006},{48,0.643346},{49,0.658837},{50,0.67449},
		{51,0.690309},{52,0.706303},{53,0.722479},{54,0.738847},{55,0.755415},{56,0.772193},{57,0.789191},{58,0.806421},{59,0.823893},{60,0.841621},
		{61,0.859618},{62,0.877896},{63,0.896473},{64,0.915365},{65,0.934589},{66,0.954165},{67,0.974114},{68,0.994458},{69,1.015222},{70,1.036433},
		{71,1.058122},{72,1.080319},{73,1.103063},{74,1.126391},{75,1.15035},{76,1.174987},{77,1.200359},{78,1.226528},{79,1.253565},{80,1.281551},
		{81,1.310579},{82,1.340755},{83,1.372203},{84,1.405072},{85,1.439531},{86,1.475791},{87,1.514102},{88,1.554774},{89,1.598193},{90,1.644854},
		{91,1.695397},{92,1.750686},{93,1.811911},{94,1.880794},{95,1.959964},{96,2.053749},{97,2.170091},{98,2.326348},{99,2.575829}
	};
	if (confidence_level_percent < 0)    THROW_GAENARI_INVALID_PARAMETER("invalid confidence_level_percent.");
	if (confidence_level_percent >= 100) THROW_GAENARI_INVALID_PARAMETER("invalid confidence_level_percent.");
	return m.find(confidence_level_percent)->second;
}

inline std::string& trim_right(_in _out std::string& s, _in const char* chars) {
	s.erase(s.find_last_not_of(chars)+1);
	return s;
}

inline std::string& trim_left(_in _out std::string&s, _in const char* chars) {
	s.erase(0, s.find_first_not_of(chars));
	return s;
}

inline std::string& trim_both(_in _out std::string&s, _in const char* chars) {
	trim_right(s, chars);
	trim_left(s, chars);
	return s;
}

inline std::string& string_replace(_in _out std::string& s, _in const std::string& before, _in const std::string& after) {
	std::string::size_type pos = 0;
	while ((pos = s.find(before, pos)) != std::string::npos) {
		s.replace(pos, before.length(), after);
		pos += after.length();
	}
	return s;
}

inline bool strcmp_ignore_case(_in const std::string& a, _in const std::string& b) {
	return std::equal(a.begin(), a.end(), b.begin(), b.end(), [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

inline std::string str_lower_const(_in const std::string& s) {
	std::string ret;
	auto length = s.length();
	ret.resize(length);
	for (size_t i=0; i<length; i++) ret[i] = std::tolower(s[i]);
	return ret;
}

inline std::string str_upper_const(_in const std::string& s) {
	std::string ret;
	auto length = s.length();
	ret.resize(length);
	for (size_t i=0; i<length; i++) ret[i] = std::toupper(s[i]);
	return ret;
}

inline std::string& str_lower(_in _out std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return std::tolower(c);});
	return s;
}

inline std::string& str_upper(_in _out std::string& s) {
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){return std::toupper(c);});
	return s;
}

inline void save_to_file(_in const std::string& file_path, _in const std::string& t) {
	std::ofstream f;
	f.open(file_path.c_str(), std::ios::binary);
	if (not f.is_open()) THROW_GAENARI_ERROR1("fail to open %0.", file_path);
	f.write(t.c_str(), t.length());
	if (f.fail()) THROW_GAENARI_ERROR1("fail to write %0.", file_path);
	f.close();
}

inline bool save_to_file_noexcept(_in const std::string& file_path, _in const std::string& t) noexcept try {
	save_to_file(file_path, t);
	return true;
} catch(...) {
	return false;
}

inline void read_from_file(_in const std::string& file_path, _out std::string& t) {
	std::ifstream f(file_path);
	if (not f.is_open()) return;
	f.seekg(0, std::ios::end);
	if (f.fail()) return;
	auto size = f.tellg();
	t.resize(size);
	f.seekg(0, std::ios::beg);
	if (f.fail()) return;
	f.read(&t[0], size);
	if (f.fail()) return;
	f.close();
}

inline bool read_from_file_noexcept(_in const std::string& file_path, _out std::string& t) noexcept try {
	read_from_file(file_path, t);
	return true;
} catch(...) {
	return false;
}

template <typename T>
inline std::string vec_to_string(_in const std::vector<T>& v) {
	std::string ret;
	if (v.empty()) return ret;
	for (const auto& i: v) ret += std::to_string(i) + ", ";
	ret.pop_back(); ret.pop_back();
	return ret;
}

inline std::string vec_to_string(_in const std::vector<std::string>& v) {
	std::string ret;
	if (v.empty()) return ret;
	for (const auto& i : v) ret += i + ", ";
	ret.pop_back(); ret.pop_back();
	return ret;
}

inline const std::string& version(void) {
	static const std::string v = GAENARI_VERSION;
	return v;
}

// the order of insert_order_map is not checked.
// a simple (key, value) pair matching check.
template<typename t1, typename t2>
inline bool compare_map_content(_in const t1& m1, _in const t2& m2) {
	if (m1.size() != m2.size()) return false;
	for (const auto& it: m1) {
		const auto& k = it.first;
		const auto& v = it.second;
		const auto& f = m2.find(k);
		if (f == m2.end()) return false;
		if (v != f->second) return false;
	}
	return true;
}

} // namespace common
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_COMMON_MISC_HPP

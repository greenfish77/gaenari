#ifndef HEADER_GAENARI_GAENARI_DATASET_CSV_HPP
#define HEADER_GAENARI_GAENARI_DATASET_CSV_HPP

namespace gaenari {
namespace dataset {

class line_reader {
public:
	line_reader() {
#ifdef _WIN32
		h = INVALID_HANDLE_VALUE;
#endif
	}
	~line_reader() {close_by_mmf(); close_by_stream();}

public:
	// under windows, to read line by mmf is much faster than by stream.
	// under linux, set use_win32_mmf as false automatically.
	inline bool open(_in const std::string& path, _in bool use_win32_mmf){
		close();
	#ifndef _WIN32
		use_win32_mmf = false;
	#endif
		this->use_win32_mmf = use_win32_mmf;
		if (this->use_win32_mmf) return open_by_mmf(path);
		return open_by_stream(path);
	}

	inline void close(void){
		eof_ = false;
		if (use_win32_mmf) return close_by_mmf();
		return close_by_stream();
	}

	// return false when eof, and line is meaningless
	inline bool readline(_out std::string& line){
		bool r;
		if (use_win32_mmf) r = readline_by_mmf(line);
		else r = readline_by_stream(line);
		if (not r) eof_ = true;
		return r;
	}

	inline bool eof(void) {return eof_;}
	inline size_t get_mmf_pos(void) { return mmf_first; }
	inline size_t get_mmf_size(void) { return size; }
	inline const void* get_mmf_data(void) { return data; }

protected:
	bool use_win32_mmf = false;
	bool eof_ = false;

protected:
	inline bool open_by_stream(_in const std::string& path) try {
		// close first
		close_by_stream();
		
		// open
		file.open(path, std::fstream::in);
		if (not file) THROW_GAENARI_INTERNAL_ERROR0;
		if (file.fail()) THROW_GAENARI_INTERNAL_ERROR0;
		return true;
	} catch (...) {
		return false;
	}

	inline bool readline_by_stream(_out std::string& line) try {
		// check eof
		if (file.eof()) return false;
		// getline
		std::getline(file, line);
		if (line.empty() and file.eof()) return false;
		return true;
	} catch (...) {
		return false;
	}

	inline void close_by_stream(void) {
		file.close();
	}

	std::fstream file;

protected:
	inline bool open_by_mmf(_in const std::string& path) {
#ifdef _WIN32
		bool ret = false;

		// close first
		close_by_mmf();

		h = ::CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (INVALID_HANDLE_VALUE == h) goto FINAL;

		size = ::GetFileSize(h, NULL);
		if (INVALID_FILE_SIZE == size) goto FINAL;

		m = ::CreateFileMappingA(h, NULL, PAGE_READONLY, 0, size, NULL);
		if (not m) goto FINAL;

		data = ::MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
		if (not data) goto FINAL;

		ret = true;

	FINAL:
		if (not ret) close_by_mmf();
		return ret;
#else
		return false;
#endif
	}

	inline bool readline_by_mmf(_out std::string& line) {
		if (not data) { line.clear(); return false; }
		std::string_view text((const char*)data, size);

		if (mmf_first < text.size()) {
			const auto mmf_second = text.find_first_of('\n', mmf_first);
			if (mmf_first != mmf_second) {
				line = text.substr(mmf_first, mmf_second - mmf_first);
				if (not line.empty() and (line.back() == '\r')) line.pop_back();
			}
			if (mmf_second == std::string_view::npos) { mmf_first = text.size(); return true; }
			mmf_first = mmf_second + 1;
		}
		else { line.clear(); return false; }

		return true;
	}

	inline void close_by_mmf(void) {
#ifdef _WIN32
		if (m) ::CloseHandle(m);
		if (h) ::CloseHandle(h);
#endif
		data = nullptr;
		m = nullptr;
#ifdef _WIN32
		h = INVALID_HANDLE_VALUE;
#else
		h = (void*)((size_t)0-1);
#endif
		mmf_first = 0;
	}
	void* h = (void*)((size_t)0-1);	// HANDLE, INVALID_HANDLE_VALUE
	void* m = nullptr;				// HANDLE
	unsigned long size = 0;			// DWORD
	void* data = nullptr;
	size_t mmf_first = 0;
};

struct csv_reader {
public:
	csv_reader() {};
	~csv_reader() {close();};

protected:
	line_reader file;
	char delim = ',';
	bool skip_parse_error_line = false;
	bool force_parse_error_line = false;
	size_t csv_row_index = 0;
	bool header_converted = false;
	std::string line;
	std::vector<std::string> header;
	std::map<std::string, size_t> header_index_map;
	std::vector<std::string> items;

public:
	inline bool open(const _in std::string& path, _in char delim, _option bool use_win32_mmf) try {
		size_t i = 0;

		// close first
		close();

		// read line file open
		if (not file.open(path, use_win32_mmf)) throw("fail to open: " + path);

		// read header
		if (not file.readline(line)) throw("fail to read line: " + path);

		// delimiter parsing
		parse_delim(line, delim, header);
		if (header.empty()) throw("empty csv header: " + path);

		// index map
		if (not build_header_map(header)) throw("");

		// delimeter set
		delim = delim;
		return true;
	} catch(...) {
		return false;
	}

	// set true : move to the next line without error when the number of headers and row columns are different.
	inline void set_skip_parse_error_line(_in bool set) {
		skip_parse_error_line = set;
	}

	// set true : set empty data when the number of headers and row columns are different.
	inline void set_force_parse_error_line(_in bool set) {
		force_parse_error_line = set;
	}

	// return false
	//   - eof   : meaningless line. you must break the loop.
	//   - error
	//
	// for (;;) {
	//     if (!c.move_next_row(nullptr)) break;
	//     c["h1"];
	// }
	//
	inline bool move_next_row(_option _out bool* err) try {
		// clear output
		if (err) *err = false;

		for (;;) {

			// read a line
			if (false == file.readline(line)) {
				if (file.eof()) return false;
				THROW_GAENARI_INTERNAL_ERROR0;
			}

			// line parse and verify if header not changed
			parse_delim(line, delim, items);
			if (not header_converted) {
				if (not verify_row_data()) THROW_GAENARI_INTERNAL_ERROR0;
			}
			break;
		}
		csv_row_index++;
		return true;
	} catch(...) {
		if (err) *err = true;
		return false;
	}

	inline bool verify_row_data(void) {
		bool ret = false;
		size_t i;
		size_t count;

		if (header_index_map.size() != items.size()) {
			if (header_converted) goto FINAL;

			// when the number of headers and row columns are different,
			// go next line
			if (skip_parse_error_line) goto FINAL;

			// when the number of headers and row columns are different,
			// stop.
			if (not force_parse_error_line) goto FINAL;

			// when the number of headers and row columns are different,
			// set empty data
			if (items.size() < header_index_map.size()) {
				count = header_index_map.size() - items.size();
				for (i=0; i<count; i++) items.push_back("");
			}
		}

		ret = true;

	FINAL:
		return ret;
	}

	// if caller changed header, 
	// caller must call build_header_map().
	// ex)
	// std::vector<std::string> header = csv.get_header();
	// ...
	// csv.build_header_map(header);
	inline auto get_header(void) {
		return header;
	}

	inline auto& get_header_ref(void) {
		return header;
	}

	inline auto& get_header_map_ref(void) {
		return header_index_map;
	}

	inline bool build_header_map(const _in std::vector<std::string>& header) {
		bool ret = false;
		size_t i = 0;

		// check header changed.
		// once header_converted set to true, never set to false.
		if (this->header != header) this->header_converted = true;

		this->header = header;
		header_index_map.clear();
		for (auto& it: this->header) {
			if (header_index_map.count(it) > 0) goto FINAL;
			// index map set
			header_index_map[it] = i++;
		}
		ret = true;
	FINAL:
		return ret;
	}

	inline const std::vector<std::string> get_row_data(void) {
		return items;
	}

	inline std::vector<std::string>& get_row_data_ref(void) {
		return items;
	}

	inline bool get_row_data(_option _out std::vector<std::string>* pvec, _option _out std::map<std::string, std::string>* pmap) try {
		std::map<std::string, size_t>::const_iterator it;

		if ((not pvec) and (not pmap)) THROW_GAENARI_INTERNAL_ERROR0;

		if (not pvec) pvec = &items;
		else *pvec = items;

		if (pmap) {
			// output clear
			pmap->clear();

			if (pvec->size() != header_index_map.size()) throw("invalid csv row: " + std::to_string(csv_row_index));
			for (it=header_index_map.begin(); it!=header_index_map.end(); it++)
				(*pmap)[it->first] = (*pvec)[it->second];
		}

		return true;
	} catch(...) {
		return false;
	}

	inline std::string get_row_value(_in size_t index, const _in std::string& def) {
		if (index>=items.size()) return def;
		return items[index];
	}

	inline std::string get_row_value(const _in std::string& name, const _in std::string& def) {
		std::map<std::string,size_t>::const_iterator it;
		it = header_index_map.find(name);
		if (it == header_index_map.end()) return def;
		return get_row_value(it->second, def);
	}

	inline size_t get_csv_row_index(void) { 
		return csv_row_index;
	}

	inline void get_row(_out std::vector<std::string>& row) {
		row = items;
	}

	inline std::string operator [](const _in std::string& name) {
		return get_row_value(name, "");
	}

	inline std::string operator [](const _in std::pair<std::string,std::string>& name_with_def) {
		return get_row_value(name_with_def.first, name_with_def.second);
	}

	inline std::string operator [](const _in size_t index) {
		return get_row_value(index, "");
	}

	inline std::string operator [](const _in std::pair<size_t,std::string>& index_with_def) {
		return get_row_value(index_with_def.first, index_with_def.second);
	}

	inline void close(void) {
		file.close();
		line.clear();
		header.clear();
		header_index_map.clear();
		items.clear();
		skip_parse_error_line = false;
		force_parse_error_line = false;
		csv_row_index = 0;
	}

	inline void get_header_info(_out std::vector<std::string>& h) {
		h = header;
	}

	inline void get_header_info(_out std::map<std::string, size_t>& h) {
		h = header_index_map;
	}

	inline std::string get_line(void) {
		return line;
	}

	inline std::string& get_line_ref(void) {
		return line;
	}

	static inline void parse_delim(const _in std::string& s, _in char delim, _out std::vector<std::string>& o) {
		std::stringstream ss(s);
		std::string item;

		// clear output
		o.clear();

		while (std::getline(ss, item, delim))
			o.push_back(item);

		// getline('A,B,C,')
		// force to append "".
		if (s.back() == delim) o.push_back("");
	}
};

using fn_convert_t	= std::function<bool(_in _out std::vector<std::string>&, _in const std::map<std::string, size_t>&, _in bool)>;
using fn_for_each_t = std::function<bool(_in const std::vector<std::string>&, _in const std::map<std::string, size_t>&)>;

// csv function

// csv file for each
//
// convert : append row data. pass nullptr to igore.
//    [](auto& data, const auto& header_map, bool header_name) {
//       return true;
//    }
//
//    data : call push_back to append transformed row data. do not erase.
//           const std::vector<std::string>&
//    header_map : csv header name to index map.
//                  const std::map<std::string,size_t>& header_map
//    header_name : true  => data value is header name. change header name. (passed to true at the start of the loop)
//                  false => data value is row data. change row data.
//    return : true(=iteration continue), false(=iteration stop)
// 
// iter : for_each iteration lambda
//
//    [](const auto& data, const auto& header_map) {
//       return true;
//    }
//    
//    data       : const std::vector<std::string>&                => [index] = column_string
//    header_map : const std::map<std::string,size_t>& header_map => ['header_name'] = index
//    return     : true(=iteration continue), false(=iteration stop)
inline bool for_each_csv(const _in std::string& file_in, _in char delim, _option _in fn_convert_t const& convert, _in fn_for_each_t const& iter) {
	bool ret = false;
	bool whole = false;
	csv_reader csv;

	// open csv file
	if (not csv.open(file_in, delim, true)) goto FINAL;

	// check header convert
	if (convert) {
		auto header = csv.get_header();
		auto& header_map = csv.get_header_map_ref();
		if (not convert(header, header_map, true)) { ret = true; goto FINAL; }
		if (not csv.build_header_map(header)) goto FINAL;
	}

	// loop first line ~ last line
	for (;;) {
		if (not csv.move_next_row(nullptr)) break;

		// convert row if caller wants.
		if (convert) {
			std::vector<std::string>& new_items = csv.get_row_data_ref();
			if (not convert(new_items, csv.get_header_map_ref(), false)) break;
			if (not csv.verify_row_data()) goto FINAL;
		}

		// call for_each iteration lambda callback
		if (not iter(csv.get_row_data(), csv.get_header_map_ref())) break;
	}

	ret = true;

FINAL:
	return ret;
}

inline std::vector<std::string> get_csv_header(const _in std::string& file_in, _in char delim) {
	csv_reader csv;

	// open csv file and return header map.
	if (not csv.open(file_in, delim, true)) THROW_GAENARI_ERROR("fail to open: " + file_in);
	auto ret = csv.get_header();
	csv.close();
	return ret;
}

inline std::map<std::string, size_t> get_csv_header_map(const _in std::string& file_in, _in char delim) {
	csv_reader csv;

	// open csv file and return header map.
	if (not csv.open(file_in, delim, true)) THROW_GAENARI_ERROR("fail to open: " + file_in);
	auto ret = csv.get_header_map_ref();
	csv.close();
	return ret;
}

} // namespace dataset
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_DATASET_CSV_HPP

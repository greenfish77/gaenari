#ifndef HEADER_GAENARI_GAENARI_DATASET_DATAFRAME_TYPE_HPP
#define HEADER_GAENARI_GAENARI_DATASET_DATAFRAME_TYPE_HPP

namespace gaenari {
namespace dataset {

// column type
enum class data_type_t {data_type_unknown = 0, data_type_int = 1, data_type_int64 = 2, data_type_double = 3, data_type_string = 4, data_type_string_table = 5};

struct usecols {
	enum class selection_type {selection_type_unknown, selection_type_all, selection_type_indexes, selection_type_names, selection_type_regexes, selection_type_first_n, selection_type_last_n} _selection_type = selection_type::selection_type_unknown;
	std::vector<size_t> _indexes;
	std::vector<std::string> _names;
	std::vector<std::string> _regexes;
	size_t _first_n = 0;
	size_t _last_n = 0;
	data_type_t _data_type = data_type_t::data_type_unknown;

	void clear(void) {
		_indexes.clear();
		_names.clear();
		_regexes.clear();
		_first_n = 0;
		_last_n = 0;
		_data_type = data_type_t::data_type_unknown;
	}
	
	static usecols all(_in data_type_t data_type=data_type_t::data_type_string) {
		usecols r;
		r._selection_type = selection_type::selection_type_all;
		r._data_type = data_type;
		return r;
	}

	static usecols indexes(_in const std::vector<size_t>& indexes, _in data_type_t data_type=data_type_t::data_type_string) {
		usecols r;
		r._selection_type = selection_type::selection_type_indexes;
		r._data_type = data_type;
		r._indexes = indexes;
		return r;
	}

	static usecols names(_in const std::vector<std::string>& names, _in data_type_t data_type=data_type_t::data_type_string) {
		usecols r;
		r._selection_type = selection_type::selection_type_names;
		r._data_type = data_type;
		r._names = names;
		return r;
	}

	static usecols regexes(_in const std::vector<std::string>& regexes, _in data_type_t data_type=data_type_t::data_type_string) {
		usecols r;
		r._selection_type = selection_type::selection_type_regexes;
		r._data_type = data_type;
		r._regexes = regexes;
		return r;
	}

	static usecols first_n(_in size_t first_n, _in data_type_t data_type=data_type_t::data_type_string) {
		usecols r;
		r._selection_type = selection_type::selection_type_first_n;
		r._data_type = data_type;
		r._first_n = first_n;
		return r;
	}

	static usecols last_n(_in size_t last_n, _in data_type_t data_type=data_type_t::data_type_string) {
		usecols r;
		r._selection_type = selection_type::selection_type_last_n;
		r._data_type = data_type;
		r._last_n = last_n;
		return r;
	}
};

class column_info {
public:
	column_info()    {clear();}
	~column_info() = default;
public:
	void clear(void) {name.clear(); data_type=data_type_t::data_type_unknown; repository_index=0;}

	// compare operator for dataframe::append(...).
	bool operator == (const column_info& o) const {
		if (name != o.name) return false;
		if (data_type != o.data_type) return false;
		if (repository_index != o.repository_index) return false;
		return true;
	}

	bool operator != (const column_info& o) const {
		return not (*this == o);
	}

public:
	// column name
	std::string name;
	// column data type
	data_type_t data_type=data_type_t::data_type_unknown;
	// column data from repository data index
	size_t repository_index=0;
};

} // namespace dataset
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_DATASET_DATAFRAME_TYPE_HPP

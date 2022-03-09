#ifndef HEADER_GAENARI_GAENARI_DATASET_DATAFRAME_HPP
#define HEADER_GAENARI_GAENARI_DATASET_DATAFRAME_HPP

namespace gaenari {
namespace dataset {

// dataframe example
//
// 	gaenari::dataset::dataframe df;
//  ----------------------------- => create dataframe instance.
//
//  using repo = gaenari::dataset::repository_csv;
//        ====   ---------------------------------- => csv repository is used.
//                                                     redefine repository, it can be accessed from ini file, json file, database, ... and so on.
//                                                     only csv is supported, yet.
//
//	df.read<repo>({{"csv_file_path","D:/weather.nominal.csv"},}                                    );
//          ====   --------------- => "csv_file_path" is needed when repository_csv is used.  *** => all columns loaded as string(default)
//                                     whole data is loaded to memory, so it's blocked.
//
//  df.read<repo>({{"csv_file_path","..."},}, {usecols::names({"F1","F2"}, data_type_double), usecols::last_n(1, data_type_string)});
//          ====                               ***********************************************  **************************************
//                                             => F1, F2 column loaded as double                => and, last one column loaded as string
//                                             => F1, F2, and last one are loaded to memory.
//
//  df.read<repo>({{"csv_file_path","..."},}, {usecols::regexes({"(outlook)|(temperature)|(humidity)|(windy)",    }),});
//          ====                               ***************************************************************** *** 
//                                             => regex pattern search                                           => as string(default)
//                                             => 'outlook','temperature','humidity','windy' are loaded.
//                                                (also, 'outlook1', 'very_windy', ... can be loaded with that pattern.)
//
//  df.rows(), df.cols();
//  -------------------- => dataframe's row, and column count.
//
//  df.columns();
//  ------------ => column information vector.
//                  - df.columns()   .size()           : identical to df.cols.
//                  - df.columns()[n].name             : column name.
//                  - df.columns()[n].data_type        : column data type.
//                  - df.columns()[n].repository_index : index of repository column index.
//
//  df.get_raw(row_index, col_index, value);
//  --------------------------------------- => get [row][index] raw level union data.
//
//  df.get_int   (row_index, col_index, value); auto v = df.get_int   (row_index, col_index);
//  df.get_int64 (row_index, col_index, value); auto v = df.get_int64 (row_index, col_index);
//  df.get_double(row_index, col_index, value); auto v = df.get_double(row_index, col_index);
//  df.get_string(row_index, col_index, value); auto v = df.get_string(row_index, col_index);
//  df.get_index (row_index, col_index, value); auto v = df.get_index (row_index, col_index);
//  ------------------------------------------ => get [row][index] value with each data type.
//  type::value df.get_value(row_index, col_index);
//                   --------- => instead of df.get_*type* functions, a single function returning of value.
//
//  v = df.get(row_index, col_index);
//  -------------------------------- => return [row][index] variant value. it's more safer than other get_*type* functions, by column type checking.
//  v = df.get(row_index, col_index, true);
//                                   ---- => default false. when true, return v = size_t index of this->strings if value type is string.
//  v = df(row_index, col_index);
//  ---------------------------- => same. it's very simple.
//
//  std::vector<type::variant> v;
//  df.get_row(0, v);
//  ----------------  => get 0th row data to variant vector.
//  v = df.get_row(0);
//	----------------- => same.
//  gaenari::common::selected_span<const type::value_raw> v;
//  df.get_row(0, v);
//  ---------------- => get 0th row data to span. no additional memory is required and can be used like a vector. (but, it's raw level union type).
//
//  row iteration skeleton (ranged for loop)
//  for (auto& row: df.iter_string()) {
//      for (auto& v: row.values) {
//          std::cout << v << ' ';
//          row.row_index;
//
//  row iteration skeleton desc.
//  for (auto& row: df.iter_string()) { => row data iteration
//           -         -------------    => choice iteration type. string vector returned.
//           => using &, string vector is not copied to loop, just get shared value.
//      for (auto& v: row.values) { => column data iteration
//                    ---------- => get string data of each column values of current row.
//          std::cout << v << ' ';
//          row.row_index;
//          ------------- => current row index of iteration.
//          ...
//
//  supported iteration type.
//  ------------------------------------------------------------------------------------------------------------------------------------
//  method                       parameter             comment
//  ------------------------------------------------------------------------------------------------------------------------------------
//  df.iter_string()             void                  row data to string vector.
//  df.iter_span()               void                  row data to raw level union span. less memory.
//  df.iter_variant()            bool                  row data to variant vector. pass true, get this->strings index, not string value.
//  df.iter_selected_string()    vector<size_t>        same as df.iter_string(), but selected size_t index rows are iterated.
//  df.iter_selected_span()      vector<size_t>        same as df.iter_span(), but selected size_t index rows are iterated.
//  df.iter_selected_variant()   boo, vector<size_t>   same as df.iter_variant(), but selected size_t index rows are iterated.
//  ------------------------------------------------------------------------------------------------------------------------------------
//
//  std::cout << df << std::endl;
//  ---------------------------- => print out dataframe
//
//  df.select(std::vector<size_t>{1,3,5});
//     ------ => select only interested dataframe's column index. it does not reduce memory usage.
//  df.select({1,3,"play"});
//            ------------ => index and column names are supported.
//
//  dataframe copied = df.shallow_copy();
//              -------------------------- => just shallow copy.
//  dataframe copied = df;
//              ----------- => same.
//  dataframe copied;
//  copied = df;
//  copied = df.shallow_copy();
//  -------------------------- => all same.
//  df.clear();
//  copied;
//  ------ => it's accessible.
//  copied.clear();
//  -------------- => reference count to empty, so deallocate data, now.
//
//  dataframe copied = df.deep_copy({});
//                       -------------- => full deep copy clone.
//  auto copied = df.deep_copy({1,3,5});
//                             ------- => deep copy selected rows.
//
//  dataframe splited = df.split(0.2);
//                        ------------ => return to 20% rows of df.
//  dataframe splited = df.split(0.2, true);
//                                      ---- => return to 20% rows of df.
//                                              and, df has the remaining 80%.
class dataframe {
public:
	dataframe() = default;
	~dataframe() {
		clear();
	}

protected:
	// double pointer for sharing between dataframes.
	// by implementing a double pointer, shared dataframes can refer to the same data.
	// shared data memory block is first created by calling the read function.
	// when assign record and select are called, shared data is accessed by reference.
	struct shared_data {
		// dataframe main array body, and it's c-style buffer.
		type::value_raw* data = nullptr;
		// full data row and column count.
		size_t rows = 0;
		size_t cols = 0;
		// full data column information.
		std::vector<column_info> columns;
		// string table.
		common::string_table strings;
		// full data item count(=array size)
		size_t data_item_count = 0;
		// shared count of this. when share_count is negative, shared_data::data will be destroyed.
		int shared_count = 0;
		// repository context.
		void* repository_ctx = nullptr;
	} **_shared_data = nullptr;

	// non-shared data separately for each dataframe.
	// since the immutable shared_data can not be changed, save the differences of each dataframe.
	struct private_data {
		// is this dataframe is column selected?
		bool selected = false;
		std::vector<size_t> select_indexes;
		// current row and column count. subset shared_data.
		size_t rows = 0;
		size_t cols = 0;
		std::vector<column_info> columns;
	public:
		void clear() {
			private_data init;
			*this = init;
		}
	} _private_data;

	// shared data
	// outside, it's only accessed with const. (immutable access)
protected:
	inline shared_data& get_shared_data(void) {
		if (_shared_data and (*_shared_data)) return **_shared_data;
		THROW_GAENARI_INTERNAL_ERROR1("access to empty shared data.");
	}

public:
	inline const shared_data& get_shared_data_const(void) const {
		if (_shared_data and (*_shared_data)) return **_shared_data;
		THROW_GAENARI_INTERNAL_ERROR1("access to empty shared data.");
	}

	// dataframe's meta data
public:
	inline size_t rows(void) const { return _private_data.rows; }
	inline size_t size(void) const { return _private_data.rows; }
	inline size_t cols(void) const { return _private_data.cols; }
	inline const std::vector<column_info>& columns(void) const { return _private_data.columns; }

	// shared data double pointer allocation
protected:
	inline shared_data& alloc_shared_data(void) {
		dealloc_shared_data();
		_shared_data  = new shared_data*;
		*_shared_data = new shared_data;
		return get_shared_data();
	}

	inline void dealloc_shared_data(void) {
		if (not _shared_data) return;
		if (not (*_shared_data)) return;
		delete (*_shared_data);
		*_shared_data = nullptr;
		delete _shared_data;
		_shared_data = nullptr;
	}

	// basic operations
public:
	// usecols = {}
	// usecols = {usecols::indexes({0,1,2,3}, data_type_int)}
	// usecols = {usecols::names({"F1","F2"}, data_type_double), usecols::last_n(1, data_type_string)}
	// usecols = {usecols::regexes({"^(AAA)",})
	template <typename repository_t>
	inline bool read(_in const std::map<std::string,type::variant>& options, _option_in const std::vector<usecols>& usecols = {usecols::all(data_type_t::data_type_string)}, _option_in void* repository_ctx = nullptr) {
		repository_base* repo = nullptr;
		size_t i = 0;
		size_t j = 0;
		size_t index = 0;
		size_t data_item_count = 0;
		size_t count = false;
		dataset::usecols matched_usecol;
		column_info column_info;
		std::vector<std::string> row_data;
		std::vector<std::string> column_names;
		std::map<std::string,size_t>::iterator find;
		bool auto_growth = false;
	#ifndef NDEBUG
		const size_t auto_growth_initial_count = 2;
	#else
		const size_t auto_growth_initial_count = 4;
	#endif
		const double auto_growth_factor = 1.5;

		// read method alloc new shared data.
		auto& shared_data = alloc_shared_data();

		// check parameter validation
		// do not pass empty usecols. just use default parameter.
		if (usecols.empty()) THROW_GAENARI_ERROR("usecols empty.");

		// repository_t type check , create repository, and repository open. get repository full column names.
		if (not std::is_base_of<repository_base, repository_t>()) THROW_GAENARI_ERROR("repository_t must be derived from repository_base.");
		if (shared_data.data) THROW_GAENARI_ERROR("already dataframe read called.");
		repo = new repository_t;
		if (not repo->open(options, repository_ctx, column_names)) THROW_GAENARI_ERROR("fail to open.");
		if (column_names.empty()) THROW_GAENARI_ERROR("empty columns.");

		// clear string table.
		shared_data.strings.clear();

		// get repository row count
		count = repo->count();
		if (count == std::numeric_limits<size_t>::max()) {
			// auto growth mode on max size.
			count = auto_growth_initial_count;
			auto_growth = true;
		}

		// add columns which matched condition
		for (i=0; i<column_names.size(); i++) {
			matched_usecol.clear();
			// check condition
			for (const auto& usecol: usecols) {
				// find last matched_usecol.
				// matched_usecol can be overwriten.
				switch (usecol._selection_type) {
					case usecols::selection_type::selection_type_all:
						if (true) matched_usecol = usecol;
						break;
					case usecols::selection_type::selection_type_first_n:
						if (i<usecol._first_n) matched_usecol = usecol;
						break;
					case usecols::selection_type::selection_type_last_n:
						if (i>=column_names.size()-usecol._last_n) matched_usecol = usecol;
						break;
					case usecols::selection_type::selection_type_indexes:
						if (usecol._indexes.end() != std::find(usecol._indexes.begin(), usecol._indexes.end(), i)) matched_usecol = usecol;
						break;
					case usecols::selection_type::selection_type_names:
						if (usecol._names.end() != std::find(usecol._names.begin(), usecol._names.end(), column_names[i])) matched_usecol = usecol;
						break;
					case usecols::selection_type::selection_type_regexes:
						for (const auto& regex: usecol._regexes) {
							std::regex pattern(regex);
							if (std::regex_search(column_names[i], pattern)) {
								matched_usecol = usecol;
								break;
							}
						}
						break;
					default:
						THROW_GAENARI_INTERNAL_ERROR0;
				}
			}

			if (matched_usecol._data_type != data_type_t::data_type_unknown) {
				// column has been matched.
				column_info.clear();
				column_info.name = column_names[i];
				column_info.data_type = matched_usecol._data_type;
				column_info.repository_index = i;
				shared_data.columns.push_back(column_info);
			}
		}

		if (shared_data.columns.empty()) THROW_GAENARI_ERROR("empty matched usecol.");

		// completed get indexes to select.
		// allocation memory : data[row*col]
		//
		// ex) col=3, row=2
		//          ------------------------
		//          COL[0]   COL[1]   COL[3]
		//          ------------------------
		// row(0) : data[0], data[1], data[2]
		// row(1) : data[3], data[4], data[5]
		shared_data.data = (type::value_raw*)malloc(sizeof(type::value_raw) * shared_data.columns.size() * count);
		if (not shared_data.data) THROW_GAENARI_ERROR("not enough memory.");
		memset(shared_data.data, 0, sizeof(type::value_raw) * shared_data.columns.size() * count);

		// set rows, cols
		shared_data.rows = count;
		shared_data.cols = shared_data.columns.size();

		// read rows
		i = 0;
		repo->read([&](const auto& row_data) -> bool {
			if (auto_growth) {
				if (i == count) {
					size_t new_count = count + static_cast<size_t>(count * (auto_growth_factor - 1.0));
					auto t = (type::value_raw*)realloc(shared_data.data, sizeof(type::value_raw) * shared_data.columns.size() * new_count);
					if (not t) THROW_GAENARI_ERROR("not enough memory.");
					memset(&t[count * shared_data.columns.size()], 0, sizeof(type::value_raw) * shared_data.columns.size() * (new_count - count));
					count = new_count;
					shared_data.data = t;
					shared_data.rows = count;
				}
			} else {
				if (i >= count) return false;
			}

			// get row pointer to save
			auto col  = &shared_data.data[i*shared_data.cols];
			auto size = shared_data.columns.size();
			for (j=0; j<size; j++) {
				const auto& column_info = shared_data.columns[j];
				const auto& column_data = row_data[column_info.repository_index];
				size_t column_data_type = column_data.index();	// monostate, int, int64_t, double, std::string&.
				if (column_data_type == 0) THROW_GAENARI_INTERNAL_ERROR0;
				else if (column_data_type == 1) {	// int
					auto& int_value = std::get<1>(column_data);
					switch (column_info.data_type) {
						case data_type_t::data_type_int:
						case data_type_t::data_type_string_table:
							col[j].numeric_int32 = int_value;
							break;
						case data_type_t::data_type_int64:
							col[j].numeric_int64 = static_cast<int64_t>(int_value);
							break;
						case data_type_t::data_type_double:
							col[j].numeric_double = static_cast<double>(int_value);
							break;
						case data_type_t::data_type_string:
							// the type of value from repository is int, but the column type is string.
							// this is an error because the size of dataframe::strings can become very large.
						default:
							THROW_GAENARI_INTERNAL_ERROR0;
							break;
					}
				} else if (column_data_type == 2) {	// int64_t
					auto& int64_value = std::get<2>(column_data);
					switch (column_info.data_type) {
						case data_type_t::data_type_int:
						case data_type_t::data_type_string_table:
							col[j].numeric_int32 = static_cast<int>(int64_value);
							break;
						case data_type_t::data_type_int64:
							col[j].numeric_int64 = int64_value;
							break;
						case data_type_t::data_type_double:
							col[j].numeric_double = static_cast<double>(int64_value);
							break;
						case data_type_t::data_type_string:
							// the type of value from repository is int64_t, but the column type is string.
							// this is an error because the size of dataframe::strings can become very large.
						default:
							THROW_GAENARI_INTERNAL_ERROR0;
							break;
					}
				} else if (column_data_type == 3) {	// double
					auto& double_value = std::get<3>(column_data);
					switch (column_info.data_type) {
						case data_type_t::data_type_int:
							col[j].numeric_int32 = static_cast<int>(double_value);
							break;
						case data_type_t::data_type_int64:
							col[j].numeric_int64 = static_cast<int64_t>(double_value);
							break;
						case data_type_t::data_type_double:
							col[j].numeric_double = double_value;
							break;
						case data_type_t::data_type_string_table:
							// the type of value from repository is int(string table id), but the column type is double.
							// it is assumed that real type fields cannot represent string table id.
						case data_type_t::data_type_string:
							// the type of value from repository is double, but the column type is string table id(int).
							// this is an error because the size of dataframe::strings can become very large.
						default:
							THROW_GAENARI_INTERNAL_ERROR0;
							break;
					}
				} else if (column_data_type == 4) {	// std::string& (std::reference_wrapper<const std::string>)
					auto& string_value = std::get<4>(column_data).get();
					switch (column_info.data_type) {
						case data_type_t::data_type_int:
							col[j].numeric_int32 = std::stoi(string_value);
							break;
						case data_type_t::data_type_int64:
							col[j].numeric_int64 = std::stoll(string_value);
							break;
						case data_type_t::data_type_double:
							col[j].numeric_double = std::stod(string_value);
							break;
						case data_type_t::data_type_string:
							col[j].index = shared_data.strings.add(string_value);
							break;
						case data_type_t::data_type_string_table:
							// the type of value from repository is string, but the column type is string table id(int).
							// this is an error because the size of dataframe::strings can become very large.
						default:
							THROW_GAENARI_INTERNAL_ERROR0;
					}
				} else {
					THROW_GAENARI_INTERNAL_ERROR0;
				}
			}

			i++;
			return true;
		});

		// set rows, cols
		shared_data.rows = i;
		shared_data.cols = shared_data.columns.size();
		shared_data.data_item_count = shared_data.rows * shared_data.cols;

		// set private data
		_private_data.clear();
		_private_data.cols = shared_data.cols;
		_private_data.rows = shared_data.rows;
		_private_data.columns = shared_data.columns;

		// all data loaded. so close repository.
		repo->close();
		delete repo;
		repo = nullptr;

		return true;
	}

	inline void clear(void) {
		if (not _shared_data) return;
		if (not (*_shared_data)) {
			delete _shared_data;
			_shared_data = nullptr;
			return;
		}

		// get shared data
		auto& shared_data = get_shared_data();
		shared_data.shared_count--;
		if (shared_data.shared_count < 0) {
			if (shared_data.data) {
				free(shared_data.data);
				shared_data.data = nullptr;
			}
			dealloc_shared_data();
		}
		_shared_data = nullptr;
		_private_data.clear();
	}

	inline void set_string_table_reference_from(_in const common::string_table& s) {
		auto& shared_data = get_shared_data();
		shared_data.strings.reference_from_const(&s);
	}

	inline bool empty(void) const {
		return (rows() == (size_t)0);
	}

	inline dataframe& operator=(const dataframe& s) {
		bool same_shared = false;
		if (this == &s) return *this;

		if ((_shared_data) and (get_shared_data_const().data == s.get_shared_data_const().data)) {
			// same parent copying?
			same_shared = true;
		}

		if (not same_shared) {
			// clear before assign
			clear();
		}

		// assign
		_shared_data  = s._shared_data;
		_private_data = s._private_data;

		// get shared data
		shared_data& shared_data = get_shared_data();

		// increment only for different shared.
		if (not same_shared) shared_data.shared_count++;

		return *this;
	}

	// low level data access
public:
	inline bool get_raw(_in size_t row, _in size_t col, _out type::value_raw& data) const {
		auto& shared_data = get_shared_data_const();
		size_t index = (_private_data.selected) ? row * shared_data.cols + _private_data.select_indexes[col] : row * shared_data.cols + col;
		if (index >= shared_data.data_item_count) return false;
		data = shared_data.data[index];
		return true;
	}

	inline const type::value_raw& get_raw(_in size_t row, _in size_t col) const {
		auto& shared_data = get_shared_data_const();
		size_t index = (_private_data.selected) ? row * shared_data.cols + _private_data.select_indexes[col] : row * shared_data.cols + col;
		if (index >= shared_data.data_item_count) THROW_GAENARI_ERROR("index overflow.");
		return shared_data.data[index];
	}

	// mid level data access
public:
	inline bool get_int(_in size_t row, _in size_t col, _out int& value) const {
		type::value_raw v;
		if (not get_raw(row, col, v)) return false;
		value = v.numeric_int32;
		return true;
	}

	inline int get_int(_in size_t row, _in size_t col) const {
		int v;
		if (not get_int(row, col, v)) THROW_GAENARI_ERROR("fail to get value.");
		return v;
	}

	inline bool get_int64(_in size_t row, _in size_t col, _out int64_t& value) const {
		type::value_raw v;
		if (not get_raw(row, col, v)) return false;
		value = v.numeric_int64;
		return true;
	}

	inline int64_t get_int64(_in size_t row, _in size_t col) const {
		int64_t v;
		if (not get_int64(row, col, v)) THROW_GAENARI_ERROR("fail to get value.");
		return v;
	}

	inline bool get_double(_in size_t row, _in size_t col, _out double& value) const {
		type::value_raw v;
		if (not get_raw(row, col, v)) return false;
		value = v.numeric_double;
		return true;
	}

	inline double get_double(_in size_t row, _in size_t col) const {
		double v;
		if (not get_double(row, col, v)) THROW_GAENARI_ERROR("fail to get value.");
		return v;
	}

	inline bool get_string(_in size_t row, _in size_t col, _out std::string& value) const {
		type::value_raw v;
		auto& shared_data = get_shared_data_const();
		if (not get_raw(row, col, v)) return false;
		try {
			value = shared_data.strings.get_string(v.index);
			return true;
		} catch(...) {
			return false;
		}
	}

	inline std::string get_string(_in size_t row, _in size_t col) const {
		std::string v;
		if (not get_string(row, col, v)) THROW_GAENARI_ERROR("fail to get value.");
		return v;
	}

	inline size_t get_index(_in size_t row, _in size_t col) const {
		type::value_raw v;
		if (not get_raw(row, col, v)) THROW_GAENARI_ERROR("fail to get raw.");
		return v.index;
	}

	inline type::value get_value(_in size_t row, _in size_t col) const {
		const auto& value = get_raw(row, col);
		
		if (row >= rows()) THROW_GAENARI_ERROR("row index overflow.");
		if (col >= cols()) THROW_GAENARI_ERROR("col index overflow.");
		switch (columns()[col].data_type) {
			case data_type_t::data_type_string:
			case data_type_t::data_type_string_table:
				return type::value(value, type::value_type::value_type_size_t);
			case data_type_t::data_type_double:
				return type::value{value, type::value_type::value_type_double};
			case data_type_t::data_type_int:
				return type::value{value, type::value_type::value_type_int};
			case data_type_t::data_type_int64:
				return type::value{value, type::value_type::value_type_int64};
		}
		THROW_GAENARI_ERROR("invalid data type.");
		return type::value{};
	}

	inline type::variant get(_in size_t row, _in size_t col, _in bool get_index_if_string=false) const {
		return (*this)(row, col, get_index_if_string);
	}

	// column management
public:
	inline std::optional<size_t> find_column_index(_in const std::string& name, _in std::optional<gaenari::dataset::data_type_t> type = {}) const {
		size_t ret = 0;
		const auto& _columns = columns();
		for (const auto& column: _columns) {
			if (column.name == name) {
				if ((type.has_value()) and (column.data_type != type.value())) return {};
				return ret;
			}
			ret++;
		}
		return {};
	}

	// high level data access
public:
	// df(row,col) = value return
	inline type::variant operator()(_in size_t row, _in size_t col, _in bool get_index_if_string=false) const {
		if (row >= rows()) THROW_GAENARI_ERROR("row index overflow.");
		if (col >= cols()) THROW_GAENARI_ERROR("col index overflow.");
		switch (columns()[col].data_type) {
			case data_type_t::data_type_string:
			case data_type_t::data_type_string_table:
				if (get_index_if_string) return get_index(row, col);
				return get_string(row, col);
			case data_type_t::data_type_double: return get_double(row, col);
			case data_type_t::data_type_int:    return get_int(row, col);
			case data_type_t::data_type_int64:  return get_int64(row, col);
		}
		THROW_GAENARI_ERROR("invalid data type.");
		return 0;
	}

	inline void get_row(_in size_t row, _out std::vector<type::variant>& values, _in bool get_index_if_string=false) const {
		size_t i;
		auto _cols = cols(); 
		if (row >= rows()) THROW_GAENARI_ERROR("row index overflow.");
		values.resize(_cols);
		for (i=0; i<_cols; i++) {
			std::variant r = (*this)(row, i, get_index_if_string);
			values[i] = std::move(r);
		}
	}

	inline void get_row(_in size_t row, _out common::selected_span<const type::value_raw>& values) const {
		auto& shared_data = get_shared_data_const();
		if (row >= _private_data.rows) THROW_GAENARI_ERROR("row index overflow.");
		values.set_pointer(&shared_data.data[row * shared_data.cols]);
	}

	inline std::vector<type::variant> get_row(_in size_t row, _in bool get_index_if_string=false) const {
		std::vector<type::variant> v;
		get_row(row, v, get_index_if_string);
		return v;
	}

	inline std::map<std::string, type::variant> get_row_as_map(_in size_t row, _in bool get_index_if_string=false) const {
		std::map<std::string, type::variant> ret;
		get_row_as_map(row, ret, get_index_if_string, false);
		return ret;
	}

	inline std::map<std::string, type::variant>& get_row_as_map(_in size_t row, _in _out std::map<std::string, type::variant>& m, _in bool get_index_if_string=false, _in bool clear_m=true) const {
		if (clear_m) m.clear();
		size_t i;
		auto& _columns = columns();
		size_t _cols = _columns.size();
		if (row >= rows()) THROW_GAENARI_ERROR("row index overflow.");
		for (i=0; i<_cols; i++) {
			std::variant r = (*this)(row, i, get_index_if_string);
			m[_columns[i].name] = std::move(r);
		}
		return m;
	}

	// data management operation
public:
	// select column.
	// column selection is logical, not physical. that is, the memory does not shrink.
	// pass empty index, all columns are used.
	// select_indexes can be consist of size_t(index) and std::string(column_name).
	// ex) {0, 1, "result"}
	inline bool select(_option_in const std::vector<type::variant>& select_indexes) {
		std::set<size_t> check_dup;
		std::vector<size_t> __select_indexes;
		auto& shared_data = get_shared_data();

		// already selected?
		if (_private_data.selected) THROW_GAENARI_ERROR("already selected.");

		// variant vector -> size_t vector
		for (const auto& select_index: select_indexes) {
			auto variant_type = select_index.index();
			if ((type::variant_size_t == variant_type) or (type::variant_int == variant_type) or (type::variant_int64 == variant_type)) {
				__select_indexes.push_back(common::to_size_t(select_index));
			} else if (type::variant_string == variant_type) {
				const auto find = std::find_if(shared_data.columns.begin(), shared_data.columns.end(), [&select_index](const column_info& column_info) {
					return (common::to_string(select_index) == column_info.name);
				});
				if (find == shared_data.columns.end()) THROW_GAENARI_ERROR("not found column name.");
				__select_indexes.push_back(find - shared_data.columns.begin());
			} else {
				THROW_GAENARI_ERROR("select_indexes is not size_t or string.");
			}
		}

		// restore with full selected?
		if (__select_indexes.empty()) {
			_private_data.rows = shared_data.rows;
			_private_data.cols = shared_data.cols;
			_private_data.columns = shared_data.columns;
			return true;
		}

		// set column info and check duplications.
		_private_data.columns.clear();
		for (const auto index: __select_indexes) {
			if (check_dup.count(index) > 1) THROW_GAENARI_ERROR("already index selected.");
			if (index >= shared_data.cols) THROW_GAENARI_ERROR("index overflow.");
			check_dup.insert(index);
			_private_data.columns.push_back(shared_data.columns[index]);
		}

		// just set data.
		// memory is not reduced.
		_private_data.selected = true;
		_private_data.select_indexes = __select_indexes;
		_private_data.rows = shared_data.rows;
		_private_data.cols = __select_indexes.size();

		return true;
	}

	// select column with size_t indexes, no variant.
	inline bool select(_option_in const std::vector<size_t>& select_indexes) {
		std::vector<type::variant> s;
		for (const size_t i: select_indexes) s.push_back(i);
		return select(s);
	}

	// dataframe shallow copy (share memory)
	// dataframe copied = df.shallow_copy();
	// is equal to
	// dataframe copied = df;
	inline dataframe shallow_copy(void) const {
		return *this;
	}

	// deep copy data array.
	// column selection is not affected. that's, full data row and column data is duplicated.
	// it's possible to copy a specific row index.
	//
	// dataframe copied = df.deep_copy();
	// dataframe select_copied = df.deep_copy({1,3,5});
	inline dataframe deep_copy(_option_in const std::vector<size_t>& selected_row_indexes) const {
		dataframe df;
		df.deep_copy(*this, selected_row_indexes);
		return df;
	}

	// copy constructor
	dataframe(_in const dataframe& src) {
		// call assignment operator, increase shared count.
		*this = src;
	}

	// split dataframe with this dataframe shrink.
	// - remove the split row from the source dataframe.
	// - CAUTION
	//   dataframe is designed to be immutable. however, (remove_split=true) violates the rule of immutable.
	//   nevertheless, the reason it is used is that memory allocation is minimized during the split process.
	//   PLEASE NODE, an access error may occur in the other dataframe referenced to this dataframe.
	//   so, memory may be wasted, but for more safe use, call split.
	inline dataframe split_with_shrink(_in double split_ratio, _option_in int random_seed = 0) {
		dataframe ret;

		// get shared data
		auto& shared_data = get_shared_data();

		// get split indexes to first and second.
		auto split_index = common::random_shuffle_with_split(shared_data.rows, split_ratio, true, random_seed);

		// deep-copy from src with second indexes.
		ret.deep_copy(*this, split_index.second);

		// first index to move top, and realloc.
		// no additional memory required.
		size_t index = 0;
		size_t bytes = sizeof(type::value_raw) * shared_data.cols * split_index.first.size();
		size_t row_bytes = sizeof(type::value_raw) * shared_data.cols;
		for (const auto row_index: split_index.first) {
			memcpy(&shared_data.data[index], &shared_data.data[row_index * shared_data.cols], row_bytes);
			index += get_shared_data_const().cols;
		}
		// realloc.
		auto realloced = (type::value_raw*)realloc(shared_data.data, bytes);
		if (not realloced) THROW_GAENARI_INTERNAL_ERROR0;
		shared_data.data = realloced;
		if (not shared_data.data) THROW_GAENARI_ERROR("fail to realloc.");
		// change row, and item count.
		shared_data.rows = split_index.first.size();
		_private_data.rows = shared_data.rows;
		shared_data.data_item_count = shared_data.cols * shared_data.rows;

		return ret;
	}

	// auto split = df.split(0.1)
	// split.first  -> 90%
	// split.second -> 10%
	// since start with a deep_copy, memory can be wasted.
	// to take the risk and minimize memory waste, call split_with_shrink directly.
	// ex)
	// auto split = df.split(0.1)
	// auto train = split.first;	// 90%
	// auto test  = split.second;	// 10%
	inline std::pair<dataframe,dataframe> split(_in double split_ratio, _option_in int random_seed = 0) const {
		auto copied = deep_copy({});
		auto split  = copied.split_with_shrink(split_ratio, random_seed);
		return {copied, split};
	}

	// row iterator
public:
	// default iterator value(=*it) for row iterator meta informations.
	// for (auto& i: iterator) {
	//    i.row_index;
	//    i.row_count;
	struct iterator_value_base {
		std::ptrdiff_t row_index = -1;
		std::ptrdiff_t row_count = -1;
	};

	// row iterator target value default type
	// item_t added.
	// for (auto& i: iterator) {
	//    i.row_index;
	//    i.row_count;
	//    i.values;
	template<typename item_t>
	struct iterator_value_default: public iterator_value_base {
		item_t values;
	};

	// full row iterator base type
	template<typename value>
	struct iterator_base {
		// override the constructor for additional parameter.
		iterator_base(_in const dataframe& df, _in bool begin): df{df}, col_count{df.cols()} {
			v.row_count = (std::ptrdiff_t)df.rows();
			if (begin) v.row_index = 0;
		}
		friend bool operator!= (const iterator_base& a, const iterator_base& b) {
			return a.v.row_index != b.v.row_index;
		};
		virtual iterator_base& operator++() {
			v.row_index++;
			if (v.row_index >= v.row_count) v.row_index = -1;
			return *this;
		}
		// you must override (*it) method.
		// return value is the result of the ranged for loop.
		// it should end by returning this->_v.
		virtual const value& operator*() = 0;
	protected:
		const dataframe& df;
		std::size_t col_count = 0;
		value v;
	};

	// selected row iterator base type
	template<typename value>
	struct iterator_selected_base {
		// override the constructor for additional parameter.
		iterator_selected_base(_in const dataframe& df, _in bool begin, _in const std::vector<size_t>& row_select): df{df}, col_count{df.cols()}, row_select{row_select} {
			v.row_count = (std::ptrdiff_t)row_select.size();
			if (begin) v.row_index = 0;
		}
		friend bool operator!= (const iterator_selected_base& a, const iterator_selected_base& b) {
			return a.v.row_index != b.v.row_index;
		};
		virtual iterator_selected_base& operator++() {
			v.row_index++;
			if (v.row_index >= v.row_count) v.row_index = -1;
			return *this;
		}
		// you must override (*it) method.
		// return value is the result of the ranged for loop.
		// it should end by returning _v.
		virtual const value& operator*() = 0;
	protected:
		const dataframe& df;
		std::size_t col_count = 0;
		value v;
		const std::vector<size_t>& row_select;	// referencing, caller(=wrapper) has the vector body. so iterators from same wrapper share it.
	};

	// support multiple iterator in dataframe, a wrapper is required.
	// default wrapper.
	// only one dataframe record is required.
	template<typename iterator>
	struct iterator_wrapper_default {
		iterator_wrapper_default(_in const dataframe& df): _df{df}, _end{df, false} {}
		iterator  begin() {if (_df.empty()) return _end; return iterator(_df, true);}
		iterator& end()   {return _end;}
	protected:
		const dataframe& _df;
		iterator _end;
	};

	// default wrapper with one parameter
	template<typename iterator, typename T1>
	struct iterator_wrapper_with_one_param {
		iterator_wrapper_with_one_param(_in const dataframe& df, _in T1 t1): _df{df}, _end{df, false, t1}, _t1{t1} {}
		iterator  begin() {if (_df.empty()) return _end; return iterator(_df, true, _t1);}
		iterator& end()   {return _end;}
	protected:
		const dataframe& _df;
		iterator _end;
		T1 _t1;
	};

	// default wrapper with two parameters
	template<typename iterator, typename T1, typename T2>
	struct iterator_wrapper_with_two_param {
		iterator_wrapper_with_two_param(_in const dataframe& df, _in T1 t1, _in T2 t2): _df{df}, _end{df, false, t1, t2}, _t1{t1}, _t2{t2} {}
		iterator  begin() {if (_df.empty()) return _end; return iterator(_df, true, _t1, _t2);}
		iterator& end()   {return _end;}
	protected:
		const dataframe& _df;
		iterator _end;
		T1 _t1;
		T2 _t2;
	};

	// string vector iterator.

	// re-define iterator base.
	// - using default value.      -> iterator_value_default
	// - using string vector item. -> std::vector<std::string>

	// string iterator wrapper.
	// - using default iterator wrapper.
	struct iterator_string: public iterator_base<iterator_value_default<std::vector<std::string>>> {
		// the size of the vector is allocated in the constructor.
		iterator_string(_in const dataframe& df, _in bool begin): iterator_base{df, begin} {
			v.values.resize(df.cols());
		}
		virtual const iterator_value_default<std::vector<std::string>>& operator*() {
			for (size_t col=0; col<col_count; col++) {
				auto variant = df(v.row_index, col);
				v.values[col] = common::to_string(variant);
			}
			return v;
		}
	};

	// value iterator
	struct iterator_value: public iterator_base<iterator_value_default<std::vector<type::value>>> {
		// the size of the vector is allocated in the constructor.
		iterator_value(_in const dataframe& df, _in bool begin): iterator_base{df, begin} {
			v.values.resize(df.cols());
		}
		virtual const iterator_value_default<std::vector<type::value>>& operator*() {
			for (size_t col=0; col<col_count; col++) {
				v.values[col] = df.get_value(v.row_index, col);
			}
			return v;
		}
	};

	// raw data span iterator
	struct iterator_span: public iterator_base<iterator_value_default<common::selected_span<const type::value_raw>>> {
		iterator_span(_in const dataframe& df, _in bool begin): iterator_base{df, begin} {
			v.values.set_pointer(df.get_shared_data_const().data);
			v.values.set_len(df.cols());
			if (df._private_data.selected) v.values.set_indexes_reference(df._private_data.select_indexes);
		}
		virtual const iterator_value_default<common::selected_span<const type::value_raw>>& operator*() {
			df.get_row((size_t)v.row_index, v.values);
			return v;
		}
	};

	// variant vector iterator
	struct iterator_variant: public iterator_base<iterator_value_default<std::vector<type::variant>>> {
		// the size of the vector is allocated in the constructor.
		iterator_variant(_in const dataframe& df, _in bool begin, _in bool get_index_if_string): iterator_base{df, begin}, get_index_if_string{get_index_if_string} {
			v.values.resize(df.cols());
		}
		virtual const iterator_value_default<std::vector<type::variant>>& operator*() {
			for (size_t col=0; col<col_count; col++) {
				v.values[col] = df(v.row_index, col, get_index_if_string);
			}
			return v;
		}
	protected:
		bool get_index_if_string = false;
	};

	// selected row iterators
	struct iterator_selected_string: public iterator_selected_base<iterator_value_default<std::vector<std::string>>> {
		// the size of the vector is allocated in the constructor.
		iterator_selected_string(_in const dataframe& df, _in bool begin, _in const std::vector<size_t>& row_select): iterator_selected_base{df, begin, row_select} {
			v.values.resize(df.cols());
		}
		virtual const iterator_value_default<std::vector<std::string>>& operator*() {
			for (size_t col=0; col<col_count; col++) {
				auto variant = df(row_select[v.row_index], col);
				v.values[col] = common::to_string(variant);
			}
			return v;
		}
	};

	struct iterator_selected_value: public iterator_selected_base<iterator_value_default<std::vector<type::value>>> {
		// the size of the vector is allocated in the constructor.
		iterator_selected_value(_in const dataframe& df, _in bool begin, _in const std::vector<size_t>& row_select): iterator_selected_base{df, begin, row_select} {
			v.values.resize(df.cols());
		}
		virtual const iterator_value_default<std::vector<type::value>>& operator*() {
			for (size_t col=0; col<col_count; col++) {
				v.values[col] = df.get_value(row_select[v.row_index], col);
			}
			return v;
		}
	};

	struct iterator_selected_span: public iterator_selected_base<iterator_value_default<common::selected_span<const type::value_raw>>> {
		iterator_selected_span(_in const dataframe& df, _in bool begin, _in const std::vector<size_t>& row_select): iterator_selected_base{df, begin, row_select} {
			v.values.set_pointer(df.get_shared_data_const().data);
			v.values.set_len(df.cols());
			if (df._private_data.selected) v.values.set_indexes_reference(df._private_data.select_indexes);
		}
		virtual const iterator_value_default<common::selected_span<const type::value_raw>>& operator*() {
			df.get_row((size_t)row_select[v.row_index], v.values);
			return v;
		}
	};

	struct iterator_selected_variant: public iterator_selected_base<iterator_value_default<std::vector<type::variant>>> {
		// the size of the vector is allocated in the constructor.
		iterator_selected_variant(_in const dataframe& df, _in bool begin, _in bool get_index_if_string, _in const std::vector<size_t>& row_select): iterator_selected_base{df, begin, row_select}, get_index_if_string{get_index_if_string} {
			v.values.resize(df.cols());
		}
		virtual const iterator_value_default<std::vector<type::variant>>& operator*() {
			for (size_t col=0; col<col_count; col++) {
				v.values[col] = df(row_select[v.row_index], col, get_index_if_string);
			}
			return v;
		}
	protected:
		bool get_index_if_string = false;
	};

	// return iterator wrapper

	// default iterators
	iterator_wrapper_default<iterator_string> iter_string(void) const {
		return iterator_wrapper_default<iterator_string>(*this);
	}

	iterator_wrapper_default<iterator_value> iter_value(void) const {
		return iterator_wrapper_default<iterator_value>(*this);
	}

	iterator_wrapper_default<iterator_span> iter_span(void) const {
		return iterator_wrapper_default<iterator_span>(*this);
	}

	iterator_wrapper_with_one_param<iterator_variant, bool> iter_variant(_option_in bool get_index_if_string = false) const {
		return iterator_wrapper_with_one_param<iterator_variant, bool>(*this, get_index_if_string);
	}

	// iterators with row selection.
	iterator_wrapper_with_one_param<iterator_selected_string, std::vector<size_t>> iter_selected_string(_in const std::vector<size_t>& row_selected) const {
		return iterator_wrapper_with_one_param<iterator_selected_string, std::vector<size_t>>(*this, row_selected);
	}

	iterator_wrapper_with_one_param<iterator_selected_value, std::vector<size_t>> iter_selected_value(_in const std::vector<size_t>& row_selected) const {
		return iterator_wrapper_with_one_param<iterator_selected_value, std::vector<size_t>>(*this, row_selected);
	}

	iterator_wrapper_with_one_param<iterator_selected_span, std::vector<size_t>> iter_selected_span(_in const std::vector<size_t>& row_selected) const {
		return iterator_wrapper_with_one_param<iterator_selected_span, std::vector<size_t>>(*this, row_selected);
	}

	iterator_wrapper_with_two_param<iterator_selected_variant, bool, std::vector<size_t>> iter_selected_variant(_in const std::vector<size_t>& row_selected, _option_in bool get_index_if_string = false) const {
		return iterator_wrapper_with_two_param<iterator_selected_variant, bool, std::vector<size_t>>(*this, get_index_if_string, row_selected);
	}

	// cout dataframe
public:
	// std::cout << df << std::endl;
	// recommend calling gaenari::method::stringfy::logger(df) rather than cout.
	friend inline auto operator<<(std::ostream &os, dataframe& df) -> std::ostream& {
		std::string column_line;
		std::string line;
		for (const auto& column: df._private_data.columns) {
			column_line += common::fixed_length(column.name, 12, ' ', 1, 1) + '|';
		}
		if (column_line.empty()) return os;
		column_line.pop_back();
		os << std::string(column_line.length(), '-') << std::endl;
		os << column_line                            << std::endl;
		os << std::string(column_line.length(), '-') << std::endl;
		for (auto& row: df.iter_string()) {
			line.clear();
			for (auto& value: row.values) {
				line += common::fixed_length(value, 12, ' ', 1, 1) + '|';
			}
			line.pop_back();
			os << line << std::endl;
		}
		os << std::string(column_line.length(), '-') << std::endl;
		return os;
	}

	// vertical dataframe append.
public:
	inline bool append(_in const dataframe& df) {
		auto& shared_data_out = get_shared_data();
		auto& shared_data_in  = df.get_shared_data_const();
		
		// assume it is not selected.
		if (_private_data.selected) return false;

		// it is assumed that the columns and strings of two dataframes are the same.
		if (shared_data_out.columns != shared_data_in.columns) return false;
		if (shared_data_out.strings != shared_data_in.strings) return false;

		// new rows, new count.
		size_t new_rows = shared_data_out.rows + shared_data_in.rows;
		size_t new_data_item_count = shared_data_out.data_item_count + shared_data_in.data_item_count;

		// realloc data.
		type::value_raw* new_data = (type::value_raw*)realloc(shared_data_out.data, new_data_item_count * sizeof(*new_data));
		if (not new_data) return false;

		// copy data to be appended.
		memcpy(&new_data[shared_data_out.data_item_count], shared_data_in.data, shared_data_in.data_item_count * sizeof(*new_data));

		// assign.
		shared_data_out.data = new_data;
		shared_data_out.rows = new_rows;
		shared_data_out.data_item_count = new_data_item_count;
		_private_data.rows = new_rows;

		// success.
		return true;
	}

	// misc private methods.
protected:
	inline void deep_copy(_in const dataframe& src, _option_in const std::vector<size_t>& selected_row_indexes) {
		size_t index = 0;

		// get src shared data
		auto& shared_data_src = src.get_shared_data_const();

		// simple copy private member variable from src
		_private_data = src._private_data;

		// alloc new shared data
		auto& new_shared_data = alloc_shared_data();	// alloc new shared data.

		if (selected_row_indexes.empty()) {
			// full copy

			// copy meta data
			new_shared_data = shared_data_src;

			// duplicate data array
			size_t bytes = sizeof(type::value_raw) * shared_data_src.cols * shared_data_src.rows;
			new_shared_data.data = (type::value_raw*)malloc(bytes);
			if (not new_shared_data.data) THROW_GAENARI_ERROR("not enough memory.");
			memcpy(new_shared_data.data, shared_data_src.data, bytes);
		} else {
			// subset copy
			size_t bytes = sizeof(type::value_raw) * shared_data_src.cols * selected_row_indexes.size();
			size_t row_bytes = sizeof(type::value_raw) * shared_data_src.cols;
			new_shared_data = shared_data_src;						// content copied from shared_data of src
			new_shared_data.data = (type::value_raw*)malloc(bytes);	// same, but different memory array
			new_shared_data.data_item_count = shared_data_src.cols * selected_row_indexes.size();
			if (not new_shared_data.data) THROW_GAENARI_ERROR("not enough memory.");
			for (const auto row_index: selected_row_indexes) {
				memcpy(&new_shared_data.data[index], &shared_data_src.data[row_index * shared_data_src.cols], row_bytes);
				index += new_shared_data.cols;
			}
			new_shared_data.rows = selected_row_indexes.size();
			_private_data.rows = selected_row_indexes.size();
		}

		// initialize shared count
		new_shared_data.shared_count = 0;
	}
};

} // namespace dataset
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_DATASET_DATAFRAME_HPP

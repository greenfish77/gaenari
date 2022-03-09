#ifndef HEADER_GAENARI_GAENARI_DATASET_REPOSITORY_HPP
#define HEADER_GAENARI_GAENARI_DATASET_REPOSITORY_HPP

namespace gaenari {
namespace dataset {

///////////////////////////////////////////////////////////////////////////////
// base repository class
class repository_base {
public:
	repository_base()          = default;
	virtual ~repository_base() = default;
public:
	// callback type.
	using callback_column_type = std::variant<std::monostate, int, int64_t, double, std::reference_wrapper<const std::string>>;
	using callback_read = std::function<bool(const std::vector<callback_column_type>& data)>;

	// re-define following function.
	// it's called from dataframe.
	// `options` parameter is passed from dataframe::read(...).
	virtual bool open(_in const std::map<std::string,type::variant>& options, _option_in void* repository_ctx, _out std::vector<std::string>& column_names) = 0;
	// dataframe row instance size.
	// if std::numeric_limit<size_t>::max() is returned,
	// it operates in `auto growth mode`.
	// until false return of callback_read, the row size is automatically incremented.
	// use it when calculating the count takes a lot of time.
	// (however, there is a loss of realloc calls.)
	virtual size_t count(void) = 0;
	// pass row data. when eof, return false, and vector data will be ignored.
	virtual void read(_in callback_read cb) = 0;
	// close opened repository.
	virtual void close(void) = 0;
};

///////////////////////////////////////////////////////////////////////////////
// csv repository class
class repository_csv : public repository_base {
public:
	repository_csv()          = default;
	virtual ~repository_csv() { close(); }
protected:
	csv_reader _csv;
	size_t _count = 0;
public:
	// options = {
	//     {"csv_file_path","..."} : csv file path (std::string type)
	// }
	virtual bool open(_in const std::map<std::string,type::variant>& options, _option_in void* repository_ctx, _out std::vector<std::string>& column_names) {
		std::map<std::string,type::variant> _options;
		std::vector<std::string> _header;

		// context is not required.
		if (repository_ctx) THROW_GAENARI_ERROR("csv do not need context.");

		// check required options
		_options = options;
		if (0 == _options.count("csv_file_path")) THROW_GAENARI_ERROR("require csv_file_path.");

		// open csv and pass header as column name
		if (not this->_csv.open(common::to_string(_options["csv_file_path"]), ',', true)) THROW_GAENARI_ERROR("fail to read csv_file_path");
		_csv.get_header_info(column_names);

		// get count
		this->_count = 0;
		for (this->_count=0; ;this->_count++) {
			if (not this->_csv.move_next_row(nullptr)) break;
		}

		// re-open
		this->_csv.close();
		if (not this->_csv.open(common::to_string(_options["csv_file_path"]), ',', true)) THROW_GAENARI_ERROR("fail to read csv_file_path");
		_csv.get_header_info(_header);
		if (_header != column_names) THROW_GAENARI_ERROR("file changed.");

		// open completed
		return true;
	}

	virtual size_t count(void) {return this->_count;}

	virtual void read(_in callback_read cb) {
		size_t i = 0;
		size_t column_count = _csv.get_header_ref().size();
		std::vector<callback_column_type> data;
		data.resize(column_count);

		for (;;) {
			if (not this->_csv.move_next_row(nullptr)) break;
			auto& row_data = _csv.get_row_data_ref();
			for (i=0; i<column_count; i++) {
				data[i] = row_data[i];
			}

			if (not (cb)(data)) break;
		}
	}

	virtual void close(void) {this->_csv.close();}
};

} // namespace dataset
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_DATASET_REPOSITORY_HPP

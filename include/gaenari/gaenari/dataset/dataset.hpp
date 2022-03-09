#ifndef HEADER_GAENARI_GAENARI_DATASET_BASE_HPP
#define HEADER_GAENARI_GAENARI_DATASET_BASE_HPP

namespace gaenari {
namespace dataset {

class feature_select {
public:
	feature_select()  = default;
	~feature_select() = default;
protected:
	struct name_info {
		std::string name;
		bool regex = false;
	};
public:
	void set_last_y_rest_x(void) {
		mode = mode::last_y_rest_x;
	}
	void x_add(_in const std::vector<std::string>& names, _in bool regex) {
		name_info n;
		for (const auto& name: names) {
			n.name  = name;
			n.regex = regex;
			x.push_back(n);
		}
	}
	void x_add(_in const std::string& name, _in bool regex) {
		std::vector<std::string> names = {name};
		x_add(names, regex);
	}
	void y_set(_in const std::string& name, _in bool regex) {
		y.name  = name;
		y.regex = regex;
	}
public:
	std::vector<size_t> get_x_indexes(_in const dataframe& df) const {
		size_t i = 0;
		std::vector<size_t> r;
		auto& shared_data = df.get_shared_data_const();

		if (shared_data.cols <= 1) THROW_GAENARI_ERROR("dataframe cols <= 1.");
		if (mode == mode::last_y_rest_x) {
			for (i=0; i<shared_data.cols-1; i++) r.push_back(i);
			return r;
		}

		// loop x names
		for (const auto& name: x) {
			// start first column
			auto start = shared_data.columns.begin();
			// find in dataframe column.
			for (;;) {
				const auto find = std::find_if(start, shared_data.columns.end(), [&name](const column_info& column_info) {
					if (not name.regex) return (column_info.name == name.name);
					std::regex pattern(name.name);
					return std::regex_search(column_info.name, pattern);
				});

				// not found
				if (find == shared_data.columns.cend()) break;

				// push index
				size_t i = find - shared_data.columns.cbegin(); // static_cast<size_t>(std::distance(df.columns.cbegin(), find)) is smarter?
				r.push_back(i);

				// next column from found
				start = find + 1;
			}
		}
		r = common::remove_duplicated(r);
		std::sort(r.begin(), r.end());
		if (r.empty()) THROW_GAENARI_ERROR("empty x indexes.");
		return r;
	}
	size_t get_y_index(_in const dataframe& df) const {
		auto& shared_data = df.get_shared_data_const();

		if (shared_data.cols <= 1) THROW_GAENARI_ERROR("dataframe cols <= 1.");
		if (mode == mode::last_y_rest_x) {
			return shared_data.cols-1;
		}

		const auto find = std::find_if(shared_data.columns.begin(), shared_data.columns.end(), [&y=y](const column_info& column_info) {
			if (not y.regex) return (column_info.name == y.name);
			std::regex pattern(y.name);
			return std::regex_search(column_info.name, pattern);
		});

		// not found
		if (find == shared_data.columns.cend()) THROW_GAENARI_ERROR("empty x index.");
		size_t i = find - shared_data.columns.cbegin(); // static_cast<size_t>(std::distance(df.columns.cbegin(), find)) is smarter?
		return i;
	}

protected:
	std::vector<name_info> x;
	name_info y;
	enum class mode {name_select, last_y_rest_x} mode = mode::name_select;
};

// since dataframe is data itself, it is not divided into x and y.
// the dataset separates the dataframe into x and y.
// it is a shallow copy of the source dataframe.
class dataset {
public:
	// with feature select.
	dataset(_in const dataframe& df, _in const feature_select& feature_select) {
		// shallow copy, so do not copy dataframe full array, just reference it.
		_df = df.shallow_copy();
		x   = df.shallow_copy();
		y   = df.shallow_copy();

		auto x_indexes = feature_select.get_x_indexes(df);
		auto y_index   = feature_select.get_y_index(df);

		// dataframe column select
		x.select(x_indexes);
		y.select(std::vector<size_t>{y_index});

		// metadata
		metadata.feature_count  = x.cols();
		metadata.instance_count = x.rows();
	}

	// the last is y and the rest is x.
	dataset(_in const dataframe& df) {
		// shallow copy, so do not copy dataframe full array, just reference it.
		_df = df.shallow_copy();
		x   = df.shallow_copy();
		y   = df.shallow_copy();

		feature_select feature_select;
		feature_select.set_last_y_rest_x();

		auto x_indexes = feature_select.get_x_indexes(df);
		auto y_index   = feature_select.get_y_index(df);

		// dataframe column select
		x.select(x_indexes);
		y.select(std::vector<size_t>{y_index});

		// metadata
		metadata.feature_count  = x.cols();
		metadata.instance_count = x.rows();
	}

	struct metadata {
		size_t feature_count = 0;
		size_t instance_count = 0;
	};

public:
	dataframe x;
	dataframe y;
	metadata metadata;
protected:
	dataframe _df;
};

} // namespace dataset
} // namespace gaenari

#endif // HEADER_GAENARI_GAENARI_DATASET_BASE_HPP

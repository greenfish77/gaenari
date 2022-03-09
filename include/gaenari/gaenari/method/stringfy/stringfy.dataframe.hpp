#ifndef HEADER_GAENARI_GAENARI_METHOD_STRINGFY_DATAFRAME_HPP
#define HEADER_GAENARI_GAENARI_METHOD_STRINGFY_DATAFRAME_HPP

namespace gaenari {
namespace method {
namespace stringfy {

inline std::string dataframe(_in const dataset::dataframe& df, _in const std::string& mime_type, _option_in const stringfy::style& style = stringfy::style()) {
	size_t width = 0;
	size_t index = 0;
	size_t row_index = 0;
	size_t row_cutoff_first_count = 0;
	size_t row_cutoff_last_count = 0;
	size_t row_cutoff_last_index = 0;
	bool column_cutoff = false;
	bool row_cutoff = false;
	bool row_ellipsis = false;
	std::string border;
	std::string ret;
	std::string line;
	std::string ellipsis = "...";
	std::string name;

	// color setting.
	struct {
		std::string name1,  name2;
		std::string value1, value2;
	} clr;

	// get mime type.
	auto type = stringfy::util::get_mime_type(mime_type);
	if (not ((type == stringfy::type::text_plain) or (type == stringfy::type::text_colortag))) {
		THROW_GAENARI_ERROR("invalid mime type.");
	}

	// set color theme.
	if (type == stringfy::type::text_colortag) {
		clr.name1  = "<" + style.name + ">";  clr.name2  = "</" + style.name  + ">";
		clr.value1 = "<" + style.value + ">"; clr.value2 = "</" + style.value + ">";
	}

	const auto& columns = df.columns();
	if (columns.empty()) THROW_GAENARI_ERROR("empty dataframe.");

	// verify style.
	if (not style.valid()) THROW_GAENARI_ERROR("invalid style.");

	// column cut-off?
	if (columns.size() > style.max_column_count) column_cutoff = true;

	// row cut-off?
	if (style.max_row_count != std::numeric_limits<size_t>::max()) {
		if (df.rows() > style.max_row_count) {
			row_cutoff = true;
			row_cutoff_first_count = static_cast<size_t>((static_cast<double>(style.max_row_count) / 2.0));
			row_cutoff_last_count  = style.max_row_count - row_cutoff_first_count - 1;
			row_cutoff_last_index = df.rows() - row_cutoff_last_count;
		}
	}

	// get column line.
	index = 0;
	for (const auto& column: columns) {
		if (column_cutoff) {
			if (index < style.max_column_count - 2) {
				name = common::fixed_length(column.name, style.column_width, ' ', 1, 1);
				line += clr.name1 + name + clr.name2 + '|';
			} else if (index == style.max_column_count - 2) {
				name = common::fixed_length(ellipsis, style.column_width, ' ', 1, 1);
				line += clr.name1 + name + clr.name2 + '|';
			} else if (index == columns.size() - 1) {
				name = common::fixed_length(column.name, style.column_width, ' ', 1, 1);
				line += clr.name1 + name + clr.name2 + '|';
			}
			width += name.length() + 1;	// 1 for '|'.
			index++;
		} else {
			name = common::fixed_length(column.name, style.column_width, ' ', 1, 1);
			line += clr.name1 + name + clr.name2 + '|';
			width += name.length() + 1;	// 1 for '|'.
		}
	}
	line.pop_back();

	// get width.
	width -= 1;	// pop_back last '|' character.

	// get '----+----+----' string.
	border = std::string(width, '-');
	for (size_t i=style.column_width; i<width; i+=style.column_width+1) border[i] = '+';

	// first line : -----
	ret  = border + '\n';

	// second line : column names
	ret += line + '\n';

	// third line : -----
	ret += border + '\n';

	// row iteration.
	row_index = 0;
	for (auto& row: df.iter_string()) {
		if (row_cutoff) {
			if (row_index < row_cutoff_first_count) {
				// first half before omission.
				// do column iteration next code-block.
				// do not continue.
			} else if (row_index == row_cutoff_first_count) {
				row_ellipsis = true;
				// do not continue.
			} else if (row_index >= row_cutoff_last_index) {
				if (row_ellipsis) row_ellipsis = false;
				// second half before omission.
				// do column iteration next code-block.
				// do not continue.
			} else {
				// skip it
				row_index++;
				continue;
			}
			
			row_index++;
		}

		line.clear();
		index = 0;
		for (auto& value: row.values) {
			const std::string& v = row_ellipsis ? ellipsis : value;
			if (column_cutoff) {
				if (index < style.max_column_count - 2) {
					line += clr.value1 + common::fixed_length(v, style.column_width, ' ', 1, 1) + clr.value2 + '|';
				} else if (index == style.max_column_count - 2) {
					line += clr.value1 + common::fixed_length(ellipsis, style.column_width, ' ', 1, 1) + clr.value2 + '|';
				} else if (index == columns.size() - 1) {
					line += clr.value1 + common::fixed_length(v, style.column_width, ' ', 1, 1) + clr.value2 + '|';
				}
				index++;
			} else {
				line += clr.value1 + common::fixed_length(v, style.column_width, ' ', 1, 1) + clr.value2 + '|';
			}
		}
		line.pop_back();
		ret += line + '\n';
	}

	// last line : -----
	ret += border + '\n';

	return ret;
}

// simple logger.
// stringfy::logger(df)
void logger(_in const dataset::dataframe& df, _option_in const std::string& text="", _option_in size_t rows=50, _option_in size_t columns=16, _option_in size_t column_width=12) {
	std::string stringfy;

	// style setting.
	stringfy::style style;
	style.column_width = column_width;
	style.max_row_count = rows;
	style.max_column_count = columns;

	// stringfy.
	stringfy = stringfy::dataframe(df, "text/colortag", style);

	// start with \n.
	stringfy.insert(stringfy.begin(), '\n');

	// stat.
	stringfy += "cols=" + std::to_string(df.cols()) + ", rows=" + std::to_string(df.rows());

	// logger with colortag.
	logger::info(text + stringfy, true);
}

} // stringfy
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_STRINGFY_DATAFRAME_HPP

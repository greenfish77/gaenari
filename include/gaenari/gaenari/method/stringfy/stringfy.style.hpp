#ifndef HEADER_GAENARI_GAENARI_METHOD_STRINGFY_STYLE_HPP
#define HEADER_GAENARI_GAENARI_METHOD_STRINGFY_STYLE_HPP

namespace gaenari {
namespace method {
namespace stringfy {

struct style {
	// default color theme.

	std::string name		= "yellow";
	std::string value		= "green";
	std::string label		= "lblue";
	std::string score		= "gray";
	std::string id          = "gray";

	// default dataframe style.

	// number of characters per column(output two less characters by margin). min 3. max 80.
	size_t column_width     = 12;
	// max number of columns. the middle columns are marked with '...'. std::numeric_limits<size_t>::max() -> whole columns. min 3.
	size_t max_column_count = 16;
	// max number of rows.    the middle rows are marked with '...'.    std::numeric_limits<size_t>::max() -> whole rows.
	size_t max_row_count    = 50;

	// functions.

	// check valid function.
	bool valid(void) const {
		const logger::tag_info* info = logger::get_tag_info();
		std::set<std::string> colors;

		for (size_t i=0;;i++) {
			if (not info[i].name) break;
			colors.insert(info[i].name);
		}

		// color check
		for (auto& v: {this->name, this->value, this->label, this->score}) 
			if (colors.end() == colors.find(v)) return false;

		if (this->column_width <= 2) return false;
		if (this->column_width >= 81) return false;
		if (this->max_column_count <= 2) return false;

		return true;
	}
};

} // stringfy
} // method
} // gaenari

#endif // HEADER_GAENARI_GAENARI_METHOD_STRINGFY_STYLE_HPP

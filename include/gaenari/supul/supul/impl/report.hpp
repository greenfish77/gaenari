#ifndef HEADER_GAENARI_SUPUL_SUPUL_IMPL_REPORT_HPP
#define HEADER_GAENARI_SUPUL_SUPUL_IMPL_REPORT_HPP

// footer included from supul.hpp
namespace supul {
namespace supul {

// default option.
const static char* default_option 
	=	"{"
			"\"categories\":[],"			// choose categories to report.
			"\"datetime_as_index\":false"	// change a datetime in the format yyyymmddhhmmss
											// to a zero-based index value. used for testing.
		"}";

// option format:
// {
//   "categories":["","",...],
//   "datetime_as_index":false,
// }
inline auto supul_t::report::json(_in const std::string& option_as_json) -> std::string {
	std::string ret;

	// in read transaction.
	if (not supul.db) THROW_SUPUL_ERROR("database is not initialized.");
	db::transaction_guard transaction{*supul.db, false};

	// parse option.
	gaenari::common::json_insert_order_map option;
	if (option_as_json.empty()) option.parse(default_option);
	else option.parse(option_as_json.c_str());

	// get categories.
	std::vector<std::string> categories;
	auto categories_size = option.array_size_noexcept({"categories"});
	if (categories_size > 0) {
		for (size_t i=0; i<categories_size; i++) {
			auto name = option.get_value_as_string({"categories", i});
			if (std::find(supported_categories.begin(), supported_categories.end(), name) == supported_categories.end()) THROW_SUPUL_ERROR1("`%0` is not supported.", name);
			categories.emplace_back(std::move(name));
		}
	} else {
		// empty category: all.
		categories = supported_categories;
	}

	// get report.
	// json format:
	// {
	//		"doc_ver": 1,
	//		"error": false,
	//		"category": {
	//			"<category_name>": {
	//				...
	//			},
	//			...
	//		}
	// }
	gaenari::common::json_insert_order_map json;

	// default data.
	json.set_value({"doc_ver"}, 1);
	json.set_value({"error"}, false);
	json.set_value_empty_object({"category"});

	// add each category json.
	for (const auto& name: categories) {
		if (name == "chunk_history")			chunk_history_to_json(option, json); 
		else if (name == "global")				global_to_json(option, json);
		else if (name == "confusion_matrix")	confusion_matrix_to_json(option, json);
		else if (name == "generation_history")	generation_history_to_json(option, json);
		else gaenari::logger::warn("{0} is not supported.", {name});
	}

	// json to string.
	return json.to_string<gaenari::common::json_insert_order_map::minimize>();
}

inline auto supul_t::report::global_for_gnuplot(_in const gaenari::common::json_insert_order_map& j) -> type::map_variant {
	type::map_variant ret;

	auto names = j.object_names({"category", "global"});
	for (const auto& name: names) {
		auto value = j.find_value_const({"category", "global", name});
		// std::variant<std::monostate, std::string, numeric, object, array, bool>;
		auto index = value.get().index();
		if (index == 1) {
			ret[name] = std::get<1>(value.get());
		} else if (index == 2) {
			// std::variant<int, double, int64_t>
			auto n = std::get<2>(value.get());
			auto index = n.index();
			if (index == 0) ret[name] = static_cast<int64_t>(std::get<0>(n));
			else if (index == 1) ret[name] = std::get<1>(n);
			else if (index == 2) ret[name] = std::get<2>(n);
			else THROW_SUPUL_INTERNAL_ERROR0;
		} else if (index == 5) {
			if (std::get<5>(value.get())) ret[name] = "true"; else ret[name] = "false";
		} else THROW_SUPUL_INTERNAL_ERROR0;
	}

	// remove unnecessary values.
	ret.erase("id");
	ret.erase("acc_weak_instance_count");

	return ret;
}

inline void supul_t::report::chunk_history_for_gnuplot(_in const gaenari::common::json_insert_order_map& j, _out common::gnuplot::vnumbers& chunk_histories, _out std::vector<std::string>& chunk_datetime) {
	std::vector<double>  initial_accuracy;
	std::vector<int64_t> total_count;

	// clear output.
	chunk_histories.clear();
	chunk_datetime.clear();

	auto array_size_for_chunk_histroy = j.array_size_noexcept({"category", "chunk_history", "initial_accuracy"});
	if (array_size_for_chunk_histroy == 0) return;

	initial_accuracy.reserve(array_size_for_chunk_histroy);
	for (size_t i=0; i<array_size_for_chunk_histroy; i++) {
		auto v = j.get_value_numeric({"category", "chunk_history", "initial_accuracy", i}, 0.0);
		initial_accuracy.emplace_back(v);
		auto c = j.get_value_numeric({"category", "chunk_history", "total_count", i}, 0);
		total_count.emplace_back(c);
		auto d = j.get_value_numeric({"category", "chunk_history", "datetime", i}, 0LL);
		chunk_datetime.emplace_back(std::to_string(d));
	}

	chunk_histories = common::gnuplot::vnumbers {
		std::move(initial_accuracy), 
		std::move(total_count)
	};
}

inline void supul_t::report::generation_history_for_gnuplot(_in const gaenari::common::json_insert_order_map& j, _out common::gnuplot::vnumbers& generation_histories, _out std::vector<std::string>& generation_datetime) {
	std::vector<double> weak_instance_ratios;
	std::vector<double> before_weak_instance_accuracy;
	std::vector<double> after_weak_instance_accuracy;
	std::vector<double> before_instance_accuracy;
	std::vector<double> after_instance_accuracy;

	// clear output.
	generation_histories.clear();
	generation_datetime.clear();

	auto array_size_for_generation_history = j.array_size_noexcept({"category", "generation_history", "datetime"});
	if (array_size_for_generation_history == 0) return;

	// validation.
	if ((array_size_for_generation_history != j.array_size_noexcept({"category", "generation_history", "weak_instance_ratio"})) or
		(array_size_for_generation_history != j.array_size_noexcept({"category", "generation_history", "before_weak_instance_accuracy"})) or
		(array_size_for_generation_history != j.array_size_noexcept({"category", "generation_history", "after_weak_instance_accuracy"})) or
		(array_size_for_generation_history != j.array_size_noexcept({"category", "generation_history", "before_instance_accuracy"})) or
		(array_size_for_generation_history != j.array_size_noexcept({"category", "generation_history", "after_instance_accuracy"})))
		THROW_SUPUL_ERROR("invalid generation_history data.");

	// reserve.
	weak_instance_ratios.reserve(array_size_for_generation_history);
	before_weak_instance_accuracy.reserve(array_size_for_generation_history);
	after_weak_instance_accuracy.reserve(array_size_for_generation_history);
	before_instance_accuracy.reserve(array_size_for_generation_history);
	after_instance_accuracy.reserve(array_size_for_generation_history);
	generation_datetime.reserve(array_size_for_generation_history);

	// set.
	for (size_t i=0; i<array_size_for_generation_history; i++) {
		auto v1 = j.get_value_numeric({"category", "generation_history", "weak_instance_ratio", i}, 0.0);
		weak_instance_ratios.emplace_back(v1);
		auto v2 = j.get_value_numeric({"category", "generation_history", "before_weak_instance_accuracy", i}, 0.0);
		before_weak_instance_accuracy.emplace_back(v2);
		auto v3 = j.get_value_numeric({"category", "generation_history", "after_weak_instance_accuracy", i}, 0.0);
		after_weak_instance_accuracy.emplace_back(v3);
		auto v4 = j.get_value_numeric({"category", "generation_history", "before_instance_accuracy", i}, 0.0);
		before_instance_accuracy.emplace_back(v4);
		auto v5 = j.get_value_numeric({"category", "generation_history", "after_instance_accuracy", i}, 0.0);
		after_instance_accuracy.emplace_back(v5);
		auto d = j.get_value_numeric({"category", "generation_history", "datetime", i}, 0LL);
		generation_datetime.emplace_back(std::to_string(d));
	}

	generation_histories = common::gnuplot::vnumbers {
		std::move(weak_instance_ratios),
		std::move(before_weak_instance_accuracy),
		std::move(after_weak_instance_accuracy),
		std::move(before_instance_accuracy),
		std::move(after_instance_accuracy)
	};
}

// build two data-block.
// $name << EOD
// ,Apple,Bacon,Cream,Donut,Eclair
// Row 1, 5, 4, 3, 1, 0
// Row 2, 2, 2, 0, 0, 1
// ...
// EOD
// $name_label << EOD
// 5, 4, 3, 1, 0
// 2, 2, 0, 0, 1
// ...
// EOD
inline std::string supul_t::report::global_confusion_matrix_data_block_for_gnuplot(_in const gaenari::common::json_insert_order_map& json, _in const std::string& data_block_name, _out std::vector<std::string>& label_names, _out int64_t& max_count) {
	std::string ret;
	std::string first;
	std::string second;
	std::vector<std::vector<int64_t>> data;
	std::string category_name = "confusion_matrix";

	// clear output.
	label_names.clear();
	max_count = 0;

	auto item_count = json.array_size_noexcept({"category", category_name, "label_name"});
	if (item_count == 0) return "";

	// get label names.
	for (size_t i=0; i<item_count; i++) {
		auto name = json.get_value({"category", category_name, "label_name", i}, std::string(""));
		label_names.emplace_back(std::move(name));
	}

	// set 2-d vector.
	data.resize(item_count);
	for (size_t i=0; i<item_count; i++) data[i].resize(item_count);

	// verify data.
	if (json.array_size({"category", category_name, "data"}) != item_count) THROW_SUPUL_ERROR2("confusion matrix data row size error(%0, %1)", json.array_size({"category", category_name, "data"}), item_count);
	for (size_t row=0; row<item_count; row++) {
		for (size_t col=0; col<item_count; col++)
			if (json.array_size({"category", category_name, "data", row}) != item_count) THROW_SUPUL_ERROR2("confusion matrix data row size error(%0, %1)", json.array_size({"category", category_name, "data", row}), item_count);
	}

	// set data.
	for (size_t row=0; row<item_count; row++) {
		for (size_t col=0; col<item_count; col++) {
			data[row][col] = json.get_value_numeric({"category", category_name, "data", row, col}, 0LL);
			if (max_count < data[row][col]) max_count = data[row][col];
		}
	}

	// script.

	// set first eod.
	first  = F("$%0 << EOD\n", {data_block_name});	// $name << EOD
	first += ',';
	for (const auto& label_name: label_names) first += '\"' + label_name + "\",";
	first.pop_back(); first += '\n';

	// set second eod.
	second = F("$%0_label << EOD\n", {data_block_name});	// $name_label << EOD

	// data.
	size_t row_index = 0;
	for (const auto& row: data) {
		first += '\"' + label_names[row_index++] + "\",";
		for (const auto& col: row) {
			first  += std::to_string(col) + ',';
			second += std::to_string(col) + ',';
		}
		first.pop_back();  first += '\n';
		second.pop_back(); second += '\n';
	}

	first  += "EOD\n";
	second += "EOD\n";

	ret = std::move(first) + '\n' + std::move(second);
	return ret;
}

inline auto supul_t::report::gnuplot(_in const std::string& json, _in const std::map<std::string, std::string>& options) -> std::string {
	std::string ret;
	common::gnuplot g;
	common::gnuplot::vnumbers chunk_histories;
	std::vector<std::string> chunk_datetime;
	std::vector<std::string> label_names;
	common::gnuplot::vnumbers generation_histories;
	std::vector<std::string> generation_datetime;
	std::string cm_min = "-0.5";
	int64_t cm_max_count = 0;
	std::string cm_max;

	// parse json.
	gaenari::common::json_insert_order_map j;
	j.parse(json.c_str());

	// get some data.
	chunk_history_for_gnuplot(j, chunk_histories, chunk_datetime);
	generation_history_for_gnuplot(j, generation_histories, generation_datetime);
	auto global = global_for_gnuplot(j);

	auto confusion_matrix_data_block = global_confusion_matrix_data_block_for_gnuplot(j, "confusion_matrix", label_names, cm_max_count);
	cm_max = gaenari::common::dbl_to_string(static_cast<double>(label_names.size()) - 0.5);

	// config.
	g.short_yyyymmdd(true);

	// set terminal macro.
	g.add_cmd(g.set_terminal_deferred());

	// gnuplot common settings.
	g.add_cmd("\n# common\n");

	// set data_blocks.
	g.add_cmd("\n# data block\n");
	g.add_cmd(g.data_block("data_block_chunk_history", chunk_histories, 0));
	g.add_cmd("\n");
	g.add_cmd(g.data_block("data_block_generation_history", generation_histories, 0));
	g.add_cmd('\n' + confusion_matrix_data_block);

	// script.
	g.add_cmd("\n# script\n");
	g.add_cmd(g.set_multiplot());

	// loop categories.
	auto categories = j.object_names({"category"});
	for (const auto& category: categories) {
		auto& category_name = category.get();
		if (category_name == "global") {
			g.add_cmd("\n# multiplot: global\n");
			g.add_cmd(g.set_origin(0.0, 0));
			g.add_cmd(g.set_size(0.5, 0.33));
			g.add_cmd(g.set_title(category, "font `,15`"));
			g.add_cmd(g.properties(global, 27, common::gnuplot::properties_style{}));
			g.add_cmd(g.reset_all());
		} else if (category_name == "confusion_matrix") {
			g.add_cmd("\n# multiplot: confusion_matrix\n");
			g.add_cmd(g.set_origin(0.5, 0));
			g.add_cmd(g.set_size(0.5, 0.33));
			g.add_cmd(g.set_title(category, "font `,15`"));
			g.add_cmd(g.unset({"key", "cbtics", "xtics"}));
			g.add_cmd(g.set_x2tics(label_names));
			g.add_cmd(g.cmd("set ytics"));
			g.add_cmd(g.cmd(F("set xrange [%0:%1] noreverse nowriteback", {cm_max, cm_min})));
			g.add_cmd(g.cmd("set x2range [*:*] noreverse writeback"));
			g.add_cmd(g.cmd(F("set yrange [%0:%1] noreverse nowriteback", {cm_min, cm_max})));
			g.add_cmd(g.cmd("set y2range [*:*] noreverse writeback"));
			g.add_cmd(g.cmd("set zrange [*:*] noreverse writeback"));
			g.add_cmd(g.cmd("set cblabel `count`"));
			g.add_cmd(g.cmd(F("set cbrange [0:%0] noreverse nowriteback", {cm_max_count})));
			g.add_cmd(g.cmd("set rrange [*:*] noreverse writeback"));
			g.add_cmd(g.cmd("set palette rgbformulae -7, 2, -7"));
			g.add_cmd(g.cmd("set datafile separator comma"));
			g.add_cmd(g.cmd("set lmargin 4"));
			g.add_cmd(g.cmd(F("plot $%0 matrix rowheaders columnheaders using 1:2:3 with image,\\\n"
							  "     $%0_label matrix using 1:2:(sprintf(`%g`,$3)) with labels", {"confusion_matrix"})));
			g.add_cmd(g.cmd("set datafile separator"));
			g.add_cmd(g.reset_all());
			// for arrow.
			g.add_cmd(g.set_origin(0.5, 0.0));
			g.add_cmd(g.set_size(0.5, 0.33));
			g.add_cmd(g.set_title(" ", "font `,13`"));	// moves up 2 by the title font size.
			g.add_cmd(g.unset({"xtics", "ytics", "xlabel", "ylabel", "border"}));
			g.add_cmd(g.cmd("set xrange [0:1]"));
			g.add_cmd(g.cmd("set yrange [0:1]"));
			g.add_cmd(g.cmd("set key off"));
			g.add_cmd(g.cmd("set arrow from 0,1 to 0,0.85 linecolor rgb `dark-gray`"));
			g.add_cmd(g.cmd("set label `actual` at 0.02,1 textcolor rgb `dark-gray`"));
			g.add_cmd(g.cmd("set arrow from 0.8,0.95 to 0.9,0.95 linecolor rgb `dark-gray`"));
			g.add_cmd(g.cmd("set label `predicted` at 0.8,1 textcolor rgb `dark-gray`"));
			g.add_cmd(g.cmd("plot 2 with lines"));
			g.add_cmd(g.reset_all());
		} else if (category_name == "chunk_history") {
			g.add_cmd("\n# multiplot: chunk_history\n");
			g.add_cmd(g.set_origin(0.0, 0.67));
			g.add_cmd(g.set_size(1.0, 0.34));
			g.add_cmd(g.set_title(category, "font `,15`"));
			g.add_cmd(g.set_style_line(1, "linewidth 1 linecolor `light-red`"));
			g.add_cmd(g.set_style_line(2, "linewidth 1 linecolor `blue`"));
			g.add_cmd(g.build_x1y2());
			g.add_cmd(g.set_grid());
			g.add_cmd(g.set_ylabel("initial_accuracy(%)", "textcolor rgb `light-red`"));
			g.add_cmd(g.set_y2label("total_count", "textcolor rgb `blue`"));
			g.add_cmd(g.set_xtics(chunk_datetime, 5));
			// do plot.
			g.begin_plot();
			g.add_plot(g.plot_lines("data_block_chunk_history", 1, "initial_accuracy", 1));
			g.add_plot(g.plot_lines("data_block_chunk_history", 2, "total_count", 2, "x1y2"));
			g.end_plot();
			g.add_cmd(g.reset_all());
		} else if (category_name == "generation_history") {
			g.add_cmd("\n# multiplot: generation_history\n");
			g.add_cmd(g.set_origin(0.0, 0.34));
			g.add_cmd(g.set_size(1.0, 0.33));
			g.add_cmd(g.set_title(category, "font `,15`"));
			g.add_cmd(g.set_style_line(1, "linewidth 1 linecolor `light-red`"));
			g.add_cmd(g.set_style_line(2, "linewidth 1 linecolor `dark-green`"));
			g.add_cmd(g.set_style_line(3, "linewidth 1 linecolor `dark-blue`"));
			g.add_cmd(g.set_style_line(4, "linewidth 1 linecolor `light-magenta`"));
			g.add_cmd(g.set_style_line(5, "linewidth 1 linecolor `orange-red`"));
			g.add_cmd(g.set_grid());
			g.add_cmd(g.cmd("set key"));
			g.add_cmd(g.set_xtics(generation_datetime, 3));
			// do plot.
			g.begin_plot();
			g.add_plot(g.plot_lines("data_block_generation_history", 1, "weak_instance_ratio", 1));
			g.add_plot(g.plot_lines("data_block_generation_history", 2, "before_weak_instance_accuracy", 2));
			g.add_plot(g.plot_lines("data_block_generation_history", 3, "after_weak_instance_accuracy", 3));
			g.add_plot(g.plot_lines("data_block_generation_history", 4, "before_instance_accuracy", 4));
			g.add_plot(g.plot_lines("data_block_generation_history", 5, "after_instance_accuracy", 5));
			g.end_plot();
			g.add_cmd(g.reset_all());
		}
	}
	g.add_cmd(g.unset_multiplot());
	g.run(options);
	return g.get_script();
}

// "chunk_history": {
//		"datetime": [20220101000000, ...],
//		"total_count": [100, ...],
//		"initial_accuracy": [0.99, ...]
// }
inline void supul_t::report::chunk_history_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json) {
	// get chunk information as column data.
	if (not supul.db) THROW_SUPUL_INTERNAL_ERROR0;
	auto chunk = supul.db->get_chunk_for_report();

	// get options.
	auto datetime_as_index = option.get_value({ "datetime_as_index" }, false);

	// get column data as vector.
	auto& datetimes				= chunk["datetime"];
	auto& total_counts			= chunk["total_count"];
	auto& initial_accuracies	= chunk["initial_accuracy"];
	if ((datetimes.size() != total_counts.size()) or (total_counts.size() != initial_accuracies.size())) THROW_SUPUL_ERROR("chunk row count mis-match.");

	// set json array.
	json.set_value_empty_array({"category", "chunk_history", "datetime"});
	json.set_value_empty_array({"category", "chunk_history", "total_count"});
	json.set_value_empty_array({"category", "chunk_history", "initial_accuracy"});

	// exit on empty.
	if (datetimes.empty()) return;

	// yyyymmddhhmmss -> index.
	if (datetime_as_index) {
		// just like: std::iota(datetimes.begin(), datetimes.end(), type::value_variant{0LL});
		for (size_t i = 0; i < datetimes.size(); i++) datetimes[i] = static_cast<int64_t>(i);
	}

	// check data type.
	if (datetimes[0].index() != 1) THROW_SUPUL_INTERNAL_ERROR0;
	if (total_counts[0].index() != 1) THROW_SUPUL_INTERNAL_ERROR0;
	if (initial_accuracies[0].index() != 2) THROW_SUPUL_INTERNAL_ERROR0;

	using path_option = gaenari::common::json_insert_order_map::path_option;
	for (size_t i=0; i<datetimes.size(); i++) {
		json.set_value({"category", "chunk_history", "datetime",		path_option::array_append}, std::get<1>(datetimes[i]));
		json.set_value({"category", "chunk_history", "total_count",		path_option::array_append}, std::get<1>(total_counts[i]));
		json.set_value({"category", "chunk_history", "initial_accuracy",path_option::array_append}, std::get<2>(initial_accuracies[i]));
	}
}

// "global": {
//		"schema_version":1,
//		"instance_count":...,
//		...
// }
inline void supul_t::report::global_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json) {
	if (not supul.db) THROW_SUPUL_INTERNAL_ERROR0;
	auto g = supul.db->get_global();
	if (common::get_variant_int64(g, "instance_count") == 0) THROW_SUPUL_ERROR("empty instance");
	for (auto& it: g) {
		auto& name  = it.first;
		auto& value = it.second;
		if (name == "id") continue;
		auto index = value.index();
		if (index == 1)			json.set_value({"category", "global", name}, std::get<1>(value));
		else if (index == 2)	json.set_value({"category", "global", name}, std::get<2>(value));
		else if (index == 3)	json.get_value({"category", "global", name}, std::get<3>(value));
		else THROW_SUPUL_INTERNAL_ERROR0;
	}
}

// "confusion_matrix": {
//		"label_names": ["a0", "a1"],
//		"data": [[100,2],
//				 [3,400]]
//	}
// vertical: actual, horizontal: predicted.
// actual      a0   a1 --> predicted(classified)
//    |   a0  100    2
//    V   a1    3  400
inline void supul_t::report::confusion_matrix_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json) {
	std::vector<int64_t> ids;
	std::vector<std::string> label_names;
	std::map<int64_t, size_t> id_to_index;
	std::vector<std::vector<int64_t>> data;

	// get global confusion matrix.
	if (not supul.db) THROW_SUPUL_INTERNAL_ERROR0;
	auto cm = supul.db->get_global_confusion_matrix();

	// get labels.
	for (auto& row: cm) {
		if ((row["actual"].index() != 1) or (row["predicted"].index() != 1) or (row["count"].index() != 1)) THROW_SUPUL_INTERNAL_ERROR0;
		if (ids.end() == std::find(ids.begin(), ids.end(), std::get<1>(row["actual"]))) ids.push_back(std::get<1>(row["actual"]));
		if (ids.end() == std::find(ids.begin(), ids.end(), std::get<1>(row["predicted"]))) ids.push_back(std::get<1>(row["predicted"]));
	}

	// by sorting the ids, the order of values in the confusion matrix is fixed.
	std::sort(ids.begin(), ids.end());

	// ids -> names.
	for (auto& id: ids) {
		label_names.emplace_back(supul.string_table.get_table().get_string(id));
		id_to_index[id] = label_names.size() - 1;
	}
	if (label_names.size() <= 1) THROW_SUPUL_ERROR("invalid label names.");

	// set 2-d vector.
	data.resize(label_names.size());
	for (size_t i=0; i<label_names.size(); i++) data[i].resize(label_names.size());

	// set data.
	for (auto& row: cm) {
		int64_t& actual_id    = std::get<1>(row["actual"]);
		int64_t& predicted_id = std::get<1>(row["predicted"]);
		int64_t& count        = std::get<1>(row["count"]);

		auto actual_f = id_to_index.find(actual_id);
		auto predicted_f = id_to_index.find(predicted_id);
		if ((actual_f == id_to_index.end()) or (predicted_f == id_to_index.end())) THROW_SUPUL_INTERNAL_ERROR0;

		auto actual_index = actual_f->second;
		auto predicted_index = predicted_f->second;
		data[actual_index][predicted_index] = count;
	}

	using _path_option = gaenari::common::json_insert_order_map::path_option;
	std::string category_name = "confusion_matrix";
	json.set_value_empty_array({"category", category_name, "label_name"});
	for (const auto& name: label_names)
		json.set_value({"category", category_name, "label_name", _path_option::array_append}, name);

	json.set_value_empty_array({"category", category_name, "data"});
	for (size_t row=0; row<label_names.size(); row++) {
		json.set_value_empty_array({"category", category_name, "data", _path_option::array_append});
		for (size_t col=0; col<label_names.size(); col++) {
			json.set_value({"category", category_name, "data", row, _path_option::array_append}, data[row][col]);
		}
	}
}

// "generation_history": {
//		"datetime": [20220220121212, ...],
//		"weak_instance_ratio": [1.0, ...],
//		"before_weak_instance_accuracy": [0.0, ...],
//		"after_weak_instance_accuracy": [0.99, ...],
//		"before_instance_accuracy": [0.0, ...],
//		"after_instance_accuracy": [0.99, ...]
// }
inline void supul_t::report::generation_history_to_json(_in const gaenari::common::json_insert_order_map& option, _in_out gaenari::common::json_insert_order_map& json) {
	// get generation information as column data.
	if (not supul.db) THROW_SUPUL_INTERNAL_ERROR0;
	auto generation = supul.db->get_generation_for_report();

	// get options.
	auto datetime_as_index = option.get_value({"datetime_as_index"}, false);

	// get column data as vector.
	auto& datetimes = generation["datetime"];

	// get values.
	auto& weak_instance_ratios			= generation["weak_instance_ratio"];
	auto& before_weak_instance_accuracy	= generation["before_weak_instance_accuracy"];
	auto& after_weak_instance_accuracy	= generation["after_weak_instance_accuracy"];
	auto& before_instance_accuracy		= generation["before_instance_accuracy"];
	auto& after_instance_accuracy		= generation["after_instance_accuracy"];
	if (datetimes.size() != weak_instance_ratios.size()) THROW_SUPUL_ERROR2("invalid weak_instance_ratios.", weak_instance_ratios.size(), datetimes.size());
	if (datetimes.size() != before_weak_instance_accuracy.size()) THROW_SUPUL_ERROR2("invalid before_weak_instance_accuracy.", before_weak_instance_accuracy.size(), datetimes.size());
	if (datetimes.size() != after_weak_instance_accuracy.size()) THROW_SUPUL_ERROR2("invalid after_weak_instance_accuracy.", after_weak_instance_accuracy.size(), datetimes.size());
	if (datetimes.size() != before_instance_accuracy.size()) THROW_SUPUL_ERROR2("invalid before_instance_accuracy.", before_instance_accuracy.size(), datetimes.size());
	if (datetimes.size() != after_instance_accuracy.size()) THROW_SUPUL_ERROR2("invalid after_instance_accuracy.", after_instance_accuracy.size(), datetimes.size());

	// yyyymmddhhmmss -> index.
	if (datetime_as_index) {
		// just like: std::iota(datetimes.begin(), datetimes.end(), type::value_variant{0LL});
		for (size_t i=0; i<datetimes.size(); i++) datetimes[i] = static_cast<int64_t>(i);
	}

	// set json array.
	json.set_value_empty_array({"category", "generation_history", "datetime"});
	json.set_value_empty_array({"category", "generation_history", "weak_instance_ratio"});
	json.set_value_empty_array({"category", "generation_history", "before_weak_instance_accuracy"});
	json.set_value_empty_array({"category", "generation_history", "after_weak_instance_accuracy"});
	json.set_value_empty_array({"category", "generation_history", "before_instance_accuracy"});
	json.set_value_empty_array({"category", "generation_history", "after_instance_accuracy"});

	// empty on empty.
	if (datetimes.empty()) return;

	// check data type.
	if ((datetimes[0].index() != 1) or (before_weak_instance_accuracy[0].index() != 2) or
		(after_weak_instance_accuracy[0].index() != 2) or (before_instance_accuracy[0].index() != 2) or
		(after_instance_accuracy[0].index() != 2)) THROW_SUPUL_INTERNAL_ERROR0;

	using path_option = gaenari::common::json_insert_order_map::path_option;
	for (size_t i=0; i<datetimes.size(); i++) {
		json.set_value({"category", "generation_history", "datetime", path_option::array_append}, std::get<1>(datetimes[i]));
		json.set_value({"category", "generation_history", "weak_instance_ratio", path_option::array_append}, std::get<2>(weak_instance_ratios[i]));
		json.set_value({"category", "generation_history", "before_weak_instance_accuracy", path_option::array_append}, std::get<2>(before_weak_instance_accuracy[i]));
		json.set_value({"category", "generation_history", "after_weak_instance_accuracy", path_option::array_append}, std::get<2>(after_weak_instance_accuracy[i]));
		json.set_value({"category", "generation_history", "before_instance_accuracy", path_option::array_append}, std::get<2>(before_instance_accuracy[i]));
		json.set_value({"category", "generation_history", "after_instance_accuracy", path_option::array_append}, std::get<2>(after_instance_accuracy[i]));
	}
}

} // supul
} // supul

#endif // HEADER_GAENARI_SUPUL_SUPUL_IMPL_REPORT_HPP

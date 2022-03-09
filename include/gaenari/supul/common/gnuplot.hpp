#ifndef HEADER_GAENARI_SUPUL_COMMON_GNUPLOT_HPP
#define HEADER_GAENARI_SUPUL_COMMON_GNUPLOT_HPP

namespace supul {
namespace common {

// gnuplot helper.
//
// ` character is replaced with " for better readability.
// ex) cmd("set title \"this is title\"")
//     =
//     cmd("set title `this is title`")

class gnuplot {
public:
	gnuplot() = default;
	~gnuplot() { close(); }

public:
	void run(_in const std::map<std::string, std::string>& options);
	std::string& get_script(void) {return script;}

protected:
	void open(void);
	void close(void);

public:
	inline void short_yyyymmdd(_in bool set) {conf_short_yyyymmdd = set;}

public:
	// type alias.
	using vint			= std::vector<int>;
	using vint64		= std::vector<int64_t>;
	using vdouble		= std::vector<double>;
	using vvariant		= std::variant<vint, vint64, vdouble>;
	using vnumbers		= std::vector<vvariant>;
	using option_int	= std::optional<int>;
	using option_size_t	= std::optional<size_t>;
	using option_str	= std::optional<std::string>;

	// others.
	struct properties_style {
		std::string name_color	= "dark-orange";
		std::string value_color	= "dark-green";
		std::string delim		= ":";
	};

	enum class unset_option {
		all = 0,
		except_style = 1,
	};

	// gnuplot cmds / plots.
	// do not call directly.
	// use helper functions.
public:
	// basic.
	std::string cmd(_in const std::string& text) const;
	std::string data_block(_in const std::string& name, _in const gnuplot::vnumbers& vec_numbers, _option_in int start_x) const;
	std::string set_xtics(_in const std::vector<std::string>& names, _in const option_size_t& count=std::nullopt) const;
	std::string set_x2tics(_in const std::vector<std::string>& names, _in const option_size_t& count=std::nullopt) const;
	std::string set_terminal(_in const std::string& terminal, _in const option_str& etc=std::nullopt) const;
	std::string set_terminal_deferred(void) const;
	std::string set_style_line(_in int linestyle_index, _in const std::string& style) const;
	std::string set_grid(void) const;
	std::string set_title(_in const std::string& title, _in const option_str& add=std::nullopt) const;
	std::string set_ylabel(_in const std::string& name, _in const option_str& add=std::nullopt) const;
	std::string set_y2label(_in const std::string& name, _in const option_str& add=std::nullopt) const;

	// multiplot.
	std::string set_multiplot(void) const;
	std::string set_size(_in double width, _in double height) const;
	std::string set_origin(_in double x, _in double y) const;
	std::string unset_multiplot(void) const;

	// helper.
	std::string build_x1y2(void) const;
	std::string reset_all(void) const;
	std::string unset(_in const std::vector<std::string>& names) const;
	static void check_valid_name(_in const std::string& name);

	// plot.
	std::string plot_lines(_in const std::string& data_block_name, _in size_t data_block_index, _in const std::string& title, _in const option_int& linestyle = std::nullopt, _in const option_str& axis = std::nullopt) const;
	std::string plot_linespoint(_in const std::string& data_block_name, _in size_t data_block_index, _in const std::string& title, _in const option_int& linestyle = std::nullopt, _in const option_str& axis = std::nullopt) const;
	std::string plot_smooth(_in const std::string& data_block_name, _in size_t data_block_index, _in const std::string& title) const;

	// template.
	std::string properties(_in const type::map_variant& data, _in size_t name_width_characters, _in const properties_style& style);

	// add gnuplot elements helper functions.
public:
	// call cmd_* functions.
	void add_cmd(_in const std::string& s);

	// call plot_* functions.
	void begin_plot(void);
	void add_plot(_in const std::string& s);
	void end_plot(void);

protected:
	bool is_gnuplot_installed(void) const;
	static std::string escape(_in const std::string s);
	std::string yyyymmdd(_in const std::string& yyyymmddhhmmss) const;

protected:
	bool installed = false;
	bool conf_short_yyyymmdd = false;
	FILE* pipe = NULL;
	std::string script;
};

inline void gnuplot::open(void) {
	if (not installed) {
		installed = is_gnuplot_installed();
		if (not installed) return;
	}
	
#ifdef _WIN32
	pipe = _popen("gnuplot.exe -persist", "w");
#else
	pipe = popen("gnuplot -persist", "w");
#endif
	if (not pipe) {
#ifdef _WIN32
		char buf[1024] = {0,}; strerror_s(buf, 1024, errno); const char* msg = buf;
#else
		const char* msg = strerror(errno);
#endif
		gaenari::logger::error("fail to popen gnuplot({0}).", {msg});
		return;
	}
}

inline void gnuplot::run(_in const std::map<std::string, std::string>& options) {
	if (not pipe) open();
	if (script.empty()) THROW_SUPUL_ERROR("empty script.");
	if (pipe) {
		std::string terminal_cmd;
		auto& terminal = gaenari::common::map_find(options, "terminal");
		auto  terminal_option = gaenari::common::map_find_noexcept(options, "terminal_option");
		auto  output_filepath = gaenari::common::map_find_noexcept(options, "output_filepath");
		auto  plt_filepath = gaenari::common::map_find_noexcept(options, "plt_filepath");
		terminal_cmd = "set terminal " + terminal;
		if (terminal_option) terminal_cmd += ' ' + terminal_option.value().get();
		terminal_cmd += '\n';
		// ` -> "
		gaenari::common::string_replace(terminal_cmd, "`", "\"");

		if (output_filepath) {
			// path escape.
			auto path = output_filepath.value().get();
			gaenari::common::string_replace(path, "\\", "\\\\");
			terminal_cmd += "set output \"" + path + "\"\n";
		}

		auto s = get_script();
		if (plt_filepath) gaenari::common::save_to_file(plt_filepath.value().get(), s);

		if (not strstr(s.c_str(), "# ${SET_TERMINAL}")) THROW_SUPUL_ERROR("terminal is not deferred.");
		gaenari::common::string_replace(s, "# ${SET_TERMINAL}", terminal_cmd);
		fprintf(pipe, "%s", s.c_str());
		fflush(pipe);
	} else THROW_SUPUL_ERROR("fail to open gnuplot.");
}

inline void gnuplot::close(void) {
	if (not pipe) return;
#ifdef _WIN32
	_pclose(pipe);
#else
	pclose(pipe);
#endif
	pipe = NULL;
}

inline bool gnuplot::is_gnuplot_installed(void) const {
	int exitcode = 0;
	gaenari::logger::info("check gnuplut version.");
	auto version = common::exec("gnuplot --version", &exitcode);
	gaenari::common::trim_right(version, "\n");
	if (version.empty() or (exitcode != 0)) {
		gaenari::logger::warn("gnuplot is not installed. install gnuplot and add environment path.");
#ifdef _WIN32
		gaenari::logger::warn("(do not use gnuplot 5.4.3. it has a bug(https://sourceforge.net/p/gnuplot/bugs/2490/)).");
#endif
		return false;
	}
	gaenari::logger::info("gnuplot({0}) installed.", {version});
	return true;
}

inline std::string gnuplot::escape(_in const std::string s) {
	std::string ret = s;
	gaenari::common::string_replace(ret, "\"", "\\\"");
	return ret;
}

inline std::string gnuplot::yyyymmdd(_in const std::string& yyyymmddhhmmss) const {
	if (not conf_short_yyyymmdd) return yyyymmddhhmmss;
	if (yyyymmddhhmmss.length() != 14) return yyyymmddhhmmss;
	return yyyymmddhhmmss.substr(0,4) + '-' + yyyymmddhhmmss.substr(4,2) + '-' + yyyymmddhhmmss.substr(6,2);
}

inline std::string gnuplot::cmd(_in const std::string& text) const {
	return text + '\n';
}

// auto s = data_block("data", {std::vector<int>({1,2,3,4}), std::vector<double>({1.1,2.2,3.3,4.4})});
// returns,
// $data << EOD
// 0 1 1.1
// 1 2 2.2
// 2 3 3.3
// 3 4 4.4
// EOD
inline std::string gnuplot::data_block(_in const std::string& name, _in const gnuplot::vnumbers& vec_numbers, _option_in int start_x) const {
	std::string ret;
	size_t size = 0;
	size_t row = 0;

	// name must be numeric or alphabetical and _.
	check_valid_name(name);

	// vector size check.
	for (const auto& it: vec_numbers) {
		auto index = it.index();
		size_t _size = 0;
		if (index == 0)			_size = std::get<0>(it).size();
		else if (index == 1)	_size = std::get<1>(it).size();
		else if (index == 2)	_size = std::get<2>(it).size();
		if (_size == 0) THROW_SUPUL_ERROR("empty data can not build data_block.");
		if (size == 0) size = _size;
		if (size != _size) THROW_SUPUL_ERROR2("data size is not same(%0, %1).", size, _size);
	}

	// build.
	ret = "$" + name + " << EOD\n";
	for (row=0; row<size; row++) {
		ret += std::to_string(row + start_x) + ' ';
		for (const auto& it: vec_numbers) {
			auto index = it.index();
			if		(index == 0) ret += std::to_string(std::get<0>(it)[row]) + ' ';
			else if (index == 1) ret += std::to_string(std::get<1>(it)[row]) + ' ';
			else if (index == 2) ret += gaenari::common::dbl_to_string(std::get<2>(it)[row]) + ' ';
		}
		ret.pop_back();
		ret += '\n';
	}
	ret += "EOD\n";
	return ret;
}

inline std::string gnuplot::set_xtics(_in const std::vector<std::string>& names, _in const option_size_t& count) const {
	std::string ret = "set xtics (";
	size_t i = 0;
	size_t step = 1;
	std::vector<size_t> indexes;

	if (count and (count.value() <= 1)) THROW_SUPUL_INTERNAL_ERROR0;
	if (names.empty()) return "set xtics ()\n";

	if (count) {
		if (count.value() < names.size()) {
			step = static_cast<size_t>(static_cast<double>(names.size() - 1) / static_cast<double>(count.value() - 1));
		}
	}

	for (size_t i=0; i<names.size(); i+=step) indexes.emplace_back(i);
	indexes.pop_back();
	indexes.emplace_back(names.size()-1);	// force last item.

	for (auto index: indexes) ret += '`' + yyyymmdd(names[index]) + "` " + std::to_string(index) + ", ";
	ret.pop_back(); ret.pop_back();
	ret += ")\n";
	return ret;
}

inline std::string gnuplot::set_x2tics(_in const std::vector<std::string>& names, _in const option_size_t& count) const {
	std::string ret = "set x2tics (";
	size_t i = 0;
	size_t step = 1;
	std::vector<size_t> indexes;

	if (count and (count.value() <= 1)) THROW_SUPUL_INTERNAL_ERROR0;
	if (names.empty()) return "set xtics ()\n";

	if (count) {
		if (count.value() < names.size()) {
			step = static_cast<size_t>(static_cast<double>(names.size() - 1) / static_cast<double>(count.value() - 1));
		}

		for (size_t i=0; i<names.size(); i+=step) indexes.emplace_back(i);
		indexes.pop_back();
		if (step != 1) indexes.emplace_back(names.size() - 1);	// force last item.
	} else {
		indexes.resize(names.size());
		std::iota(indexes.begin(), indexes.end(), 0);
	}

	for (auto index: indexes) ret += '`' + yyyymmdd(names[index]) + "` " + std::to_string(index) + ", ";
	ret.pop_back(); ret.pop_back();
	ret += ")\n";
	return ret;
}

// set_terminal("wxt");
// set_terminal("wxt", "font `Times-New-Roman,10` size 1000,600");
inline std::string gnuplot::set_terminal(_in const std::string& terminal, _in const option_str& etc) const {
	if (not etc) return F("set terminal %0\n", {terminal});
	return F("set terminal %0 %1\n", {terminal, etc.value()});
}

// determined when get_script() is called.
inline std::string gnuplot::set_terminal_deferred(void) const {
	return "# ${SET_TERMINAL}\n";
}

// style:
//   manual: http://www.gnuplot.info/docs_4.2/node237.html
//   ex) linetype 2 linewidth 2 pointtype 2 pointsize 2 linecolor "#AAAAAA"
//   ex) linetype 2 linewidth 2 pointtype 2 pointsize 2 linecolor "blue"
//   predefined color table: https://ayapin-film.sakura.ne.jp/Gnuplot/Primer/Misc/colornames.html
inline std::string gnuplot::set_style_line(_in int linestyle_index, _in const std::string& style) const {
	return F("set style line %0 %1\n", {linestyle_index, style});
}

inline std::string gnuplot::set_grid(void) const {
	return "set grid\n";
}

// set_title("main_chart", "font `,15`");
inline std::string gnuplot::set_title(_in const std::string& title, _in const option_str& add) const {
	if (not add) return F("set title `%0` noenhanced \n", {escape(title)});
	return F("set title `%0` noenhanced %1\n", {escape(title), add.value()});
}

inline std::string gnuplot::set_ylabel(_in const std::string& name, _in const option_str& add) const {
	if (not add) return F("set ylabel `%0` noenhanced\n", {escape(name)});
	return F("set ylabel `%0` noenhanced %1\n", {escape(name), add.value()});
}

inline std::string gnuplot::set_y2label(_in const std::string& name, _in const option_str& add) const {
	if (not add) return F("set y2label `%0` noenhanced\n", {escape(name)});
	return F("set y2label `%0` noenhanced %1\n", {escape(name), add.value()});	
}

inline std::string gnuplot::set_multiplot(void) const {
	return "set multiplot\n";
}

// ex) set_size(1.0, 0.5)
inline std::string gnuplot::set_size(_in double width, _in double height) const {
	return F("set size %0, %1\n", {gaenari::common::dbl_to_string(width), gaenari::common::dbl_to_string(height)});
}

// ex) set_origin(0.0, 0.3): bottom position is at 0.3
inline std::string gnuplot::set_origin(_in double x, _in double y) const {
	return F("set origin %0, %1\n", {gaenari::common::dbl_to_string(x), gaenari::common::dbl_to_string(y)});
}

inline std::string gnuplot::unset_multiplot(void) const {
	return "unset multiplot\n";
}

inline std::string gnuplot::build_x1y2(void) const {
	return "set ytics nomirror\n"
		   "set y2tics\n";
}

inline std::string gnuplot::reset_all(void) const {
	return "reset\n"
		   "unset key\n";
}

inline std::string gnuplot::unset(_in const std::vector<std::string>& names) const {
	std::string ret;
	for (const auto& i: names) ret += F("unset %0\n", {i});
	return ret;
}

inline void gnuplot::check_valid_name(_in const std::string& name) {
	if (name.empty()) THROW_SUPUL_ERROR("invalid data block name.");
	if (not std::all_of(std::begin(name), std::end(name), [](char c) {
		if (std::isalnum(c)) return true;
		if (c == '_') return true;
		return false;
	})) THROW_SUPUL_ERROR1("invalid name(%0).", name);
	if (not std::isalpha(name[0])) THROW_SUPUL_ERROR1("invalid name(%0).", name);
}

inline void gnuplot::add_cmd(_in const std::string& s) {
	auto _s = s;
	script += gaenari::common::string_replace(_s, "`", "\"");
}

inline void gnuplot::begin_plot(void) {
	script += "plot ";
}

inline void gnuplot::add_plot(_in const std::string& s) {
	auto _s = s;
	script += gaenari::common::string_replace(_s, "`", "\"") + ",\\\n";
}

inline void gnuplot::end_plot(void) {
	if (script.size() <= 3) return;
	auto len = script.length();
	if ((script[len-1] == '\n') and (script[len-2] == '\\') and (script[len-3] == ',')) {
		script.pop_back();
		script.pop_back();
		script.pop_back();
		script += '\n';
	} else THROW_SUPUL_INTERNAL_ERROR0;
}

// axis: "x1y2"
inline std::string gnuplot::plot_linespoint(_in const std::string& data_block_name, _in size_t data_block_index, _in const std::string& title, _in const option_int& linestyle, _in const option_str& axis) const {
	auto ret = F("`$%0` using 1:%1 with linespoint title `%2` noenhanced", {data_block_name, data_block_index + 1, escape(title)});
	if (linestyle) ret += F(" linestyle %0", {linestyle.value()});
	if (axis) ret += F(" axis %0", {axis.value()});
	return ret;
}

// axis: "x1y2"
inline std::string gnuplot::plot_lines(_in const std::string& data_block_name, _in size_t data_block_index, _in const std::string& title, _in const option_int& linestyle, _in const option_str& axis) const {
	auto ret = F("`$%0` using 1:%1 with lines title `%2` noenhanced", {data_block_name, data_block_index + 1, escape(title)});
	if (linestyle) ret += F(" linestyle %0", {linestyle.value()});
	if (axis) ret += F(" axis %0", {axis.value()});
	return ret;
}

inline std::string gnuplot::plot_smooth(_in const std::string& data_block_name, _in size_t data_block_index, _in const std::string& title) const {
	return F("`$%0` using 1:%1 smooth csplines notitle, `$%0` with points title `%2` noenhanced", {data_block_name, data_block_index + 1, escape(title)});
}

inline std::string gnuplot::properties(_in const type::map_variant& data, _in size_t name_width_characters, _in const properties_style& style) {
	double y = 0.0;
	std::string ret;

	if (name_width_characters <= 3) THROW_SUPUL_INTERNAL_ERROR0;

	// set.
	ret += "unset border\n";
	ret += "unset tics\n";
	
	// set names first.
	y = -0.5;
	for (const auto& it: data) {
		const auto& name  = it.first;
		const auto  value = common::get_variant_string(data, name, false);
		// special characters: https://ayapin-film.sakura.ne.jp/Gnuplot/Docs/ps_guide.pdf
		ret += F("set label `{\\267}` back offset 0, %0 font `Symbol,7`\n", {y - 0.1});
		ret += F("set label `%0` back noenhanced offset 1.3, %1 textcolor rgb `%2`\n", {name, y, style.name_color});
		y += -1.0;
	}

	// set delimieter and value.
	int i = 0;
	y = -0.5;
	for (const auto& it : data) {
		const auto& name = it.first;
		const auto  value = common::get_variant_string(data, name, false);
		ret += F("set label `%0` noenhanced front offset %1, %2\n", {style.delim, name_width_characters-2, y});
		ret += F("set label `%0` noenhanced front offset %1, %2 textcolor rgb `%3`\n",
			{value, name_width_characters-1, y, style.value_color});
		y += -1.0;
	}

	// plot.
	ret += "plot [0:1][1:0] NaN notitle\n";

	// revert.
	ret += "set border\nset tics\n";

	return ret;
}

} // common
} // supul

#endif // HEADER_GAENARI_SUPUL_COMMON_GNUPLOT_HPP

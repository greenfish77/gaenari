#include "gaenari-web-config.h"
#include "gaenari/gaenari.hpp"
#include "type.hpp"
#include "exceptions.hpp"
#include "supul_tester.hpp"
#include "global.hpp"
#include "util.hpp"
#include "cpp-httplib/httplib.h"

// $ cmake ..
// $ cmake --build . --config release
// $ cmake --install . --prefix install
// after running the cmake process,
// just run ./install/bin/gaenari-web to start the web server.
// edit ./install/bin/web-root/properties.txt to change the directory.
//
// the properties.txt search order:
//  - ${gaenari-web binary directory}/properties.txt
//  - ${gaenari-web binary directory}/web-root/properties.txt
//  - `gaenari-web-property-file-path` environment value.
//
// you can change the `data_dir` property by setting the `gaenari-web-data-dir` environment variable.
//

int main(void) try {
	util::initialize();
	gaenari::logger::info("**********************");
	gaenari::logger::info("welcome to <yellow>gaenari-web</yellow>", true);
	gaenari::logger::info("**********************");
	util::show_properties();

	// supul instance.
	supul::supul::supul_t supul;
	set_supul(&supul);

	// supul open.
	if (util::get_config("project_created", false)) {
		// project created, but open failed.
		if (not supul.api.lifetime.open(path.project_dir)) ERROR0("fail to supul open.");
	}

	// http server context.
	httplib::Server svr;

	// static file server.
	if (not svr.set_mount_point("/www", path.www_dir)) ERROR0("fail to set_mount_point.");

	// set logger.
	svr.set_logger([](const httplib::Request& req, const httplib::Response& res) {
		gaenari::logger::info("[http] {0} {1} -> status:{2}", {req.method, req.path, res.status});
	});

	// set exception handler.
	svr.set_exception_handler([](const auto& req, auto& res, std::exception& e) {
		std::string w = e.what();
		gaenari::logger::error("<red>fail</red>: <yellow>" + w + "</yellow>", true);
		res.status = 500;
		std::string content = "<h1>Internal Server Error 500</h1><p>" + w + "</p>";
		res.set_content(content, "text/html");
	});

	svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
		// set redirect / -> /index.html or /create_project.html
		if (req.path == "/") {
			res.status = 302; // temporarily.
			// the redirect depends on whether the project exists or not.
			if (util::get_config("project_created", false)) {
				res.set_redirect("/www/index.html");				
			} else {
				res.set_redirect("/www/create_project.html");
			}
			return httplib::Server::HandlerResponse::Handled;
		}
		return httplib::Server::HandlerResponse::Unhandled;
	});

	svr.set_file_request_handler([](const httplib::Request& req, httplib::Response& res) {
		if (0) {
			res.location = "/www/index.html";
			res.body = "/www/index.html";
			res.status = 302;
			return;
		}
		// res.body = "aaaa";
		// res.content_length_ = 4;
	});

	svr.Get("/api/v1/report", [&supul](const httplib::Request& req, httplib::Response& res) {
		auto report = supul.api.report.json("");
		res.set_content(report, "application/json");
	});

	svr.Get("/api/v1/global", [&supul](const httplib::Request& req, httplib::Response& res) {
		auto global = supul_tester(supul).get_db().get_global();
		res.set_content(util::to_json_map_variant(global), "application/json");
	});

	svr.Get("/api/v1/project/name", [&supul](const httplib::Request& req, httplib::Response& res) {
		auto project_name = util::get_config("project_name", "");
		res.set_content(util::to_json_map_variant(supul::type::map_variant{{"project_name", project_name}}), "application/json");
	});

	svr.Post("/api/v1/project", [](const httplib::Request& req, httplib::Response& res) {
		// req.params["database_type"]	= "sqlite"
		// req.params["index0"]			= "attrib_1/INTEGER/X"
		// ...
		// req.params["index5"]			= "attrib_5/TEXT_ID/Y"
		// req.params["project_name"]	= "prj_name"
		// remark 1) attribute name / data type / selection(X, Y and blank)
		// remark 2) n of indexn such as index0, index1, ... may not start from 0,
		//           or may not be contiguous.
		
		// valid check.
		util::check_map_has_keys(req.params, {"project_name", "database_type"});
		if (req.params.size() <= 4) ERROR0("not enough parameters.");

		// get data.
		std::string project_name;
		std::string database_type;
		std::vector<std::string> attributes;
		for (const auto& it: req.params) {
			const auto& name  = it.first;
			const auto& value = it.second;
			if (name == "project_name")  { project_name  = value; continue; }
			if (name == "database_type") { database_type = value; continue; }
			if (strncmp(name.c_str(), "index", 5) != 0) ERROR1("unknown parameter name: %0.", name);
			attributes.emplace_back(value);
		}

		// create project and set data.
		using supul_t = supul::supul::supul_t;
		if (not supul_t::api::project::create(path.project_dir)) ERROR0("fail to create project.");
		if (not supul_t::api::project::set_property(path.project_dir, "db.type", database_type)) ERROR0("fail to set database type.");
		std::vector<std::string> x;
		std::string y;
		for (const auto& attribute: attributes) {
			std::vector<std::string> items;
			gaenari::dataset::csv_reader::parse_delim(attribute, '/', items);
			if (items.size() != 3) ERROR1("invalid attribute data: %0.", attribute);
			const auto& name = items[0];
			const auto& type = items[1];
			const auto& sel  = items[2];
			if (name.empty() or type.empty()) ERROR1("invalid attribute data: %0.", attribute);
			if (not supul_t::api::project::add_field(path.project_dir, name, type)) ERROR1("fail to add_field, %0.", attribute);
			if (sel == "X") x.emplace_back(name);
			else if (sel == "Y") {
				if (not y.empty()) ERROR0("already used y.");
				y = name;
			}
		}
		if (not supul_t::api::project::x(path.project_dir, x)) ERROR0("fail to set project x.");
		if (not supul_t::api::project::y(path.project_dir, y)) ERROR0("fail to set project y.");

		// create_project completed.
		util::set_config("project_created", "true");
		util::set_config("project_name", project_name);

		// go to index.html.
		res.status = 302; // temporarily.
		res.set_redirect("/");
	});

	svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
		res.set_content("Hello World!", "text/plain");
	});

	svr.listen(option.server.host.c_str(), option.server.port);

	supul.api.lifetime.close();
	set_supul(nullptr);
	return 0;
} catch(const error& e) {
	std::string text;
	std::string w = e.what();
	gaenari::logger::error("<red>fail</red>: <yellow>" + w + "</yellow>", true);
	gaenari::logger::error("      <lmagenta>" + e.pretty_function + "</lmagenta>", true);
	gaenari::logger::error("      <lyellow>" + e.file + "</lyellow> @ <lyellow>" + std::to_string(e.line) + "</lyellow>", true);
	return 1;
} catch (...) {
	supul::exceptions::catch_all();
	return 1;
}

// under windows, memory leak can be reported.
#ifdef _WIN32
static struct DUMP_MEMORY_LEAK {DUMP_MEMORY_LEAK() {_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);}} g_dml;
#endif

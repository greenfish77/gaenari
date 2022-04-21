#include "gaenari/gaenari.hpp"
#include "cpp-httplib/httplib.h"
#include "type.hpp"
#include "global.hpp"
#include "exceptions.hpp"
#include "util.hpp"

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
			if (util::get_config_status("project_created", false)) {
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

	svr.Post("/api/v1/project", [](const httplib::Request& req, httplib::Response& res) {
		// res.set_content("Hello World!", "text/plain");
		res.status = 302; // temporarily.
		res.set_redirect("/");
	});

	svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
		res.set_content("Hello World!", "text/plain");
	});

	svr.listen(option.server.host.c_str(), option.server.port);
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

#include <stdio.h>
#include "cpp-httplib/httplib.h"
#include "gaenari/gaenari.hpp"

// $ cmake ..
// $ cmake --build . --config release
// $ cmake --install . --prefix install
// after running the cmake process,
// just run ./install/bin/gaenari-web to start the web server.
// edit ./install/bin/web-root/properties.txt to change the directory.
//
// the properties.txt search order:
//  - ${target binary directory}/properties.txt
//  - ${target binary directory}/web-root/properties.txt
//
// you can easily develop with the following settings:
//   dir_www  = <gaenari_repo_dir>/gaenari-web/web-root/www
//   dir_data = <some temp directory>
//
int main(void) {
	// HTTP
	httplib::Server svr;

	svr.Get("/hi", [](const httplib::Request&, httplib::Response& res) {
		res.set_content("Hello World!", "text/plain");
		});

	svr.listen("0.0.0.0", 8080);
}

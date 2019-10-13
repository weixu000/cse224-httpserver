#include <iostream>
#include <sysexits.h>

#include "inih/INIReader.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "HttpdServer.hpp"

namespace {
    void initLogging() {
        auto err_logger = spdlog::stderr_color_mt("stderr");
        spdlog::set_level(spdlog::level::debug);
        spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] [thread %t] %v");
        spdlog::set_default_logger(err_logger); // set this as default
    }
}

int main(int argc, char **argv) {
    using namespace std;

    initLogging();

    // Handle the command-line argument
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " [config_file]" << endl;
        return EX_USAGE;
    }

    // Read in the configuration file
    INIReader config(argv[1]);

    if (config.ParseError() < 0) {
        cerr << "Error parsing config file " << argv[1] << endl;
        return EX_CONFIG;
    }

    if (config.GetBoolean("httpd", "enabled", true)) {
        spdlog::info("Web server enabled");

        auto port = config.Get("httpd", "port", "");
        auto root = config.Get("httpd", "doc_root", "");
        auto mime_path = config.Get("httpd", "mime_types", "");
        if (port.empty() || root.empty() || mime_path.empty()) {
            spdlog::error("Not enough parameters for httpd");
            return EX_CONFIG;
        }

        HttpdServer httpd(port, root, mime_path);
        httpd.launch();
    } else {
        spdlog::info("Web server disabled");
    }

    return 0;
} 

/*
 * SPDX-FileCopyrightText: 2026 M5Stack Technology CO LTD
 *
 * SPDX-License-Identifier: MIT
 */

#include "app.h"

#include <string>

int main(int argc, char** argv) {
    std::string socket_path;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--socket" && i + 1 < argc) {
            socket_path = argv[++i];
        }
        else if (arg.rfind("--socket=", 0) == 0) {
            socket_path = arg.substr(9);
        }
    }
    app::Application application;
    return application.run(socket_path);
}

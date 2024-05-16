#ifndef XRPLORER_SHELL_HPP
#define XRPLORER_SHELL_HPP

#include <xrplorer/export.hpp>
#include <xrplorer/OperatingSystem.hpp>

#include <cstdio>
#include <string_view>

namespace xrplorer {

class XRPLORER_EXPORT Shell {
private:
    OperatingSystem& os_;
    std::FILE* out_;

public:
    Shell(OperatingSystem& os, std::FILE* out) : os_(os), out_(out) {}

    int main(int argc, char** argv);

private:
    void ls(std::string_view path) {
        auto abspath = os_.getcwd() / path;
        std::fprintf(out_, "list contents: %s\n", abspath.c_str());
    }

    int exit(int argc, char** argv);
};

}

#endif

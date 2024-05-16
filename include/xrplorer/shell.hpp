#ifndef XRPLORER_SHELL_HPP
#define XRPLORER_SHELL_HPP

#include <xrplorer/export.hpp>
#include <xrplorer/operating-system.hpp>

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
    int cat(int argc, char** argv);
    int cd(int argc, char** argv);
    int echo(int argc, char** argv);
    int exit(int argc, char** argv);
    int hostname(int argc, char** argv);
    int ls(int argc, char** argv);
    int pwd(int argc, char** argv);
};

}

#endif

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <xrplorer/xrplorer.hpp>

int main(int argc, char** argv) {
    xrplorer::OperatingSystem os;
    xrplorer::Shell shell{os, stdout};
    return shell.main(argc, argv);
}

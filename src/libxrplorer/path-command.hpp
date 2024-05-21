#ifndef XRPLORER_PATH_COMMAND_HPP
#define XRPLORER_PATH_COMMAND_HPP

#include <xrplorer/operating-system.hpp>

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <string>

namespace xrplorer {

namespace fs = std::filesystem;

class PathCommand {
public:
    /**
     * The action to take at the end of a path.
     */
    enum Action {
        CD,
        LS,
        CAT,
    };

    struct Exception {
        int code;
        fs::path path;
        std::string message;
    };

    OperatingSystem& os_;
    std::FILE* out_;
    fs::path path_;
    Action action_;

public:
    PathCommand(OperatingSystem& os, std::FILE* out, fs::path path, Action action)
        : os_(os), out_(out), path_(path), action_(action)
    {
    }

    void execute() {
        auto it = path_.begin();
        assert(*it == "/");
        rootLayer(std::next(it));
    }

private:
    void notFile(fs::path::iterator it);
    void notDirectory(fs::path::iterator it);
    void notExists(fs::path::iterator it);
    void rootLayer(fs::path::iterator it);
    void nodesLayer(fs::path::iterator it);
};

}

#endif

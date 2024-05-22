#ifndef XRPLORER_PATH_COMMAND_HPP
#define XRPLORER_PATH_COMMAND_HPP

#include <xrplorer/operating-system.hpp>

#include <xrpl/basics/base_uint.h>
#include <xrpl/nodestore/NodeObject.h>

#include <cassert>
#include <filesystem>
#include <iterator>
#include <string>
#include <string_view>

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
    fs::path path_;
    fs::path::iterator it_;
    Action action_;

public:
    PathCommand(OperatingSystem& os, fs::path path, Action action)
        : os_(os), path_(path.lexically_normal()), it_(path_.begin()), action_(action)
    {
        assert(*it_ == "/");
        ++it_;
        rootLayer();
    }

private:
    void throw_(int code, std::string_view message);
    void notFile();
    void notDirectory();
    void notExists();
    void rootLayer();
    void nodesLayer();
    void nodeBranch(ripple::uint256 const& digest);
    void headerLayer(ripple::NodeObject const& object);
};

}

#endif

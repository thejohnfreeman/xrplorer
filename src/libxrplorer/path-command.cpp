#include <src/libxrplorer/path-command.hpp>

#include <xrpl/basics/base_uint.h>

#include <functional>
#include <iterator>
#include <numeric>

namespace xrplorer {

fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
    return std::accumulate(begin, end, fs::path{}, std::divides{});
}

enum ErrorCode {
    DOES_NOT_EXIST,
    NOT_A_FILE,
    NOT_A_DIRECTORY,
    NOT_A_DIGEST,
};

void PathCommand::throw_(int code, std::string_view message) {
    throw Exception{code, make_path(path_.begin(), std::next(it_)), std::string{message}};
}

void PathCommand::notFile() {
    return throw_(NOT_A_FILE, "not a file");
}

void PathCommand::notDirectory() {
    return throw_(NOT_A_DIRECTORY, "not a directory");
}

void PathCommand::notExists() {
    return throw_(DOES_NOT_EXIST, "no such file or directory");
}

void PathCommand::rootLayer() {
    if (it_ != path_.end()) {
        auto const& name = *it_;
        if (name == "nodes") {
            ++it_;
            return nodesLayer();
        }
        return notExists();
    }
    if (action_ == CD) {
        os_.chdir(path_.native());
        os_.setenv("PWD", path_.native());
        return;
    }
    if (action_ == LS) {
        std::fprintf(os_.stdout, "nodes\n");
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

void PathCommand::nodesLayer() {
    if (it_ != path_.end()) {
        auto const& name = *it_;
        ripple::uint256 digest;
        if (!digest.parseHex(name)) {
            return throw_(NOT_A_DIGEST, "not a digest");
        }
        auto object = os_.db()->fetchNodeObject(digest);
        if (!object) {
            return notExists();
        }
        std::fprintf(os_.stdout, "%d\n", object->getType());
        return;
    }
    if (action_ == CD) {
        os_.chdir(path_.native());
        os_.setenv("PWD", path_.native());
        return;
    }
    if (action_ == LS) {
        std::fprintf(os_.stdout, "node ids...\n");
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

}

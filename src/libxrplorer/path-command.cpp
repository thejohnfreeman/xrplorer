#include <src/libxrplorer/path-command.hpp>

#include <functional>
#include <iterator>
#include <numeric>

namespace xrplorer {

fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
    return std::accumulate(begin, end, fs::path{}, std::divides{});
}

void PathCommand::notFile(fs::path::iterator it) {
    throw Exception{1, path_, "not a file"};
}

void PathCommand::notDirectory(fs::path::iterator it) {
    throw Exception{2, make_path(path_.begin(), std::next(it)), "not a directory"};
}

void PathCommand::notExists(fs::path::iterator it) {
    throw Exception{3, make_path(path_.begin(), std::next(it)), "no such file or directory"};
}

void PathCommand::rootLayer(fs::path::iterator it) {
    if (it != path_.end()) {
        auto name = *it;
        if (name == "nodes") {
            return nodesLayer(std::next(it));
        }
        return notExists(it);
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
        return notFile(it);
    }
}

void PathCommand::nodesLayer(fs::path::iterator it) {
    if (it != path_.end()) {
        auto name = *it;
        // TODO: Load the named node.
        std::fprintf(os_.stdout, "digest: %s\n", name.c_str());
        return notExists(it);
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
        return notFile(it);
    }
}

}

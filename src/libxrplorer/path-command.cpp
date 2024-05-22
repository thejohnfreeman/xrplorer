#include <src/libxrplorer/path-command.hpp>

#include <xrpl/protocol/Serializer.h>

#include <fmt/core.h>
#include <fmt/std.h>

#include <functional>
#include <iterator>
#include <numeric>

namespace ripple {
template <std::size_t Bits, class Tag>
auto format_as(base_uint<Bits, Tag> const& uint) {
    return to_string(uint);
}
}

namespace xrplorer {

fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
    return std::accumulate(begin, end, fs::path{}, std::divides{});
}

// TODO: Pair these with their message strings.
enum ErrorCode {
    // Path is not an entry in its parent directory.
    DOES_NOT_EXIST,
    NOT_A_FILE,
    NOT_A_DIRECTORY,
    NOT_A_DIGEST,
    // Path contents are missing.
    OBJECT_MISSING,
    TYPE_UNKNOWN,
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
        auto const& name = *it_++;
        if (name == "nodes") {
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
        fmt::print(os_.stdout, "nodes\n");
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

void PathCommand::nodesLayer() {
    if (it_ != path_.end()) {
        auto const& name = *it_++;
        ripple::uint256 digest;
        if (!digest.parseHex(name)) {
            return throw_(NOT_A_DIGEST, "not a digest");
        }
        return nodeBranch(digest);
    }
    if (action_ == CD) {
        os_.chdir(path_.native());
        os_.setenv("PWD", path_.native());
        return;
    }
    if (action_ == LS) {
        fmt::print(os_.stdout, "node ids...\n");
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

void PathCommand::nodeBranch(ripple::uint256 const& digest) {
    auto object = os_.db()->fetchNodeObject(digest);
    if (!object) {
        return throw_(OBJECT_MISSING, "object missing");
    }
    switch (object->getType()) {
        case ripple::hotLEDGER: return headerLayer(*object);
    }
    return throw_(TYPE_UNKNOWN, "type unknown");
}

void PathCommand::headerLayer(ripple::NodeObject const& object) {
    auto const& slice = object.getData();
    ripple::SerialIter sit(slice.data(), slice.size());
    auto sequence = sit.get32();
    sit.skip(8);
    auto parentDigest = sit.get256();
    auto txnsDigest = sit.get256();
    auto stateDigest = sit.get256();
    if (it_ != path_.end()) {
        auto const& name = *it_++;
        if (name == "parent") {
            return nodeBranch(parentDigest);
        }
        if (name == "txns") {
            return nodeBranch(txnsDigest);
        }
        if (name == "state") {
            return nodeBranch(stateDigest);
        }
        return notExists();
    }
    if (action_ == CD) {
        os_.chdir(path_.native());
        os_.setenv("PWD", path_.native());
        return;
    }
    if (action_ == LS) {
        fmt::print(os_.stdout, "sequence\n");
        fmt::print(os_.stdout, "parent -> /nodes/{}\n", parentDigest);
        fmt::print(os_.stdout, "txns -> /nodes/{}\n", txnsDigest);
        fmt::print(os_.stdout, "state -> /nodes/{}\n", stateDigest);
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}


}

#include <src/libxrplorer/path-command.hpp>

#include <fmt/core.h>
#include <fmt/std.h>
#include <xrpl/protocol/LedgerHeader.h>

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

void PathCommand::skipEmpty() {
    for (; it_ != path_.end() && (*it_ == "" || *it_ == "."); ++it_);
}

void PathCommand::rootDirectory() {
    skipEmpty();
    if (it_ != path_.end()) {
        auto const& name = *it_++;
        if (name == "nodes") {
            return nodesDirectory();
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

void PathCommand::nodesDirectory() {
    skipEmpty();
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
        case ripple::hotLEDGER: return headerDirectory(*object);
    }
    return throw_(TYPE_UNKNOWN, "type unknown");
}

void PathCommand::headerDirectory(ripple::NodeObject const& object) {
    skipEmpty();
    // TODO: Shouldn't Slice have an implicit constructor from vector of bytes?
    auto const& slice = ripple::makeSlice(object.getData());
    ripple::LedgerHeader header{ripple::deserializePrefixedHeader(slice)};
    if (it_ != path_.end()) {
        auto const& name = *it_++;
        if (name == "sequence") {
            return valueFile(header.seq);
        }
        if (name == "parent") {
            return nodeBranch(header.parentHash);
        }
        if (name == "txns") {
            return nodeBranch(header.txHash);
        }
        if (name == "state") {
            return nodeBranch(header.accountHash);
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
        fmt::print(os_.stdout, "parent -> /nodes/{}\n", header.parentHash);
        fmt::print(os_.stdout, "txns -> /nodes/{}\n", header.txHash);
        fmt::print(os_.stdout, "state -> /nodes/{}\n", header.accountHash);
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

template <typename T>
void PathCommand::valueFile(T const& value) {
    if (it_ != path_.end()) {
        return notDirectory();
    }
    if (action_ == CD) {
        return notDirectory();
    }
    if (action_ == LS) {
        fmt::print(os_.stdout, "{}\n", path_);
        return;
    }
    if (action_ == CAT) {
        fmt::print(os_.stdout, "{}\n", value);
        return;
    }
}

}

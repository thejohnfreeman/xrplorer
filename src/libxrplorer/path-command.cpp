#include <src/libxrplorer/path-command.hpp>

#include <fmt/core.h>
#include <fmt/std.h>
#include <xrpl/protocol/LedgerHeader.h>
#include <xrpl/beast/utility/Zero.h>

#include <functional>
#include <iterator>
#include <numeric>
#include <utility>

namespace ripple {
template <std::size_t Bits, class Tag>
auto format_as(base_uint<Bits, Tag> const& uint) {
    return to_string(uint);
}
auto format_as(NodeObjectType const& type) {
    return to_string(type);
}
auto format_as(HashPrefix prefix) {
    // Prefix is 3 ASCII characters packed into a std::uint32_t.
    // TODO: Can we get down to one string allocation?
    std::uint32_t i = static_cast<std::underlying_type_t<HashPrefix>>(prefix);
    char a = (i >> 24) & 0xFF;
    char b = (i >> 16) & 0xFF;
    char c = (i >>  8) & 0xFF;
    return fmt::format("0x{:X} ({}{}{})", i, a, b, c);
}
}

namespace xrplorer {

ripple::HashPrefix deserializePrefix(ripple::Slice const& slice) {
    ripple::SerialIter sit{slice.data(), slice.size()};
    auto prefix = ripple::safe_cast<ripple::HashPrefix>(sit.get32());
    return prefix;
}

fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
    return std::accumulate(begin, end, fs::path{}, std::divides{});
}

// TODO: Pair these with their message strings.
enum ErrorCode {
    NOT_IMPLEMENTED,
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
    auto const& path = make_path(path_.begin(), it_);
    throw Exception{code, path, std::string{message}};
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

void PathCommand::notImplemented() {
    return throw_(NOT_IMPLEMENTED, "not implemented");
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
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
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
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
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
    auto const& slice = ripple::makeSlice(object->getData());
    auto prefix = deserializePrefix(slice);
    switch (prefix) {
        case ripple::HashPrefix::ledgerMaster: return headerDirectory(*object);
        case ripple::HashPrefix::txNode: return sndDirectory(*object);
        case ripple::HashPrefix::innerNode: return innerDirectory(*object);
    }
    spdlog::error("type unknown: {}", prefix);
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
            return stateDirectory(header.accountHash);
        }
        return notExists();
    }
    if (action_ == CD) {
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
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

void PathCommand::stateDirectory(ripple::uint256 const& digest) {
    skipEmpty();
    if (it_ != path_.end()) {
        auto const& name = *it_++;
        if (name == "root") {
            return nodeBranch(digest);
        }
        return notImplemented();
    }
    if (action_ == CD) {
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
        return;
    }
    if (action_ == LS) {
        fmt::print(os_.stdout, "accounts\n");
        fmt::print(os_.stdout, "root -> /nodes/{}\n", digest);
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

void PathCommand::innerDirectory(ripple::NodeObject const& object) {
    skipEmpty();
    auto const& slice = ripple::makeSlice(object.getData());
    ripple::SerialIter sit(slice.data(), slice.size());
    // Consume the prefix.
    sit.get32();
    // libxrpl does not have a deserialized representation of inner nodes.
    // An inner node is a directory with a subdirectory for each non-null child.
    if (it_ != path_.end()) {
        auto name = (it_++)->generic_string();
        // `name` must be one hexadecimal character from 0 to F.
        if (name.length() != 1) {
            return notExists();
        }
        char const c = name[0];
        int index;
        if (c >= '0' && c <= '9') {
            index = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            index = 10 + c - 'A';
        } else {
            return notExists();
        }
        sit.skip((256 / 8) * index);
        auto childDigest = sit.get256();
        return nodeBranch(childDigest);
    }
    if (action_ == CD) {
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
        return;
    }
    if (action_ == LS) {
        // TODO: Is this constant exported by libxrpl?
        // SHAMapInnerNode::branchFactor is not exported.
        constexpr auto branchFactor = 16;
        for (auto i = 0; i < branchFactor; ++i) {
            auto childDigest = sit.get256();
            if (childDigest == beast::zero)
            {
                continue;
            }
            fmt::print(os_.stdout, "{:X}\n", i);
        }
        return;
    }
    if (action_ == CAT) {
        // TODO: Should we add a file named "blob"?
        return notFile();
    }
}

void PathCommand::sndDirectory(ripple::NodeObject const& object) {
    skipEmpty();
    return notImplemented();
}

template <typename T>
void PathCommand::valueFile(T const& value) {
    if (it_ != path_.end()) {
        ++it_;
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

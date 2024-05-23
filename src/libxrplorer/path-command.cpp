#include <src/libxrplorer/path-command.hpp>

#include <xrplorer/tlpush.hpp>

#include <fmt/core.h>
#include <fmt/std.h>
#include <xrpl/protocol/AccountID.h>
#include <xrpl/protocol/Indexes.h>
#include <xrpl/protocol/Keylet.h>
#include <xrpl/protocol/LedgerFormats.h>
#include <xrpl/protocol/LedgerHeader.h>
#include <xrpl/protocol/STLedgerEntry.h>
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
auto format_as(LedgerEntryType type) {
    return fmt::format("0x{:04X}", std::underlying_type_t<LedgerEntryType>(type));
}
auto format_as(HashPrefix prefix) {
    // Prefix is 3 ASCII characters packed into a std::uint32_t.
    std::uint32_t i = static_cast<std::underlying_type_t<HashPrefix>>(prefix);
    char a = (i >> 24) & 0xFF;
    char b = (i >> 16) & 0xFF;
    char c = (i >>  8) & 0xFF;
    return fmt::format("0x{:X} ({}{}{})", i, a, b, c);
}
}

namespace xrplorer {

static constexpr int NBYTES_DIGEST = 256 / 8;

ripple::HashPrefix deserializePrefix(ripple::Slice const& slice) {
    ripple::SerialIter sit{slice.data(), slice.size()};
    auto prefix = ripple::safe_cast<ripple::HashPrefix>(sit.get32());
    return prefix;
}

fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
    return std::accumulate(begin, end, fs::path{}, std::divides{});
}

SLE make_sle(NodePtr const& object) {
    auto slice = ripple::makeSlice(object->getData());
    slice.remove_prefix(4);
    ripple::Serializer serializer{slice.data(), slice.size()};
    ripple::uint256 key;
    bool success = serializer.getBitString(key, serializer.size() - key.bytes);
    assert(success);
    serializer.chop(key.bytes);
    auto slice2 = serializer.slice();
    ripple::SerialIter sit{slice2.data(), slice2.size()};
    return SLE{sit, key};
}

SLE make_sle(ripple::Keylet const& keylet, NodePtr const& object) {
    // A `Slice` is passed to `SHAMapTreeNode::makeFromPrefix`
    // which removes the 4 byte prefix.
    auto slice = ripple::makeSlice(object->getData());
    slice.remove_prefix(4);
    // It passes the remaining slice to `SHAMapTreeNode::makeAccountState`
    // which constructs a `Serializer`
    // (which calls itself the replacement for now-deprecated `SerialIter`)
    // and removes the 32 byte suffix.
    // (That suffix should match the keylet key, by the way).
    ripple::Serializer serializer{slice.data(), slice.size()};
    serializer.chop(32);
    // Then it constructs a _new_ `Slice` from the `Serializer`
    // and passes it to `ripple::make_shamapitem`,
    // which `memcpy`s the bytes into a `SHAMapItem`.
    // I don't know where an `STObject` is ever constructed by `SHAMap`,
    // but that final slice is the one it is expecting.
    auto slice2 = serializer.slice();
    // An `STLedgerEntry` cannot be constructed
    // with a `Slice` or a `Serializer`, though. Only a `SerialIter`.
    ripple::SerialIter sit{slice2.data(), slice2.size()};
    return SLE{sit, keylet.key};
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
        fmt::print(os_.stdout, "<node ID>\n");
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
        case ripple::HashPrefix::ledgerMaster: return headerDirectory(object);
        case ripple::HashPrefix::txNode: return sndDirectory(object);
        case ripple::HashPrefix::innerNode: return innerDirectory(object);
        case ripple::HashPrefix::leafNode: return leafDirectory(object);
    }
    spdlog::error("type unknown: {}", prefix);
    return throw_(TYPE_UNKNOWN, "type unknown");
}

void PathCommand::headerDirectory(NodePtr const& object) {
    skipEmpty();
    // TODO: Shouldn't Slice have an implicit constructor from vector of bytes?
    auto const& slice = ripple::makeSlice(object->getData());
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
        if (name == "accounts") {
            auto root = os_.db()->fetchNodeObject(digest);
            if (!root) {
                return notExists();
            }
            tlpush _root{root_, std::move(root)};
            return accountsDirectory();
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

// TODO: Is this constant exported by libxrpl?
// SHAMapInnerNode::branchFactor is not exported.
constexpr auto BRANCH_FACTOR = 16;

unsigned int selectBranch(int depth, ripple::uint256 const& key) {
    auto branch = static_cast<unsigned int>(*(key.begin() + (depth / 2)));
    if (depth & 1)
        branch &= 0xf;
    else
        branch >>= 4;
    assert(branch < BRANCH_FACTOR);
    return branch;
}

NodePtr PathCommand::load(ripple::Keylet const& keylet) {
    assert(root_);
    NodePtr object{root_};
    // One-past-end depth is 256 / 4 = 64.
    for (auto depth = 0; depth < 64; ++depth) {
        auto const& slice = ripple::makeSlice(object->getData());
        ripple::SerialIter sit{slice.data(), slice.size()};
        auto prefix = ripple::safe_cast<ripple::HashPrefix>(sit.get32());
        if (prefix == ripple::HashPrefix::leafNode) {
            break;
        }
        if (prefix != ripple::HashPrefix::innerNode) {
            return {};
        }
        auto childIndex = selectBranch(depth, keylet.key);
        sit.skip(NBYTES_DIGEST * childIndex);
        auto childDigest = sit.get256();
        if (childDigest == beast::zero) {
            return {};
        }
        object = os_.db()->fetchNodeObject(childDigest);
        if (!object) {
            return {};
        }
    }
    return object;
}

void PathCommand::accountsDirectory() {
    skipEmpty();
    if (it_ != path_.end()) {
        auto const& b58AccountID = *it_++;
        auto optAccountID = ripple::parseBase58<ripple::AccountID>(b58AccountID);
        if (!optAccountID) {
            return notExists();
        }
        auto keylet = ripple::keylet::account(*optAccountID);
        auto object = load(keylet);
        if (!object) {
            return notExists();
        }
        auto account = make_sle(keylet, object);
        return sleDirectory(account);
    }
    if (action_ == CD) {
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
        return;
    }
    if (action_ == LS) {
        fmt::print(os_.stdout, "<base58 address, e.g. rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh>\n");
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

void PathCommand::sleDirectory(SLE const& sle) {
    // `root` is unused for now, but we'll want it later to follow links.
    skipEmpty();
    if (it_ != path_.end()) {
        auto const& name = *it_++;
        if (name == ".key") {
            return valueFile(sle.key());
        }
        for (auto const& field : sle) {
            if (field.getFName().getName() == name) {
                return sfieldFile(field);
            }
        }
        return notExists();
    }
    if (action_ == CD) {
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
        return;
    }
    if (action_ == LS) {
        fmt::print(os_.stdout, ".key\n");
        for (auto const& field : sle) {
            if (field.isDefault() && field.getText() == "") {
                continue;
            }
            fmt::print(os_.stdout, "{}\n", field.getFName().getName());
        }
        return;
    }
    if (action_ == CAT) {
        return notFile();
    }
}

void PathCommand::sfieldFile(ripple::STBase const& sfield) {
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
        fmt::print(os_.stdout, "{}\n", sfield.getText());
        return;
    }
}

void PathCommand::innerDirectory(NodePtr const& object) {
    skipEmpty();
    auto const& slice = ripple::makeSlice(object->getData());
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
        int i;
        if (c >= '0' && c <= '9') {
            i = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            i = 10 + c - 'A';
        } else {
            return notExists();
        }
        sit.skip(NBYTES_DIGEST * i);
        auto childDigest = sit.get256();
        return nodeBranch(childDigest);
    }
    if (action_ == CD) {
        os_.chdir(path_.generic_string());
        os_.setenv("PWD", path_.generic_string());
        return;
    }
    if (action_ == LS) {
        for (auto i = 0; i < BRANCH_FACTOR; ++i) {
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

void PathCommand::leafDirectory(NodePtr const& object) {
    return sleDirectory(make_sle(object));
}

void PathCommand::sndDirectory(NodePtr const& object) {
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

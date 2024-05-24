#include <xrplorer/filesystem.hpp>
#include <xrplorer/tlpush.hpp>

#include <fmt/core.h>
#include <spdlog/spdlog.h>
#include <xrpl/basics/Slice.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/basics/safe_cast.h>
#include <xrpl/nodestore/NodeObject.h>
#include <xrpl/protocol/HashPrefix.h>
#include <xrpl/protocol/LedgerFormats.h> // LedgerEntryType
#include <xrpl/protocol/LedgerHeader.h>
#include <xrpl/protocol/Serializer.h>
#include <xrpl/protocol/STLedgerEntry.h>
#include <xrpl/protocol/STTx.h>
#include <xrpl/protocol/TxMeta.h>

#include <cstdint>
#include <memory>
#include <type_traits>

// TODO: Move these shims to their own TU.
namespace ripple {
using NodePtr = std::shared_ptr<NodeObject>;
using SLE = STLedgerEntry;
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
HashPrefix deserializePrefix(NodePtr const& object) {
    // TODO: Shouldn't Slice have an implicit constructor from vector of bytes?
    auto const& slice = makeSlice(object->getData());
    SerialIter sit{slice.data(), slice.size()};
    auto prefix = safe_cast<HashPrefix>(sit.get32());
    return prefix;
}
LedgerHeader deserializePrefixedHeader(NodePtr const& object) {
    auto const& slice = makeSlice(object->getData());
    return deserializePrefixedHeader(slice);
}
// TODO: Is this constant exported by libxrpl?
// SHAMapInnerNode::branchFactor is not exported.
struct SHAMapInnerNode {
    static constexpr unsigned int branchFactor = 16;
};
unsigned int selectBranch(uint256 const& key, unsigned int depth) {
    auto branch = static_cast<unsigned int>(*(key.begin() + (depth / 2)));
    if (depth & 1)
        branch &= 0xf;
    else
        branch >>= 4;
    assert(branch < SHAMapInnerNode::branchFactor);
    return branch;
}
SLE make_sle(NodePtr const& object) {
    auto slice = makeSlice(object->getData());
    slice.remove_prefix(4);
    Serializer serializer{slice.data(), slice.size()};
    uint256 key;
    bool success = serializer.getBitString(key, serializer.size() - key.bytes);
    assert(success);
    serializer.chop(key.bytes);
    auto slice2 = serializer.slice();
    SerialIter sit{slice2.data(), slice2.size()};
    return SLE{sit, key};
}
SLE make_sle(Keylet const& keylet, NodePtr const& object) {
    // A `Slice` is passed to `SHAMapTreeNode::makeFromPrefix`
    // which removes the 4 byte prefix.
    auto slice = makeSlice(object->getData());
    slice.remove_prefix(4);
    // It passes the remaining slice to `SHAMapTreeNode::makeAccountState`
    // which constructs a `Serializer`
    // (which calls itself the replacement for now-deprecated `SerialIter`)
    // and removes the 32 byte suffix.
    // (That suffix should match the keylet key, by the way).
    Serializer serializer{slice.data(), slice.size()};
    serializer.chop(32);
    // Then it constructs a _new_ `Slice` from the `Serializer`
    // and passes it to `make_shamapitem`,
    // which `memcpy`s the bytes into a `SHAMapItem`.
    // I don't know where an `STObject` is ever constructed by `SHAMap`,
    // but that final slice is the one it is expecting.
    auto slice2 = serializer.slice();
    // An `STLedgerEntry` cannot be constructed
    // with a `Slice` or a `Serializer`, though. Only a `SerialIter`.
    SerialIter sit{slice2.data(), slice2.size()};
    return SLE{sit, keylet.key};
}
STObject make_txm(NodePtr const& object) {
    auto slice = makeSlice(object->getData());
    slice.remove_prefix(4);
    Serializer serializer{slice.data(), slice.size()};
    uint256 key;
    bool success = serializer.getBitString(key, serializer.size() - key.bytes);
    assert(success);
    serializer.chop(key.bytes);
    auto slice2 = serializer.slice();
    SerialIter sit{slice2.data(), slice2.size()};
    return STObject(std::move(sit), sfMetadata);
}
}

namespace xrplorer {

using SLE = ripple::SLE;

struct FakeNamespace {

struct NodesDirectory : public Directory<NodesDirectory> {
    static std::vector<std::string> list(Context& ctx) {
        return {"<node ID>"};
    }
    static void open(Context& ctx, fs::path const& name) {
        ripple::uint256 digest;
        if (!digest.parseHex(name)) {
            throw ctx.throw_(NOT_A_DIGEST, "not a digest");
        }
        return nodeBranch(ctx, digest);
    }
};

static void nodeBranch(Context& ctx, ripple::uint256 const& digest) {
    auto object = ctx.os.db()->fetchNodeObject(digest);
    if (!object) {
        throw ctx.throw_(NODE_MISSING, "node missing");
    }
    auto prefix = ripple::deserializePrefix(object);
    switch (prefix) {
        case ripple::HashPrefix::ledgerMaster: return HeaderDirectory::call(ctx, object);
        case ripple::HashPrefix::txNode: return TxmDirectory::call(ctx, object);
        case ripple::HashPrefix::innerNode: return InnerDirectory::call(ctx, object);
        case ripple::HashPrefix::leafNode: return sleDirectory(ctx, object);
    }
    spdlog::error("type unknown: {}", prefix);
    throw ctx.throw_(TYPE_UNKNOWN, "type unknown");
}

struct HeaderDirectory : public SpecialDirectory<HeaderDirectory, const NodePtr> {
    static std::vector<std::string> list(Context& ctx, value_type const& object) {
        auto header{ripple::deserializePrefixedHeader(object)};
        return {
            "sequence",
            fmt::format("parent -> /nodes/{}", header.parentHash),
            fmt::format("txns -> /nodes/{}", header.txHash),
            fmt::format("state -> /nodes/{}", header.accountHash),
        };
    }
    static void open(Context& ctx, value_type const& object, fs::path const& name) {
        auto header{ripple::deserializePrefixedHeader(object)};
        if (name == "sequence") {
            return valueFile(ctx, header.seq);
        }
        if (name == "parent") {
            return nodeBranch(ctx, header.parentHash);
        }
        if (name == "txns") {
            return nodeBranch(ctx, header.txHash);
        }
        if (name == "state") {
            return StateDirectory::call(ctx, header.accountHash);
        }
        throw ctx.notExists();
    }
};

struct StateDirectory : public SpecialDirectory<StateDirectory, const ripple::uint256> {
    static std::vector<std::string> list(Context& ctx, value_type const& digest) {
        return {"accounts", fmt::format("root -> /nodes/{}", digest)};
    }
    static void open(Context& ctx, value_type const& digest, fs::path const& name) {
        if (name == "root") {
            return nodeBranch(ctx, digest);
        }
        if (name == "accounts") {
            auto root = ctx.os.db()->fetchNodeObject(digest);
            if (!root) {
                throw ctx.notExists();
            }
            tlpush _root{ctx.root, std::move(root)};
            return AccountsDirectory::call(ctx);
        }
        throw ctx.notImplemented();
    }
};

struct InnerDirectory : public SpecialDirectory<InnerDirectory, const NodePtr> {
    // TODO: Factor out common preamble to calling function.
    static std::vector<std::string> list(Context& ctx, value_type const& object) {
        // libxrpl does not have a deserialized representation of inner nodes.
        // An inner node is a directory with a subdirectory for each non-null child.
        auto const& slice = ripple::makeSlice(object->getData());
        ripple::SerialIter sit(slice.data(), slice.size());
        // Consume the prefix.
        sit.get32();
        std::vector<std::string> names;
        for (auto i = 0; i < ripple::SHAMapInnerNode::branchFactor; ++i) {
            auto childDigest = sit.get256();
            if (childDigest == beast::zero)
            {
                continue;
            }
            names.push_back(fmt::format("{:X}", i));
        }
        return names;
    }
    static void open(Context& ctx, value_type const& object, fs::path const& path) {
        auto const& slice = ripple::makeSlice(object->getData());
        ripple::SerialIter sit(slice.data(), slice.size());
        // Consume the prefix.
        sit.get32();
        auto name = path.generic_string();
        // `name` must be one hexadecimal character from 0 to F.
        if (name.length() != 1) {
            throw ctx.notExists();
        }
        char const c = name[0];
        int i;
        if (c >= '0' && c <= '9') {
            i = c - '0';
        } else if (c >= 'A' && c <= 'F') {
            i = 10 + c - 'A';
        } else {
            throw ctx.notExists();
        }
        sit.skip(NBYTES_DIGEST * i);
        auto childDigest = sit.get256();
        return nodeBranch(ctx, childDigest);
    }
};

struct AccountsDirectory : public Directory<AccountsDirectory> {
    static std::vector<std::string> list(Context& ctx) {
        return {"<base58 address, e.g. rHb9CJAWyB4rj91VRWn96DkukG4bwdtyTh>"};
    }
    static void open(Context& ctx, fs::path const& b58AccountId) {
        auto optAccountId = ripple::parseBase58<ripple::AccountID>(b58AccountId);
        if (!optAccountId) {
            throw ctx.notExists();
        }
        auto keylet = ripple::keylet::account(*optAccountId);
        auto object = load(ctx, keylet);
        if (!object) {
            throw ctx.notExists();
        }
        auto account = ripple::make_sle(keylet, object);
        return SleDirectory::call(ctx, account);
    }
};

static constexpr unsigned int NBYTES_DIGEST = 256 / 8;

static NodePtr load(Context& ctx, ripple::Keylet const& keylet) {
    assert(ctx.root);
    NodePtr object{ctx.root};
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
        auto childIndex = ripple::selectBranch(keylet.key, depth);
        sit.skip(NBYTES_DIGEST * childIndex);
        auto childDigest = sit.get256();
        if (childDigest == beast::zero) {
            return {};
        }
        object = ctx.os.db()->fetchNodeObject(childDigest);
        if (!object) {
            return {};
        }
    }
    return object;
}

static void sleDirectory(Context& ctx, NodePtr const& object) {
    return SleDirectory::call(ctx, make_sle(object));
}

// TODO: Factor Sle and Txm directories to Sto directory.
struct SleDirectory : public SpecialDirectory<SleDirectory, const SLE> {
    static std::vector<std::string> list(Context& ctx, value_type const& sle) {
        std::vector<std::string> names{".key"};
        for (auto const& field : sle) {
            if (field.isDefault() && field.getText() == "") {
                continue;
            }
            names.push_back(field.getFName().getName());
        }
        return names;
    }
    static void open(Context& ctx, value_type const& sle, fs::path const& name) {
        if (name == ".key") {
            return valueFile(ctx, sle.key());
        }
        for (auto const& field : sle) {
            if (field.getFName().getName() == name) {
                return SfieldFile::call(ctx, field);
            }
        }
        throw ctx.notExists();
    }
};

struct TxmDirectory : public SpecialDirectory<TxmDirectory, const NodePtr> {
    static std::vector<std::string> list(Context& ctx, value_type const& object) {
        auto txm = make_txm(object);
        std::vector<std::string> names;
        for (auto const& field : txm) {
            if (field.isDefault() && field.getText() == "") {
                continue;
            }
            names.push_back(field.getFName().getName());
        }
        return names;
    }
    static void open(Context& ctx, value_type const& object, fs::path const& name) {
        auto txm = make_txm(object);
        for (auto const& field : txm) {
            if (field.getFName().getName() == name) {
                return SfieldFile::call(ctx, field);
            }
        }
        throw ctx.notExists();
    }
};

struct SfieldFile : public SpecialFile<SfieldFile, const ripple::STBase> {
    static std::string stream(Context& ctx, value_type const& sfield) {
        return sfield.getText();
    }
};

template <typename T>
static void valueFile(Context& ctx, T const& value) {
    return ValueFile<T>::call(ctx, value);
}

template <typename T>
struct ValueFile : public SpecialFile<ValueFile<T>, const T> {
    static std::string stream(Context& ctx, T const& value) {
        return fmt::format("{}", value);
    }
};

};

void RootDirectory::open(Context& ctx, fs::path const& name) {
    if (name == "nodes") {
        return FakeNamespace::NodesDirectory::call(ctx);
    }
}

}

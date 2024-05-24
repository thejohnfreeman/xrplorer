#ifndef XRPLORER_PATH_COMMAND_HPP
#define XRPLORER_PATH_COMMAND_HPP

#include <xrplorer/operating-system.hpp>
#include <xrplorer/context.hpp>

#include <fmt/std.h>
#include <spdlog/spdlog.h>
#include <xrpl/basics/base_uint.h>
#include <xrpl/nodestore/NodeObject.h>
#include <xrpl/protocol/Keylet.h>
#include <xrpl/protocol/STBase.h>
#include <xrpl/protocol/STLedgerEntry.h>

#include <cassert>
#include <filesystem>
#include <iterator>
#include <memory>
#include <string>
#include <string_view>

namespace xrplorer {

namespace fs = std::filesystem;

using NodePtr = std::shared_ptr<ripple::NodeObject>;
using SLE = ripple::STLedgerEntry;

class PathCommand {
public:
    OperatingSystem& os_;
    fs::path path_;
    fs::path::iterator it_;
    Action action_;
    // The nearest SHAMap root, if any.
    NodePtr root_;

public:
    PathCommand(OperatingSystem& os, fs::path path, Action action)
        : os_(os), path_(path.lexically_normal()), it_(path_.begin()), action_(action)
    {
        // TODO: Control log level.
        // spdlog::info("path: {}", path_);
        // for (auto it = path_.begin(); it != path_.end(); ++it) {
        //     spdlog::info("step: {}", *it);
        // }
        assert(*it_ == "/");
        ++it_;
        rootDirectory();
    }

private:
    void throw_(ErrorCode code, std::string_view message);
    void notFile();
    void notDirectory();
    void notExists();
    void notImplemented();
    void skipEmpty();
    NodePtr load(ripple::Keylet const& keylet);
    void rootDirectory();
    void nodesDirectory();
    void nodeBranch(ripple::uint256 const& digest);
    void headerDirectory(NodePtr const& object);
    void stateDirectory(ripple::uint256 const& digest);
    void accountsDirectory();
    void sleDirectory(SLE const& sle);
    void sfieldFile(ripple::STBase const& sfield);
    void innerDirectory(NodePtr const& object);
    void leafDirectory(NodePtr const& object);
    // A transaction with metadata.
    void sndDirectory(NodePtr const& object);
    template <typename T>
    void valueFile(T const& value);
};

}

#endif

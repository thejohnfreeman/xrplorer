#ifndef XRPLORER_CONTEXT_HPP
#define XRPLORER_CONTEXT_HPP

#include <xrplorer/export.hpp>
#include <xrplorer/operating-system.hpp>

#include <xrpl/nodestore/NodeObject.h>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace xrplorer {

namespace fs = std::filesystem;

using NodePtr = std::shared_ptr<ripple::NodeObject>;

enum XRPLORER_EXPORT Action {
    CD,
    LS,
    CAT,
};

// TODO: Pair these with their message strings.
enum XRPLORER_EXPORT ErrorCode {
    NOT_IMPLEMENTED,
    // Path is not an entry in its parent directory.
    DOES_NOT_EXIST,
    NOT_A_FILE,
    NOT_A_DIRECTORY,
    NOT_A_DIGEST,
    // Path is an entry in its parent directory, but contents are missing.
    NODE_MISSING,
    TYPE_UNKNOWN,
};

struct XRPLORER_EXPORT Exception {
    ErrorCode code;
    fs::path path;
    std::string message;
};

struct XRPLORER_EXPORT Context {
    OperatingSystem& os;
    // The path argument as written.
    std::string_view argument;
    // The path argument as interpreted.
    fs::path& path;
    fs::path::iterator it;
    Action action;
    // The prefix for tab-completion, if any.
    std::string_view prefix;
    // The nearest SHAMap root, if any.
    NodePtr root;

    Exception throw_(ErrorCode code, std::string_view message);
    Exception notFile();
    Exception notDirectory();
    Exception notExists();
    Exception notImplemented();
    void skipEmpty();
    void list(std::vector<std::string> const& names);
    void echo(std::string_view text);
};

}

#endif

#include <xrplorer/root-directory.hpp>

#include <xrplorer/nodes-directory.hpp>

namespace xrplorer {

RootDirectory::RootDirectory(std::filesystem::path const& path) : db_(path) {
}

std::unique_ptr<Directory> RootDirectory::openDir(std::string_view name) const {
    if (name != "nodes") {
        return {};
    }
    return std::make_unique<NodesDirectory>(*this);
}

}

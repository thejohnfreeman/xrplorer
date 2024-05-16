#include <xrplorer/nodes-directory.hpp>

#include <xrplorer/object-directory.hpp>

#include <xrpl/basics/base_uint.h>

#include <utility>

namespace xrplorer {

std::unique_ptr<Directory> NodesDirectory::openDir(std::string_view name) const {
    ripple::uint256 digest;
    if (!digest.parseHex(name)) {
        return {};
    }
    auto object = root_.db_->fetchNodeObject(digest);
    if (!object) {
        return {};
    }
    return std::make_unique<ObjectDirectory>(std::move(object));
}

}

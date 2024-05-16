#ifndef XRPLORER_NODES_DIRECTORY_HPP
#define XRPLORER_NODES_DIRECTORY_HPP

#include <xrplorer/directory.hpp>
#include <xrplorer/export.hpp>
#include <xrplorer/root-directory.hpp>

namespace xrplorer {

class XRPLORER_EXPORT NodesDirectory : public Directory {
private:
    RootDirectory const& root_;

public:
    NodesDirectory(RootDirectory const& root) : root_(root) {}

    std::unique_ptr<Directory> openDir(std::string_view name) const override;
};

}

#endif

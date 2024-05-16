#ifndef XRPLORER_ROOT_DIRECTORY_HPP
#define XRPLORER_ROOT_DIRECTORY_HPP

#include <xrplorer/export.hpp>
#include <xrplorer/database.hpp>
#include <xrplorer/directory.hpp>

#include <filesystem>
#include <memory>

namespace xrplorer {

class XRPLORER_EXPORT RootDirectory : public Directory {
private:
    mutable Database db_;

    friend class NodesDirectory;

public:
    RootDirectory(std::filesystem::path const& path);

    std::unique_ptr<Directory> openDir(std::string_view name) const override;
};

}

#endif

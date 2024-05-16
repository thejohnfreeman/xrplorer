#ifndef XRPLORER_DIRECTORY_HPP
#define XRPLORER_DIRECTORY_HPP

#include <xrplorer/export.hpp>

#include <memory>
#include <string_view>

namespace xrplorer {

class XRPLORER_EXPORT Directory {
public:
    virtual std::unique_ptr<Directory> openDir(std::string_view name) const = 0;
    virtual ~Directory() {}
};

}

#endif

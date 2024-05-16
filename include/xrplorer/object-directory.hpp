#ifndef XRPLORER_OBJECT_DIRECTORY_HPP
#define XRPLORER_OBJECT_DIRECTORY_HPP

#include <xrplorer/directory.hpp>
#include <xrplorer/export.hpp>

#include <xrpl/nodestore/NodeObject.h>

#include <memory>
#include <utility>

namespace xrplorer {

class XRPLORER_EXPORT ObjectDirectory : public Directory {
private:
    std::shared_ptr<ripple::NodeObject> object_;

public:
    ObjectDirectory(std::shared_ptr<ripple::NodeObject>&& object) : object_(std::move(object)) {}

    std::unique_ptr<Directory> openDir(std::string_view name) const override;
};


}

#endif

#ifndef XRPLORER_FILESYSTEM_HPP
#define XRPLORER_FILESYSTEM_HPP

#include <xrplorer/bases.hpp>
#include <xrplorer/export.hpp>

#include <xrpl/basics/base_uint.h>

namespace xrplorer {

struct RootDirectory : public Directory<RootDirectory> {
    static std::vector<std::string> list(Context& ctx) {
        return {"nodes"};
    }
    static void open(Context& ctx, fs::path const& name);
};

void nodeBranch(Context& ctx, ripple::uint256 const& digest);

}

#endif

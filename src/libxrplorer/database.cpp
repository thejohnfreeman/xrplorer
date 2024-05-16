#include <xrplorer/database.hpp>

#include <xrpl/basics/ByteUtilities.h> // megabytes()
#include <xrpl/nodestore/Manager.h>
#include <xrpl/nodestore/backend/NuDBFactory.h>

#include <cstdint>

namespace xrplorer {

static ripple::NodeStore::NuDBFactory theNudbFactory;

Database::Database(std::filesystem::path path) {
    int readThreads{4};
    std::size_t burstSize{ripple::megabytes(32)};
    ripple::Section section;
    section.set("type", "NuDB");
    section.set("path", path);
    db_ = ripple::NodeStore::Manager::instance().make_Database(
            burstSize,
            scheduler_,
            readThreads,
            section,
            journal_);
}

}

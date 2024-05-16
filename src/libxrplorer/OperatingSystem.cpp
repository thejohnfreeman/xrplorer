#include <xrplorer/OperatingSystem.hpp>

#include <xrpl/basics/ByteUtilities.h> // megabytes()
#include <xrpl/basics/Log.h>  // Logs
#include <xrpl/beast/insight/NullCollector.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/jobqueue/JobQueue.h>
#include <xrpl/jobqueue/NullPerfLog.h>
#include <xrpl/nodestore/Database.h>
#include <xrpl/nodestore/Manager.h>
#include <xrpl/nodestore/NodeStoreScheduler.h>
#include <xrpl/nodestore/backend/NuDBFactory.h>

#include <memory>

namespace xrplorer {

struct Database {

    std::shared_ptr<beast::insight::Collector> collector_{
        beast::insight::NullCollector::New()};
    beast::Journal journal_{};
    ripple::Logs logs_{beast::severities::kFatal};
    ripple::perf::NullPerfLog perflog_{};
    ripple::JobQueue jobQueue_{
        /*threadCount=*/4, collector_, journal_, logs_, perflog_};
    ripple::NodeStoreScheduler scheduler_{jobQueue_};
    std::unique_ptr<ripple::NodeStore::Database> db_;

    Database(std::filesystem::path path);

    operator bool () const {
        return !!db_;
    }

    ripple::NodeStore::Database& operator* () {
        return *db_;
    }

    ripple::NodeStore::Database const& operator* () const {
        return *db_;
    }

    ripple::NodeStore::Database* operator-> () {
        return db_.get();
    }

    ripple::NodeStore::Database const* operator-> () const {
        return db_.get();
    }

};

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

std::filesystem::path const& OperatingSystem::getcwd() const {
    return cwd_;
}

void OperatingSystem::chdir(std::string_view path) {
    cwd_ /= path;
    cwd_ = cwd_.lexically_normal();
}

std::string_view OperatingSystem::getenv(std::string_view name) const {
    // C++26: can pass std::string_view.
    auto it = env_.find(std::string{name});
    return (it == env_.end()) ? "" : it->second;
}

void OperatingSystem::setenv(std::string_view name, std::string_view value) {
    // C++26: can pass std::string_view.
    env_[std::string{name}] = value;
}

void OperatingSystem::unsetenv(std::string_view name) {
    // C++26: can pass std::string_view.
    env_.erase(std::string{name});
}

std::string_view OperatingSystem::gethostname() const {
    return hostname_;
}

void OperatingSystem::sethostname(std::string_view hostname) {
    Database db{hostname};
    db->fetchNodeObject({});
    hostname_ = hostname;
}

}

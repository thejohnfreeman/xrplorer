#ifndef XRPLORER_DATABASE_HPP
#define XRPLORER_DATABASE_HPP

#include <xrplorer/export.hpp>

#include <xrpl/basics/Log.h>  // Logs
#include <xrpl/beast/insight/NullCollector.h>
#include <xrpl/beast/utility/Journal.h>
#include <xrpl/jobqueue/JobQueue.h>
#include <xrpl/jobqueue/NullPerfLog.h>
#include <xrpl/nodestore/Database.h>
#include <xrpl/nodestore/NodeStoreScheduler.h>

#include <filesystem>
#include <memory>

namespace xrplorer {

struct XRPLORER_EXPORT Database {

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

}

#endif

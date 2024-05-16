#ifndef XRPLORER_OPERATING_SYSTEM_HPP
#define XRPLORER_OPERATING_SYSTEM_HPP

#include <xrplorer/export.hpp>
#include <xrplorer/directory.hpp>
#include <xrplorer/root-directory.hpp>

#include <filesystem>
#include <unordered_map>
#include <string>
#include <string_view>

namespace xrplorer {

class XRPLORER_EXPORT OperatingSystem {
private:
    std::filesystem::path cwd_{"/", std::filesystem::path::generic_format};
    std::unordered_map<std::string, std::string> env_;
    std::string hostname_;
    std::unique_ptr<RootDirectory> root_;

public:
    std::filesystem::path const& getcwd() const;
    void chdir(std::string_view path);

    std::string_view getenv(std::string_view name) const;
    void setenv(std::string_view name, std::string_view value);
    void unsetenv(std::string_view name);

    std::string_view gethostname() const;
    void sethostname(std::string_view hostname);

    std::unique_ptr<Directory> opendir(std::filesystem::path const& path);
};

}

#endif

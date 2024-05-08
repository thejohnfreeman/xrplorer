#ifndef XRPLORER_XRPLORER_HPP
#define XRPLORER_XRPLORER_HPP

#include <xrplorer/export.hpp>

#include <filesystem>
#include <unordered_map>
#include <string>
#include <string_view>

namespace xrplorer {

class XRPLORER_EXPORT OperatingSystem {
private:
    std::filesystem::path cwd_{"/", std::filesystem::path::generic_format};
    std::unordered_map<std::string, std::string> env_;

public:
    std::filesystem::path getcwd() const;
    void chdir(std::string_view path);

    std::string getenv(std::string_view name) const;
    void setenv(std::string_view name, std::string_view value);
    void unsetenv(std::string_view name);

};

}

#endif

#include <xrplorer/OperatingSystem.hpp>

namespace xrplorer {

std::filesystem::path OperatingSystem::getcwd() const {
    return cwd_;
}

void OperatingSystem::chdir(std::string_view path) {
    cwd_ = path;
}

std::string OperatingSystem::getenv(std::string_view name) const {
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

}

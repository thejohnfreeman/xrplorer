#include <xrplorer/operating-system.hpp>

namespace xrplorer {

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
    hostname_ = hostname;
}

}

#include <xrplorer/context.hpp>

#include <fmt/core.h>

#include <functional> // divides
#include <numeric> // accumulate
#include <string>
#include <string_view>
#include <vector>

namespace xrplorer {

fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
    return std::accumulate(begin, end, fs::path{}, std::divides{});
}

Exception Context::throw_(ErrorCode code, std::string_view message) {
    auto const& path_ = make_path(path.begin(), it);
    throw Exception{code, path_, std::string{message}};
}

Exception Context::notFile() {
    throw throw_(NOT_A_FILE, "not a file");
}

Exception Context::notDirectory() {
    throw throw_(NOT_A_DIRECTORY, "not a directory");
}

Exception Context::notExists() {
    throw throw_(DOES_NOT_EXIST, "no such file or directory");
}

Exception Context::notImplemented() {
    throw throw_(NOT_IMPLEMENTED, "not implemented");
}

void Context::skipEmpty() {
    for (; it != path.end() && (*it == "" || *it == "."); ++it);
}

void Context::list(std::vector<std::string> const& names) {
    // TODO: Conditional long listing.
    // TODO: Sort alphabetically?
    for (auto const& name : names) {
        // TODO: Format into columns.
        echo(name);
    }
}

void Context::echo(std::string_view text) {
    fmt::print(os.stdout, "{}\n", text);
}

}

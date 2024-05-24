#include <xrplorer/context.hpp>

#include <numeric>

namespace xrplorer {

fs::path make_path(fs::path::iterator begin, fs::path::iterator end);
// fs::path make_path(fs::path::iterator begin, fs::path::iterator end) {
//     return std::accumulate(begin, end, fs::path{}, std::divides{});
// }

void Context::throw_(ErrorCode code, std::string_view message) {
    auto const& path_ = make_path(path.begin(), it);
    throw Exception{code, path_, std::string{message}};
}

void Context::notFile() {
    return throw_(NOT_A_FILE, "not a file");
}

void Context::notDirectory() {
    return throw_(NOT_A_DIRECTORY, "not a directory");
}

void Context::notExists() {
    return throw_(DOES_NOT_EXIST, "no such file or directory");
}

void Context::notImplemented() {
    return throw_(NOT_IMPLEMENTED, "not implemented");
}

void Context::skipEmpty() {
    for (; it != path.end() && (*it == "" || *it == "."); ++it);
}

}

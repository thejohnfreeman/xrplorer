#include <xrplorer/shell.hpp>
#include <xrplorer/context.hpp>
#include <xrplorer/filesystem.hpp>

#include <boost/program_options/parsers.hpp>
#include <fmt/core.h>
#include <fmt/std.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <cassert>
#include <cstdlib>
#include <string_view>

using namespace std::literals;

namespace xrplorer {

class LineReader {
private:
    std::unique_ptr<char> line_{nullptr};

public:
    LineReader() {}
    LineReader(LineReader const&) = delete;
    LineReader(LineReader&& rhs) : line_(std::move(rhs.line_)) {}
    LineReader& operator= (LineReader const&) = delete;
    LineReader& operator= (LineReader&& rhs) {
        line_ = std::move(rhs.line_);
        return *this;
    }

    char const* readline(char const* prompt) {
        while (true) {
            line_.reset(::readline(prompt));
            if (!line_) break;
            if (*line_ == 0 || *line_ == '#') continue;
            if (*line_ != ' ') {
                ::add_history(line_.get());
            }
            break;
        }
        return line_.get();
    }
};

int Shell::main(int argc, char** argv) {
    // Copy the default nodestore path from the example rippled.cfg.
    char const* hostname = (argc > 1) ? argv[1] : "/var/lib/rippled/db/nudb";
    os_.sethostname(hostname);

    namespace po = boost::program_options;
    LineReader lineReader;
    while (auto line = lineReader.readline("> ")) {
        std::string cmdline{line};

        auto strings = po::split_unix(cmdline);
        int argc = strings.size();
        if (argc < 1) {
            continue;
        }
        std::vector<char*> pointers;
        pointers.reserve(argc);
        for (auto& string : strings) {
            pointers.push_back(string.data());
        }
        char** argv = pointers.data();

        std::string command = argv[0];
        if (command == "exit") {
            return this->exit(argc, argv);
        }
        if (command == "cd") {
            this->cd(argc, argv);
            continue;
        }
        if (command == "echo") {
            this->echo(argc, argv);
            continue;
        }
        if (command == "pwd") {
            this->pwd(argc, argv);
            continue;
        }

        if (command == "cat") {
            this->cat(argc, argv);
            continue;
        }
        if (command == "help") {
            this->help(argc, argv);
            continue;
        }
        if (command == "hostname") {
            this->hostname(argc, argv);
            continue;
        }
        if (command == "ls") {
            this->ls(argc, argv);
            continue;
        }
        fmt::print(os_.stdout, "{}: command not found\n", argv[0]);
    }
    return 0;
}

void command(OperatingSystem& os, std::string_view argument, Action action) {
    auto path = (os.getcwd() / argument).lexically_normal();
    auto it = path.begin();
    assert(*it == "/");
    ++it;
    Context ctx {
        .os = os,
        .argument = argument,
        .path = path,
        .it = std::move(it),
        .action = action,
    };
    RootDirectory::call(ctx);
}

int Shell::cat(int argc, char** argv) {
    assert(argv[0] == "cat"sv);
    for (int i = 1; i < argc; ++i) {
        try {
            command(os_, argv[i], Action::CAT);
        } catch (Exception const& ex) {
            fmt::print(os_.stdout, "{}: {}: {}\n", argv[0], ex.path, ex.message);
            return ex.code;
        }
    }
    return 0;
}

int Shell::cd(int argc, char** argv) {
    assert(argv[0] == "cd"sv);
    char const* suffix = (argc > 1) ? argv[1] : "/";
    try {
        command(os_, suffix, Action::CD);
    } catch (Exception const& ex) {
        fmt::print(os_.stdout, "{}: {}: {}\n", argv[0], ex.path, ex.message);
        return ex.code;
    }
    return 0;
}

int Shell::echo(int argc, char** argv) {
    assert(argv[0] == "echo"sv);
    for (auto i = 1; i < argc; ++i) {
        if (i > 1) {
            std::fputc(' ', os_.stdout);
        }
        std::fputs(argv[i], os_.stdout);
    }
    std::fputs("\n", os_.stdout);
    return 0;
}

int Shell::exit(int argc, char** argv) {
    assert(argv[0] == "exit"sv);
    if (argc == 1) {
        return 0;
    }
    if (argc == 2) {
        try {
            return std::stoi(argv[1]);
        } catch (...) {
            fmt::print(os_.stdout, "{}: {}: numeric argument required", argv[0], argv[1]);
            return 2;
        }
    }
    fmt::print(os_.stdout, "{}: too many arguments", argv[0]);
    return 1;
}

int Shell::help(int argc, char** argv) {
    fmt::print(os_.stdout, "cat [file]\n");
    fmt::print(os_.stdout, "cd [dir]\n");
    fmt::print(os_.stdout, "echo [arg ...]\n");
    fmt::print(os_.stdout, "exit [n]\n");
    fmt::print(os_.stdout, "help\n");
    fmt::print(os_.stdout, "hostname [name]\n");
    fmt::print(os_.stdout, "ls [dir]\n");
    fmt::print(os_.stdout, "pwd\n");
    return 0;
}

int Shell::hostname(int argc, char** argv) {
    assert(argv[0] == "hostname"sv);
    if (argc > 1) {
        fmt::print(os_.stdout, "{}: changing hostname not yet implemented\n", argv[0]);
    }
    auto sv = os_.gethostname();
    fmt::print(os_.stdout, "{}\n", sv);
    return 0;
}

char const* DEFAULT_ARGV_LS[] = {"ls", "."};

int Shell::ls(int argc, char** argv_) {
    char const* const* argv = argv_;
    if (argc == 1) {
        argc = 2;
        argv = DEFAULT_ARGV_LS;
    }
    assert(argv[0] == "ls"sv);
    for (int i = 1; i < argc; ++i) {
        try {
            command(os_, argv[i], Action::LS);
        } catch (Exception const& ex) {
            fmt::print(os_.stdout, "{}: {}: {}\n", argv[0], ex.path, ex.message);
            return ex.code;
        }
    }
    return 0;
}

int Shell::pwd(int argc, char** argv) {
    assert(argv[0] == "pwd"sv);
    auto const& path = os_.getcwd();
    fmt::print(os_.stdout, "{}\n", path);
    return 0;
}

}

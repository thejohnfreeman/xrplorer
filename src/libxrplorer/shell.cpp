#include <xrplorer/shell.hpp>

#include <src/libxrplorer/path-command.hpp>

#include <boost/program_options/parsers.hpp>
#include <readline/readline.h>
#include <readline/history.h>

#include <cassert>
#include <cstdlib>

namespace xrplorer {

// TODO: Forget about copy and move.
// TODO: Free line before each read.
class LineReader {
private:
    char* line_ = nullptr;

public:
    LineReader() {}
    LineReader(LineReader const&) = delete;
    LineReader(LineReader&& rhs) : line_(rhs.line_) { rhs.line_ = nullptr; }
    LineReader& operator= (LineReader const&) = delete;
    LineReader& operator= (LineReader&& rhs) {
        line_ = rhs.line_;
        rhs.line_ = nullptr;
        return *this;
    }
    ~LineReader() {
        std::free(line_);
    }

    char const* readline(char const* prompt) {
        line_ = ::readline(prompt);
        if (line_ && *line_ && *line_ != ' ') {
            ::add_history(line_);
        }
        return line_;
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
        std::fprintf(os_.stdout, "%s: command not found\n", argv[0]);
    }
    return 0;
}

int Shell::cat(int argc, char** argv) {
    assert(argv[0] == "cat"sv);
    for (int i = 1; i < argc; ++i) {
        auto path = os_.getcwd() / argv[i];
        PathCommand cmd(os_, path, PathCommand::Action::CAT);
        try {
            cmd.execute();
        } catch (PathCommand::Exception const& ex) {
            std::fprintf(os_.stdout, "%s: %s: %s\n", argv[0], ex.path.c_str(), ex.message.c_str());
            return ex.code;
        }
    }
    return 0;
}

int Shell::cd(int argc, char** argv) {
    assert(argv[0] == "cd"sv);
    char const* suffix = (argc > 1) ? argv[1] : "/";
    auto path = os_.getcwd() / suffix;
    PathCommand cmd(os_, path, PathCommand::Action::CD);
    try {
        cmd.execute();
    } catch (PathCommand::Exception const& ex) {
        std::fprintf(os_.stdout, "%s: %s: %s\n", argv[0], ex.path.c_str(), ex.message.c_str());
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
            std::fprintf(os_.stdout, "%s: %s: numeric argument required", argv[0], argv[1]);
            return 2;
        }
    }
    std::fprintf(os_.stdout, "%s: too many arguments", argv[0]);
    return 1;
}

int Shell::help(int argc, char** argv) {
    std::fprintf(os_.stdout, "cat [file]\n");
    std::fprintf(os_.stdout, "cd [dir]\n");
    std::fprintf(os_.stdout, "echo [arg ...]\n");
    std::fprintf(os_.stdout, "exit [n]\n");
    std::fprintf(os_.stdout, "help\n");
    std::fprintf(os_.stdout, "hostname [name]\n");
    std::fprintf(os_.stdout, "ls [dir]\n");
    std::fprintf(os_.stdout, "pwd\n");
    return 0;
}

int Shell::hostname(int argc, char** argv) {
    assert(argv[0] == "hostname"sv);
    if (argc > 1) {
        std::fprintf(os_.stdout, "%s: changing hostname not yet implemented\n", argv[0]);
    }
    auto sv = os_.gethostname();
    std::fprintf(os_.stdout, "%.*s\n", static_cast<int>(sv.length()), sv.data());
    return 0;
}

char const* DEFAULT_ARGV_LS[] = {"ls", "."};

int Shell::ls(int argc, char** argv_) {
    assert(argv[0] == "ls"sv);
    char const* const* argv = argv_;
    if (argc == 1) {
        argc = 2;
        argv = DEFAULT_ARGV_LS;
    }
    for (int i = 1; i < argc; ++i) {
        auto path = os_.getcwd() / argv[i];
        PathCommand cmd(os_, path, PathCommand::Action::LS);
        try {
            cmd.execute();
        } catch (PathCommand::Exception const& ex) {
            std::fprintf(os_.stdout, "%s: %s: %s\n", argv[0], ex.path.c_str(), ex.message.c_str());
            return ex.code;
        }
    }
    return 0;
}

int Shell::pwd(int argc, char** argv) {
    assert(argv[0] == "pwd"sv);
    auto const& path = os_.getcwd();
    std::fprintf(os_.stdout, "%s\n", path.c_str());
    return 0;
}

}

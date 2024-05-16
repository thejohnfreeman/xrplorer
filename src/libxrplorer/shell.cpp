#include <xrplorer/shell.hpp>

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
        if (command == "hostname") {
            this->hostname(argc, argv);
            continue;
        }
        if (command == "ls") {
            this->ls(argc, argv);
            continue;
        }
        std::fprintf(out_, "%s: command not found\n", argv[0]);
    }
    return 0;
}

int Shell::cat(int argc, char** argv) {
    assert(argv[0] == "cat"sv);
    std::fprintf(out_, "%s: uuhhh\n", argv[0]);
    return 0;
}

int Shell::cd(int argc, char** argv) {
    assert(argv[0] == "cd"sv);
    char const* path = (argc > 1) ? argv[1] : "/";
    // TODO: Check that path is legal.
    os_.chdir(path);
    os_.setenv("PWD", path);
    return 0;
}

int Shell::echo(int argc, char** argv) {
    assert(argv[0] == "echo"sv);
    for (auto i = 1; i < argc; ++i) {
        if (i > 1) {
            std::fputc(' ', out_);
        }
        std::fputs(argv[i], out_);
    }
    std::fputs("\n", out_);
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
            std::fprintf(out_, "%s: %s: numeric argument required", argv[0], argv[1]);
            return 2;
        }
    }
    std::fprintf(out_, "%s: too many arguments", argv[0]);
    return 1;
}

int Shell::hostname(int argc, char** argv) {
    assert(argv[0] == "hostname"sv);
    if (argc > 1) {
        std::fprintf(out_, "%s: changing hostname not yet implemented\n", argv[0]);
    }
    auto sv = os_.gethostname();
    std::fprintf(out_, "%.*s\n", static_cast<int>(sv.length()), sv.data());
    return 0;
}

int Shell::ls(int argc, char** argv) {
    assert(argv[0] == "ls"sv);
    std::fprintf(out_, "%s: uhhhh\n", argv[0]);
    return 0;
}

int Shell::pwd(int argc, char** argv) {
    assert(argv[0] == "pwd"sv);
    auto const& path = os_.getcwd();
    std::fprintf(out_, "%s\n", path.c_str());
    return 0;
}

}

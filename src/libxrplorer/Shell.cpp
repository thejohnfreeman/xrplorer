#include <xrplorer/Shell.hpp>

#include <boost/program_options/parsers.hpp>
#include <readline/readline.h>
#include <readline/history.h>

#include <cassert>
#include <cstdlib>

namespace xrplorer {

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
        if (command == "ls") {
            std::printf("list directory\n");
        } else {
            std::printf("%s: command not found\n", argv[0]);
        }
    }
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
            std::printf("exit: %s: numeric argument required", argv[1]);
            return 2;
        }
    }
    std::printf("exit: too many arguments");
    return 1;
}

}

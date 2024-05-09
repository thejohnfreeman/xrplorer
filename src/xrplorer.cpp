#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include <xrplorer/xrplorer.hpp>

#include <boost/program_options/parsers.hpp>
#include <readline/readline.h>
#include <readline/history.h>

class LineReader {
private:
    char* line_;

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
        if (*line_) {
            ::add_history(line_);
        }
        return line_;
    }
};

namespace po = boost::program_options;

int main(int argc, const char** argv) {
    LineReader lineReader;
    while (auto line = lineReader.readline("> ")) {
        std::string cmdline{line};

        auto strings = po::split_unix(cmdline);
        int argc = strings.size();
        if (argc < 1) {
            continue;
        }
        std::vector<char const*> pointers;
        pointers.reserve(argc);
        for (auto const& string : strings) {
            pointers.push_back(string.c_str());
        }
        char const** argv = pointers.data();

        std::string command = argv[0];
        if (command == "ls") {
            std::printf("list directory\n");
        } else {
            std::printf("%s: command not found\n", argv[0]);
        }
    }
    return 0;
}

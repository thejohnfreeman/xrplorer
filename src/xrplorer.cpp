#include <cstdio>
#include <cstdlib>

#include <xrplorer/xrplorer.hpp>

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
        return line_;
    }
};

int main(int argc, const char** argv) {
    LineReader lineReader;
    while (auto line = lineReader.readline("> ")) {
        if (!*line) {
            continue;
        }
        ::add_history(line);
        std::printf("echo: \"%s\"\n", line);
    }
    return 0;
}

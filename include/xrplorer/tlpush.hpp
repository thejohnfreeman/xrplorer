#ifndef XRPLORER_TLPUSH_HPP
#define XRPLORER_TLPUSH_HPP

#include <xrplorer/export.hpp>

#include <utility>

namespace xrplorer {

/**
 * "Push" a value at this point in the call stack.
 * "Pop" the value when exiting the current scope.
 * Typically used with thread-local globals.
 */
template <typename T>
struct XRPLORER_EXPORT tlpush
{
    T& variable;
    T previous;

    tlpush(T& variable, T&& next)
        : variable(variable), previous(std::move(variable))
    {
        variable = std::move(next);
    }

    ~tlpush()
    {
        variable = std::move(previous);
    }
};

}

#endif

#ifndef XRPLORER_BASES_HPP
#define XRPLORER_BASES_HPP

#include <xrplorer/context.hpp>
#include <xrplorer/export.hpp>

#include <cassert>
#include <string>
#include <vector>

namespace xrplorer {

constexpr bool UNREACHABLE = false;

template <typename Derived, typename T = void>
struct Entry {
    static void call(Context& ctx, T* spl = nullptr) {
        ctx.skipEmpty();
        if (ctx.it != ctx.path.end()) {
            auto const& name = *ctx.it++;
            return Derived::_open(ctx, spl, name);
        }
        if (ctx.action == CD) {
            return Derived::chdir(ctx);
        }
        if (ctx.action == LS) {
            return ctx.list(Derived::_list(ctx, spl));
        }
        if (ctx.action == CAT) {
            return ctx.echo(Derived::_stream(ctx, spl));
        }
        assert(UNREACHABLE);
    }
    static void _open(Context& ctx, T* spl, fs::path const& name) {
        return Derived::open(ctx, name);
    }
    static std::vector<std::string> _list(Context& ctx, T* spl) {
        return Derived::list(ctx);
    }
    static std::string _stream(Context& ctx, T* spl) {
        return Derived::stream(ctx);
    }
};

template <typename Derived, typename T = void>
struct Directory : public Entry<Derived, T> {
    static void open(Context& ctx, fs::path const& name) {
        throw ctx.notImplemented();
    }
    static void chdir(Context& ctx) {
        ctx.os.chdir(ctx.path.generic_string());
        ctx.os.setenv("PWD", ctx.path.generic_string());
    }
    static std::vector<std::string> list(Context& ctx) {
        throw ctx.notImplemented();
    }
    static std::string stream(Context& ctx) {
        throw ctx.notFile();
    }
};

template <typename Derived, typename T = void>
struct File : public Entry<Derived, T> {
    static void open(Context& ctx, fs::path const& name) {
        throw ctx.notDirectory();
    }
    static void chdir(Context& ctx) {
        throw ctx.notDirectory();
    }
    static std::vector<std::string> list(Context& ctx) {
        return {std::string{ctx.argument}};
    }
    static std::string stream(Context& ctx) {
        throw ctx.notImplemented();
    }
};

template <template <typename, typename> typename Base, typename Derived, typename T>
struct Special : public Base<Derived, T> {
    using Base<Derived, T>::call;
    using Base<Derived, T>::open;
    using Base<Derived, T>::list;
    using Base<Derived, T>::stream;
    using value_type = T;
    static void call(Context& ctx, T& spl) {
        return Base<Derived, T>::call(ctx, &spl);
    }
    static void _open(Context& ctx, T* spl, fs::path const& name) {
        return Derived::open(ctx, *static_cast<T*>(spl), name);
    }
    static void open(Context& ctx, T& spl, fs::path const& name) {
        return Derived::open(ctx, name);
    }
    static std::vector<std::string> _list(Context& ctx, T* spl) {
        return Derived::list(ctx, *static_cast<T*>(spl));
    }
    static std::vector<std::string> list(Context& ctx, T& spl) {
        return Derived::list(ctx);
    }
    static std::string _stream(Context& ctx, T* spl) {
        return Derived::stream(ctx, *static_cast<T*>(spl));
    }
    static std::string stream(Context& ctx, T& spl) {
        return Derived::stream(ctx);
    }
};

template <typename Derived, typename T>
using SpecialDirectory = Special<Directory, Derived, T>;

template <typename Derived, typename T>
using SpecialFile = Special<File, Derived, T>;

}

#endif

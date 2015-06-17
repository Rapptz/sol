// The MIT License (MIT)

// Copyright (c) 2013 Danny Y., Rapptz

// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef SOL_STACK_HPP
#define SOL_STACK_HPP

#include "error.hpp"
#include "reference.hpp"
#include "tuple.hpp"
#include "traits.hpp"
#include "usertype_traits.hpp"
#include <utility>
#include <array>
#include <cstring>
#include <functional>

namespace sol {
namespace detail {
template<typename T>
inline T* get_ptr(T& val) {
    return std::addressof(val);
}

template<typename T>
inline T* get_ptr(std::reference_wrapper<T> val) {
    return std::addressof(val.get());
}

template<typename T>
inline T* get_ptr(T* val) {
    return val;
}

template<typename Decorated>
struct return_forward {
    typedef Unqualified<Decorated> T;

    T& operator()(T& value) const {
        return value;
    }

    T&& operator()(T&& value) const {
        return std::move(value);
    }

    T operator()(const T& value) const {
        return value;
    }

    // handle retarded libraries
    T operator()(const T&& value) const {
        return value;
    }
};
} // detail

namespace stack {
namespace detail {
template<typename T, typename Key, typename... Args>
inline int push_userdata(lua_State* L, Key&& metatablekey, Args&&... args) {
    T* pdatum = static_cast<T*>(lua_newuserdata(L, sizeof(T)));
    std::allocator<T> alloc{};
    alloc.construct(pdatum, std::forward<Args>(args)...);
    luaL_getmetatable(L, std::addressof(metatablekey[0]));
    lua_setmetatable(L, -2);
    return 1;
}
} // detail

template<typename T, typename = void>
struct getter;
template<typename T, typename = void>
struct pusher;
template<typename T, type = lua_type_of<T>::value, typename = void>
struct checker;

template<typename T, typename... Args>
inline int push(lua_State* L, T&& t, Args&&... args) {
    return pusher<Unqualified<T>>{}.push(L, std::forward<T>(t), std::forward<Args>(args)...);
}

template<typename T, std::size_t... I>
inline int push_tuple(lua_State* L, indices<I...>, T&& tuplen) {
    using swallow = char[1 + sizeof...(I)];
    int pushcount = 0;
    swallow {'\0', (pushcount += sol::stack::push(L, std::get<I>(tuplen)), '\0')... };
    return pushcount;
}

// overload allows to use a pusher of a specific type, but pass in any kind of args
template<typename T, typename Arg, typename... Args>
inline int push(lua_State* L, Arg&& arg, Args&&... args) {
    return pusher<Unqualified<T>>{}.push(L, std::forward<Arg>(arg), std::forward<Args>(args)...);
}

inline int push_args(lua_State*) {
    // do nothing
    return 0;
}

template<typename T, typename... Args>
inline int push_args(lua_State* L, T&& t, Args&&... args) {
    int pushcount = push(L, std::forward<T>(t));
    using swallow = char[];
    void(swallow{'\0', (pushcount += push(L, std::forward<Args>(args)), '\0')... });
    return pushcount;
}

template<typename T, typename U = Unqualified<T>>
inline auto get(lua_State* L, int index = -1) -> decltype(getter<U>{}.get(L, index)) {
    return getter<U>{}.get(L, index);
}

template<typename T>
auto pop(lua_State* L) -> decltype(get<T>(L)) {
    typedef decltype(get<T>(L)) ret_t;
    ret_t r = get<T>(L);
    lua_pop(L, 1);
    return r;
}

template<typename T>
struct get_return {
    typedef decltype(get<T>(nullptr)) type;
};

template <typename T, type expected, typename>
struct checker {
    template <typename Handler>
    static bool check (lua_State* L, int index, const Handler& handler) {
        const type indextype = type_of(L, index);
        bool success = expected == indextype;
        if (!success) {
            // expected type, actual type
            handler(L, index, expected, indextype);
        }
        return success;
    }
};

template <typename T, type expected>
struct checker<T, expected, typename std::enable_if<has_check_lua<T>::value>::type> {
    template <typename Handler>
    static bool check (lua_State* L, int index, const Handler& handler) {
        bool success = check_lua(types<T>(), L, index);
        if (!success) {
            // expected type, actual type
            const type indextype = type_of(L, index);
            handler(L, index, expected, indextype);
        }
        return success;
    }
};

template <typename T, typename Handler>
bool check(lua_State* L, int index, Handler&& handler) {
    typedef Unqualified<T> Tu;
    checker<Tu> c;
    return c.check(L, index, std::forward<Handler>(handler));
}

template <typename T>
bool check(lua_State* L, int index) {
    auto handler = type_panic;
    return check<T>(L, index, handler);
}

template<typename T, typename>
struct getter {
    template<typename U = T, EnableIf<std::is_floating_point<U>> = 0>
    static U get(lua_State* L, int index = -1) {
        return static_cast<U>(lua_tonumber(L, index));
    }

    template<typename U = T, EnableIf<std::is_integral<U>, std::is_signed<U>> = 0>
    static U get(lua_State* L, int index = -1) {
        return static_cast<T>(lua_tointeger(L, index));
    }

    template<typename U = T, EnableIf<std::is_integral<U>, std::is_unsigned<U>> = 0>
    static U get(lua_State* L, int index = -1) {
        typedef typename std::make_signed<U>::type signed_int;
        return static_cast<U>(stack::get<signed_int>(L, index));
    }

    template<typename U = T, EnableIf<std::is_base_of<reference, U>> = 0>
    static U get(lua_State* L, int index = -1) {
        return T(L, index);
    }

    template<typename U = T, EnableIf<Not<std::is_base_of<reference, U>>, Not<std::is_integral<U>>, Not<std::is_floating_point<U>>> = 0>
    static auto get(lua_State* L, int index = -1) -> decltype(get_from_lua<U>(has_from_lua<U>(), L, index)) {
        return get_from_lua<U>(has_from_lua<U>(), L, index);
    }

    template <typename U>
    static auto get_from_lua(std::true_type, lua_State* L, int index = -1) -> decltype(from_lua(types<T>(), L, index)) {
        return from_lua(types<U>(), L, index);
    }

    template <typename U>
    static T& get_from_lua(std::false_type, lua_State* L, int index = -1) {
        void* udata = lua_touserdata(L, index);
        T* obj = static_cast<T*>(udata);
        return *obj;
    }
};

template<typename T>
struct getter<T*> {
    static T* get(lua_State* L, int index = -1) {
        void* udata = lua_touserdata(L, index);
        T** obj = static_cast<T**>(udata);
        return *obj;
    }
};

template<typename T>
struct getter<T&> {
    static T& get(lua_State* L, int index = -1) {
        void* udata = lua_touserdata(L, index);
        T** obj = static_cast<T**>(udata);
        return **obj;
    }
};

template<typename T>
struct getter<std::reference_wrapper<T>> {
    static T& get(lua_State* L, int index = -1) {
        return getter<T&>{}.get(L, index);
    }
};

template<>
struct getter<type> {
    static type get(lua_State *L, int index){
        return static_cast<type>(lua_type(L, index));
    }
};

template<>
struct getter<bool> {
    static bool get(lua_State* L, int index) {
        return lua_toboolean(L, index) != 0;
    }
};

template<>
struct getter<std::string> {
    static std::string get(lua_State* L, int index = -1) {
        std::string::size_type len;
        auto str = lua_tolstring(L, index, &len);
        return { str, len };
    }
};

template<>
struct getter<const char*> {
    static const char* get(lua_State* L, int index = -1) {
        return lua_tostring(L, index);
    }
};

template<>
struct getter<nil_t> {
    static nil_t get(lua_State* L, int index = -1) {
        if(lua_isnil(L, index) == 0) {
            throw sol::error("not nil");
        }
        return nil_t{ };
    }
};

template<>
struct getter<userdata> {
    static userdata get(lua_State* L, int index = -1) {
        return{ lua_touserdata(L, index) };
    }
};

template<>
struct getter<light_userdata> {
    static light_userdata get(lua_State* L, int index = 1) {
        return{ lua_touserdata(L, index) };
    }
};

template<>
struct getter<upvalue> {
    static upvalue get(lua_State* L, int index = 1) {
        return{ lua_touserdata(L, lua_upvalueindex(index)) };
    }
};

template<>
struct getter<void*> {
    static void* get(lua_State* L, int index = 1) {
        return lua_touserdata(L, index);
    }
};

template<typename T, typename>
struct pusher {
    template<typename U = T, EnableIf<std::is_floating_point<U>> = 0>
    static int push(lua_State* L, const T& value) {
        lua_pushnumber(L, value);
        return 1;
    }

    template<typename U = T, EnableIf<std::is_integral<U>, std::is_signed<U>> = 0>
    static int push(lua_State* L, const T& value) {
        lua_pushinteger(L, value);
        return 1;
    }

    template<typename U = T, EnableIf<std::is_integral<U>, std::is_unsigned<U>> = 0>
    static int push(lua_State* L, const T& value) {
         typedef typename std::make_signed<T>::type signed_int;
         return stack::push(L, static_cast<signed_int>(value));
    }

    template<typename U = T, EnableIf<has_begin_end<U>, Not<has_key_value_pair<U>>> = 0>
    static int push(lua_State* L, const T& cont) {
        lua_createtable(L, cont.size(), 0);
        unsigned index = 1;
        for(auto&& i : cont) {
            // push the index
            pusher<unsigned>{}.push(L, index++);
            // push the value
            pusher<Unqualified<decltype(i)>>{}.push(L, i);
            // set the table
            lua_settable(L, -3);
        }
        return 1;
    }

    template<typename U = T, EnableIf<has_begin_end<U>, has_key_value_pair<U>> = 0>
    static int push(lua_State* L, const T& cont) {
        lua_createtable(L, cont.size(), 0);
        for(auto&& pair : cont) {
            pusher<Unqualified<decltype(pair.first)>>{}.push(L, pair.first);
            pusher<Unqualified<decltype(pair.second)>>{}.push(L, pair.second);
            lua_settable(L, -3);
        }
        return 1;
    }

    template<typename U = T, EnableIf<std::is_base_of<reference, U>> = 0>
    static int push(lua_State*, T& ref) {
       return ref.push();
    }

    template<typename Arg, typename U = Unqualified<T>, EnableIf<Not<has_begin_end<U>>, Not<std::is_base_of<reference, U>>, Not<std::is_integral<U>>, Not<std::is_floating_point<U>>> = 0>
    static int push(lua_State* L, Arg&& arg) {
        return push_to_lua(has_to_lua<T>(), L, std::forward<Arg>(arg));
    }

    template<typename Arg, typename U = Unqualified<T>, EnableIf<Not<has_begin_end<U>>, Not<std::is_base_of<reference, U>>, Not<std::is_integral<U>>, Not<std::is_floating_point<U>>> = 0>
    static int push_to_lua(std::true_type, lua_State* L, Arg&& arg) {
        return to_lua(L, std::forward<Arg>(arg));
    }

    template<typename Arg, typename U = Unqualified<T>, EnableIf<Not<has_begin_end<U>>, Not<std::is_base_of<reference, U>>, Not<std::is_integral<U>>, Not<std::is_floating_point<U>>> = 0>
    static int push_to_lua(std::false_type, lua_State* L, Arg&& arg) {
        return detail::push_userdata<U>(L, usertype_traits<T>::metatable, std::forward<Arg>(arg));
    }
};

template<typename T>
struct pusher<T*> {
    static int push(lua_State* L, T* obj) {
        return detail::push_userdata<T*>(L, usertype_traits<T*>::metatable, obj);
    }
};

template<typename T>
struct pusher<std::reference_wrapper<T>> {
    static int push(lua_State* L, const std::reference_wrapper<T>& t) {
        return stack::push(L, std::addressof(t.get()));
    }
};

template<>
struct pusher<bool> {
    static int push(lua_State* L, const bool& b) {
        lua_pushboolean(L, b);
        return 1;
    }
};

template<>
struct pusher<nil_t> {
    static int push(lua_State* L, const nil_t&) {
        lua_pushnil(L);
        return 1;
    }
};

template<>
struct pusher<lua_CFunction> {
    static int push(lua_State* L, lua_CFunction func, int n = 0) {
        lua_pushcclosure(L, func, n);
        return 1;
    }
};

template<>
struct pusher<void*> {
    static int push(lua_State* L, void* userdata) {
        lua_pushlightuserdata(L, userdata);
        return 1;
    }
};

template<>
struct pusher<upvalue> {
    static int push(lua_State* L, upvalue upvalue) {
        lua_pushlightuserdata(L, upvalue);
        return 1;
    }
};

template<>
struct pusher<light_userdata> {
    static int push(lua_State* L, light_userdata userdata) {
        lua_pushlightuserdata(L, userdata);
        return 1;
    }
};

template<>
struct pusher<userdata> {
    template<typename T, typename U = Unqualified<T>>
    static int push(lua_State* L, T&& data) {
        U* userdata = static_cast<U*>(lua_newuserdata(L, sizeof(U)));
        new(userdata)U(std::forward<T>(data));
        return 1;
    }
};

template<>
struct pusher<const char*> {
    static int push(lua_State* L, const char* str) {
        lua_pushlstring(L, str, std::char_traits<char>::length(str));
        return 1;
    }
};

template<size_t N>
struct pusher<char[N]> {
    static int push(lua_State* L, const char (&str)[N]) {
        lua_pushlstring(L, str, N - 1);
        return 1;
    }
};

template<>
struct pusher<std::string> {
    static int push(lua_State* L, const std::string& str) {
        lua_pushlstring(L, str.c_str(), str.size());
        return 1;
    }
};

template<typename... Args>
struct pusher<std::tuple<Args...>> {
    template <typename Tuple>
    static int push(lua_State* L, Tuple&& tuplen) {
        return push_tuple(L, build_indices<sizeof...(Args)>(), std::forward<Tuple>(tuplen));
    }
};

namespace detail {
template<typename T>
inline int push_as_upvalues(lua_State* L, T& item) {
    typedef typename std::decay<T>::type TValue;
    const static std::size_t itemsize = sizeof(TValue);
    const static std::size_t voidsize = sizeof(void*);
    const static std::size_t voidsizem1 = voidsize - 1;
    const static std::size_t data_t_count = (sizeof(TValue) + voidsizem1) / voidsize;
    typedef std::array<void*, data_t_count> data_t;

    data_t data{{}};
    std::memcpy(std::addressof(data[0]), std::addressof(item), itemsize);
    int pushcount = 0;
    for(auto&& v : data) {
        pushcount += push(L, upvalue(v));
    }
    return pushcount;
}

template<typename T>
inline std::pair<T, int> get_as_upvalues(lua_State* L, int index = 1) {
    const static std::size_t data_t_count = (sizeof(T)+(sizeof(void*)-1)) / sizeof(void*);
    typedef std::array<void*, data_t_count> data_t;
    data_t voiddata{ {} };
    for(std::size_t i = 0, d = 0; d < sizeof(T); ++i, d += sizeof(void*)) {
        voiddata[i] = get<upvalue>(L, index++);
    }
    return std::pair<T, int>(*reinterpret_cast<T*>(static_cast<void*>(voiddata.data())), index);
}

template <bool b>
struct check_arguments {
    template <std::size_t... I, typename... Args>
    static bool check(lua_State* L, int firstargument, indices<I...>, types<Args...>) {
        bool checks = true;
        using swallow = int[sizeof...(Args)+2];
        (void)swallow {
            0, (checks &= stack::check<Args>(L, firstargument + I))..., 0
        };
        return checks;
    }
};

template <>
struct check_arguments<false> {
    template <std::size_t... I, typename... Args>
    static bool check(lua_State*, int, indices<I...>, types<Args...>) {
        return true;
    }
};

template <bool checkargs = false, std::size_t... I, typename R, typename... Args, typename Fx, typename... FxArgs, typename = typename std::enable_if<!std::is_void<R>::value>::type>
inline R call(lua_State* L, int start, indices<I...>, types<R>, types<Args...> ta, Fx&& fx, FxArgs&&... args) {
    const int stacksize = lua_gettop(L);
    const int firstargument = start + stacksize - std::max(sizeof...(Args)-1, static_cast<std::size_t>(0));

    detail::check_arguments<checkargs>{}.check(L, firstargument, ta, ta);

    return fx(std::forward<FxArgs>(args)..., stack::get<Args>(L, firstargument + I)...);
}

template <bool checkargs = false, std::size_t... I, typename... Args, typename Fx, typename... FxArgs>
inline void call(lua_State* L, int start, indices<I...>, types<void>, types<Args...> ta, Fx&& fx, FxArgs&&... args) {
    const int stacksize = lua_gettop(L);
    const int firstargument = start + stacksize - std::max(sizeof...(Args)-1, static_cast<std::size_t>(0));

    bool checks = detail::check_arguments<checkargs>{}.check(L, firstargument, ta, ta);
    if ( !checks )
        throw error("Arguments not of the proper types for this function call");

    fx(std::forward<FxArgs>(args)..., stack::get<Args>(L, firstargument + I)...);
}
} // detail

template <bool checkargs = false, typename R, typename... Args, typename Fx, typename... FxArgs, typename = typename std::enable_if<!std::is_void<R>::value>::type>
inline R call(lua_State* L, int start, types<R> tr, types<Args...> ta, Fx&& fx, FxArgs&&... args) {
    return detail::call(L, start, ta, tr, ta, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
}

template <bool checkargs = false, typename R, typename... Args, typename Fx, typename... FxArgs, typename = typename std::enable_if<!std::is_void<R>::value>::type>
inline R call(lua_State* L, types<R> tr, types<Args...> ta, Fx&& fx, FxArgs&&... args) {
    return call(L, 0, ta, tr, ta, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
}

template <bool checkargs = false, typename... Args, typename Fx, typename... FxArgs>
inline void call(lua_State* L, int start, types<void> tr, types<Args...> ta, Fx&& fx, FxArgs&&... args) {
    detail::call(L, start, ta, tr, ta, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
}

template <bool checkargs = false, typename... Args, typename Fx, typename... FxArgs>
inline void call(lua_State* L, types<void> tr, types<Args...> ta, Fx&& fx, FxArgs&&... args) {
    call(L, 0, ta, tr, ta, std::forward<Fx>(fx), std::forward<FxArgs>(args)...);
}

inline call_syntax get_call_syntax(lua_State* L, const std::string& meta) {
    if (get<type>(L, 1) == type::table) {
        if (luaL_newmetatable(L, meta.c_str()) == 0) {
            lua_settop(L, -2);
            return call_syntax::colon;
        }
    }
    return call_syntax::dot;
}
} // stack
} // sol

#endif // SOL_STACK_HPP

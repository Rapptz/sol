.. default-domain:: cpp
.. highlight:: cpp
.. _sol-api-types:

Lua Types
===============

In order to interact seamlessly with the Lua C API, Sol specifies some types and enums that interop with the C and
C++ API.

This file can be included through::

    #include <sol/types.hpp>

.. namespace:: sol

.. class:: nil_t

    Defines the type equivalent to ``nil`` in Lua to C++.
.. c:var:: const nil_t nil

    Defines an instance of the :class:`nil_t` class for use with the API.

.. function:: bool operator==(nil_t, nil_t) noexcept
.. function:: bool operator!=(nil_t, nil_t) noexcept

    Implements strict equivalence with two :class:`nil_t` types.
.. type:: enum type

    A type safe enum representation of the enum returned by :lua:`type`. Note that this is an
    ``enum class`` and not a regular ``enum``.

    .. c:var:: none

        Equivalent to LUA_TNONE.
    .. c:var:: nil

        Equivalent to LUA_TNIL.
    .. c:var:: string

        Equivalent to LUA_TSTRING.
    .. c:var:: number

        Equivalent to LUA_TNUMBER.
    .. c:var:: thread

        Equivalent to LUA_TTHREAD.
    .. c:var:: boolean

        Equivalent to LUA_TBOOLEAN.
    .. c:var:: function

        Equivalent to LUA_TFUNCTION.
    .. c:var:: userdata

        Equivalent to LUA_TUSERDATA.
    .. c:var:: lightuserdata

        Equivalent to LUA_TLIGHTUSERDATA.
    .. c:var:: table

        Equivalent to LUA_TTABLE.
    .. c:var:: poly

        This is the equivalent to all the enum types available with ``operator|`` to chain them. This
        is typically used for :class:`object` and shouldn't be used manually.

.. function:: void type_error(lua_State* L, int expected, int actual)

    A wrapped up mechanism to call :lual:`error` with the string ``"expected <type>, received <type>"``.
.. function:: void type_assert(lua_State* L, int index, type expected)

    Asserts a if the current :type:`type` at the specified index is the one expected. If it isn't, then
    :func:`type_error` is called.
.. function:: std::string type_name(lua_State* L, type t)

    Returns a string representation of the :type:`type` provided. This is the same as calling :lua:`typename`.
.. function:: type type_of<T>()

    Returns a :type:`type` based on the template type provided. If the type is a class, enum, or some unknown type
    then ``type::userdata`` is returned.

    Note that this doesn't take into account cv and ref qualifiers. If this is an issue, then using
    ``std::decay`` would be a good way to get a nicer result.

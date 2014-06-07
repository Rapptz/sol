#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <sol.hpp>
#include <vector>
#include <map>

void test_free_func(std::function<void()> f) {
    f();
}

void test_free_func2(std::function<int(int)> f, int arg1) {
    int val = f(arg1);
    if(val != arg1)
        throw sol::error("failed function call!");
}

std::function<int()> makefn() {
    auto fx = []() -> int {
        return 0x1456789;
    };
    return fx;
}

void takefn(std::function<int()> purr) {
    if (purr() != 0x1456789)
        throw 0;
}

std::string free_function() {
    std::cout << "free_function()" << std::endl;
    return "test";
}

std::vector<int> test_table_return_one() {
    return { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
}

std::vector<std::pair<std::string, int>> test_table_return_two() {
    return {{ "one", 1 }, { "two", 2 }, { "three", 3 }};
}

std::map<std::string, std::string> test_table_return_three() {
    return {{ "name", "Rapptz" }, { "friend", "ThePhD" }, { "project", "sol" }};
}

struct self_test {
    int bark;

    self_test() : bark(100) {

    }

    void g(const std::string& str) {
        std::cout << str << '\n';
        bark += 1;
    }

    void f(const self_test& t) {
        std::cout << "got test" << '\n';
        if (t.bark != bark)
            throw sol::error("bark values are not the same for self_test f function");
        if (&t != this)
            throw sol::error("call does not reference self for self_test f function");
    }
};

struct object {
    std::string operator() () {
        std::cout << "member_test()" << std::endl;
        return "test";
    }
};

struct fuser {
    int x;
    fuser() : x(0) {}

    fuser(int x) : x(x) {}

    int add(int y) {
       return x + y;
    }

    int add2(int y) {
       return x + y + 2;
    }
};

namespace crapola {
struct fuser {
    int x;
    fuser() : x(0) {}
    fuser(int x) : x(x) {}
    fuser(int x, int x2) : x(x * x2) {}

    int add(int y) {
       return x + y;
    }
    int add2(int y) {
       return x + y + 2;
    }
};
} // crapola

int plop_xyz(int x, int y, std::string z) {
    std::cout << x << " " << y << " " << z << std::endl;
    return 11;
}

class Base {
public:
    Base(int a_num) : m_num(a_num) { }

    int get_num() {
        return m_num;
    }

protected:
    int m_num;
};

class Derived : public Base {
public:
    Derived(int a_num) : Base(a_num) { }

    int get_num_10() {
        return 10 * m_num;
    }
};

TEST_CASE("simple/set_global", "Check if the set_global works properly.") {
    sol::state lua;

    lua.set("a", 9);
    REQUIRE_NOTHROW(lua.script("if a ~= 9 then error('wrong value') end"));

    lua.set("d", "hello");
    REQUIRE_NOTHROW(lua.script("if d ~= 'hello' then error('expected \\'hello\\', got '.. tostring(d)) end"));

    lua.set("e", std::string("hello"));
    REQUIRE_NOTHROW(lua.script("if d ~= 'hello' then error('expected \\'hello\\', got '.. tostring(d)) end"));

    lua.set("f", true);
    REQUIRE_NOTHROW(lua.script("if f ~= true then error('wrong value') end"));
}

TEST_CASE("simple/get", "Tests if the get function works properly.") {
    sol::state lua;

    lua.script("a = 9");
    auto a = lua.get<int>("a");
    REQUIRE(a == 9.0);

    lua.script("b = nil");
    REQUIRE_NOTHROW(lua.get<sol::nil_t>("b"));

    lua.script("d = 'hello'");
    auto d = lua.get<std::string>("d");
    REQUIRE(d == "hello");

    lua.script("e = true");
    auto e = lua.get<bool>("e");
    REQUIRE(e == true);
}

TEST_CASE("simple/addition", "check if addition works and can be gotten through lua.get and lua.set") {
    sol::state lua;

    lua.set("b", 0.2);
    lua.script("c = 9 + b");
    auto c = lua.get<double>("c");

    REQUIRE(c == 9.2);
}

TEST_CASE("simple/if", "check if if statements work through lua") {
    sol::state lua;

    std::string program = "if true then f = 0.1 else f = 'test' end";
    lua.script(program);
    auto f = lua.get<double>("f");

    REQUIRE(f == 0.1);
    REQUIRE((f == lua["f"]));
}

TEST_CASE("simple/call_with_parameters", "Lua function is called with a few parameters from C++") {
    sol::state lua;

    REQUIRE_NOTHROW(lua.script("function my_add(i, j, k) return i + j + k end"));
    auto f = lua.get<sol::function>("my_add");
    REQUIRE_NOTHROW(lua.script("function my_nothing(i, j, k) end"));
    auto fvoid = lua.get<sol::function>("my_nothing");
    int a;
    REQUIRE_NOTHROW(fvoid(1, 2, 3));
    REQUIRE_NOTHROW(a = f.call<int>(1, 2, 3));
    REQUIRE(a == 6);
    REQUIRE_THROWS(a = f.call<int>(1, 2, "arf"));
}

TEST_CASE("simple/call_c++_function", "C++ function is called from lua") {
    sol::state lua;

    lua.set_function("plop_xyz", plop_xyz);
    lua.script("x = plop_xyz(2, 6, 'hello')");

    REQUIRE(lua.get<int>("x") == 11);
}

TEST_CASE("simple/call_lambda", "A C++ lambda is exposed to lua and called") {
    sol::state lua;

    int x = 0;

    lua.set_function("foo", [&x] { x = 1; });

    lua.script("foo()");

    REQUIRE(x == 1);
}

TEST_CASE("advanced/get_and_call", "Checks for lambdas returning values after a get operation") {
    const static std::string lol = "lol", str = "str";
    const static std::tuple<int, float, double, std::string> heh_tuple = std::make_tuple(1, 6.28f, 3.14, std::string("heh"));
    sol::state lua;

    REQUIRE_NOTHROW(lua.set_function("a", [] { return 42; }));
    REQUIRE(lua.get<sol::function>("a").call<int>() == 42);

    REQUIRE_NOTHROW(lua.set_function("b", [] { return 42u; }));
    REQUIRE(lua.get<sol::function>("b").call<unsigned int>() == 42u);

    REQUIRE_NOTHROW(lua.set_function("c", [] { return 3.14; }));
    REQUIRE(lua.get<sol::function>("c").call<double>() == 3.14);

    REQUIRE_NOTHROW(lua.set_function("d", [] { return 6.28f; }));
    REQUIRE(lua.get<sol::function>("d").call<float>() == 6.28f);

    REQUIRE_NOTHROW(lua.set_function("e", [] { return "lol"; }));
    REQUIRE(lua.get<sol::function>("e").call<std::string>() == lol);

    REQUIRE_NOTHROW(lua.set_function("f", [] { return true; }));
    REQUIRE(lua.get<sol::function>("f").call<bool>());

    REQUIRE_NOTHROW(lua.set_function("g", [] { return std::string("str"); }));
    REQUIRE(lua.get<sol::function>("g").call<std::string>() == str);

    REQUIRE_NOTHROW(lua.set_function("h", [] { }));
    REQUIRE_NOTHROW(lua.get<sol::function>("h").call());

    REQUIRE_NOTHROW(lua.set_function("i", [] { return sol::nil; }));
    REQUIRE(lua.get<sol::function>("i").call<sol::nil_t>() == sol::nil);
    REQUIRE_NOTHROW(lua.set_function("j", [] { return std::make_tuple(1, 6.28f, 3.14, std::string("heh")); }));
    REQUIRE((lua.get<sol::function>("j").call<int, float, double, std::string>() == heh_tuple));
}

TEST_CASE("advanced/operator[]_calls", "Checks for lambdas returning values using operator[]") {
    const static std::string lol = "lol", str = "str";
    const static std::tuple<int, float, double, std::string> heh_tuple = std::make_tuple(1, 6.28f, 3.14, std::string("heh"));
    sol::state lua;

    REQUIRE_NOTHROW(lua.set_function("a", [] { return 42; }));
    REQUIRE(lua["a"].call<int>() == 42);

    REQUIRE_NOTHROW(lua.set_function("b", [] { return 42u; }));
    REQUIRE(lua["b"].call<unsigned int>() == 42u);

    REQUIRE_NOTHROW(lua.set_function("c", [] { return 3.14; }));
    REQUIRE(lua["c"].call<double>() == 3.14);

    REQUIRE_NOTHROW(lua.set_function("d", [] { return 6.28f; }));
    REQUIRE(lua["d"].call<float>() == 6.28f);

    REQUIRE_NOTHROW(lua.set_function("e", [] { return "lol"; }));
    REQUIRE(lua["e"].call<std::string>() == lol);

    REQUIRE_NOTHROW(lua.set_function("f", [] { return true; }));
    REQUIRE(lua["f"].call<bool>());

    REQUIRE_NOTHROW(lua.set_function("g", [] { return std::string("str"); }));
    REQUIRE(lua["g"].call<std::string>() == str);

    REQUIRE_NOTHROW(lua.set_function("h", [] { }));
    REQUIRE_NOTHROW(lua["h"].call());

    REQUIRE_NOTHROW(lua.set_function("i", [] { return sol::nil; }));
    REQUIRE(lua["i"].call<sol::nil_t>() == sol::nil);
    REQUIRE_NOTHROW(lua.set_function("j", [] { return std::make_tuple(1, 6.28f, 3.14, std::string("heh")); }));
    REQUIRE((lua["j"].call<int, float, double, std::string>() == heh_tuple));
}

TEST_CASE("advanced/call_lambdas", "A C++ lambda is exposed to lua and called") {
    sol::state lua;

    int x = 0;
    lua.set_function("set_x", [&] (int new_x) {
        x = new_x;
        return 0;
    });

    lua.script("set_x(9)");
    REQUIRE(x == 9);
}

TEST_CASE("negative/basic_errors", "Check if error handling works correctly") {
    sol::state lua;

    REQUIRE_THROWS(lua.script("nil[5]"));
}

TEST_CASE("libraries", "Check if we can open libraries") {
    sol::state lua;
    REQUIRE_NOTHROW(lua.open_libraries(sol::lib::base, sol::lib::os));
}

TEST_CASE("tables/variables", "Check if tables and variables work as intended") {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::os);
    lua.get<sol::table>("os").set("name", "windows");
    REQUIRE_NOTHROW(lua.script("assert(os.name == \"windows\")"));
}

TEST_CASE("tables/functions_variables", "Check if tables and function calls work as intended") {
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::os);
    auto run_script = [] (sol::state& lua) -> void {
        lua.script("assert(os.fun() == \"test\")");
    };

    lua.get<sol::table>("os").set_function("fun",
        [] () {
            std::cout << "stateless lambda()" << std::endl;
            return "test";
        }
   );
    REQUIRE_NOTHROW(run_script(lua));

    lua.get<sol::table>("os").set_function("fun", &free_function);
    REQUIRE_NOTHROW(run_script(lua));

    // l-value, can optimise
    auto lval = object();
    lua.get<sol::table>("os").set_function("fun", &object::operator(), lval);
    REQUIRE_NOTHROW(run_script(lua));

    // stateful lambda: non-convertible, cannot be optimised
    int breakit = 50;
    lua.get<sol::table>("os").set_function("fun",
        [&breakit] () {
        std::cout << "stateless lambda()" << std::endl;
        return "test";
    }
);
    REQUIRE_NOTHROW(run_script(lua));

    // r-value, cannot optimise
    lua.get<sol::table>("os").set_function("fun", &object::operator(), object());
    REQUIRE_NOTHROW(run_script(lua));

    // r-value, cannot optimise
    auto rval = object();
    lua.get<sol::table>("os").set_function("fun", &object::operator(), std::move(rval));
    REQUIRE_NOTHROW(run_script(lua));
}

TEST_CASE("functions/return_order_and_multi_get", "Check if return order is in the same reading order specified in Lua") {
    const static std::tuple<int, int, int> triple = std::make_tuple(10, 11, 12);
    sol::state lua;
    lua.set_function("f", [] {
        return std::make_tuple(10, 11, 12);
    });
    lua.script("function g() return 10, 11, 12 end\nx,y,z = g()");
    auto tcpp = lua.get<sol::function>("f").call<int, int, int>();
    auto tlua = lua.get<sol::function>("g").call<int, int, int>();
    auto tluaget = lua.get<int, int, int>("x", "y", "z");
    std::cout << "cpp: " << std::get<0>(tcpp) << ',' << std::get<1>(tcpp) << ',' << std::get<2>(tcpp) << std::endl;
    std::cout << "lua: " << std::get<0>(tlua) << ',' << std::get<1>(tlua) << ',' << std::get<2>(tlua) << std::endl;
    std::cout << "lua.xyz: " << lua.get<int>("x") << ',' << lua.get<int>("y") << ',' << lua.get<int>("z") << std::endl;
    REQUIRE(tcpp == triple);
    REQUIRE(tlua == triple);
    REQUIRE(tluaget == triple);
}

TEST_CASE("functions/sol::function to std::function", "check if conversion to std::function works properly and calls with correct arguments") {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua.set_function("testFunc", test_free_func);
    lua.set_function("testFunc2", test_free_func2);
    lua.script(
        "testFunc(function() print(\"hello std::function\") end)"
      );
    lua.script(
        "function m(a)\n"
        "     print(\"hello std::function with arg \", a)\n"
        "     return a\n"
        "end\n"
        "\n"
        "testFunc2(m, 1)"
      );
}

TEST_CASE("functions/returning functions from C++ and getting in lua", "check to see if returning a functor and getting a functor from lua is possible") {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua.set_function("makefn", makefn);
    lua.set_function("takefn", takefn);
    lua.script("afx = makefn()\n"
                "print(afx())\n"
                "takefn(afx)\n");
}

TEST_CASE("tables/operator[]", "Check if operator[] retrieval and setting works properly") {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua.script("foo = 20\nbar = \"hello world\"");
    // basic retrieval
    std::string bar = lua["bar"];
    int foo = lua["foo"];
    REQUIRE(bar == "hello world");
    REQUIRE(foo == 20);
    // test operator= for stringification
    // errors due to ambiguous operators
    bar = lua["bar"];

    // basic setting
    lua["bar"] = 20.4;
    lua["foo"] = "goodbye";

    // doesn't modify the actual values obviously.
    REQUIRE(bar == "hello world");
    REQUIRE(foo == 20);

    // function setting
    lua["test"] = plop_xyz;
    REQUIRE_NOTHROW(lua.script("assert(test(10, 11, \"hello\") == 11)"));

    // function retrieval
    sol::function test = lua["test"];
    REQUIRE(test.call<int>(10, 11, "hello") == 11);

    // setting a lambda
    lua["lamb"] = [](int x) {
        return x * 2;
    };

    REQUIRE_NOTHROW(lua.script("assert(lamb(220) == 440)"));

    // function retrieval of a lambda
    sol::function lamb = lua["lamb"];
    REQUIRE(lamb.call<int>(220) == 440);

    // test const table retrieval
    auto assert1 = [](const sol::table& t) {
        std::string a = t["foo"];
        int b = t["bar"];
        std::cout << a << ',' << b << '\n';
    };

    REQUIRE_NOTHROW(assert1(lua.global_table()));
}

TEST_CASE("tables/userdata", "Show that we can create classes from userdata and use them") {

    sol::state lua;

    sol::userdata<fuser> lc{ "add", &fuser::add, "add2", &fuser::add2 };
    lua.set_userdata(lc);

    lua.script("a = fuser:new()\n"
               "b = a:add(1)\n"
               "c = a:add2(1)\n");

    sol::object a = lua.get<sol::object>("a");
    sol::object b = lua.get<sol::object>("b");
    sol::object c = lua.get<sol::object>("c");
    REQUIRE((a.is<sol::userdata_t>()));
    auto atype = a.get_type();
    auto btype = b.get_type();
    auto ctype = c.get_type();
    REQUIRE((atype == sol::type::userdata));
    REQUIRE((btype == sol::type::number));
    REQUIRE((ctype == sol::type::number));
    int bresult = b.as<int>();
    int cresult = c.as<int>();
    REQUIRE(bresult == 1);
    REQUIRE(cresult == 3);
}

TEST_CASE("tables/userdata constructors", "Show that we can create classes from userdata and use them with multiple destructors") {

    sol::state lua;

    sol::constructors<sol::types<>, sol::types<int>, sol::types<int, int>> con;
    sol::userdata<crapola::fuser> lc("crapola_fuser", con, "add", &crapola::fuser::add, "add2", &crapola::fuser::add2);
    lua.set_userdata(lc);

    lua.script(
        "a = crapola_fuser.new(2)\n"
        "u = a:add(1)\n"
        "v = a:add2(1)\n"
        "b = crapola_fuser:new()\n"
        "w = b:add(1)\n"
        "x = b:add2(1)\n"
        "c = crapola_fuser.new(2, 3)\n"
        "y = c:add(1)\n"
        "z = c:add2(1)\n");
    sol::object a = lua.get<sol::object>("a");
    auto atype = a.get_type();
    REQUIRE((atype == sol::type::userdata));
    sol::object u = lua.get<sol::object>("u");
    sol::object v = lua.get<sol::object>("v");
    REQUIRE((u.as<int>() == 3));
    REQUIRE((v.as<int>() == 5));

    sol::object b = lua.get<sol::object>("b");
    auto btype = b.get_type();
    REQUIRE((btype == sol::type::userdata));
    sol::object w = lua.get<sol::object>("w");
    sol::object x = lua.get<sol::object>("x");
    REQUIRE((w.as<int>() == 1));
    REQUIRE((x.as<int>() == 3));

    sol::object c = lua.get<sol::object>("c");
    auto ctype = c.get_type();
    REQUIRE((ctype == sol::type::userdata));
    sol::object y = lua.get<sol::object>("y");
    sol::object z = lua.get<sol::object>("z");
    REQUIRE((y.as<int>() == 7));
    REQUIRE((z.as<int>() == 9));
}

TEST_CASE("tables/userdata utility", "Show internal management of classes registered through new_userdata") {
    sol::state lua;

    lua.new_userdata<fuser>("fuser", "add", &fuser::add, "add2", &fuser::add2);

    lua.script("a = fuser.new()\n"
        "b = a:add(1)\n"
        "c = a:add2(1)\n");

    sol::object a = lua.get<sol::object>("a");
    sol::object b = lua.get<sol::object>("b");
    sol::object c = lua.get<sol::object>("c");
    REQUIRE((a.is<sol::userdata_t>()));
    auto atype = a.get_type();
    auto btype = b.get_type();
    auto ctype = c.get_type();
    REQUIRE((atype == sol::type::userdata));
    REQUIRE((btype == sol::type::number));
    REQUIRE((ctype == sol::type::number));
    int bresult = b.as<int>();
    int cresult = c.as<int>();
    REQUIRE(bresult == 1);
    REQUIRE(cresult == 3);
}

TEST_CASE("tables/userdata utility derived", "userdata classes must play nice when a derived class does not overload a publically visible base function") {
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    sol::constructors<sol::types<int>> basector;
    sol::userdata<Base> baseuserdata("Base", basector, "get_num", &Base::get_num);

    lua.set_userdata(baseuserdata);

    lua.script("base = Base.new(5)");
    lua.script("print(base:get_num())");

    sol::constructors<sol::types<int>> derivedctor;
    sol::userdata<Derived> deriveduserdata("Derived", derivedctor, "get_num", &Derived::get_num, "get_num_10", &Derived::get_num_10);

    lua.set_userdata(deriveduserdata);

    lua.script("derived = Derived.new(7)");
    lua.script("dgn10 = derived:get_num_10()\nprint(dgn10)");
    lua.script("dgn = derived:get_num()\nprint(dgn)");

    REQUIRE((lua.get<int>("dgn10") == 70));
    REQUIRE((lua.get<int>("dgn") == 7));
}

TEST_CASE("tables/self-referential userdata", "userdata classes must play nice when C++ object types are requested for C++ code") {
    sol::state lua;
    lua.open_libraries(sol::lib::base);

    lua.new_userdata<self_test>("test", "g", &self_test::g, "f", &self_test::f);

    lua.script(
        "local a = test.new()\n"
        "a:g(\"woof\")\n"
        "a:f(a)\n"
      );
}

TEST_CASE("tables/arbitrary-creation", "tables should be created from standard containers") {
    sol::state lua;
    lua.open_libraries(sol::lib::base);
    lua.set_function("test_one", test_table_return_one);
    lua.set_function("test_two", test_table_return_two);
    lua.set_function("test_three", test_table_return_three);

    REQUIRE_NOTHROW(lua.script("a = test_one()"));
    REQUIRE_NOTHROW(lua.script("b = test_two()"));
    REQUIRE_NOTHROW(lua.script("c = test_three()"));

    REQUIRE_NOTHROW(lua.script("assert(#a == 10, 'error')"));
    REQUIRE_NOTHROW(lua.script("assert(a[3] == 3, 'error')"));
    REQUIRE_NOTHROW(lua.script("assert(b.one == 1, 'error')"));
    REQUIRE_NOTHROW(lua.script("assert(b.three == 3, 'error')"));
    REQUIRE_NOTHROW(lua.script("assert(c.name == 'Rapptz', 'error')"));
    REQUIRE_NOTHROW(lua.script("assert(c.project == 'sol', 'error')"));

    auto&& a = lua.get<sol::table>("a");
    auto&& b = lua.get<sol::table>("b");
    auto&& c = lua.get<sol::table>("c");

    REQUIRE(a.size() == 10ULL);
    REQUIRE(a.get<int>(3) == 3);
    REQUIRE(b.get<int>("one") == 1);
    REQUIRE(b.get<int>("three") == 3);
    REQUIRE(c.get<std::string>("name") == "Rapptz");
    REQUIRE(c.get<std::string>("project") == "sol");
}

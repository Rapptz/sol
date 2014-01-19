#include <string>
#include <cassert>
#include <sol.hpp>


int main() {
    sol::state lua;
    
    // you can write scripts right inside C++
    // and execute them
    lua.script( "local get_me = 0"
        "function run_me()"
        "    get_me = 24;"
        "end"
    );

    // get the value from the script we just executed
    int check_get_me = lua.get<int>( "get_me" );
    assert( check_get_me == 0 );
    // get the function from the script we just executed
    // and use it
    lua.get<sol::function>( "run_me" )( );
    check_get_me = lua.get<int>( "get_me" );
    assert( check_get_me == 24 );
}
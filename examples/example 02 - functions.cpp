#include <string>
#include <cassert>
#include <iostream>
#include <sol.hpp>

int main( ) {
    // make a lua state: all you have to do is declare it
    sol::state state;
    // pop open a fresh script file
    state.open_file( "example 02 - functions.lua" );
    
    int difference;
    std::string greeting;

    // get an object out of the current lua state
    auto sub_and_hello = state[ "subtract_and_hello" ];
    std::tie( difference, greeting ) = sub_and_hello.call<int, std::string>( 1, 3 );
    assert( difference == -2 && greeting == "hello" );
    
    int woof_before = state.get<int>( "woof" );
    // you can also call functions with no parameters or returns
    state.get<sol::function>( "change_woof" )( );
    // and observe changes afterwards
    int woof_after = state.get<int>( "woof" );

    std::cout << "woof before: " << woof_before << std::endl;
    std::cout << "woof after: " << woof_after << std::endl;
}
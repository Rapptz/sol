#include <string>
#include <iostream>
#include <sol.hpp>

int main ( ) {
    sol::state state; 
    
    state.open_file( "example 01 - get and set.lua" );
    // you can get stuff from lua
    int variable1 = state[ "variable1" ];
    std::string variable2 = state.get<std::string>( "variable2" );
    double variable3 = state[ "variable3" ];

    std::cout << "variable1: " << variable1 << std::endl;
    std::cout << "variable2: " << variable2 << std::endl;
    std::cout << "variable3: " << variable3 << std::endl;

    // and you also set them
    state.set( "variable1", 24 );
    state.set[ "variable2" ] = "A string from C++!";
    state.set( "variable3", 100.2 );

    // when you get them again, they'll be different!
    variable1 = state[ "variable1" ];
    variable2 = state.get<std::string>( "variable2" );
    variable3 = state[ "variable3" ];

    std::cout << "variable1: " << variable1 << std::endl;
    std::cout << "variable2: " << variable2 << std::endl;
    std::cout << "variable3: " << variable3 << std::endl;

}

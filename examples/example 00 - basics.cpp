// shows how to load basic data to a struct
#include <string>
#include <iostream>
// include sol header
#include <sol.hpp>

struct config {
    std::string name;
    int width;
    int height;

    void print() {
        std::cout << "Name: "   << name   << '\n'
                  << "Width: "  << width  << '\n'
                  << "Height: " << height << '\n';
    }
};

int main() {
    // start by opening the lua state -- 
    // all you need to do is just create a sol::state
    sol::state lua;
    config screen;
    
    // you can open a file using this function
    // on the state object
    lua.open_file("example 00 - basics.lua");

    screen.name = lua.get<std::string>( "name" );
    screen.width = lua.get<int>( "width" );
    screen.height = lua.get<int>( "height" );
    screen.print( );

    // For shortcut syntax, use square bracket access
    screen.name = lua[ "name" ];
    screen.width = lua[ "width" ];
    screen.height = lua[ "height" ];
    screen.print( );
}


# (An as of now) Unnamed Programming Language

A programming language I have been working on. A relatively small dynamically typed, garbage collected language. 

I originally started putting it together as a scripting language for a game engine, that is still a goal
but I also want a usable language out of it as well. 


Some of the code is a bit messy and needs a rewrite in some areas but it's not too bad. 

> A lot of the initial commits are in my private game engine repo not this one. If that goes public I will link to it here. 

### Libraries

The language has some built-in libraries. Typically these are native functions in C++ bound to the language so they are as fast as possible. 
These include.

- `std:io` - This is for IO including printing and basic user input. 
- `std:maths` - This is your maths library mirroring most of the functions in C/C++ `math.h`. With some extra. 


### Project Structure

`Lang/` - Contains the code for the interpreter. 

`Test/` - Contains code I use in testing and building the language.

`Cmd/` - Contains the command line executable to run the lang from the command line. 

### Dependencies

The only dependency is the C++ stdlib. 

### Things that have helped me along the way

- https://craftinginterpreters.com/
- https://github.com/wren-lang/wren
- https://www.lua.org/
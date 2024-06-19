
# (An as of now) Unnamed Programming Language

A programming language I have been working on. A relatively small dynamically typed, garbage collected language. 

I originally started putting it together as a scripting language for a game engine, that is still a goal
but I also want a usable language out of it as well. 


Some of the code is a bit messy and needs a rewrite in some areas but it's not too bad. 

> A lot of the initial commits are in my private game engine repo not this one. If that goes public I will link to it here. 

## What does the language look like? 

It's a simple language and hello world can be achieved in just a few lines. 

```
import "std:io" as std; 

std.println("Hello World");
```

Here's something slightly cooler: 

```
import "std:io" as std; 
import "std:maths" as std; 

var x = 25; 

var y = std.sqrt(x);

std.println("The square root of $x is $y");

var xc = std.cos(x); 

std.println("cos of $x is $xc");
```
And here's a class: 
```
class HelloClass {

    construct() {
        self.x = 100; 
    }

    func print() {
        var v = self.x; 
        println("Hello $v");
    }

}

var hello = HelloClass();

hello.print(); 
```



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

- C++ stdlib
- nlohmann JSON for the built-in json library. 

### Things that have helped me along the way

- https://craftinginterpreters.com/
- https://github.com/wren-lang/wren
- https://www.lua.org/
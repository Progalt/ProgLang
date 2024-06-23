
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{
	  
	const std::string benchmark =
R"(

import "std:io";
import "std:time" as time;

func fib(n) { 
	if (n < 2) { 
		return n;
	}

	return fib(n - 2) + fib(n - 1); 
}


var total = 0.0;
var itrs = 50;
var fibnth = 24;

for (var i in 0..itrs) {

	println("Iteration $i");

	var start = time.now(); 
	var f = fib(fibnth);
	var elapsed = time.now() - start;

	println("Fib: $f");
	println("Elapsed Time: $elapsed");

	total = total + elapsed;

}

var avg = total / itrs;

println("-------------");
println("Average: $avg");

)";  

	const std::string source =
		R"(

import "std:io";

async func doSomething() {

	println("Hello part 1");
	
	await 250;

	println("Hello part 2");

}

doSomething();

println("Hello from main");

)";



	script::ObjFunction* function = script::CompileScript(source);

	if (function == nullptr)
	{
		printf("Compiler Error\n");
		return 1;
	}

    script::VMCreateInfo createInfo{};

	script::VM vm(createInfo);



	script::InterpretResult result = vm.Interpret(function);

	if (result != script::INTERPRET_ALL_GOOD)
	{
		printf("\n\nInterpret Error\n");
	}

	return 0;
}
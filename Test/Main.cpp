
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{
	  
	const std::string source =
R"(

import "std:io";



var x = [ 2, 4, 5, 6, 67, 234, 24, 424];

println(x);


	var len = x.length();

	for (var i in 0..len) {

		var b = x[i];
		println(b);
	}




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
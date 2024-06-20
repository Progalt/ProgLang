
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{
	 
	const std::string source =
R"(

import "std:io" as std; 

var l = [ 2, 4, 5, 1, 2, 3, 10, 11, 13, 14, 145, 1 ];



	for (var i in l) 
	{
		std.println(i);
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
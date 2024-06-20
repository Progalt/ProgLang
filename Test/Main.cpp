
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{
	  
	const std::string source =
R"(

import "std:io";



var x = (4..15).expand();

println(x);

func fromFunc() {
	var len = x.length();

	for (var i in 0..len) {
		
		var t = i;

		println(t);
		println("--");

		var b = x[i];
		println(b);
	}
}

fromFunc();



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
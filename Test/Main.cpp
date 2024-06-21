
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{
	  
	const std::string source =
R"(

import "std:io";

func fizzbuzz(max) {

	for (var i in 0..max) {
		var mul3 = (i % 3) == 0; 
		var mul5 = (i % 5) == 0;

		var output = "";	

		if (mul3) {
			output = output + "Fizz";
		}

		if (mul5) {
			output = output + "Buzz";
		}
		
		if ((!mul3) and (!mul5)) {
			output = "$i";
		}

		println(output);
		
	}

}

fizzbuzz(50);

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
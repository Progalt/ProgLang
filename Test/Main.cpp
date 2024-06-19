
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{

	const std::string source =
R"(

import "std:io" as std; 
import "std:filesystem" as std; 
import "std:json" as json; 

var filepath = "F:/Dev/ProgLang/TestFiles/test.json"; 

var exists = std.fileExists(filepath);

if (exists) 
{
    var jsonFile = std.readFile(filepath);

    std.println(jsonFile);

    var parsedJson = json.parse(jsonFile);

    std.println(parsedJson);
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


	vm.AddNativeFunction("clock", [&](int argCount, script::Value* args)
		{
			return script::Value((double)std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count());
		}, 1);



	script::InterpretResult result = vm.Interpret(function);

	if (result != script::INTERPRET_ALL_GOOD)
	{
		printf("\n\nInterpret Error\n");
	}

	return 0;
}
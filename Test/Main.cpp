
#include <Lang/Compiler.h>
#include <Lang/VM.h>

#include <chrono>

int main(int argc, char* argv[])
{

	const std::string source =
R"(
import "F:/Dev/ProgLang/TestFiles/test" as test; 


test.helloWorld();


)";  


	script::ObjFunction* function = script::CompileScript(source);

	if (function == nullptr)
	{
		printf("Compiler Error\n");
		return 1;
	}

    script::VMCreateInfo createInfo{};

	script::VM vm(createInfo);

	vm.AddNativeFunction("Dictionary", [&](int argCount, script::Value* args)
		{
			return script::Value(script::AllocateDictionary());
		}, 0);

	vm.AddNativeFunction("List", [&](int argCount, script::Value* args)
		{
			return script::Value(script::AllocateArray({}));
		}, 0);

	vm.AddNativeFunction("sqrt", [&](int argCount, script::Value* args)
		{
			return script::Value(sqrt(args[0].ToNumber()));
		}, 1);

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

#include <cstdio>
#include <string>
#include <fstream>
#include <sstream>

#include <Lang/Compiler.h>
#include <Lang/VM.h>

int main(int argc, char* argv[])
{
    if (argc == 1)
    {
        printf("Please provide some arguments.\n");
        return 0;
    }

    if (argc >= 2)
    {
    
        // The first arg is the exe name
        // The second arg should always be the file to execute 

        std::string filepath(argv[1]);

        std::ifstream file(filepath);

        if (!file.is_open())
        {
            printf("Failed to open file: %s\n", filepath.c_str());
            return 1;
        }

        std::stringstream stream; 
        stream << file.rdbuf();

        std::string src = stream.str();

        if (src.empty())
        {
            printf("File is empty\n");
            return 1;
        }

        script::ObjFunction* compiledFunction = script::CompileScript(src);

        if (compiledFunction == nullptr)
        {
            printf("Compiler Error\n");
            return 1;
        }

        script::VMCreateInfo createInfo{};

        script::VM vm(createInfo);
        
        script::InterpretResult result = vm.Interpret(compiledFunction);

        if (result == script::INTERPRET_RUNTIME_ERROR)
        {
            printf("\n\nInterpret runtime error.\n");

            return 1;
        }
    }

    return 0;
}
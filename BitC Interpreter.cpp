// BitC Interpreter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer.h"
#include "parser.h"
#include "interpreter.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: BitC <file.bc>" << std::endl;
        return 1;
    }

    // read source file
    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: could not open file '" << argv[1] << "'" << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string src = buffer.str();

    try {
        Lexer lexer(src);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto tree = parser.parse();

        Interpreter interpreter;
        interpreter.runProgram(tree);
    }
    catch (std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file

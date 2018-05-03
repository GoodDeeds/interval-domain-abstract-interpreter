#include "arg_parser.h"

#include <algorithm>
#include <iostream>

using namespace intervalai;

ArgParser::ArgParser(int argc, char **argv) {
    for (int i = 0; i < argc; i++) {
        args.push_back(std::string(argv[i]));
    }
}

bool ArgParser::argExists(std::string check_arg) {
    if (std::find(args.begin(), args.end(), check_arg) != args.end()) {
        return true;
    }
    return false;
}

std::string ArgParser::getArgValue(std::string get_arg) {
    if (std::find(args.begin(), args.end(), get_arg) == args.end()) {
        return "";
    }
    auto it = std::find(args.begin(), args.end(), get_arg);
    it++;
    if (it != args.end()) {
        return *it;
    }
    return "";
}
#include <vector>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <tuple>
#include <sstream>
#include <variant>

#ifndef CLI_H_Swirl
#define CLI_H_Swirl

namespace cli {
    struct Argument {
        std::tuple<std::string, std::string> flags;
        std::string desc;

        bool value_required;
        std::string value;
    };	

	const std::string USAGE = R"(The Swirl compiler
Usage: Swirl <input-file> [flags]

Flags:
)";

    std::string generate_help(const std::vector<Argument> &flags);

	std::vector<Argument> parse(int argc, const char** argv, const std::vector<Argument>& flags);

    /**
    * This function returns the value of the flag requested from the provided args vector.
    * If the flag is not supplied, it returns false.
    * If the flag is supplied, it returns the value of the flag.
    * If the flag is not required to have a value, it returns true.
    *
    * @param flag Requested flag
    * @param args Vector of arguments
    * @return The value of the flag
    */
    std::variant<std::string, bool>
        get_flag(std::string_view flag, const std::vector<Argument>& args);
}

#endif

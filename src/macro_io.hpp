#pragma once
#include <string>

namespace jbot {
    std::string sanitizeName(std::string const& name);
    bool saveMacro(std::string const& name);
    bool loadMacro(std::string const& name);
}

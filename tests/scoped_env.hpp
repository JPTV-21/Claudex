#pragma once

#include "platform.hpp"

#include <optional>
#include <string>
#include <utility>

namespace testfw {

// Sets an environment variable for the lifetime of the object. Restoring an
// unset variable writes an empty value, the closest portable "unset".
class ScopedEnv {
public:
    ScopedEnv(std::string name, const std::string& value)
        : name_(std::move(name)), previous_(claudex::getEnvUtf8(name_)) {
        std::string detail;
        claudex::setEnvUtf8(name_, value, detail);
    }
    ScopedEnv(const ScopedEnv&) = delete;
    ScopedEnv& operator=(const ScopedEnv&) = delete;
    ~ScopedEnv() {
        std::string detail;
        claudex::setEnvUtf8(name_, previous_ ? *previous_ : std::string(),
                            detail);
    }

private:
    std::string name_;
    std::optional<std::string> previous_;
};

}  // namespace testfw

#include "config_bootstrap.hpp"

#include "config_types.hpp"
#include "text_util.hpp"

#include <fstream>
#include <iterator>

namespace claudex {
namespace {

bool readFileText(const fs::path& path, std::string& text) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    text.assign(std::istreambuf_iterator<char>(input),
                std::istreambuf_iterator<char>());
    return !input.bad();
}

void stripUtf8Bom(std::string& text) {
    if (text.size() >= 3 &&
        static_cast<unsigned char>(text[0]) == 0xEF &&
        static_cast<unsigned char>(text[1]) == 0xBB &&
        static_cast<unsigned char>(text[2]) == 0xBF) {
        text.erase(0, 3);
    }
}

// Expands placeholders and environment variables, then resolves a relative
// path against |base|. Mirrors how "theme" selects its file.
bool resolveLocation(const std::string& raw, const fs::path& base,
                     const PathResolver& resolver, fs::path& out) {
    const std::string value = trim(raw);
    if (value.empty()) {
        return false;
    }
    PathResolution resolution = resolver.resolveWithBase(
        value, base, PathKind::File, MultiMatchPolicy::FirstSorted);
    if (resolution.resolved) {
        out = resolution.path;
        return true;
    }
    fs::path literal = pathFromUtf8(resolution.expanded);
    if (literal.empty()) {
        return false;
    }
    if (literal.is_relative()) {
        literal = base / literal;
    }
    out = literal.lexically_normal();
    return !out.empty();
}

}  // namespace

ConfigBootstrapResult bootstrapConfigPath(
    const fs::path& rootDir, const fs::path& initialConfigFile,
    const PathResolver& resolver) {
    ConfigBootstrapResult result;
    result.configFile = initialConfigFile;

    std::string text;
    if (!readFileText(initialConfigFile, text)) {
        return result;
    }
    stripUtf8Bom(text);

    json document;
    try {
        document = json::parse(text);
    } catch (const std::exception&) {
        // Let ConfigManager report the parse error.
        return result;
    }
    if (!document.is_object() || !document.contains("configFile") ||
        !document["configFile"].is_string()) {
        return result;
    }

    fs::path target;
    if (!resolveLocation(document["configFile"].get<std::string>(), rootDir,
                         resolver, target) ||
        pathsEqual(target, initialConfigFile)) {
        return result;
    }

    result.configFile = std::move(target);
    result.redirected = true;

    std::error_code ec;
    result.targetMissing = !fs::exists(result.configFile, ec) || ec;
    if (result.targetMissing) {
        result.messageKey = "configRedirectTargetMissing";
        result.messageConfig = pathToUtf8(result.configFile);
    }
    return result;
}

}  // namespace claudex

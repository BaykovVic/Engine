#pragma once

#include <algorithm>
#include <compare>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>

namespace sky::package {

/// A parsed semantic version. Missing components parse as zero, so "1.2"
/// is 1.2.0 and "2" is 2.0.0.
struct Version {
    int major = 0;
    int minor = 0;
    int patch = 0;

    auto operator<=>(const Version&) const = default;

    [[nodiscard]] std::string str() const {
        char buffer[48];
        std::snprintf(buffer, sizeof(buffer), "%d.%d.%d", major, minor, patch);
        return buffer;
    }
};

/// Parses "X", "X.Y" or "X.Y.Z" (non-negative integers). Anything else —
/// including empty input — is nullopt.
inline std::optional<Version> parseVersion(const std::string& text) {
    Version version;
    int consumed = 0;
    const int fields = std::sscanf(text.c_str(), "%d.%d.%d%n", &version.major,
                                   &version.minor, &version.patch, &consumed);
    if (fields < 1 || version.major < 0 || version.minor < 0 || version.patch < 0) {
        return std::nullopt;
    }
    // Reject trailing garbage ("1.2.3-beta" is not supported yet).
    std::size_t expected = 0;
    for (std::size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if ((c >= '0' && c <= '9') || c == '.') {
            expected = i + 1;
        } else {
            break;
        }
    }
    return expected == text.size() ? std::optional(version) : std::nullopt;
}

/// True when `version` satisfies a requirement:
///   "*" / ""      any version
///   ">=X[.Y[.Z]]" at least X.Y.Z
///   "^X[.Y[.Z]]"  at least X.Y.Z and the same major
///   "X.Y.Z"       exactly that version
///   "X.Y"         that major.minor, any patch
///   "X"           that major, any minor/patch
/// Malformed requirements match nothing (fail closed).
inline bool satisfies(const Version& version, const std::string& requirement) {
    if (requirement.empty() || requirement == "*") {
        return true;
    }
    if (requirement.rfind(">=", 0) == 0) {
        const auto minimum = parseVersion(requirement.substr(2));
        return minimum && version >= *minimum;
    }
    if (requirement[0] == '^') {
        const auto minimum = parseVersion(requirement.substr(1));
        return minimum && version >= *minimum && version.major == minimum->major;
    }
    const auto exact = parseVersion(requirement);
    if (!exact) {
        return false;
    }
    // Exactness follows the written precision: "1.2" pins major.minor only.
    const auto dots = std::count(requirement.begin(), requirement.end(), '.');
    if (dots >= 2) {
        return version == *exact;
    }
    if (dots == 1) {
        return version.major == exact->major && version.minor == exact->minor;
    }
    return version.major == exact->major;
}

} // namespace sky::package

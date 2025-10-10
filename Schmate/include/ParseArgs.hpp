#pragma once
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cstdlib>

// Splits command payload into numeric and text components
struct ParsedArgs {
    std::vector<std::string> args; // numeric or tokens before text
    std::string query;             // remainder of command (query text)
};

// Generic parser for lines like:
// "5 what is AI"   -> args={"5"}, query="what is AI"
// "what is AI"     -> args={},   query="what is AI"
// "0.9 5 20 0.1 what is AI" -> args={"0.9","5","20","0.1"}, query="what is AI"
inline ParsedArgs parseCommandArgs(const std::string &payload, size_t maxArgs = 4) {
    ParsedArgs parsed;
    std::istringstream iss(payload);
    std::string tok;

    // read up to maxArgs numeric-like args
    while (iss >> tok) {
        // stop when token contains alphabetic characters (start of query)
        if (std::any_of(tok.begin(), tok.end(), ::isalpha) && !std::all_of(tok.begin(), tok.end(), ::isdigit)) {
            parsed.query = tok;
            break;
        }
        parsed.args.push_back(tok);
        if (parsed.args.size() >= maxArgs)
            break;
    }

    // read the rest of the line as query tail
    std::string tail;
    std::getline(iss, tail);
    if (!parsed.query.empty())
        parsed.query += tail;
    else
        parsed.query = tail;

    // Trim leading space
    if (!parsed.query.empty() && parsed.query[0] == ' ')
        parsed.query.erase(0, 1);

    return parsed;
}

// Convenience float/int parsers
inline bool parseFloat(const std::string &s, float &out) {
    char *end;
    float v = std::strtof(s.c_str(), &end);
    if (end != s.c_str() && *end == '\0') { out = v; return true; }
    return false;
}
inline bool parseInt(const std::string &s, size_t &out) {
    char *end;
    long v = std::strtol(s.c_str(), &end, 10);
    if (end != s.c_str() && *end == '\0') { out = v; return true; }
    return false;
}


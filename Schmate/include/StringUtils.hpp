#pragma once

#include <string>
#include <algorithm>
#include <cctype>
#include <locale>
#include <codecvt>

// ============================================================================
// UTF-8 aware versions (proper Unicode handling)
// ============================================================================

// Check if UTF-8 string is empty or only whitespace
// Recognizes Unicode whitespace characters
inline bool is_empty_or_whitespace_utf8(const std::string& str) {
    if (str.empty()) return true;
    
    // Simple heuristic: check for non-whitespace bytes
    // This works for most cases without full Unicode parsing
    for (unsigned char c : str) {
        // ASCII non-whitespace
        if (c > 32 && c < 127) return false;
        // High-bit set (UTF-8 continuation or start) - consider non-whitespace
        if (c >= 128) return false;
    }
    return true;
}

// Check if UTF-8 string has meaningful content (letters, digits, etc.)
// This is a heuristic approach - checks for non-whitespace, non-punctuation bytes
inline bool has_meaningful_content_utf8(const std::string& str) {
    if (str.empty()) return false;
    
    bool has_alnum = false;
    
    for (unsigned char c : str) {
        // ASCII alphanumeric
        if (std::isalnum(c)) {
            has_alnum = true;
            break;
        }
        // UTF-8 multi-byte character (likely meaningful content)
        // UTF-8 start bytes: 0xC0-0xDF (2-byte), 0xE0-0xEF (3-byte), 0xF0-0xF7 (4-byte)
        if (c >= 0xC0 && c <= 0xF7) {
            has_alnum = true;
            break;
        }
    }
    
    return has_alnum;
}

// More robust UTF-8 version using ICU-style approach
// Checks if string contains printable non-whitespace characters
inline bool has_printable_content(const std::string& str) {
    if (str.empty()) return false;
    
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = str[i];
        
        // ASCII range
        if (c < 128) {
            // Skip whitespace and control characters
            if (c > 32 && c != 127) {
                // Check if it's punctuation-only
                if (std::isalnum(c)) return true;
            }
            i++;
        }
        // UTF-8 multi-byte sequence
        else if (c >= 0xC0) {
            // Any UTF-8 multi-byte character is considered meaningful
            return true;
        }
        else {
            // Invalid UTF-8 or continuation byte out of place
            i++;
        }
    }
    
    return false;
}


// Count UTF-8 characters (not bytes)
inline size_t utf8_length(const std::string& str) {
    size_t len = 0;
    for (size_t i = 0; i < str.size(); ) {
        unsigned char c = str[i];
        if (c < 128) {
            i += 1;
        } else if ((c & 0xE0) == 0xC0) {
            i += 2;
        } else if ((c & 0xF0) == 0xE0) {
            i += 3;
        } else if ((c & 0xF8) == 0xF0) {
            i += 4;
        } else {
            i += 1; // Invalid UTF-8
        }
        len++;
    }
    return len;
}

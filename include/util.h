#ifndef UTIL_H
#define UTIL_H

// Small, dependency-free text helpers shared by every module.
namespace util {

// Trims leading/trailing whitespace in place and returns the new length.
int trimInPlace(char* text);

// Uppercases an ASCII string in place (used for command tokens).
void toUpperInPlace(char* text);

// Splits text on runs of whitespace in place. Returns the token count and
// stores at most maxTokens pointers in tokens.
int splitTokens(char* text, char* tokens[], int maxTokens);

// Strict decimal parse: the whole string must be an optional sign plus digits.
bool parseIntStrict(const char* text, int* value);

// Bounded copy that always NUL-terminates (cap includes the terminator).
void copyText(char* dest, int cap, const char* src);

bool startsWith(const char* text, const char* prefix);

// Case-insensitive comparison for ASCII identifiers such as room/item ids.
bool equalsIgnoreCase(const char* a, const char* b);

}  // namespace util

#endif  // UTIL_H

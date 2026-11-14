#ifndef UTIL_H
#define UTIL_H

namespace util {

// Removes spaces and newlines from the start and end of a string
int trimInPlace(char* text);

// Converts all lowercase letters in a string to uppercase
void toUpperInPlace(char* text);

// Breaks a command line into separate words separated by spaces
int splitTokens(char* text, char* tokens[], int maxTokens);

// Converts a text string into an integer if all characters are valid digits
bool parseIntStrict(const char* text, int* value);

// Safely copies src into dest without exceeding cap
void copyText(char* dest, int cap, const char* src);

// Returns true if text begins with prefix
bool startsWith(const char* text, const char* prefix);

// Compares two strings ignoring uppercase/lowercase differences
bool equalsIgnoreCase(const char* a, const char* b);

// Appends a string onto the end of dest safely
void appendStr(char* dest, int cap, const char* src);

// Appends a single character onto the end of dest safely
void appendChar(char* dest, int cap, char ch);

// Converts an integer to text and appends it onto the end of dest
void appendInt(char* dest, int cap, int value);

}  // namespace util

#endif
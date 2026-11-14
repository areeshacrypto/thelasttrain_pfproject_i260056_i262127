#include "util.h"

#include <cctype>
#include <cstring>

namespace {

bool isSpaceChar(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

char lowerAscii(char c) {
    if (c >= 'A' && c <= 'Z') {
        return static_cast<char>(c - 'A' + 'a');
    }
    return c;
}

}  // namespace

int util::trimInPlace(char* text) {
    if (text == nullptr) {
        return 0;
    }
    const int len = static_cast<int>(std::strlen(text));
    int start = 0;
    while (start < len && isSpaceChar(text[start])) {
        ++start;
    }
    int end = len;
    while (end > start && isSpaceChar(text[end - 1])) {
        --end;
    }
    int out = 0;
    for (int i = start; i < end; ++i) {
        text[out++] = text[i];
    }
    text[out] = '\0';
    return out;
}

void util::toUpperInPlace(char* text) {
    if (text == nullptr) {
        return;
    }
    for (int i = 0; text[i] != '\0'; ++i) {
        text[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
    }
}

int util::splitTokens(char* text, char* tokens[], int maxTokens) {
    if (text == nullptr || tokens == nullptr || maxTokens <= 0) {
        return 0;
    }
    int count = 0;
    int i = 0;
    while (text[i] != '\0') {
        while (text[i] != '\0' && isSpaceChar(text[i])) {
            ++i;
        }
        if (text[i] == '\0' || count == maxTokens) {
            break;
        }
        tokens[count++] = &text[i];
        while (text[i] != '\0' && !isSpaceChar(text[i])) {
            ++i;
        }
        if (text[i] != '\0') {
            text[i] = '\0';
            ++i;
        }
    }
    return count;
}

bool util::parseIntStrict(const char* text, int* value) {
    if (text == nullptr || value == nullptr || text[0] == '\0') {
        return false;
    }
    int i = 0;
    int sign = 1;
    if (text[i] == '-' || text[i] == '+') {
        if (text[i] == '-') {
            sign = -1;
        }
        ++i;
    }
    if (text[i] == '\0') {
        return false;
    }
    long result = 0;
    for (; text[i] != '\0'; ++i) {
        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        result = result * 10 + (text[i] - '0');
        if (result > 1000000L) {
            return false;
        }
    }
    *value = static_cast<int>(result * sign);
    return true;
}

void util::copyText(char* dest, int cap, const char* src) {
    if (dest == nullptr || cap <= 0) {
        return;
    }
    if (src == nullptr) {
        dest[0] = '\0';
        return;
    }
    int i = 0;
    while (i < cap - 1 && src[i] != '\0') {
        dest[i] = src[i];
        ++i;
    }
    dest[i] = '\0';
}

bool util::startsWith(const char* text, const char* prefix) {
    if (text == nullptr || prefix == nullptr) {
        return false;
    }
    return std::strncmp(text, prefix, std::strlen(prefix)) == 0;
}

bool util::equalsIgnoreCase(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    while (*a != '\0' && *b != '\0') {
        if (lowerAscii(*a) != lowerAscii(*b)) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

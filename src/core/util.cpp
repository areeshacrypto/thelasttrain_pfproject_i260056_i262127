#include "util.h"
#include <cctype>
#include <cstring>

using namespace std;

static bool isSpaceChar(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static char toLowerChar(char c) {
    if (c >= 'A' && c <= 'Z') {
        return (char)(c - 'A' + 'a');
    }
    return c;
}

int util::trimInPlace(char* text) {
    if (text == nullptr) return 0;

    int len = (int)strlen(text);
    int start = 0;
    while (start < len && isSpaceChar(text[start])) {
        start++;
    }

    int end = len;
    while (end > start && isSpaceChar(text[end - 1])) {
        end--;
    }

    int out = 0;
    for (int i = start; i < end; i++) {
        text[out++] = text[i];
    }
    text[out] = '\0';
    return out;
}

void util::toUpperInPlace(char* text) {
    if (text == nullptr) return;
    for (int i = 0; text[i] != '\0'; i++) {
        text[i] = (char)toupper((unsigned char)text[i]);
    }
}

int util::splitTokens(char* text, char* tokens[], int maxTokens) {
    if (text == nullptr || tokens == nullptr || maxTokens <= 0) return 0;

    int count = 0;
    int i = 0;
    while (text[i] != '\0') {
        while (text[i] != '\0' && isSpaceChar(text[i])) {
            i++;
        }
        if (text[i] == '\0' || count == maxTokens) {
            break;
        }
        tokens[count++] = &text[i];
        while (text[i] != '\0' && !isSpaceChar(text[i])) {
            i++;
        }
        if (text[i] != '\0') {
            text[i] = '\0';
            i++;
        }
    }
    return count;
}

bool util::parseIntStrict(const char* text, int* value) {
    if (text == nullptr || value == nullptr || text[0] == '\0') return false;

    int i = 0;
    int sign = 1;
    if (text[i] == '-' || text[i] == '+') {
        if (text[i] == '-') sign = -1;
        i++;
    }
    if (text[i] == '\0') return false;

    long result = 0;
    for (; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') return false;
        result = result * 10 + (text[i] - '0');
        if (result > 1000000L) return false;
    }

    *value = (int)(result * sign);
    return true;
}

void util::copyText(char* dest, int cap, const char* src) {
    if (dest == nullptr || cap <= 0) return;
    if (src == nullptr) {
        dest[0] = '\0';
        return;
    }
    int i = 0;
    while (i < cap - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

bool util::startsWith(const char* text, const char* prefix) {
    if (text == nullptr || prefix == nullptr) return false;
    return strncmp(text, prefix, strlen(prefix)) == 0;
}

bool util::equalsIgnoreCase(const char* a, const char* b) {
    if (a == nullptr || b == nullptr) return false;
    while (*a != '\0' && *b != '\0') {
        if (toLowerChar(*a) != toLowerChar(*b)) {
            return false;
        }
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

void util::appendStr(char* dest, int cap, const char* src) {
    if (dest == nullptr || src == nullptr || cap <= 0) return;
    int len = (int)strlen(dest);
    int i = 0;
    while (len < cap - 1 && src[i] != '\0') {
        dest[len++] = src[i++];
    }
    dest[len] = '\0';
}

void util::appendChar(char* dest, int cap, char ch) {
    if (dest == nullptr || cap <= 0) return;
    int len = (int)strlen(dest);
    if (len < cap - 1) {
        dest[len++] = ch;
        dest[len] = '\0';
    }
}

void util::appendInt(char* dest, int cap, int value) {
    if (dest == nullptr || cap <= 0) return;
    if (value < 0) {
        appendChar(dest, cap, '-');
        value = -value;
    }
    char digits[16];
    int count = 0;
    if (value == 0) {
        digits[count++] = '0';
    } else {
        while (value > 0 && count < 15) {
            digits[count++] = (char)('0' + (value % 10));
            value /= 10;
        }
    }
    // Reverse digits into dest
    for (int i = count - 1; i >= 0; i--) {
        appendChar(dest, cap, digits[i]);
    }
}
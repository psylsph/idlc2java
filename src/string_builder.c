#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdarg.h>

typedef struct string_builder {
    char *buffer;
    size_t length;
    size_t capacity;
} string_builder_t;

string_builder_t *sb_create(void) {
    string_builder_t *sb = calloc(1, sizeof(string_builder_t));
    if (sb) {
        sb->capacity = 1024;
        sb->buffer = malloc(sb->capacity);
        sb->buffer[0] = '\0';
    }
    return sb;
}

void sb_destroy(string_builder_t *sb) {
    if (sb) {
        free(sb->buffer);
        free(sb);
    }
}

int sb_append(string_builder_t *sb, const char *str) {
    size_t len = strlen(str);
    if (sb->length + len + 1 > sb->capacity) {
        size_t new_capacity = sb->capacity * 2;
        while (sb->length + len + 1 > new_capacity) {
            new_capacity *= 2;
        }
        char *new_buffer = realloc(sb->buffer, new_capacity);
        if (!new_buffer) return -1;
        sb->buffer = new_buffer;
        sb->capacity = new_capacity;
    }
    memcpy(sb->buffer + sb->length, str, len + 1);
    sb->length += len;
    return 0;
}

int sb_appendf(string_builder_t *sb, const char *format, ...) {
    va_list args;
    va_start(args, format);
    
    char tmp[1024];
    int len = vsnprintf(tmp, sizeof(tmp), format, args);
    va_end(args);
    
    if (len < 0) return -1;
    if ((size_t)len >= sizeof(tmp)) {
        char *buf = malloc(len + 1);
        va_start(args, format);
        vsnprintf(buf, len + 1, format, args);
        va_end(args);
        int ret = sb_append(sb, buf);
        free(buf);
        return ret;
    }
    
    return sb_append(sb, tmp);
}

const char *sb_string(const string_builder_t *sb) {
    return sb ? sb->buffer : "";
}

size_t sb_length(const string_builder_t *sb) {
    return sb ? sb->length : 0;
}

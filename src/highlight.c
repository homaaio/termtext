#include <string.h>
#include <ctype.h>
#include "highlight.h"
#include "platform.h"

Lang highlight_detect_lang(const char *filename) {
    const char *dot = strrchr(filename, '.');
    if (dot == NULL || dot[1] == 0) return LANG_NONE;
    dot++;

    if (strcmp(dot, "c") == 0 || strcmp(dot, "h") == 0 ||
        strcmp(dot, "cpp") == 0 || strcmp(dot, "hpp") == 0 ||
        strcmp(dot, "cc") == 0 || strcmp(dot, "cxx") == 0)
        return LANG_C;
    if (strcmp(dot, "rs") == 0)
        return LANG_RUST;
    if (strcmp(dot, "py") == 0)
        return LANG_PYTHON;
    if (strcmp(dot, "asm") == 0 || strcmp(dot, "s") == 0 || strcmp(dot, "S") == 0)
        return LANG_ASM;
    return LANG_NONE;
}

/* Списки минимальны и не претендуют на полноту — цель в том, чтобы код
 * не выглядел "скучным" одноцветным текстом, а не в точном соответствии
 * грамматике языка. */

static const char *c_keywords[] = {
    "if", "else", "while", "for", "do", "switch", "case", "break",
    "continue", "return", "goto", "default", "sizeof", "struct", "union",
    "enum", "typedef", "static", "const", "volatile", "extern", "inline",
    "class", "public", "private", "protected", "namespace", "template",
    "new", "delete", "try", "catch", "throw", "using", "virtual", "this",
    "true", "false", "nullptr", NULL
};
static const char *c_types[] = {
    "int", "char", "float", "double", "long", "short", "unsigned",
    "signed", "void", "bool", "size_t",
    "int8_t", "int16_t", "int32_t", "int64_t",
    "uint8_t", "uint16_t", "uint32_t", "uint64_t", NULL
};

static const char *rust_keywords[] = {
    "fn", "let", "mut", "if", "else", "while", "for", "loop", "match",
    "return", "break", "continue", "struct", "enum", "impl", "trait",
    "pub", "use", "mod", "crate", "self", "Self", "super", "as", "in",
    "ref", "move", "dyn", "static", "const", "unsafe", "where", "async",
    "await", "true", "false", NULL
};
static const char *rust_types[] = {
    "i8", "i16", "i32", "i64", "i128", "isize",
    "u8", "u16", "u32", "u64", "u128", "usize",
    "f32", "f64", "bool", "char", "str", "String", "Vec", "Option", "Result", NULL
};

static const char *py_keywords[] = {
    "def", "class", "if", "elif", "else", "while", "for", "in", "return",
    "break", "continue", "pass", "import", "from", "as", "with", "try",
    "except", "finally", "raise", "lambda", "yield", "global", "nonlocal",
    "del", "assert", "is", "not", "and", "or", "True", "False", "None",
    "async", "await", NULL
};
static const char *py_types[] = {
    "int", "float", "str", "bool", "list", "dict", "tuple", "set", "bytes", NULL
};

static const char *asm_keywords[] = {
    "mov", "add", "sub", "mul", "div", "jmp", "je", "jne", "jz", "jnz",
    "jg", "jl", "cmp", "push", "pop", "call", "ret", "lea", "nop", "int",
    "xor", "and", "or", "not", "shl", "shr", "inc", "dec", "loop",
    "section", "global", "extern", "db", "dw", "dd", "dq", NULL
};
static const char *asm_types[] = {
    "eax", "ebx", "ecx", "edx", "esi", "edi", "esp", "ebp",
    "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rsp", "rbp",
    "al", "bl", "cl", "dl", NULL
};

static const char **keywords_for(Lang lang) {
    switch (lang) {
        case LANG_C:      return c_keywords;
        case LANG_RUST:   return rust_keywords;
        case LANG_PYTHON: return py_keywords;
        case LANG_ASM:    return asm_keywords;
        default:          return NULL;
    }
}

static const char **types_for(Lang lang) {
    switch (lang) {
        case LANG_C:      return c_types;
        case LANG_RUST:   return rust_types;
        case LANG_PYTHON: return py_types;
        case LANG_ASM:    return asm_types;
        default:          return NULL;
    }
}

static int is_word_char(char c) {
    return isalnum((unsigned char)c) || c == '_';
}

static int word_in_list(const char *word, int len, const char **list) {
    if (list == NULL) return 0;
    for (int i = 0; list[i] != NULL; i++) {
        if ((int)strlen(list[i]) == len && strncmp(list[i], word, len) == 0)
            return 1;
    }
    return 0;
}

void highlight_line(Lang lang, const char *line, int *out_colors, int len) {
    for (int i = 0; i < len; i++) out_colors[i] = PLAT_COLOR_DEFAULT;
    if (lang == LANG_NONE) return;

    const char **kw = keywords_for(lang);
    const char **ty = types_for(lang);

    /* Директивы препроцессора — только C/C++, вся строка одним цветом */
    if (lang == LANG_C) {
        int i = 0;
        while (i < len && (line[i] == ' ' || line[i] == '\t')) i++;
        if (i < len && line[i] == '#') {
            for (int k = i; k < len; k++) out_colors[k] = PLAT_COLOR_PREPROC;
            return;
        }
    }

    /* Маркер однострочного комментария для языка (второй символ 0, если
     * маркер однобайтовый, например '#' у Python или ';' у asm) */
    char cm1 = 0, cm2 = 0;
    if (lang == LANG_C || lang == LANG_RUST) { cm1 = '/'; cm2 = '/'; }
    else if (lang == LANG_PYTHON)            { cm1 = '#'; }
    else if (lang == LANG_ASM)               { cm1 = ';'; }

    int i = 0;
    while (i < len) {
        char c = line[i];

        if (cm1 && c == cm1 && (cm2 == 0 || (i + 1 < len && line[i + 1] == cm2))) {
            for (int k = i; k < len; k++) out_colors[k] = PLAT_COLOR_COMMENT;
            break;
        }

        if (c == '"' || c == '\'') {
            char quote = c;
            int start = i;
            i++;
            while (i < len && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < len) i++;
                i++;
            }
            if (i < len) i++; /* закрывающая кавычка */
            for (int k = start; k < i; k++) out_colors[k] = PLAT_COLOR_STRING;
            continue;
        }

        if (isdigit((unsigned char)c)) {
            int start = i;
            while (i < len && (isalnum((unsigned char)line[i]) || line[i] == '.')) i++;
            for (int k = start; k < i; k++) out_colors[k] = PLAT_COLOR_NUMBER;
            continue;
        }

        if (is_word_char(c) && !isdigit((unsigned char)c)) {
            int start = i;
            while (i < len && is_word_char(line[i])) i++;
            int wlen = i - start;
            if (word_in_list(line + start, wlen, kw)) {
                for (int k = start; k < i; k++) out_colors[k] = PLAT_COLOR_KEYWORD;
            } else if (word_in_list(line + start, wlen, ty)) {
                for (int k = start; k < i; k++) out_colors[k] = PLAT_COLOR_TYPE;
            }
            continue;
        }

        i++;
    }
}

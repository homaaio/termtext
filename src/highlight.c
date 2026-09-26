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

/* The lists are minimal and don't aim to be complete — the goal is for
 * code to not look like "boring" single-color text, not to exactly
 * match each language's grammar. */

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

/* Length of a number literal starting at line[i], or 0 if there isn't
 * one there. Handles plain integers/floats, a 0x/0X hex prefix,
 * '_' digit separators (Rust/C++14-style), a leading-dot decimal
 * (".5"), an exponent (1e10, 1.5E-3), and a run of trailing
 * alphanumeric type-suffix characters (42u, 3.0f, 10L) the same
 * permissive way the rest of this tokenizer treats identifiers. */
static int number_token_len(const char *line, int len, int i) {
    int start = i;

    if (line[i] == '0' && i + 1 < len && (line[i + 1] == 'x' || line[i + 1] == 'X')) {
        i += 2;
        while (i < len && (isxdigit((unsigned char)line[i]) || line[i] == '_')) i++;
        return i - start;
    }

    int has_digits = 0;
    while (i < len && (isdigit((unsigned char)line[i]) || line[i] == '_')) { i++; has_digits = 1; }

    if (i < len && line[i] == '.' && i + 1 < len && isdigit((unsigned char)line[i + 1])) {
        i++;
        while (i < len && (isdigit((unsigned char)line[i]) || line[i] == '_')) i++;
        has_digits = 1;
    }

    if (!has_digits) return 0;

    if (i < len && (line[i] == 'e' || line[i] == 'E')) {
        int j = i + 1;
        if (j < len && (line[j] == '+' || line[j] == '-')) j++;
        if (j < len && isdigit((unsigned char)line[j])) {
            i = j;
            while (i < len && isdigit((unsigned char)line[i])) i++;
        }
    }

    /* trailing suffix letters, e.g. u/U/l/L/f/F */
    while (i < len && isalpha((unsigned char)line[i])) i++;

    return i - start;
}

/* Marks a run starting at line[i] (which must be a backslash followed
 * by at least one more character) as an escape sequence and returns
 * its length: 2 for a plain escape like \n or \", more for \xHH (up to
 * 2 hex digits) or an octal escape \0..\777 (up to 3 octal digits). */
static int escape_token_len(const char *line, int len, int i) {
    int start = i;
    i++; /* backslash */
    char ec = line[i];
    i++; /* the escaped character itself */

    if (ec == 'x') {
        int n = 0;
        while (i < len && n < 2 && isxdigit((unsigned char)line[i])) { i++; n++; }
    } else if (ec >= '0' && ec <= '7') {
        int n = 0;
        while (i < len && n < 2 && line[i] >= '0' && line[i] <= '7') { i++; n++; }
    }

    return i - start;
}

void highlight_line(Lang lang, const char *line, int *out_colors, int len) {
    for (int i = 0; i < len; i++) out_colors[i] = PLAT_COLOR_DEFAULT;
    if (lang == LANG_NONE) return;

    const char **kw = keywords_for(lang);
    const char **ty = types_for(lang);

    /* Line-comment marker for the language (second char is 0 if the
     * marker is a single byte, e.g. '#' for Python or ';' for asm) */
    char cm1 = 0, cm2 = 0;
    if (lang == LANG_C || lang == LANG_RUST) { cm1 = '/'; cm2 = '/'; }
    else if (lang == LANG_PYTHON)            { cm1 = '#'; }
    else if (lang == LANG_ASM)               { cm1 = ';'; }

    int i = 0;

    /* Preprocessor directives — C/C++ only. Only the '#' and the
     * directive word itself (include/define/ifndef/...) get
     * PLAT_COLOR_PREPROC; the rest of the line falls through to the
     * normal tokenizer below, so a macro's value, a comment, or a
     * quoted #include path still get their own colors instead of the
     * whole line turning into one flat color (which is especially
     * dull on header files, mostly made up of directives). */
    if (lang == LANG_C) {
        while (i < len && (line[i] == ' ' || line[i] == '\t')) i++;
        if (i < len && line[i] == '#') {
            int hash = i;
            i++;
            while (i < len && (line[i] == ' ' || line[i] == '\t')) i++;
            int dstart = i;
            while (i < len && is_word_char(line[i])) i++;
            for (int k = hash; k < i; k++) out_colors[k] = PLAT_COLOR_PREPROC;

            /* #include <path> — angle-bracket form isn't a normal
             * string literal, so color it as one by hand. The quoted
             * form (#include "path") is already handled by the normal
             * string-scanning code below once we fall through. */
            if (i - dstart == 7 && strncmp(line + dstart, "include", 7) == 0) {
                int k = i;
                while (k < len && (line[k] == ' ' || line[k] == '\t')) k++;
                if (k < len && line[k] == '<') {
                    int start = k;
                    k++;
                    while (k < len && line[k] != '>') k++;
                    if (k < len) k++; /* include the closing '>' */
                    for (int m = start; m < k; m++) out_colors[m] = PLAT_COLOR_STRING;
                    i = k;
                }
            }
        } else {
            i = 0; /* not a directive line after all — rewind */
        }
    }

    while (i < len) {
        char c = line[i];

        if (cm1 && c == cm1 && (cm2 == 0 || (i + 1 < len && line[i + 1] == cm2))) {
            for (int k = i; k < len; k++) out_colors[k] = PLAT_COLOR_COMMENT;
            break;
        }

        if (c == '"' || c == '\'') {
            char quote = c;
            i++;
            out_colors[i - 1] = PLAT_COLOR_STRING; /* opening quote */
            while (i < len && line[i] != quote) {
                if (line[i] == '\\' && i + 1 < len) {
                    int elen = escape_token_len(line, len, i);
                    for (int k = i; k < i + elen && k < len; k++) out_colors[k] = PLAT_COLOR_ESCAPE;
                    i += elen;
                    continue;
                }
                out_colors[i] = PLAT_COLOR_STRING;
                i++;
            }
            if (i < len) { out_colors[i] = PLAT_COLOR_STRING; i++; } /* closing quote */
            continue;
        }

        if (isdigit((unsigned char)c) ||
            (c == '.' && i + 1 < len && isdigit((unsigned char)line[i + 1]))) {
            int tlen = number_token_len(line, len, i);
            for (int k = i; k < i + tlen; k++) out_colors[k] = PLAT_COLOR_NUMBER;
            i += tlen;
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

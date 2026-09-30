#ifndef CSON
#define CSON

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

/* Public Types */

typedef enum cson_item_type_e {
    CSON_NUMBER,
    CSON_BOOL,
    CSON_STRING,
    CSON_OBJECT,
    CSON_ARRAY,
    CSON_NULL,
} cson_item_type_t;

typedef struct cson_item_s cson_item_t;

/* Public API */

void cson_print(const cson_item_t *root);

const cson_item_t  *cson_parse          (const char *str);
const cson_item_t  *cson_parse_with_len (const char *str, size_t length);
void                cson_free           (const cson_item_t *root);
const cson_item_t  *cson_next_sibling   (const cson_item_t *item);
const cson_item_t  *cson_parent         (const cson_item_t *item);
const cson_item_t  *cson_array_at       (const cson_item_t *item, uint32_t idx);
const cson_item_t  *cson_object_get     (const cson_item_t *item, const char *key);
const cson_item_t  *cson_find           (const cson_item_t *item, const char *path);

cson_item_type_t    cson_type           (const cson_item_t *item); 
const char         *cson_string         (const cson_item_t *item); 
double              cson_number         (const cson_item_t *item); 
bool                cson_bool           (const cson_item_t *item); 

/* Implementation */

#ifdef CSON_IMPL

typedef enum {
    CSON_NON_STANDARD = 0,
    CSON_LEXER_TOKEN_LBRACE,
    CSON_LEXER_TOKEN_RBRACE,
    CSON_LEXER_TOKEN_LBRACKET,
    CSON_LEXER_TOKEN_RBRACKET,
    CSON_LEXER_TOKEN_COLON,
    CSON_LEXER_TOKEN_COMMA,
    CSON_LEXER_TOKEN_STRING,
    CSON_LEXER_TOKEN_NUMBER,
    CSON_LEXER_TOKEN_BOOLEAN,
    CSON_LEXER_TOKEN_NULL
} cson_lexer_token_type_t;

typedef struct {
    cson_lexer_token_type_t type;
    union {
        bool boolean;
        char* ptr;
        double number;
    } value;
} cson_lexer_token_t;

/*
char *table[] = {
    "CSON_NON_STANDARD",
    "CSON_LEXER_TOKEN_LBRACE",
    "CSON_LEXER_TOKEN_RBRACE",
    "CSON_LEXER_TOKEN_LBRACKET",
    "CSON_LEXER_TOKEN_RBRACKET",
    "CSON_LEXER_TOKEN_COLON",
    "CSON_LEXER_TOKEN_COMMA",
    "CSON_LEXER_TOKEN_STRING",
    "CSON_LEXER_TOKEN_NUMBER",
    "CSON_LEXER_TOKEN_BOOLEAN",
    "CSON_LEXER_TOKEN_NULL",
};

static void print_token(cson_lexer_token_t t)
{
    printf("Type: %s Val: ", table[t.type]);
    if(t.type == CSON_LEXER_TOKEN_STRING)
        printf("%s", t.value.ptr);
    else if(t.type == CSON_LEXER_TOKEN_NUMBER)
        printf("%g", t.value.number);
    else if(t.type == CSON_LEXER_TOKEN_BOOLEAN)
        printf("%s", t.value.boolean ? "true" : "false");
    else if(t.type == CSON_LEXER_TOKEN_NULL)
        printf("null");
    else
        printf("%c", *(char *)t.value.ptr);
    
    printf("\n");
}*/



// helper
static int hex_val(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return 10 + c - 'a';
    if (c >= 'A' && c <= 'F') return 10 + c - 'a';
    return -1;
}

// Assumes str ptr points to array of at least four chars
static int parse_unicode(const char *str)
{
    int val = 0;
    for(int i = 0; i < 4; i++)
    {
        int digit = hex_val(str[i]);
        if(digit == -1) return -1;
        val = (val << 4) | digit;
    }
    return val;
}

static int encode_utf8(int cp, char *dest)
{
    if(cp <= 0x7F)
    {
        dest[0] = (char)cp;
        return 1;
    }
    else if(cp <= 0x7FF)
    {
        dest[0] = (char)((cp >> 6) | 0xC0);
        dest[1] = (char)((cp & 0x3F) | 0x80);
        return 2;
    }
    else if(cp <= 0xFFFF)
    {
        dest[0] = (char)((cp >> 12) | 0xE0);
        dest[1] = (char)(((cp >> 6) & 0x3F) | 0x80);
        dest[2] = (char)((cp & 0x3F) | 0x80);
        return 3;
    }
    return 0;

} 

static inline bool _is_space(char c)
{
    return c == ' ' ||
           c == '\t' ||
           c == '\n' ||
           c == '\r';
}

static inline bool _is_token_char(char c)
{
    return c == '{' ||
           c == '}' ||
           c == '[' ||
           c == ']' ||
           c == ':' ||
           c == ',' ||
           c == '"';
}

#ifdef CSON_SIMD
#include <immintrin.h>

static cson_lexer_token_t *cson_lexer_simd(char *str, uint32_t *num_toks_r)
{
    for(int i = 0; str[i]; i++)
    {
        __m256i input = _mm256_loadu_si256((const __m256i *)(str + i));
        __m256i quotes = _mm256_cmhtq_epi8(input, _mm256_set1_epi8)
    }
}
#endif

static cson_lexer_token_t *cson_lexer_temp(char *str, uint32_t *num_toks_r)
{
    uint32_t num_toks = 0;
    //size_t length = strlen(str);
    for(size_t i = 0; str[i] != '\0'; i++){
        char c = str[i];
        switch(c)
        {
            case '{':
            case '}':
            case '[':
            case ']':
            case ':':
            case ',':
                num_toks++;
                break;
            case '"':
                num_toks++;
                for(++i; str[i] != '\0'; i++)
                {
                    if(str[i] == '"')
                        break;
                    if(str[i] == '\\')
                        i++;
                }
                break;
            default:
                if(isspace(c)) break;
                // Parse bool, null, int, and float types

                for(; str[i] != '\0' && !_is_space(str[i]) && !_is_token_char(str[i]); i++);

                num_toks++;
                i--;
                break;
        }

    }

    *num_toks_r = num_toks;
    return NULL;
}

cson_lexer_token_type_t types[128] = {
    ['{'] = CSON_LEXER_TOKEN_LBRACE,
    ['}'] = CSON_LEXER_TOKEN_RBRACE,
    ['['] = CSON_LEXER_TOKEN_LBRACKET,
    [']'] = CSON_LEXER_TOKEN_RBRACKET,
    [':'] = CSON_LEXER_TOKEN_COLON,
    [','] = CSON_LEXER_TOKEN_COMMA,
};

char escape_characters[128] = {
    ['"'] = '"',
    ['/'] = '/',
    ['\\'] = '\\',
    ['n'] = '\n',
    ['t'] = '\t',
    ['r'] = '\r',
    ['b'] = '\b',
    ['f'] = '\f'
};

#include <errno.h>   // Required for errno and ERANGE
static cson_lexer_token_t *cson_lexer(char *str, size_t length, uint32_t *num_toks_r)
{
    size_t toks_capacity = length / 4;
    cson_lexer_token_t *toks = (cson_lexer_token_t*)malloc(sizeof(cson_lexer_token_t) * toks_capacity);

    uint32_t num_toks = 0;
    for(size_t i = 0; i < length; i++){
        char c = str[i];
        if(types[c] != CSON_NON_STANDARD)
            toks[num_toks++] = (cson_lexer_token_t){types[c], .value.ptr = &str[i]};
        else if(_is_space(c))
            continue;
        else if(c == '"')
        {
            toks[num_toks++] = (cson_lexer_token_t){CSON_LEXER_TOKEN_STRING, .value.ptr = &str[++i]};
            uint32_t offset = 0;    // Since we are reusing the duped/allocated json string for storing keys and strings we need an offset value to know in which position to overwrite the string. This is required due to escape character conversion.

            uint32_t start_index = i;

            bool has_escapes = false;
            while(i + 8 <= length)
            {
                uint64_t chunk = *(uint64_t*)(str+i);

                uint64_t quote_mask = chunk ^ 0x2222222222222222ULL;
                uint64_t escape_mask = chunk ^ 0x5C5C5C5C5C5C5C5CULL;

                uint64_t has_quote  = (quote_mask - 0x0101010101010101ULL) & ~quote_mask & 0x8080808080808080ULL;
                uint64_t has_escape = (escape_mask - 0x0101010101010101ULL) & ~escape_mask & 0x8080808080808080ULL;

                if(has_quote || has_escape)
                    break;

                i += 8;
            }

            for(; i < length && str[i] != '"'; i++)
            {
                if(str[i] == '\\') // Escape characters
                {
                    if(str[i+1] == 'u')
                    {
                        int cp = parse_unicode(&str[i+2]);
                        offset += encode_utf8(cp, &str[i - offset]);
                        i += 5; // u character plus four digits
                    }
                    else
                        str[i - (offset++)] = escape_characters[str[i+1]];

                    i++;
                }
                else if(offset)
                    str[i - offset] = str[i];
            }
            str[i - offset] = '\0';
        }
        else
        {
            // Parse bool, null, int, and float types
            char *string_to_be_parsed = &str[i];

            // 1: iterate until we hit another token ({}[]:,") or whitespace.
            for(; str[i] && !_is_space(str[i]) && !_is_token_char(str[i]); i++);

            char stored = str[i];
            str[i] = '\0';

            int difference = &str[i] - string_to_be_parsed;
            if(string_to_be_parsed[0] == 'f' && difference == 5 && *(uint32_t*)(string_to_be_parsed + 1) == 0x65736C61)
                toks[num_toks++] = (cson_lexer_token_t){CSON_LEXER_TOKEN_BOOLEAN, .value.boolean = false};
            else if(difference == 4 && *(uint32_t*)string_to_be_parsed == 0x65757274)
                toks[num_toks++] = (cson_lexer_token_t){CSON_LEXER_TOKEN_BOOLEAN, .value.boolean = true};
            else if(difference == 4 && *(uint32_t*)string_to_be_parsed == 0x6C6C756E)
                toks[num_toks++] = (cson_lexer_token_t){CSON_LEXER_TOKEN_NULL, .value.ptr = NULL};
            else
            {

                char *endptr;
                errno = 0;
                toks[num_toks++] = (cson_lexer_token_t){CSON_LEXER_TOKEN_NUMBER, .value.number = strtod(string_to_be_parsed, &endptr)};
                if(string_to_be_parsed == endptr || errno == ERANGE)
                {
                    free(toks);
                    return NULL;
                }
            }
            str[i--] = stored;
        }

        if(num_toks == toks_capacity)
        {
            toks_capacity *= 2;
            toks = realloc(toks, sizeof(cson_lexer_token_t) * toks_capacity);
        }
    }

    *num_toks_r = num_toks;
    return toks;
}

struct cson_item_s {
    cson_item_type_t type;
    uint32_t parent_offset; // positive value which is subtraced (implied negative)
    uint32_t next_offset;
    const char *key; // null for empty objects or arrays or values in arrays
    union {
        bool boolean;
        double number;
        const char *string;
        uint32_t num_children;
    } data;
};

/*
char *item_table[] = {
    "CSON_NUMBER",
    "CSON_BOOL",
    "CSON_STRING",
    "CSON_OBJECT",       // JSON object
    "CSON_ARRAY",
    "CSON_NULL"
};

static void print_item(cson_item_t item)
{
    printf("\{Type: %s, Parent: %d, Key: %s, ", item_table[item.type], -item.parent_offset, item.key ? item.key : "NULL");
    switch(item.type)
    {
        case CSON_NUMBER:
            printf("Data: %g", item.data.number);
            break;
        case CSON_BOOL:
            printf("Data: %s", item.data.boolean ? "true":"false");
            break;
        case CSON_STRING:
            printf("Data: %s", item.data.string);
            break;
        case CSON_OBJECT:       // JSON object
            printf("Children: %d", item.data.num_children);
            break;
        case CSON_ARRAY:
            printf("Size: %d", item.data.num_children);
            break;
        case CSON_NULL:
            printf("Data: null", item.data.num_children);
            break;
    }

    printf("}\n");
}*/

typedef struct {
    uint64_t binary_stack;
    uint8_t depth;
} cson_parse_stack_t;

static inline void binary_stack_push(cson_parse_stack_t *s, bool state)
{
    s->binary_stack = (s->binary_stack << 1) | (state & 1);
    s->depth++;
}

static inline void binary_stack_pop(cson_parse_stack_t *s)
{
    s->binary_stack >>= 1;
    s->depth--;
}

static inline bool binary_stack_peek(const cson_parse_stack_t *s)
{
    return s->binary_stack & 1;
}

#define STATE_OBJECT 0
#define STATE_ARRAY 1

static uint32_t _get_num_items_and_error_check(cson_lexer_token_t *tokens, uint32_t num_tokens)
{
    uint32_t num_items = 1;

    bool awaiting_key = true;
    cson_parse_stack_t stack = {0};

    uint32_t i = 0;
    if(tokens[0].type == CSON_LEXER_TOKEN_LBRACE)
        binary_stack_push(&stack, STATE_OBJECT);
    else if(tokens[0].type == CSON_LEXER_TOKEN_LBRACKET)
        binary_stack_push(&stack, STATE_ARRAY);
    else
    {
        // TODO account for array possibility somehow
        printf("ERROR: Json must start with a left brace/bracket!\n");
        i = -1;
    }

    cson_lexer_token_t previous_token;
    while(i < num_tokens - 1)
    {
        i++;
        //printf("%s | Current token: ", binary_stack_peek(&stack) == STATE_OBJECT ? "OBJ" : "ARR");
        cson_lexer_token_t token = tokens[i];
        previous_token = tokens[i-1];
        //print_token(token);
        switch(token.type)
        {
            case CSON_LEXER_TOKEN_LBRACE:
                if(previous_token.type != CSON_LEXER_TOKEN_LBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_COLON
                    && !(previous_token.type == CSON_LEXER_TOKEN_COMMA && binary_stack_peek(&stack) == STATE_ARRAY))
                {
                    // Syntax error
                    printf("Invalid object start!\n");
                    i = -1;
                    break;
                }

                binary_stack_push(&stack, STATE_OBJECT);
                if(stack.depth > 64)
                {
                    printf("Stack overflow (max depth 64)!\n");
                    i = -1;
                    break;
                }

                num_items++;
                awaiting_key = true;
                break;
            case CSON_LEXER_TOKEN_RBRACE:
                if(previous_token.type != CSON_LEXER_TOKEN_RBRACE
                    && previous_token.type != CSON_LEXER_TOKEN_RBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_NUMBER
                    && previous_token.type != CSON_LEXER_TOKEN_BOOLEAN
                    && previous_token.type != CSON_LEXER_TOKEN_NULL
                    && (previous_token.type != CSON_LEXER_TOKEN_STRING && !awaiting_key)
                    && !(previous_token.type == CSON_LEXER_TOKEN_COMMA && binary_stack_peek(&stack) == STATE_ARRAY))
                {
                    // Syntax error
                    printf("Invalid object close!\n");
                    i = -1;
                    break;
                }

                if(binary_stack_peek(&stack) == STATE_ARRAY || stack.depth == 0) { printf("Invalid object close!\n"); i = -1; break; } // ERROR IMPROPER CLOSING OF OBJECT WHEN IN ARRAY
                binary_stack_pop(&stack);
                break;
            case CSON_LEXER_TOKEN_LBRACKET:
                if(previous_token.type != CSON_LEXER_TOKEN_LBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_COLON
                    && !(previous_token.type == CSON_LEXER_TOKEN_COMMA && binary_stack_peek(&stack) == STATE_ARRAY))
                {
                    // Syntax error
                    printf("Invalid array start!\n");
                    i = -1;
                    break;
                }
                
                num_items++;

                binary_stack_push(&stack, STATE_ARRAY);
                if(stack.depth > 64)
                {
                    printf("Stack overflow (max depth 64)!\n");
                    i = -1;
                    break;
                }
                break;
            case CSON_LEXER_TOKEN_RBRACKET:
                if(previous_token.type != CSON_LEXER_TOKEN_RBRACE
                    && previous_token.type != CSON_LEXER_TOKEN_LBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_RBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_NUMBER
                    && previous_token.type != CSON_LEXER_TOKEN_BOOLEAN
                    && previous_token.type != CSON_LEXER_TOKEN_NULL
                    && (previous_token.type != CSON_LEXER_TOKEN_STRING && !awaiting_key))
                {
                    // Syntax error
                    printf("Invalid array close!\n");
                    i = -1;
                    break;
                }
                if(binary_stack_peek(&stack) == STATE_OBJECT || stack.depth == 0) { printf("Invalid array close!\n"); i = -1; break; }
                binary_stack_pop(&stack);
                break;
            case CSON_LEXER_TOKEN_COLON:
                if(previous_token.type != CSON_LEXER_TOKEN_STRING && !awaiting_key)
                {
                    // Syntax error
                    printf("Invalid location for colon token!\n");
                    i = -1;
                    break;
                }
                awaiting_key = false;
                break;
            case CSON_LEXER_TOKEN_COMMA:
                if(previous_token.type != CSON_LEXER_TOKEN_RBRACE
                    && previous_token.type != CSON_LEXER_TOKEN_RBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_NUMBER
                    && previous_token.type != CSON_LEXER_TOKEN_BOOLEAN
                    && previous_token.type != CSON_LEXER_TOKEN_NULL
                    && (previous_token.type != CSON_LEXER_TOKEN_STRING && !awaiting_key))
                {
                    // Syntax error
                    printf("Invalid location for comma token!\n");
                    i = -1;
                    break;
                }

                if(binary_stack_peek(&stack) == STATE_OBJECT)
                    awaiting_key = true;

                break;
            case CSON_LEXER_TOKEN_STRING:
                if(binary_stack_peek(&stack) == STATE_OBJECT && awaiting_key)
                {
                    if(previous_token.type != CSON_LEXER_TOKEN_LBRACE
                        && previous_token.type != CSON_LEXER_TOKEN_COMMA)
                        {
                            printf("Invalid location for key!\n");
                            i = -1;
                        }
                    break;
                }
            case CSON_LEXER_TOKEN_NUMBER:
            case CSON_LEXER_TOKEN_BOOLEAN:
            case CSON_LEXER_TOKEN_NULL:
                if((previous_token.type != CSON_LEXER_TOKEN_LBRACKET
                    && previous_token.type != CSON_LEXER_TOKEN_COLON)
                    && !(previous_token.type == CSON_LEXER_TOKEN_COMMA && binary_stack_peek(&stack) == STATE_ARRAY))
                {
                    // Syntax error
                    printf("Invalid location for value!\n");
                    i = -1;
                    break;
                }

                num_items++;

                break;
            default:
                // unknown token
                i = -1;
                break;
        }
    }

    if(i == -1)
    {
        // Something failed!
        printf("Something failed!\n");
        return -1;
    }

    return num_items;
}

#define SHIFT_STRING(str, diff) (char *)(str + diff)

const cson_item_t* cson_parse(const char* json_str) {
    size_t len = strlen(json_str); 
    return cson_parse_with_len(json_str, len);
}

const cson_item_t *cson_parse_with_len(const char *str, size_t length)
{
    char *str_to_be_lexered = strdup(str);
    uint32_t num_tokens;
    cson_lexer_token_t *tokens = cson_lexer(str_to_be_lexered, length, &num_tokens);
    if(!tokens)
        return NULL;

    uint32_t num_items = _get_num_items_and_error_check(tokens, num_tokens);
    if(num_items == -1)
    {
        free(tokens);
        return NULL;
    }

    cson_item_t *root = malloc(sizeof(cson_item_t) * num_items + strlen(str) + 1);

    char* strings = (char *)root + (sizeof(cson_item_t) * num_items);
    memcpy(strings, str_to_be_lexered, strlen(str) + 1);

    ptrdiff_t str_ptr_diff = strings - str_to_be_lexered; // To offset the char* by the correct amount. uintptr_t should ensure this keeps working for all optimization levels

    uint32_t current_idx = 0;

    bool awaiting_key = true;
    cson_parse_stack_t stack = {0};
    if(tokens[0].type == CSON_LEXER_TOKEN_LBRACE)
    {
        binary_stack_push(&stack, STATE_OBJECT);
        root[current_idx++] = (cson_item_t){CSON_OBJECT, 0, 1, NULL, 0};
    }
    else if(tokens[0].type == CSON_LEXER_TOKEN_LBRACKET)
    {
        binary_stack_push(&stack, STATE_ARRAY);
        root[current_idx++] = (cson_item_t){CSON_ARRAY, 0, 1, NULL, 0};
        awaiting_key = false;
    }

    char *current_key = NULL;
    uint32_t parent_idx = 0;

    uint32_t previous_idx = current_idx;
    for(uint32_t i = 1; i < num_tokens; i++)
    {
        cson_lexer_token_t token = tokens[i];
        switch(token.type)
        {
            case CSON_LEXER_TOKEN_LBRACE:
                binary_stack_push(&stack, STATE_OBJECT);
                root[parent_idx].data.num_children++;
                root[current_idx] = (cson_item_t){CSON_OBJECT, current_idx - parent_idx, 1, current_key, 0};
                parent_idx = current_idx++;
                awaiting_key = true;
                break;
            case CSON_LEXER_TOKEN_LBRACKET:
                binary_stack_push(&stack, STATE_ARRAY);
                root[parent_idx].data.num_children++;
                root[current_idx] = (cson_item_t){CSON_ARRAY, current_idx - parent_idx, 1, current_key, 0};
                parent_idx = current_idx++;
                current_key = NULL;
                awaiting_key = false;
                break;
            case CSON_LEXER_TOKEN_RBRACE:
            case CSON_LEXER_TOKEN_RBRACKET:
                binary_stack_pop(&stack);
                root[parent_idx].next_offset = current_idx - parent_idx;
                parent_idx = parent_idx - root[parent_idx].parent_offset;
                break;
            case CSON_LEXER_TOKEN_COLON:
                break;
            case CSON_LEXER_TOKEN_COMMA:
                awaiting_key = binary_stack_peek(&stack) == STATE_OBJECT;
                break;
            case CSON_LEXER_TOKEN_STRING:
                if(awaiting_key)
                {
                    current_key = SHIFT_STRING(token.value.ptr, str_ptr_diff);
                    awaiting_key = false;
                }
                else
                {
                    root[parent_idx].data.num_children++;
                    root[current_idx++] = (cson_item_t){CSON_STRING, current_idx - parent_idx, 1, current_key, .data.string=SHIFT_STRING(token.value.ptr, str_ptr_diff)};
                }
                break;
            case CSON_LEXER_TOKEN_NUMBER:
                root[parent_idx].data.num_children++;
                root[current_idx++] = (cson_item_t){CSON_NUMBER, current_idx - parent_idx, 1, current_key, .data.number=token.value.number};
                break;
            case CSON_LEXER_TOKEN_BOOLEAN:
                root[parent_idx].data.num_children++;
                root[current_idx++] = (cson_item_t){CSON_BOOL, current_idx - parent_idx, 1, current_key, .data.boolean=token.value.boolean};
                break;
            case CSON_LEXER_TOKEN_NULL:
                root[parent_idx].data.num_children++;
                root[current_idx++] = (cson_item_t){CSON_NULL, current_idx - parent_idx, 1, current_key, 0};
                break;
            default:
                break;
        }

        if(previous_idx != current_idx)
        {
            previous_idx = current_idx;
            //printf("Num items: %d\n", current_idx - 1);
            //printf("%d: ", current_idx - 1);
            //print_item(object->items[current_idx-1]);
        }
    }

    free(tokens);
    free(str_to_be_lexered);

    return root;
}

static int _print_item(const cson_item_t *root, int idx, int depth)
{
    cson_item_t item = root[idx];
    printf("%*s", depth * 2, "");
    if(item.key)
        printf("\"%s\": ", item.key);
    switch(item.type)
    {
        case CSON_OBJECT:       // JSON object
            printf("{");

            if(item.data.num_children > 0)
                printf("\n");
            else
            {
                printf("}");
                return idx + 1;
            }

            int next_idx = idx + 1;
            for(int i = 0; i < item.data.num_children; i++)
            {
                next_idx = _print_item(root, next_idx, depth+1);
                if(i != item.data.num_children - 1)
                    printf(",");
                printf("\n");
            }

            printf("%*s", depth * 2, "");
            printf("}");
            return next_idx;
        case CSON_ARRAY:
            printf("[");

            if(item.data.num_children > 0)
                printf("\n");
            else
            {
                printf("]");
                return idx + 1;
            }

            next_idx = idx + 1;
            for(int i = 0; i < item.data.num_children; i++)
            {
                next_idx = _print_item(root, next_idx, depth+1);
                if(i != item.data.num_children - 1)
                    printf(",");
                printf("\n");
            }

            printf("%*s", depth * 2, "");
            printf("]");
            return next_idx;
        case CSON_NUMBER:
            printf("%g", item.data.number);
            return idx + 1;
        case CSON_BOOL:
            printf("%s", item.data.boolean ? "true" : "false");
            return idx + 1;
        case CSON_STRING:
            printf("\"%s\"", item.data.string);
            return idx + 1;
        case CSON_NULL:
            printf("null");
            return idx + 1;
    }
}

void cson_print(const cson_item_t *root)
{
    if(!root)
        return;

    _print_item(root, 0, 0);
    printf("\n");
}

const cson_item_t  *cson_next_sibling  (const cson_item_t *item) {return (cson_item_t *)(item + item->next_offset);}
const cson_item_t  *cson_parent        (const cson_item_t *item) {return (cson_item_t *)(item - item->parent_offset);}
cson_item_type_t    cson_type          (const cson_item_t *item) {return item->type;}
const char         *cson_string        (const cson_item_t *item) {return item->data.string;};
double              cson_number        (const cson_item_t *item) {return item->data.number;};
bool                cson_bool          (const cson_item_t *item) {return item->data.boolean;};
const cson_item_t  *cson_array_at      (const cson_item_t *item, uint32_t idx) {
    if(idx >= item->data.num_children) return NULL;

    const cson_item_t *current_item = item + 1;
    for(int i = 0; i < idx; i++)
        current_item = cson_next_sibling(current_item);
    
    return current_item;
};
const cson_item_t  *cson_object_get    (const cson_item_t *item, const char *key)
{
    const cson_item_t *current_item = (cson_item_t *)(item + 1);
    for(int i = 0; i < item->data.num_children; i++)
    {
        if(current_item->key && !strcmp(current_item->key, key))
            return current_item;
        
        current_item = cson_next_sibling(current_item);
    }
    return NULL;
};

#include <limits.h>  // Required for INT_MAX and INT_MIN
const cson_item_t *cson_find(const cson_item_t *item, const char *path)
{
    char *str = strdup(path);
    char *token = strtok(str, ".");
    const cson_item_t *current_item = item;
    while(token != NULL && current_item)
    {
        if(cson_type(current_item) == CSON_OBJECT)
            current_item = cson_object_get(current_item, token);
        else if(cson_type(current_item) == CSON_ARRAY)
        {
            char *endptr;
            long val = strtol(token, &endptr, 10);
            if (endptr == str || *endptr != '\0' || ((val == LONG_MAX || val == LONG_MIN) && errno == ERANGE) || val > INT_MAX || val < INT_MIN)
                return NULL;
            else
                current_item = cson_array_at(current_item, (int)val);
        }
        else
            return NULL;

        token = strtok(NULL, ".");
    }

    return current_item;
}

void cson_free(const cson_item_t *root)
{
    free((void*)root);
}

#endif

#endif
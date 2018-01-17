#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <errno.h>   /* errno, ERANGE */
#include <math.h>    /* HUGE_VAL */
#include <stdlib.h>  /* NULL, strtod() */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;
    c->json = p;
}


static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type){
    EXPECT(c, literal[0]);

    size_t i;
    for(i = 0; literal[i+1]; ++i){
        if(c->json[i] != literal[i+1]){
            return LEPT_PARSE_INVALID_VALUE;
        }
    }

    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')


static int lept_parse_number(lept_context* c, lept_value* v) {
    /* 编程技巧：开始的-号与frac部分的 + -号是可选部分，因此可以有的话，指针加1略过*/
    const char* p = c->json;
    if('-' == *p) ++p;

    if('0' == *p){
        ++p;
    } else {
        if(!ISDIGIT1TO9(*p)){ 
            return LEPT_PARSE_INVALID_VALUE; 
        } else {
            for(++p; ISDIGIT(*p); ++p);
        }
    }

    if('.' == *p){
        ++p;
        if(!ISDIGIT(*p)) { 
            return LEPT_PARSE_INVALID_VALUE; 
        } else {
            for(++p; ISDIGIT(*p); ++p);
        }
    }

    if('e' == *p || 'E' == *p) {
        ++p;
        if('-' == *p || '+' == *p) ++p;
        
        if(!ISDIGIT(*p)){ 
            return LEPT_PARSE_INVALID_VALUE;
        } else {
            for(++p; ISDIGIT(*p); ++p);
        }
    }

    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->type = LEPT_NUMBER;
    /*p 指向已经验证过的数字结尾*/
    c->json = p;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        default:   return lept_parse_number(c, v);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}

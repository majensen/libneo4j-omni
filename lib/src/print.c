/* vi:set ts=4 sw=4 expandtab:
 *
 * Copyright 2016, Chris Leishman (http://github.com/cleishm)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "../../config.h"
#include "print.h"
#include "util.h"
#include "values.h"
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdlib.h>
#include <time.h>

#define BUFLEN (100)

static ssize_t identifier_str(char *buf, size_t n, const neo4j_value_t *value);
static ssize_t identifier_fprint(const neo4j_value_t *value, FILE *stream);
static ssize_t string_str(char *buf, size_t n, char quot, const char *s,
        size_t len);
static ssize_t string_fprint(FILE *stream, char quot, const char *s,
        size_t len);
static ssize_t list_str(char *buf, size_t n, const neo4j_value_t *values,
        unsigned int nvalues);
static ssize_t list_fprint(const neo4j_value_t *values, unsigned int nvalues,
        FILE *stream);


/* null */

ssize_t neo4j_null_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_NULL);
    if (n > 0)
    {
        if (n > 5)
        {
            n = 5;
        }
        memcpy(buf, "null", n-1);
        buf[n-1] = '\0';
    }
    return 4;
}


ssize_t neo4j_null_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_NULL);
    return (fputs("null", stream) == EOF)? -1 : 4;
}


/* boolean */

ssize_t neo4j_bool_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_BOOL);
    const struct neo4j_bool *v = (const struct neo4j_bool *)value;
    if (v->value > 0)
    {
        if (n > 0)
        {
            if (n > 5)
            {
                n = 5;
            }
            memcpy(buf, "true", n-1);
            buf[n-1] = '\0';
        }
        return 4;
    }
    else
    {
        if (n > 0)
        {
            if (n > 6)
            {
                n = 6;
            }
            memcpy(buf, "false", n-1);
            buf[n-1] = '\0';
        }
        return 5;
    }
}


ssize_t neo4j_bool_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_BOOL);
    const struct neo4j_bool *v = (const struct neo4j_bool *)value;
    return (fputs((v->value > 0)? "true" : "false", stream) == EOF)?
        -1 : (v->value > 0)? 4 : 5;
}


/* integer */

ssize_t neo4j_int_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_INT ||
            neo4j_type(*value) == NEO4J_IDENTITY);
    const struct neo4j_int *v = (const struct neo4j_int *)value;
    int r = snprintf(buf, n, "%" PRId64, v->value);
    assert(r > 0);
    return (size_t)r;
}


ssize_t neo4j_int_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_INT ||
            neo4j_type(*value) == NEO4J_IDENTITY);
    const struct neo4j_int *v = (const struct neo4j_int *)value;
    return fprintf(stream, "%" PRId64, v->value);
}


/* float */

ssize_t neo4j_float_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_FLOAT);
    const struct neo4j_float *v = (const struct neo4j_float *)value;
    int r = snprintf(buf, n, "%f", v->value);
    assert(r > 0);
    return (size_t)r;
}


ssize_t neo4j_float_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_FLOAT);
    const struct neo4j_float *v = (const struct neo4j_float *)value;
    return fprintf(stream, "%f", v->value);
}


/* string */

ssize_t neo4j_string_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_STRING || neo4j_type(*value) == NEO4J_ELEMENTID);
    const struct neo4j_string *v = (const struct neo4j_string *)value;
    return string_str(buf, n, '"', (const char *)v->ustring, v->length);
}


ssize_t neo4j_string_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_STRING || neo4j_type(*value) == NEO4J_ELEMENTID);
    const struct neo4j_string *v = (const struct neo4j_string *)value;
    return string_fprint(stream, '"', (const char *)v->ustring, v->length);
}


ssize_t identifier_str(char *buf, size_t n, const neo4j_value_t *value)
{
    assert(neo4j_type(*value) == NEO4J_STRING);
    const struct neo4j_string *v = (const struct neo4j_string *)value;
    const char *s = (const char *)v->ustring;

    if (memspn_ident(s, v->length) < v->length)
    {
        return string_str(buf, n, '`', s, v->length);
    }

    if (n > 0)
    {
        size_t l = minzu(n-1, v->length);
        memcpy(buf, s, l);
        buf[l] = '\0';
    }
    return v->length;
}


ssize_t identifier_fprint(const neo4j_value_t *value, FILE *stream)
{
    assert(neo4j_type(*value) == NEO4J_STRING);
    const struct neo4j_string *v = (const struct neo4j_string *)value;
    const char *s = (const char *)v->ustring;

    if (memspn_ident(s, v->length) < v->length)
    {
        return string_fprint(stream, '`', s, v->length);
    }

    if (fwrite(s, sizeof(char), v->length, stream) < v->length)
    {
        return -1;
    }
    return v->length;
}


ssize_t string_str(char *buf, size_t n, char quot, const char *s, size_t len)
{
    const unsigned char esc[2] = { quot, '\\' };

    if (n > 0)
    {
        buf[0] = quot;
    }

    size_t l = 1;
    const char *end = s + len;
    while (s < end)
    {
        size_t i = memcspn(s, end - s, esc, 2);
        if ((l+1) < n)
        {
            memcpy(buf+l, s, minzu(n-l-1, i));
        }
        s += i;
        l += i;

        if (s >= end)
        {
            assert(s == end);
            break;
        }

        if ((l+2) < n)
        {
            buf[l] = '\\';
            buf[l+1] = *s;
        }
        else if ((l+1) < n)
        {
            buf[l] = '\0';
        }
        l += 2;
        ++s;
    }

    if ((l+1) < n)
    {
        buf[l] = quot;
    }
    l++;
    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t string_fprint(FILE *stream, char quot, const char *s, size_t len)
{
    const unsigned char esc[2] = { quot, '\\' };

    if (fputc(quot, stream) == EOF)
    {
        return -1;
    }

    size_t l = 1;
    const char *end = s + len;
    while (s < end)
    {
        size_t i = memcspn(s, end - s, esc, 2);
        if (fwrite(s, sizeof(unsigned char), i, stream) < i)
        {
            return -1;
        }
        s += i;
        l += i;

        if (s >= end)
        {
            assert(s == end);
            break;
        }

        if (fputc('\\', stream) == EOF || fputc(*s, stream) == EOF)
        {
            return -1;
        }
        l += 2;
        ++s;
    }

    if (fputc(quot, stream) == EOF)
    {
        return -1;
    }
    return ++l;
}


/* bytes */

ssize_t neo4j_bytes_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_BYTES);
    const struct neo4j_bytes *v = (const struct neo4j_bytes *)value;

    if (n > 0)
    {
        buf[0] = '#';
    }
    size_t l = 1;

    for (unsigned int i = 0; i < v->length; ++i)
    {
        int r = snprintf(buf + l, (l < n)? n-l : 0, "%02x", v->bytes[i]);
        if (r < 0)
        {
            return -1;
        }
        l += r;
    }

    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t neo4j_bytes_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_BYTES);
    const struct neo4j_bytes *v = (const struct neo4j_bytes *)value;

    if (fputc('#', stream) == EOF)
    {
        return -1;
    }

    ssize_t l = 1;
    for (unsigned int i = 0; i < v->length; ++i)
    {
        int r = fprintf(stream, "%02x", v->bytes[i]);
        if (r < 0)
        {
            return -1;
        }
        l += r;
    }

    return l;
}


/* list */

ssize_t neo4j_list_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_LIST);
    const struct neo4j_list *v = (const struct neo4j_list *)value;

    if (n > 0)
    {
        buf[0] = '[';
    }
    size_t l = 1;

    l += list_str(buf+l, (l < n)? n-l : 0, v->items, v->length);

    if ((l+1) < n)
    {
        buf[l] = ']';
    }
    l++;
    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t neo4j_list_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_LIST);
    const struct neo4j_list *v = (const struct neo4j_list *)value;

    if (fputc('[', stream) == EOF)
    {
        return -1;
    }

    ssize_t l = list_fprint(v->items, v->length, stream);
    if (l < 0)
    {
        return -1;
    }
    l++;

    if (fputc(']', stream) == EOF)
    {
        return -1;
    }
    return ++l;
}


ssize_t list_str(char *buf, size_t n, const neo4j_value_t *values,
        unsigned int nvalues)
{
    size_t l = 0;
    for (unsigned int i = 0; i < nvalues; ++i)
    {
        l += neo4j_ntostring(values[i], buf+l, (l < n)? n-l : 0);

        if ((i+1) < nvalues)
        {
            if ((l+1) < n)
            {
                buf[l] = ',';
            }
            l++;
        }
    }
    return l;
}


ssize_t list_fprint(const neo4j_value_t *values, unsigned int nvalues,
        FILE *stream)
{
    size_t l = 0;
    for (unsigned int i = 0; i < nvalues; ++i)
    {
        ssize_t ll = neo4j_fprint(values[i], stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;

        if ((i+1) < nvalues)
        {
            if (fputc(',', stream) == EOF)
            {
                return -1;
            }
            l++;
        }
    }
    return l;
}


/* map */

ssize_t neo4j_map_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_MAP);
    const struct neo4j_map *v = (const struct neo4j_map *)value;

    if (n > 0)
    {
        buf[0] = '{';
    }
    size_t l = 1;

    for (unsigned int i = 0; i < v->nentries; ++i)
    {
        const neo4j_map_entry_t *entry = v->entries + i;
        assert(neo4j_type(entry->key) == NEO4J_STRING);
        l += identifier_str(buf+l, (l < n)? n-l : 0, &(entry->key));

        if ((l+1) < n)
        {
            buf[l] = ':';
        }
        l++;

        l += neo4j_ntostring(entry->value, buf+l, (l < n)? n-l : 0);

        if ((i+1) < v->nentries)
        {
            if ((l+1) < n)
            {
                buf[l] = ',';
            }
            l++;
        }
    }

    if ((l+1) < n)
    {
        buf[l] = '}';
    }
    l++;
    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t neo4j_map_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_MAP);
    const struct neo4j_map *v = (const struct neo4j_map *)value;

    if (fputc('{', stream) == EOF)
    {
        return -1;
    }
    size_t l = 1;

    for (unsigned int i = 0; i < v->nentries; ++i)
    {
        const neo4j_map_entry_t *entry = v->entries + i;
        assert(neo4j_type(entry->key) == NEO4J_STRING);
        ssize_t ll = identifier_fprint(&(entry->key), stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;

        if (fputc(':', stream) == EOF)
        {
            return -1;
        }
        l++;

        ll = neo4j_fprint(entry->value, stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;

        if ((i+1) < v->nentries)
        {
            if (fputc(',', stream) == EOF)
            {
                return -1;
            }
            l++;
        }
    }

    if (fputc('}', stream) == EOF)
    {
        return -1;
    }
    return ++l;
}


/* node */

ssize_t neo4j_node_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_NODE);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    assert(v->nfields == 3 || v->nfields == 4);

    if (n > 0)
    {
        buf[0] = '(';
    }
    size_t l = 1;

    assert(neo4j_type(v->fields[1]) == NEO4J_LIST);
    const struct neo4j_list *labels =
        (const struct neo4j_list *)&(v->fields[1]);

    for (unsigned int i = 0; i < labels->length; ++i)
    {
        const neo4j_value_t *label = labels->items + i;
        assert(neo4j_type(*label) == NEO4J_STRING);
        if ((l+1) < n)
        {
            buf[l] = ':';
        }
        l++;
        l += identifier_str(buf+l, (l < n)? n-l : 0, label);
    }

    assert(neo4j_type(v->fields[2]) == NEO4J_MAP);
    if (neo4j_map_size(v->fields[2]) > 0)
    {
        l += neo4j_map_str(&(v->fields[2]), buf+l, (l < n)? n-l : 0);
    }

    if ((l+1) < n)
    {
        buf[l] = ')';
    }
    l++;
    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t neo4j_node_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_NODE);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    assert(v->nfields == 3 || v->nfields == 4);

    if (fputc('(', stream) == EOF)
    {
        return -1;
    }
    size_t l = 1;

    assert(neo4j_type(v->fields[1]) == NEO4J_LIST);
    const struct neo4j_list *labels =
        (const struct neo4j_list *)&(v->fields[1]);

    for (unsigned int i = 0; i < labels->length; ++i)
    {
        const neo4j_value_t *label = labels->items + i;
        assert(neo4j_type(*label) == NEO4J_STRING);
        if (fputc(':', stream) == EOF)
        {
            return -1;
        }
        l++;
        ssize_t ll = identifier_fprint(label, stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;
    }

    assert(neo4j_type(v->fields[2]) == NEO4J_MAP);
    if (neo4j_map_size(v->fields[2]) > 0)
    {
        ssize_t ll = neo4j_map_fprint(&(v->fields[2]), stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;
    }

    if (fputc(')', stream) == EOF)
    {
        return -1;
    }
    return ++l;
}


/* relationship */

ssize_t neo4j_rel_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_RELATIONSHIP);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    assert(v->nfields == 5 || v->nfields == 3 ||
	   v->nfields == 8 || v->nfields == 4);

    if (n > 0)
    {
        buf[0] = '-';
        if (n > 1)
        {
            buf[1] = '[';
        }
    }
    size_t l = 2;

    int idx = (v->nfields == 5 || v->nfields == 8)? 3 : 1;
    assert(neo4j_type(v->fields[idx]) == NEO4J_STRING);
    if ((l+1) < n)
    {
        buf[l] = ':';
    }
    l++;
    l += identifier_str(buf+l, (l < n)? n-l : 0, &(v->fields[idx]));

    assert(neo4j_type(v->fields[idx+1]) == NEO4J_MAP);
    if (neo4j_map_size(v->fields[idx+1]) > 0)
    {
        l += neo4j_map_str(&(v->fields[idx+1]), buf+l, (l < n)? n-l : 0);
    }

    if ((l+1) < n)
    {
        buf[l] = ']';
        if ((l+2) < n)
        {
            buf[l+1] = '-';
        }
    }
    l+=2;
    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t neo4j_rel_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_RELATIONSHIP);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    assert(v->nfields == 5 || v->nfields == 3 ||
	   v->nfields == 8 || v->nfields == 4);

    if (fputs("-[:", stream) == EOF)
    {
        return -1;
    }
    size_t l = 3;

    int idx = (v->nfields == 5 || v->nfields == 8)? 3 : 1;
    assert(neo4j_type(v->fields[idx]) == NEO4J_STRING);

    ssize_t ll = identifier_fprint(&(v->fields[idx]), stream);
    if (ll < 0)
    {
        return -1;
    }
    l += (size_t)ll;

    assert(neo4j_type(v->fields[idx+1]) == NEO4J_MAP);
    if (neo4j_map_size(v->fields[idx+1]) > 0)
    {
        ll = neo4j_map_fprint(&(v->fields[idx+1]), stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;
    }

    if (fputs("]-", stream) == EOF)
    {
        return -1;
    }
    return l+2;
}


/* path */

ssize_t neo4j_path_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_PATH);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    assert(v->nfields == 3);

    assert(neo4j_type(v->fields[0]) == NEO4J_LIST);
    const struct neo4j_list *nodes = (const struct neo4j_list *)&(v->fields[0]);
    assert(neo4j_type(v->fields[1]) == NEO4J_LIST);
    const struct neo4j_list *rels = (const struct neo4j_list *)&(v->fields[1]);
    assert(neo4j_type(v->fields[2]) == NEO4J_LIST);
    const struct neo4j_list *seq = (const struct neo4j_list *)&(v->fields[2]);

    assert(nodes->length > 0);
    assert(neo4j_type(nodes->items[0]) == NEO4J_NODE);

    size_t l = neo4j_node_str(&(nodes->items[0]), buf, n);

    assert(seq->length % 2 == 0);
    for (unsigned int i = 0; i < seq->length; i += 2)
    {
        assert(neo4j_type(seq->items[i]) == NEO4J_INT);
        const struct neo4j_int *ridx_val =
            (const struct neo4j_int *)&(seq->items[i]);
        assert(neo4j_type(seq->items[i+1]) == NEO4J_INT);
        const struct neo4j_int *nidx_val =
            (const struct neo4j_int *)&(seq->items[i+1]);

        assert((ridx_val->value > 0 && ridx_val->value <= rels->length) ||
               (ridx_val->value < 0 && -(ridx_val->value) <= rels->length));
        unsigned int ridx = (unsigned int)(llabs(ridx_val->value) - 1);
        assert(neo4j_type(rels->items[ridx]) == NEO4J_RELATIONSHIP);

        assert(nidx_val->value >= 0 && nidx_val->value < nodes->length);
        unsigned int nidx = (unsigned int)nidx_val->value;
        assert(neo4j_type(nodes->items[nidx]) == NEO4J_NODE);

        if (ridx_val->value < 0)
        {
            if ((l+1) < n)
            {
                buf[l] = '<';
            }
            l++;
        }

        l += neo4j_rel_str(&(rels->items[ridx]), buf+l, (l < n)? n-l : 0);

        if (ridx_val->value > 0)
        {
            if ((l+1) < n)
            {
                buf[l] = '>';
            }
            l++;
        }

        l += neo4j_node_str(&(nodes->items[nidx]), buf+l, (l < n)? n-l : 0);
    }

    if (n > 0)
    {
        buf[minzu(n - 1, l)] = '\0';
    }
    return l;
}


ssize_t neo4j_path_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_PATH);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    assert(v->nfields == 3);

    assert(neo4j_type(v->fields[0]) == NEO4J_LIST);
    const struct neo4j_list *nodes = (const struct neo4j_list *)&(v->fields[0]);
    assert(neo4j_type(v->fields[1]) == NEO4J_LIST);
    const struct neo4j_list *rels = (const struct neo4j_list *)&(v->fields[1]);
    assert(neo4j_type(v->fields[2]) == NEO4J_LIST);
    const struct neo4j_list *seq = (const struct neo4j_list *)&(v->fields[2]);

    assert(nodes->length > 0);
    assert(neo4j_type(nodes->items[0]) == NEO4J_NODE);

    ssize_t ll = neo4j_node_fprint(&(nodes->items[0]), stream);
    
    if (ll < 0)
    {
        return -1;
    }
    size_t l = (size_t)ll;

    assert(seq->length % 2 == 0);
    for (unsigned int i = 0; i < seq->length; i += 2)
    {
        assert(neo4j_type(seq->items[i]) == NEO4J_INT);
        const struct neo4j_int *ridx_val =
            (const struct neo4j_int *)&(seq->items[i]);
        assert(neo4j_type(seq->items[i+1]) == NEO4J_INT);
        const struct neo4j_int *nidx_val =
            (const struct neo4j_int *)&(seq->items[i+1]);

        assert((ridx_val->value > 0 && ridx_val->value <= rels->length) ||
               (ridx_val->value < 0 && -(ridx_val->value) <= rels->length));
        unsigned int ridx = (unsigned int)(llabs(ridx_val->value) - 1);
        assert(neo4j_type(rels->items[ridx]) == NEO4J_RELATIONSHIP);

        assert(nidx_val->value >= 0 && nidx_val->value < nodes->length);
        unsigned int nidx = (unsigned int)nidx_val->value;
        assert(neo4j_type(nodes->items[nidx]) == NEO4J_NODE);

        if (ridx_val->value < 0)
        {
            if (fputc('<', stream) == EOF)
            {
                return -1;
            }
            l++;
        }
        ll = neo4j_rel_fprint(&(rels->items[ridx]), stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;

        if (ridx_val->value > 0)
        {
            if (fputc('>', stream) == EOF)
            {
                return -1;
            }
            l++;
        }

        ll = neo4j_node_fprint(&(nodes->items[nidx]), stream);
        if (ll < 0)
        {
            return -1;
        }
        l += (size_t)ll;
    }

    return l;
}


/* structure */

ssize_t neo4j_struct_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_STRUCT);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    int hlen;
    switch (v->signature)
    {
    case NEO4J_DATE_SIGNATURE:
	return neo4j_date_str(value, buf, n);
	break;
    case NEO4J_TIME_SIGNATURE:
	return neo4j_time_str(value, buf, n);
	break;
    case NEO4J_LOCALTIME_SIGNATURE:
	return neo4j_localtime_str(value, buf, n);
	break;
    case NEO4J_DATETIME_SIGNATURE:
    case NEO4J_LEGACY_DATETIME_SIGNATURE:
	return neo4j_datetime_str(value, buf, n);
	break;
    case NEO4J_LOCALDATETIME_SIGNATURE:
	return neo4j_localdatetime_str(value, buf, n);
	break;
//    case NEO4J_DURATION_SIGNATURE:
//	return neo4j_duration_str(value, buf, n);
//	break;
    case NEO4J_POINT2D_SIGNATURE:
	return neo4j_point2d_str(value, buf, n);
	break;
    case NEO4J_POINT3D_SIGNATURE:
	return neo4j_point3d_str(value, buf, n);
	break;
    default:
	hlen = snprintf(buf, n, "struct<0x%X>", v->signature);
	assert(hlen > 10);

	size_t l = (size_t)hlen;
	if ((l+1) < n)
	{
	    buf[l] = '(';
	}
	l++;

	l += list_str(buf+l, (l < n)? n-l : 0, v->fields, v->nfields);
	
	if ((l+1) < n)
	{
	    buf[l] = ')';
	}
	l++;
	if (n > 0)
	{
	    buf[minzu(n - 1, l)] = '\0';
	}
	return l;
	break;
    }
}


ssize_t neo4j_struct_fprint(const neo4j_value_t *value, FILE *stream)
{
    REQUIRE(value != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_STRUCT);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    int hlen;
    switch (v->signature)
    {
    case NEO4J_DATE_SIGNATURE:
	return neo4j_date_fprint(value, stream);
	break;
    case NEO4J_TIME_SIGNATURE:
	return neo4j_time_fprint(value, stream);
	break;
    case NEO4J_LOCALTIME_SIGNATURE:
	return neo4j_localtime_fprint(value, stream);
	break;
    case NEO4J_DATETIME_SIGNATURE:
	return neo4j_datetime_fprint(value, stream);
	break;
    case NEO4J_LEGACY_DATETIME_SIGNATURE:
	return neo4j_datetime_fprint(value, stream);
	break;
    case NEO4J_LOCALDATETIME_SIGNATURE:
	return neo4j_localdatetime_fprint(value, stream);
	break;
    case NEO4J_DURATION_SIGNATURE:
	return neo4j_duration_fprint(value, stream);
	break;
    case NEO4J_POINT2D_SIGNATURE:
	return neo4j_point2d_fprint(value, stream);
	break;
    case NEO4J_POINT3D_SIGNATURE:
	return neo4j_point3d_fprint(value, stream);
	break;
    default:
	hlen = fprintf(stream, "struct<0x%X>", v->signature);
	if (hlen < 0)
	{
	    return -1;
	}
	assert(hlen > 10);

	if (fputc('(', stream) == EOF)
	{
	    return -1;
	}
	size_t l = (size_t)hlen + 1;

	ssize_t ll =  list_fprint(v->fields, v->nfields, stream);
	if (ll < 0)
	{
	    return -1;
	}
	l += (size_t)ll;

	if (fputc(')', stream) == EOF)
	{
	    return -1;
	}
	return ++l;
	break;
    }
}

/* date */

ssize_t neo4j_date_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_DATE);
    struct tm *bdt;
    time_t ntmt = neo4j_date_time_t(*value);
    bdt = localtime( &ntmt );
    if (bdt == NULL) {
	return -1;
    }
    size_t l = strftime(buf, n, "%Y-%m-%d", (const struct tm *)bdt);
    if (l==0) {
	l = 10;
    }
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, " (");
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%ld", ntmt);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ")");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    return l;
}

ssize_t neo4j_date_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_DATE);
    char buf[BUFLEN];
    if (neo4j_date_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* time : prints time of day and UTC offset string, with time_t value in parens;
   ignores nanosec remainder */

ssize_t neo4j_time_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_TIME);
    char frac[15];
    struct tm *bdt;
    struct timespec *ntsp = neo4j_time_timespec(*value);
    long int offset = (long int) neo4j_time_secs_offset(*value);
    bdt = gmtime( &(ntsp->tv_sec) );
    if (bdt == NULL) {
	free(ntsp);
	return -1;
    }
    size_t l = strftime(buf, n, "%T", (const struct tm *)bdt);
    if (l==0) {
	l = 8;
    }
    if (ntsp->tv_nsec > 0) {
	snprintf(frac, 15, "%.9f", 1.0e-09*(double) ntsp->tv_nsec);
	l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%s", frac+1);
    }
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%+03ld00", offset/3600);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, " (");
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%ld", ntsp->tv_sec);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ")");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    free(ntsp);
    return l;
}

ssize_t neo4j_time_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_TIME);
    char buf[BUFLEN];
    if (neo4j_time_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* localtime */

ssize_t neo4j_localtime_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_LOCALTIME);
    char frac[15];
    struct tm *bdt;
    struct timespec *ntsp = neo4j_localtime_timespec(*value);
    bdt = gmtime( (const time_t *) &(ntsp->tv_sec) );
    if (bdt == NULL) {
	free(ntsp);
	return -1;
    }
    size_t l = strftime(buf, n, "%T", (const struct tm *)bdt);
    if (l<=0) {
	l = 8;
    }
    if (ntsp->tv_nsec > 0) {
	snprintf(frac, 15, "%.9f", 1.0e-09*(double) ntsp->tv_nsec);
	l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%s", frac+1);
    }
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, " (");
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%ld", ntsp->tv_sec);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ")");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    free(ntsp);
    return l;
}

ssize_t neo4j_localtime_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_LOCALTIME);
    char buf[BUFLEN];
    if (neo4j_localtime_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* datetime */

ssize_t neo4j_datetime_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_DATETIME);
    struct tm *bdt;
    struct timespec *ntsp = neo4j_datetime_timespec(*value);
    long int offset = (long int) neo4j_datetime_secs_offset(*value);
    char frac[15];
    ntsp->tv_sec += offset;
    bdt = gmtime( &(ntsp->tv_sec) );
    if (bdt == NULL) {
	free(ntsp);
	return -1;
    }
    size_t l = strftime(buf, n, "%FT%T", (const struct tm *)bdt);
    if (l<=0) {
	l = 19;
    }
    if (ntsp->tv_nsec > 0) {
	snprintf(frac, 15, "%.9f", 1.0e-09*(double) ntsp->tv_nsec);
	l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%s", frac+1);
    }
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%+03ld00", offset/3600);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, " (");
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%ld", ntsp->tv_sec-offset);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ")");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    free(ntsp);
    return l;
}

ssize_t neo4j_datetime_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_DATETIME);
    char buf[BUFLEN];
    if (neo4j_datetime_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* localdatetime */

ssize_t neo4j_localdatetime_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_LOCALDATETIME);
    struct tm *bdt;
    struct timespec *ntsp = neo4j_localdatetime_timespec(*value);
    char frac[15];
    bdt = gmtime( &(ntsp->tv_sec) );
    if (bdt == NULL) {
	free(ntsp);
	return -1;
    }
    size_t l = strftime(buf, n, "%FT%T", (const struct tm *)bdt);
    if (l<=0) {
	l = 19;
    }
    if (ntsp->tv_nsec > 0) {
	snprintf(frac, 15, "%.9f", 1.0e-09*(double) ntsp->tv_nsec);
	l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%s", frac+1);
    }
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, " (");
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%ld", ntsp->tv_sec);
    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ")");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    free(ntsp);
    return l;
}

ssize_t neo4j_localdatetime_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_LOCALDATETIME);
    char buf[BUFLEN];
    if (neo4j_localdatetime_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* duration */

ssize_t neo4j_duration_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_DURATION);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    long long months = neo4j_int_value(v->fields[0]);
    long long days = neo4j_int_value(v->fields[1]);
    long long secs = neo4j_int_value(v->fields[2]);
    long long nsecs = neo4j_int_value(v->fields[3]);
    long long years;
    long long hours;
    long long minutes = secs / 60;
    secs = secs % 60;
    hours = minutes / 60;
    minutes = minutes % 60;
    days = days + hours / 24;
    hours = hours % 24;
    months = months + days / 30;
    days = days % 30;
    years = months / 12;
    months = months % 12;

    size_t l = snprintf(buf, n, "P");
    if (l<=0) {
	return -1;
    }    
    if (years > 0) {
	l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%lldY", years);
    }
    if (months > 0) {
	l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%lldM", months);
    }
    if (days > 0) {
	l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%lldD", days);
    }
    if (hours > 0) {
    	l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "T%lldH", hours);
	if (minutes > 0) {
	    l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%lldM", minutes);
	}
	if (nsecs > 0) {
	    double S = (double) secs + 1.0e-09*(double)nsecs;
	    l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%.9fS", S);
	}
	else {
	    if (secs > 0) {
		l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%lldS", secs);
	    }
	}
    }
    else {
	if (minutes > 0) {
	    l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "T%lldM", minutes);
	    if (nsecs > 0) {
		double S = (double) secs + 1.0e-09*(double)nsecs;
		l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%.9fS", S);
	    }
	    else {
		if (secs > 0) {
		    l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "%lldS", secs);
		}
	    }
	}
	else {
	    if (nsecs > 0) {
		double S = (double) secs + 1.0e-09*(double)nsecs;
		l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "T%.9fS", S);
	    }
	    else {
		if (secs > 0) {
		    l += snprintf(buf ? buf+l : buf, (l<n)? n-l : 0, "T%lldS", secs);
		}
	    }
	}
    }
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    return l;
}

ssize_t neo4j_duration_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_DURATION);
    char buf[BUFLEN];
    if (neo4j_duration_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* point2d */
ssize_t neo4j_point2d_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_POINT2D);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    char numbuf[BUFLEN];
    size_t l = snprintf(buf, n, "[");
    if (l<=0) {
	return -1;
    }
    for (unsigned i = 0; i < 3; ++i)
    {
	if (neo4j_ntostring(v->fields[i], numbuf, BUFLEN-1)<0) {
	    return -1;
	}
	l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%s", numbuf);
	if (i<2)
	{
	    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ",");
	}
    }
    l += snprintf(buf? buf+l : buf, (l<n) ? n-l: 0, "]");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    return l;
}


ssize_t neo4j_point2d_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_POINT2D);
    char buf[BUFLEN];
    if (neo4j_point2d_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

/* point3d */

ssize_t neo4j_point3d_str(const neo4j_value_t *value, char *buf, size_t n)
{
    REQUIRE(value != NULL, -1);
    REQUIRE(n == 0 || buf != NULL, -1);
    assert(neo4j_type(*value) == NEO4J_POINT3D);
    const struct neo4j_struct *v = (const struct neo4j_struct *)value;
    char numbuf[BUFLEN];
    size_t l = snprintf(buf, n, "[");
    if (l<=0) {
	return -1;
    }
    for (unsigned i = 0; i < 4; ++i)
    {
	if (neo4j_ntostring(v->fields[i], numbuf, BUFLEN-1)<0) {
	    return -1;
	}
	l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, "%s", numbuf);
	if (i<3)
	{
	    l += snprintf(buf? buf+l : buf, (l<n)? n-l : 0, ",");
	}
    }
    l += snprintf(buf? buf+l : buf, (l<n) ? n-l: 0, "]");
    if (buf) {
	buf[minzu(n-1,l)] = '\0';
    }
    return l;
}

ssize_t neo4j_point3d_fprint(const neo4j_value_t *value, FILE *stream) {
    assert(neo4j_type(*value) == NEO4J_POINT3D);
    char buf[BUFLEN];
    if (neo4j_point3d_str(value, buf, BUFLEN-1) < 0) {
	return -1;
    }
    return fputs( (const char *)buf, stream );
}

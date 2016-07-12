/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */

#include <jose/b64.h>
#include <jose/jwk.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

static jose_jwk_resolver_t *resolvers;
static jose_jwk_generator_t *generators;

void
jose_jwk_register_resolver(jose_jwk_resolver_t *resolver)
{
    resolver->next = resolvers;
    resolvers = resolver;
}

void
jose_jwk_register_generator(jose_jwk_generator_t *generator)
{
    generator->next = generators;
    generators = generator;
}

bool
jose_jwk_generate(json_t *jwk)
{
    const char *kty = NULL;

    for (jose_jwk_resolver_t *r = resolvers; r; r = r->next) {
        if (!r->resolve(jwk))
            return false;
    }

    if (json_unpack(jwk, "{s:s}", "kty", &kty) == -1)
        return false;

    for (jose_jwk_generator_t *g = generators; g; g = g->next) {
        if (strcmp(g->kty, kty) == 0)
            return g->generate(jwk);
    }

    return false;
}

static bool
jwk_clean(json_t *jwk)
{
    const char *prv[] = { "k", "p", "d", "q", "dp", "dq", "qi", "oth", NULL };

    for (size_t i = 0; prv[i]; i++) {
        if (!json_object_get(jwk, prv[i]))
            continue;

        if (json_object_del(jwk, prv[i]) == -1)
            return false;
    }

    return true;
}

bool
jose_jwk_clean(json_t *jwk)
{
    json_t *keys = NULL;

    if (json_is_array(jwk))
        keys = jwk;
    else if (json_is_array(json_object_get(jwk, "keys")))
        keys = json_object_get(jwk, "keys");

    if (!keys)
        return jwk_clean(jwk);

    for (size_t i = 0; i < json_array_size(keys); i++) {
        if (!jwk_clean(json_array_get(keys, i)))
            return false;
    }

    return true;
}

static bool
uallowed(const json_t *jwk, const char *use)
{
    json_t *u = NULL;

    u = json_object_get(jwk, "use");
    if (!json_is_string(u))
        return true;

    return strcmp(json_string_value(u), use) == 0;
}

static bool
oallowed(const json_t *jwk, const char *op)
{
    json_t *ko = NULL;

    ko = json_object_get(jwk, "key_ops");
    if (!json_is_array(ko))
        return true;

    for (size_t i = 0; i < json_array_size(ko); i++) {
        json_t *o = NULL;

        o = json_array_get(ko, i);
        if (!json_is_string(o))
            continue;

        if (strcmp(json_string_value(o), op) == 0)
            return true;
    }

    return false;
}

bool
jose_jwk_allowed(const json_t *jwk, const char *use, const char *op)
{
    static const struct {
        const char *use;
        const char *op;
    } table[] = {
        { "sig", "sign" },
        { "sig", "verify" },
        { "enc", "encrypt" },
        { "enc", "decrypt" },
        { "enc", "wrapKey" },
        { "enc", "unwrapKey" },
        {}
    };

    for (size_t i = 0; !use && op && table[i].use; i++)
        if (strcmp(table[i].op, op) == 0)
            use = table[i].use;

    return use ? uallowed(jwk, use) : true && op ? oallowed(jwk, op) : true;
}

/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */

#include <cmd/jose.h>
#include <string.h>

#define START "{ \"kty\": \"EC\", \"crv\": \"P-256\""
#define KEYS ", \"x\": \"...\", \"y\": \"...\", \"d\": \"...\""
#define END " }"

static const struct option opts[] = {
    { "help",      no_argument,       .val = 'h' },

    { "output",    required_argument, .val = 'o' },
    {}
};

int
jcmd_gen(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    const char *out = "-";
    json_t *jwks = NULL;

    jwks = json_array();

    for (int c; (c = getopt_long(argc, argv, "ho:t:", opts, NULL)) >= 0; ) {
        json_t *jwk = NULL;

        switch (c) {
        case 'h': goto usage;
        case 'o': out = optarg; break;
        case 't':
            jwk = jcmd_load_json(optarg, optarg, NULL);
            if (json_array_append_new(jwks, jwk) == -1) {
                fprintf(stderr, "Invalid template: %s!\n", optarg);
                goto usage;
            }
            break;
        default:
            fprintf(stderr, "Invalid option: %c!\n", c);
            goto usage;
        }
    }

    if (json_array_size(jwks) == 0) {
        fprintf(stderr, "At least one JWK template is required!\n");
        goto usage;
    }

    for (size_t i = 0; i < json_array_size(jwks); i++) {
        if (!jose_jwk_generate(json_array_get(jwks, i))) {
            fprintf(stderr, "JWK generation failed for %s!\n", argv[i]);
            goto usage;
        }
    }

    if (json_array_size(jwks) == 1) {
        if (jcmd_dump_json(json_array_get(jwks, 0), out, NULL))
            ret = EXIT_SUCCESS;
        else
            fprintf(stderr, "Error dumping JWK!\n");
    } else {
        jwks = json_pack("{s:o}", "keys", jwks);
        if (!jwks)
            goto egress;

        if (jcmd_dump_json(jwks, out, NULL))
            ret = EXIT_SUCCESS;
        else
            fprintf(stderr, "Error dumping JWKSet!\n");
    }

egress:
    json_decref(jwks);
    return ret;

usage:
    fprintf(stderr,
"jose " GEN_USE
"\n"
"\nCreates a new, random JWK for each input JWK template."
"\n"
"\n    -t FILE, --template=FILE    JWK template (file)"
"\n    -t JSON, --template=JSON    JWK template (JSON)"
"\n    -t -,    --template=-       JWK template (stdin)"
"\n"
"\n    -o FILE, --output=FILE      JWK/JWKSet output (file)"
"\n    -o -,    --output=-         JWK/JWKSet output (stdout; default)"
"\n"
"\nThe simplest way to create a new key is to specify the algorithm that "
"\nwill be used with the key. For example:"
"\n"
"\n    $ jose gen -t '{\"alg\":\"A128GCM\"}'"
"\n    { \"kty\": \"oct\", \"k\": \"...\", \"alg\": \"A128GCM\","
"\n      \"use\": \"enc\", \"key_ops\": [\"encrypt\", \"decrypt\"] }"
"\n"
"\n    $ jose gen -t '{\"alg\":\"RSA1_5\"}'"
"\n    { \"kty\": \"RSA\", \"alg\": \"RSA1_5\", \"use\": \"enc\","
"\n      \"key_ops\": [\"wrapKey\", \"unwrapKey\"], ... }"
"\n"
"\nNote that when specifying an algorithm, default parameters such as "
"\n\"use\" and \"key_ops\" will be created if not specified."
"\n"
"\nAlternatively, key parameters can be specified directly:"
"\n"
"\n    $ jose gen -t '" START END "'"
"\n    " START KEYS END "\n"
"\n    $ jose gen -t '{\"kty\": \"oct\", \"bytes\": 32}'"
"\n    { \"kty\": \"oct\", \"k\": \"...\" }\n"
"\n    $ jose gen -t '{\"kty\": \"RSA\", \"bits\": 4096}'"
"\n    { \"kty\": \"RSA\", \"n\": \"...\", \"e\": \"...\", ... }"
"\n"
"\nIf there is more than one input JWK template, the output is a JWKSet:"
"\n"
"\n    $ jose gen -t '{\"alg\":\"A128GCM\"}' -t '{\"alg\":\"RSA1_5\"}'"
"\n    { \"keys\": [...] }"
"\n\n");
    goto egress;
}

/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */

#include <cmd/jose.h>
#include <string.h>

#define START "{ \"kty\": \"EC\", \"crv\": \"P-256\", "
#define PUB "\"x\": \"...\", \"y\": \"...\""
#define END " }"

static const struct option opts[] = {
    { "help",      no_argument,       .val = 'h' },

    { "input",     required_argument, .val = 'i' },
    { "output",    required_argument, .val = 'o' },
    {}
};

int
jcmd_pub(int argc, char *argv[])
{
    int ret = EXIT_FAILURE;
    const char *out = "-";
    json_t *jwk = NULL;

    for (int c; (c = getopt_long(argc, argv, "hi:o:", opts, NULL)) >= 0; ) {
        switch (c) {
        case 'h': goto usage;
        case 'o': out = optarg; break;
        case 'i':
            json_decref(jwk);
            jwk = jcmd_load_json(optarg, NULL, NULL);
            break;
        default:
            fprintf(stderr, "Invalid option: %c!\n", c);
            goto usage;
        }
    }

    if (!jwk) {
        fprintf(stderr, "Invalid JWK!\n");
        return EXIT_FAILURE;
    }

    if (!jose_jwk_clean(jwk)) {
        fprintf(stderr, "Error removing public keys!\n");
        goto egress;
    }

    if (!jcmd_dump_json(jwk, out, NULL)) {
        fprintf(stderr, "Error dumping JWK!\n");
        goto egress;
    }

    ret = EXIT_SUCCESS;

egress:
    json_decref(jwk);
    return ret;

usage:
    fprintf(stderr,
"jose " PUB_USE
"\n"
"\nCleans private keys from a JWK."
"\n"
"\n    -i FILE, --jwk=FILE       JWK or JWKSet input (file)"
"\n    -i -,    --jwk=-          JWK or JWKSet input (stdin)"
"\n"
"\n    -o FILE, --output=FILE    JWK or JWKSet output (file)"
"\n    -o -,    --output=-       JWK or JWKSet output (stdout; default)"
"\n"
"\nThis command simply takes a JWK(Set) as input and outputs a JWK(Set):"
"\n"
"\n    $ jose pub -i ec.jwk"
"\n    " START PUB END
"\n"
"\n    $ cat ec.jwk | jose pub -i-"
"\n    " START PUB END
"\n\n");
    goto egress;
}

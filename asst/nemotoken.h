#ifndef	__NEMO_TOKEN_H__
#define	__NEMO_TOKEN_H__

#include <nemoconfig.h>

#ifdef __cplusplus
NEMO_BEGIN_EXTERN_C
#endif

struct nemotoken {
	char *contents;
	int length;

	char **tokens;
	int ntokens;
};

extern struct nemotoken *nemotoken_create(const char *str, int length);
extern struct nemotoken *nemotoken_create_format(const char *fmt, ...);
extern void nemotoken_destroy(struct nemotoken *token);

extern int nemotoken_append(struct nemotoken *token, const char *str, int length);
extern int nemotoken_append_format(struct nemotoken *token, const char *fmt, ...);

extern void nemotoken_divide(struct nemotoken *token, char div);
extern void nemotoken_replace(struct nemotoken *token, char src, char dst);
extern int nemotoken_update(struct nemotoken *token);

extern int nemotoken_get_token_count(struct nemotoken *token);
extern char **nemotoken_get_tokens(struct nemotoken *token);
extern const char *nemotoken_get_token(struct nemotoken *token, int index);
extern const char *nemotoken_get_token_pair(struct nemotoken *token, const char *name);

static inline const char *nemotoken_get_string(struct nemotoken *token, int index, const char *value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return token->tokens[index];
}

static inline int nemotoken_get_integer(struct nemotoken *token, int index, int value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return strtoul(token->tokens[index], NULL, 10);
}

static inline double nemotoken_get_double(struct nemotoken *token, int index, double value)
{
	if (index >= token->ntokens || token->tokens[index] == NULL)
		return value;

	return strtod(token->tokens[index], NULL);
}

#ifdef __cplusplus
NEMO_END_EXTERN_C
#endif

#endif

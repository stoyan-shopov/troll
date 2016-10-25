#include "engine.h"

#include "sf-word-wizard.h"

extern void do_target_fetch(void);
extern void do_panic(void);

static struct word dict_base_dummy_word[1] = { MKWORD(0, 0, "", 0), };
static const struct word custom_dict[] = {
	/* override the sforth supplied engine reset */
	MKWORD(dict_base_dummy_word,	0,		"t@",		do_target_fetch),
	MKWORD(custom_dict,		__COUNTER__,	"panic",	do_panic),

}, * custom_dict_start = custom_dict + __COUNTER__;

static void sf_opt_cortexm0_init(void) __attribute__((constructor));
static void sf_opt_cortexm0_init(void)
{
	sf_merge_custom_dictionary(dict_base_dummy_word, custom_dict_start);
}

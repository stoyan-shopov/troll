/*
Copyright (c) 2019 Stoyan Shopov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "engine.h"

#include "sf-word-wizard.h"

void do_type_stack_nonempty(void);
void do_DW_OP_plus_typed(void);
void do_DW_OP_minus_typed(void);
void do_DW_OP_regval_type(void);
void do_DW_OP_const_type(void);
void do_DW_OP_deref_type(void);

static struct word dict_base_dummy_word[1] = { MKWORD(0, 0, "", 0), };
static const struct word custom_dict[] = {
	MKWORD(dict_base_dummy_word,	0,		"type-stack-nonempty?",		do_type_stack_nonempty),
	MKWORD(custom_dict,		__COUNTER__,	"DW_OP_plus_typed",		do_DW_OP_plus_typed),
	MKWORD(custom_dict,		__COUNTER__,	"DW_OP_minus_typed",		do_DW_OP_minus_typed),
	MKWORD(custom_dict,		__COUNTER__,	"DW_OP_regval_type",		do_DW_OP_regval_type),
	MKWORD(custom_dict,		__COUNTER__,	"DW_OP_const_type",		do_DW_OP_const_type),
	MKWORD(custom_dict,		__COUNTER__,	"DW_OP_deref_type",		do_DW_OP_deref_type),

}, * custom_dict_start = custom_dict + __COUNTER__;

static void sf_opt_dwarf_type_stack_init(void) __attribute__((constructor));
static void sf_opt_dwarf_type_stack_init(void)
{
	sf_merge_custom_dictionary(dict_base_dummy_word, custom_dict_start);
}

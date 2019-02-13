/*
Copyright (c) 2017 stoyan shopov

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

extern void do_entry_value_expression_block(void);
void do_DW_OP_piece(void);

static struct word dict_base_dummy_word[1] = { MKWORD(0, 0, "", 0), };
static const struct word custom_dict[] = {
	MKWORD(dict_base_dummy_word,	0,		"entry-value-expression-block",		do_entry_value_expression_block),
	MKWORD(custom_dict,		__COUNTER__,	"DW_OP_piece",				do_DW_OP_piece),

}, * custom_dict_start = custom_dict + __COUNTER__;

static void sf_opt_dwarf_evaluator(void) __attribute__((constructor));
static void sf_opt_dwarf_evaluator(void)
{
	sf_merge_custom_dictionary(dict_base_dummy_word, custom_dict_start);
}

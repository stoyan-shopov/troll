\ dwarf expression evaluator

.( compiling dwarf expression evaluator)cr
unused

: vector ( --)
	create 0 , does> @ execute ;
: >vector ( xt name --)
	bl word find 0= abort" vectored word not found"
	>body ! ;

\ after evaluation of a dwarf expression, this is true
\ if the computed value on the top of the stack is the object's
\ value, instead of its location address - i.e., the dwarf
\ expression was terminated by the 'DW_OP_stack_value' word
false value ?stack-value
0 value frame-base-value
vector frame-base-rule
: frame-base-defined ( -- frame-base-value)
	frame-base-value ;
: frame-base-undefined ( --)
	panic ;

0 value cfa-value
: DW_OP_call_frame_cfa ( -- cfa-value)
	cfa-value ;
: DW_OP_fbreg ( offset -- address)
	frame-base-rule + ;

: DW_OP_deref ( address -- value-at-target-address)
	t@ ;
: DW_OP_plus_uconst ( x y -- x + y)
	+ ;
: DW_OP_stack_value ( x -- x)
	depth 1 <> abort" bad stack"
	true to ?stack-value ;
	
: init-dwarf-evaluator ( --)
	['] frame-base-undefined ['] frame-base-rule >body !
	false to ?stack-value ;

.( dwarf expression evaluator compiled ) unused - . .( bytes used) cr

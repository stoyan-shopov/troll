false [if]
Copyright (c) 2016-2017, 2019 Stoyan Shopov

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
[then]

\ dwarf expression evaluator

.( compiling dwarf expression evaluator)cr
unused

: vector ( --)
	create 0 , does> @ execute ;
: >vector ( xt name --)
	bl word find 0= abort" vectored word not found"
	>body ! ;

0 value frame-base-value
vector frame-base-rule
: frame-base-defined ( -- frame-base-value)
	frame-base-value ;
: frame-base-undefined ( --)
	panic ;
	
\ these constants define the type of the computed expression:
\ a constant, a memory address, or a register number
1 constant expression-is-a-constant
2 constant expression-is-a-memory-address
3 constant expression-is-a-register-number
\ at the end of an expression evaluation, this variable
\ holds the computed expression type
0 value expression-value-type


0 value cfa-value

\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
\ \\\\\ DWARF entry value evaluation support
\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

\ dwarf expression evaluation stack - used for evaluating expressions containing 'DW_OP_entry_value' opcodes
\ the convention of this stack is 'empty - ascending'
\ the layout of a frame in this stack is as follows:
\	cell 0 - 'frame-base-rule'
\	cell 1 - 'frame-base-value'
\	cell 2 - 'cfa-value'
\	cell 3 - 'expression-value-type'
16 constant expression-stack-depth
4 cells constant expression-stack-frame-size
0 value expression-stack-sp
create expression-stack expression-stack-depth expression-stack-frame-size * allot
: expression-stack-reset ( --) 0 to expression-stack-sp ;
: >expression-stack ( --)
	expression-stack-sp expression-stack-depth = abort" dwarf expression stack overflow"
	expression-stack-sp expression-stack-frame-size * expression-stack +
	[ ' frame-base-rule >body ] literal @ over 0 cells + !
	frame-base-value over 1 cells + !
	cfa-value over 2 cells + !
	expression-value-type swap 3 cells + !
	expression-stack-sp 1+ to expression-stack-sp
	;
: expression-stack> ( --)
	expression-stack-sp 0= abort" dwarf expression stack underflow"
	expression-stack-sp 1- dup to expression-stack-sp
	expression-stack-frame-size * expression-stack +
	dup 0 cells + @ [ ' frame-base-rule >body ] literal !
	dup 1 cells + @ to frame-base-value
	dup 2 cells + @ to cfa-value
	3 cells + to expression-value-type
	;
\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

: DW_OP_call_frame_cfa ( -- cfa-value)
	cfa-value ;
: DW_OP_fbreg ( offset -- address)
	frame-base-rule + ;

: DW_OP_deref ( address -- value-at-target-address)
	t@ ;

: DW_OP_plus_uconst ( x y -- x + y)
	+ ;
	
: DW_OP_regx ( register-number -- register-number)
	expression-is-a-register-number to expression-value-type ;
	
: DW_OP_bregx ( offset register-number -- target-address)
	tr@ + ;
	
: DW_OP_dup ( x -- x x)
	dup ;
	
: DW_OP_swap ( x y -- y x)
	swap ;
	
: DW_OP_over ( x y -- x y x)
	over ;
	
: DW_OP_drop ( x -- )
	drop ;
	
: DW_OP_and ( x y -- z)
	and ;
	
: DW_OP_shl ( x y -- z)
	lshift ;

: DW_OP_or ( x y -- z)
        or ;

: DW_OP_xor ( x y -- z)
        xor ;

: DW_OP_plus ( x y -- z)
        + ;

: DW_OP_minus ( x y -- z)
        - ;

: DW_OP_ne ( x y -- z)
	<> ( normalize result)if 1 else 0 then ;
	
: DW_OP_lt ( x y -- t:if x < y|f:otherwise)
	< ;
	
: DW_OP_stack_value ( x -- x)
	depth 1 <> abort" bad stack"
	expression-is-a-constant to expression-value-type
	;

: DW_OP_addr ( x -- x)
        expression-is-a-memory-address to expression-value-type
        ;
	
: init-dwarf-evaluator ( --)
	['] frame-base-undefined ['] frame-base-rule >body !
	\ by default - make the dwarf expression value a memory address
	expression-is-a-memory-address to expression-value-type
	;

.( dwarf expression evaluator compiled, ) unused - . .( bytes used) cr

\ dwarf undwinder - !!! the target architecture register count must be on the stack prior to compiling this file !!!
\ usage:
\ compiling: push the target register count on the stack, and then compile this file
\ to unwind a frame: push all of the target registers on the stack, then invoke the
\ 'init-unwinder-round' word to initialize this unwinder round, then execute the
\ dwarf unwinding code recorded by the dwarf debug information producer; in case
\ of success, a 'true' constant will be returned on the stack; the unwound register
\ values are in the 'unwound-registers' cell array

.( compiling dwarf unwinder...)cr unused

swap constant register-count

\ helper words
\ cell array access
: [] ( index array -- cell-address)
	swap cells + ;

\ generic unwind parameters
-1 value code-alignment-factor
-1 value data-alignment-factor

\ data for the DW_CFA_def_cfa instruction
-1 value cfa-register-number
-1 value cfa-register-offset
\ the computed cfa value
-1 value cfa-value
0 value cfa-rule-xt
: invalid-unwind-rule ( --)
	panic ;

\ data tables for unwinding all frame registers
\ register unwind vector table
create unwind-xts register-count cells allot
: unwind-rule-same-value ( register-number --) drop ;

\ data table for unwinding registers by the DW_CFA_offset instruction
create cfa-offset-table register-count cells allot

\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
\ state stack and initial-cie-instructions words
\ these are needed by unwinding instructions
\ DW_CFA_restore, DW_CFA_remember_state, DW_CFA_restore_state
\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

create initial-cie-unwind-xts register-count cells allot
create initial-cie-cfa-offset-table register-count cells allot

1 constant state-stack-depth
0 value state-stack-pointer

: initial-cie-instructions-defined ( --)
	unwind-xts initial-cie-unwind-xts register-count cells move
	cfa-offset-table initial-cie-cfa-offset-table register-count cells move
	0 to state-stack-pointer
	;

\ the layout of a state-stack frame is:
\ 1 cell for the cfa register number for the DW_CFA_def_cfa instruction
\ 1 cell for the cfa register offset for the DW_CFA_def_cfa instruction
\ 1 cell for xfa rule execution token
\ 'register-count' cells for the register unwind execution tokens
\ 'register-count' cells for values for the DW_CFA_offset register unwind rules

\ an empty-ascending convention is used for the cell stack
1 cells 1 cells + 1 cells + register-count cells + register-count cells + constant state-stack-frame-size
create state-stack state-stack-frame-size state-stack-depth * allot

: DW_CFA_remember_state ( --)
	state-stack-pointer state-stack-depth = abort" state stack full; consider expanding the state stack"
	state-stack-pointer state-stack-frame-size * state-stack + >r
	cfa-register-number 0 r@ [] !
	cfa-register-offset 1 r@ [] !
	cfa-rule-xt 2 r@ [] !
	unwind-xts 3 r@ [] register-count cells move
	cfa-offset-table 3 register-count + r> [] register-count cells move
	state-stack-pointer 1+ to state-stack-pointer
	;
: DW_CFA_restore_state ( --)
	state-stack-pointer 0= abort" state stack empty"
	state-stack-pointer 1- to state-stack-pointer
	state-stack-pointer state-stack-frame-size * state-stack + >r
	0 r@ [] @ to cfa-register-number
	1 r@ [] @ to cfa-register-offset
	2 r@ [] @ to cfa-rule-xt
	3 r@ [] unwind-xts register-count cells move
	3 register-count + r> [] cfa-offset-table register-count cells move
	;

: DW_CFA_restore ( register-number --)
	>r
	r@ initial-cie-unwind-xts [] @ r@ unwind-xts [] !
	r@ initial-cie-cfa-offset-table [] @ r> cfa-offset-table [] !
	;


\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
\ end of state stack and initial-cie-instruction words
\ \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

\ unwind parameters	
-1 value current-address
-1 value unwind-address
create unwound-registers register-count cells allot

: cfa-rule-register-offset ( --)
	cfa-register-number unwound-registers [] @ cfa-register-offset + to cfa-value
	;

\ dwarf unwind instructions
: DW_CFA_def_cfa ( register-number offset --)
	to cfa-register-offset
	to cfa-register-number
	['] cfa-rule-register-offset to cfa-rule-xt
	;
: DW_CFA_def_cfa_offset ( offset --)
	to cfa-register-offset
	;
: DW_CFA_def_cfa_register ( register-number --)
	to cfa-register-number
	;

: DW_CFA_nop ;

: register-rule-cfa-offset ( register-number --)
	dup cfa-offset-table [] @ data-alignment-factor * cfa-value + t@ swap unwound-registers [] !
	;
: DW_CFA_offset ( register-number offset --)
	over cfa-offset-table [] !
	['] register-rule-cfa-offset swap unwind-xts [] !
	;

: unwinding-rules-defined ( -- t= unwinding successfull | f= unwinding failed)
	." end of unwinding"cr
	\ discard rest of unwinding code
	source nip >in !
	\ apply unwinding rules
	cfa-rule-xt execute
	register-count 0 do i i unwind-xts [] @ execute loop
	\ unwinding successful
	true
	;
: DW_CFA_advance_loc ( delta --)
	code-alignment-factor * current-address + to current-address
	current-address unwind-address u> if unwinding-rules-defined then
	;

\ !!! must be called before each run of the unwinder - push all of the current target
\ registers on the stack before calling
: init-unwinder-round ( n0 .. nk --) \ expects all target registers to be on the stack before invoking
	0 register-count 1- do i unwound-registers [] ! -1 +loop
	-1 to code-alignment-factor
	-1 to data-alignment-factor
	-1 to cfa-register-number
	-1 to cfa-register-offset
	-1 to cfa-value
	-1 to current-address
	-1 to unwind-address
	['] invalid-unwind-rule to cfa-rule-xt
	register-count 0 do ['] unwind-rule-same-value i unwind-xts [] ! loop
	;

: architectural-unwind ( -- t= unwinding successfull | f= unwinding failed)
	14 unwound-registers [] @
	." architectural unwind pc, cfa: " cfa-value .s drop
	dup $fffffff1 = swap $fffffff9 = or if
		." architectural unwind triggerred"cr
		cfa-value dup 0 cells + t@ 0 unwound-registers [] !
		dup 1 cells + t@ 1 unwound-registers [] !
		dup 2 cells + t@ 2 unwound-registers [] !
		dup 3 cells + t@ 3 unwound-registers [] !
		dup 4 cells + t@ 12 unwound-registers [] !
		dup 5 cells + t@ 14 unwound-registers [] !
		dup 6 cells + t@ 15 unwound-registers [] !
		8 cells + 13 unwound-registers [] !
		true
	else false then
	;

.( dwarf unwinder compiled, ) unused - . .( bytes used)cr

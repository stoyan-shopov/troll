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
	drop
	;
: DW_CFA_offset ( register-number offset --)
	over >r
	data-alignment-factor * swap cfa-offset-table [] !
	['] register-rule-cfa-offset r> unwind-xts [] !
	;

: unwinding-rules-defined ( --)
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
	current-address unwind-address > if unwinding-rules-defined then
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

.( dwarf unwinder compiled, ) unused - . .( bytes used)cr

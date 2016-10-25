#include "cortexm0.hxx"

/*


\ helper words
\ cell array access
: [] ( index array -- cell-address)
	swap cells + ;

\ generic unwind parameters
-1 value code-alignment-factor
-1 value data-alignment-factor
16 constant register-count

\ data for the DW_CFA_def_cfa instruction
-1 value cfa-register-number
-1 value cfa-register-offset
0 value cfa-rule-xt
: cfa-rule-register-offset ( --)
	;
: invalid-unwind-rule ( --)
	panic ;

\ data tables for unwinding all frame registers
\ register unwind vector table
create unwind-xts register-count cells allot
: unwind-rule-same-value ( --) ;

\ data table for unwinding registers by the DW_CFA_offset instruction
create cfa-offset-table register-count cells allot

\ unwind parameters	
-1 value unwind-address

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

: DW_CFA_offset ( register-number offset --)
	swap cfa-offset-table [] !
	;

\ !!! must be called before each run of the unwinder
: init-unwinder-round ( --)
	-1 to code-alignment-factor
	-1 to data-alignment-factor
	-1 to cfa-register-number
	-1 to cfa-register-offset
	['] invalid-unwind-rule to cfa-rule-xt
	register-count 0 do ['] unwind-rule-same-value i unwind-xts [] ! loop
	;
 */
CortexM0::CortexM0(Sforth * sforth)
{
	this->sforth = sforth;
}

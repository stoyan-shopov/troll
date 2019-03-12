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

/* The dwarf standard attaches type information to data elements in the dwarf stack.
 * The way dwarf expressions are being handled by the troll debug engine, is
 * to use a forth engine for evaluating dwarf expressions.
 *
 * There is no simple way, that I can see, for type information of data
 * elements in the dwarf stack to be handled by the forth engine.
 *
 * By scanning the dwarf 5 standard, I prepared these notes:
 *
- In section 2.5.1 - "General Operations", it is stated:
"Each element of the stack has a type and a value, and can represent a value of
any supported base type of the target machine. Instead of a base type, elements
can have a generic type, which is an integral type that has the size of an address
on the target machine and unspecified signedness. The value on the top of the
stack after "executing" the DWARF expression is taken to be the result (the
address of the object, the value of the array bound, the length of a dynamic
string, the desired value itself, and so on)."

- It seems that the only way to push a literal to the dwarf stack, that has
a type other than the 'generic' type, is to use the 'DW_OP_const_type' dwarf
opcode. For more details, see section 2.5.1.1 - "Literal Encodings"

- It seems that the only way that the value held in a register can be cast
to a type other than the 'generic' type, is to use the 'DW_OP_regval_type'
opcode. For more details, see section 2.5.1.2 - "Register Values"

- Other opcodes that place elements in the stack along with type identifiers
other than the 'generic' type, are: 'DW_OP_deref_type', 'DW_OP_xderef_type'.
The gcc compiler does issue 'DW_OP_deref_type' opcodes

- In section 2.5.1.4 - "Arithmetic and Logical Operations", it is stated:
"Operands of an operation with two operands must have the same type, either the same base type
or the generic type. The result of the operation which is pushed back has the
same type as the type of the operand(s)."

"Operations other than DW_OP_abs, DW_OP_div, DW_OP_minus,
DW_OP_mul, DW_OP_neg and DW_OP_plus require integral types of the
operand (either integral base type or the generic type). Operations do not cause
an exception on overflow."

- In section 2.5.1.5 - "Control Flow Operations", it looks like that data types
other that the 'generic' type must be taken in account when handling relational
operations 'DW_OP_le', 'DW_OP_ge', 'DW_OP_eq', 'DW_OP_lt', 'DW_OP_gt', 'DW_OP_ne'

- Explicit type conversion operations are described in section 2.5.1.6 - "Type Conversions".
These are operations 'DW_OP_convert', and 'DW_OP_reinterpret'

..................
TODO - finish these comments, and describe how the troll handles dwarf type information */


#ifndef DWARFTYPESTACK_HXX
#define DWARFTYPESTACK_HXX

struct dwarf_type_stack_entry
{
	int	dwarf_type_encoding;
	int	dwarf_type_bytesize;
	int	depth_of_data_node_in_data_stack;
	dwarf_type_stack_entry(int encoding, int bytesize, int depth) :
		dwarf_type_encoding(encoding), dwarf_type_bytesize(bytesize), depth_of_data_node_in_data_stack(depth) {}
};

#endif // DWARFTYPESTACK_HXX

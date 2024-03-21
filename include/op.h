#ifndef moc_op_h
#define moc_op_h

/*
	Opcode handling helpers for the MOSER VM.
*/

struct op_add
{
	template<class LHS, class RHS>
	inline void operator()( VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs + rhs ) );
	}
};

struct op_minus
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs - rhs ) );
	}
};

struct op_multiply
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs * rhs ) );
	}
};

struct op_divide
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs / rhs ) );
	}
};

struct op_less
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs < rhs ) );
	}
};

struct op_greater
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs > rhs ) );
	}
};

struct op_shift_left
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs << rhs ) );
	}
};

struct op_shift_right
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs >> rhs ) );
	}
};

struct op_modulo
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs % rhs ) );
	}
};

struct op_bin_and
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs & rhs ) );
	}
};

struct op_bin_or
{
	template<class LHS, class RHS>
	inline void operator()(VM& vm, LHS& lhs, RHS& rhs)
	{
		vm.push( Value( lhs | rhs ) );
	}
};


struct op_negate
{
	template<class RHS>
	inline void operator()(VM& vm, RHS& rhs)
	{
		vm.push( Value( -rhs ) );
	}
};

struct op_bin_negate
{
	template<class RHS>
	inline void operator()(VM& vm, RHS& rhs)
	{
		vm.push( Value( ~rhs ) );
	}
};

#define IS_NUM_OPERANDS ((IS_NUMBER(vm.peek(0)) || IS_INT(vm.peek(0)) ) && (IS_NUMBER(vm.peek(1)) || IS_INT(vm.peek(1)) ))


template<class OP>
inline void num_op(VM& vm)
{
	OP op;
	if (!IS_NUM_OPERANDS) throw RuntimeException("Operands must be numbers.");
	if ( IS_INT(vm.peek(0)) && IS_INT(vm.peek(1)))
	{
		ptrdiff_t b = vm.pop().as.integer; 
		ptrdiff_t a = vm.pop().as.integer; 
	    op(vm, a, b);
	}
	else
	{
        double b = IS_INT(vm.peek(0)) ? vm.pop().as.integer : vm.pop().as.number; 
        double a = IS_INT(vm.peek(0)) ? vm.pop().as.integer : vm.pop().as.number; 
		op(vm, a, b);
	}
}


template<class OP>
inline void int_op(VM& vm)
{
	OP op;
	if (!IS_NUM_OPERANDS) throw RuntimeException("Operands must be numbers.");
	ptrdiff_t b = vm.pop().to_int();
	ptrdiff_t a = vm.pop().to_int(); 
	op(vm, a, b);
}


template<class OP>
inline void unary_op(VM& vm)
{
	OP op;
	if ( (!IS_INT(vm.peek(0))) && (!IS_NUMBER(vm.peek(0))) ) throw RuntimeException("Operands must be numbers.");
	ptrdiff_t a = vm.pop().to_int();
	op(vm, a);
}



#endif

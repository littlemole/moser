#include "vm.h"
#include "debug.h"
#include "object.h"
#include "foreign.h"
#include "lib.h"
#include "gc.h"
#include "ex.h"
#include "op.h"

#include <stdarg.h>
#include <sstream>
#include "serialize.h"

#ifdef _WIN32
#include <roapi.h>
#endif



Value& CallFrame::arg(VM& vm, int i) 
{ 
    return vm.stack[argBaseIndex+i]; 
}


VM::VM() : gc(*this)
{
    compiler = new Compiler(*this);

    stack.reserve(1024);
    frames.reserve(256);

    init_stdlib(*this);
}

VM::~VM()
{
#ifdef _WIN32
    if (coinit & CO_INIT_WINRT)
    {
        ::RoUninitialize();
    }
    else if (coinit & CO_INIT_COM)
    {
        ::CoUninitialize();
    }
#endif

}

void VM::freeObjects()
{
    gc.shutdown(true);

    gc.finalize();
    for( auto it = objects.rbegin(); it != objects.rend(); it++)
    {
        delete *it;
    }
    objects.clear();

    if(compiler) delete compiler;
}

InterpretResult VM::compile(const std::string& path, bool persist)
{
    ObjFunction* function = nullptr;

    std::string compiled_path = path + ".mbc";
    {
        std::string source = slurp(path.c_str());
        GC::Lock lock(*this);

        function = compiler->compile(path.c_str(),source.c_str());
        if (function == NULL) return InterpretResult::INTERPRET_COMPILE_ERROR;

        if(persist)
        {
            std::ostringstream oss;
            serialize(oss, *function);

            flush(compiled_path,oss.str());
        }
        push(function);
    }
    return InterpretResult::INTERPRET_OK;
}

InterpretResult VM::execute(const std::string& path)
{
    ObjFunction* function = nullptr;

    std::string compiled_path = path + ".mbc";

    long stime = get_mtime(path.c_str());
    long ctime = get_mtime(compiled_path.c_str());

    std::string compiled = slurp(compiled_path.c_str());
    if( compiled_path == path || (!compiled.empty() && stime <= ctime) )
    {
        GC::Lock lock(*this);

        std::istringstream iss(compiled);
        deserialize(*this, iss, &function);

        push(function);
    }
    else
    {
        compile(path);
    }

    // prepare closure to run
    {
        GC::Lock lock(*this);
        Value fun = pop();
        function = ::as<ObjFunction>(fun);
        if(!function) return InterpretResult::INTERPRET_COMPILE_ERROR;
        ObjClosure* closure = new ObjClosure(*this, function);
        push(closure);
        call(closure, 0);    
    }
    //unsused: Value result = 
    run();
    if(frames.empty()) return InterpretResult::INTERPRET_OK;

    InterpretResult r = frames.back().exitCode;
    return r;
}

InterpretResult VM::interpret(const std::string& bytes)
{
    ObjFunction* function = nullptr;

    size_t ip = 0;
    if(!compiler->function->chunk.code.empty())
    {
        ip = compiler->function->chunk.code.size();
    }
    {
        GC::Lock lock(*this);
        function = compiler->compile(".",bytes.c_str());
        if (function == NULL) return InterpretResult::INTERPRET_COMPILE_ERROR;
        push(function);
    }
    // prepare closure to run
    {
        GC::Lock lock(*this);
        Value fun = pop();
        function = ::as<ObjFunction>(fun);
        if(!function) return InterpretResult::INTERPRET_COMPILE_ERROR;
        ObjClosure* closure = new ObjClosure(*this, function);
        push(closure);
        call(closure, 0);    
    }
    if(ip)
        frames.back().ip = &frames.back().closure->function->chunk.code[ip];
    //unused: Value result = 
    run();
    pop();
    if(frames.empty()) return InterpretResult::INTERPRET_OK;

    InterpretResult r = frames.back().exitCode;
    return r;
}

InterpretResult VM::debug(const std::string& path)
{
    debugSession = true;
    stepping = true;
    return execute(path);
}

void VM::step()
{
    if(frames.empty()) return;
    CallFrame* frame = &frames.back();

    int offset = (int)(frame->ip - &frame->closure->function->chunk.code[0]);

    if(!stepping)
    {
        std::string file = frame->closure->function->chunk.filename;

        if(breakpoints.count(file) == 0) return;

        auto lines = breakpoints[file];

        int line = frame->closure->function->chunk.lines[offset];
        if(!lines.contains(line)) return;

        stepping = true;
    }

    disassembleInstruction(
        frame->closure->function->chunk,
        offset
    );        

    char line[1024];
    for (;;) 
    {
        printf("%s> ", frame->closure->function->chunk.filename.c_str() );

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            //break;
        }
        else 
        {
            std::string l(line);
            if(l.substr(0,3) == "src")
            {
                std::string name = "<script>";
                auto n = frame->closure->function->name();
                if(n) name = n->toString();
                disassembleChunk(frame->closure->function->chunk, name.c_str());
                continue;
            }
            if(l.substr(0,2) == "e ")
            {
                std::string exp = l.substr(2);
                stepping = false;
                this->eval(exp);
                stepping = true;
                continue;
            }
            if(l.substr(0,2) == "st")
            {
                if(!stack.empty())
                {
                    printf("      ");
                    for (Value* slot = &stack[0]; slot - &stack[0] < (int) stack.size(); slot++)
                    {
                        printf("[ ");
                        slot->print();
                        printf(" ]");
                    }
                    printf("\n");
                }
                continue;
            }
            if(l.substr(0,5) == "local")
            {
                if(!stack.empty())
                {
                    printf("      ");
                    Value* slot = &stack[frame->argBaseIndex];
                    for ( ; slot - &stack[0] < (int) stack.size(); slot++)
                    {
                        printf("[ ");
                        slot->print();
                        printf(" ]");
                    }
                    printf("\n");
                }
                continue;
            }
            if(l.substr(0,1) == "c")
            {
                stepping = false;
            }
            if(l.substr(0,2) == "i ")
            {
                auto v = split(l," ");
                if(v.size() != 2 ) continue;

                Value& value = stack[frame->argBaseIndex + atoi(v[1].c_str()) ];

                printf("%i | %s\r\n", (int)value.type, value.to_string().c_str());

                continue;
            }
            if(l.substr(0,3) == "mg ")
            {
                auto v = split(l," ");
                if(v.size() != 3 ) continue;

                Value value = stack[frame->argBaseIndex + atoi(v[1].c_str()) ];
                std::string g = trim(v[2]);

                globals[g] = value;

                printf("set new global %s \r\n", g.c_str());

                continue;
            }
            if(l.substr(0,2) == "b ")
            {
                std::string b = l.substr(2);
                auto v = split(b,":");
                if(v.size() != 2 ) continue;

                breakpoints[v[0]].insert(atoi(v[1].c_str()));
                continue;
            }
            if(l.substr(0,2) == "g ")
            {
                std::string g = trim(l.substr(2));
                printf("%s\r\n",g.c_str());
                if( globals.count(g) )
                {
                    printf("%s\r\n", globals[g].to_string().c_str());
                }
                continue;
            }
            if(l.substr(0,3) == "db ")
            {
                std::string b = l.substr(3);
                auto v = split(b,":");
                if(v.size() != 2 ) continue;

                if(breakpoints.count(v[0]))
                {
                    auto& lines = breakpoints[v[0]];
                    auto line = atoi(v[1].c_str());
                    if(lines.contains(line))
                    {
                        lines.erase(line);
                        if(lines.empty())
                        {
                            breakpoints.erase(v[0]);
                        }
                    }
                }
                continue;
            }
        }
        break;
    }

}

Value VM::eval(const std::string& src)
{
    if(src.empty()) return NIL_VAL;

    ObjFunction* function = nullptr;

	{
		GC::Lock lock(*this);
		Compiler compiler(*this, this->compiler,FunctionType::TYPE_FUNCTION);

		function = compiler.compile(compiler.filename.c_str(),src.c_str());
		push(function);
    }
    {
        GC::Lock lock(*this);
        ObjClosure* closure = nullptr;
        closure = new ObjClosure(*this,function);
        pop();
        push(closure);
        call(closure,0);
    }

    frames.back().returnToCallerOnReturn = true;
    Value r = run();
    if(!pendingEx.empty())
    {
        pendingEx.back().print();
        printf("unhandlex exception");
		return NIL_VAL;
    }
    pop();
    return r;
}


static Value meta(Value& value)
{
    if(!IS_OBJ(value)) return NIL_VAL;

    auto closure = ::as<ObjClosure>(value);
    if(closure)
    {
        auto f = closure->function;
        return f->metadata;
    }
    auto instance = ::as<ObjInstance>(value);
    if(instance)
    {
        if(!instance->klass) return NIL_VAL;
        return instance->klass->metadata;
    }
    auto method = ::as<ObjBoundMethod>(value);
    if(method)
    {
        auto closure2 = as<ObjClosure>(method->method);
        if(closure2)
        {
            return closure2->function->metadata;
        }
        auto p = as<ObjDecorator>(method->method);
        if(p)
        {
            return p->getProperty("target").as.obj->metadata;
        }
    }
    auto obj = ::as<Obj>(value);
    Value m = obj->metadata;
    return m;
}


int VM::unwind()
{
    CallFrame* frame = &frames.back();

    Value result = pop();
    closeUpvalues(&stack[frame->argBaseIndex]);
    if(frames.size() < 1)
    {
        return 0;
    }

    while( !stack.empty() && (int)stack.size()-1 != frame->argBaseIndex)
    {
        stack.pop_back();
    }
    stack.pop_back();
    push(result);

    frames.pop_back();
    return (int)frames.size();
}

void VM::concatenate() 
{
    Value rhs = pop();
    Value lhs = pop();

    std::string b = rhs.to_string();
    std::string a = lhs.to_string();
    std::string s = a + b;

    ObjString* result = new ObjString( *this, s.c_str(), s.size() );
    push(result);
}


Value VM::run() 
{
  try {

    if(frames.empty()) return NIL_VAL;

    CallFrame* frame = &frames.back();

    DEBUG_TRACE_EXECUTION_PREAMBLE

    for (;;) 
    {
        DEBUG_TRACE_EXECUTION_TRACE

       if(debugSession)
        {
            step();            
        }

        OpCode instruction;
        switch (instruction = static_cast<OpCode>(read_byte()) )
        {
            case OpCode::OP_CONSTANT: 
            {
                Value constant = read_constant();
                push(constant);
                break;
            }        
            case OpCode::OP_NIL: push(NIL_VAL); break;
            case OpCode::OP_TRUE: push(true); break;
            case OpCode::OP_FALSE: push(false); break;
            case OpCode::OP_POP: pop(); break;       
            case OpCode::OP_GET_LOCAL: 
            {
                uint16_t slot = read_short();
                push(frame->arg(*this, slot));
                break;
            }        
            case OpCode::OP_GET_LOCAL_ADDR: 
            {
                uint16_t slot = read_short();
                push(frame->arg(*this, slot).pointer(*this));
                break;
            }        
            case OpCode::OP_GET_META: 
            {
                Value v = pop();
                if(!IS_OBJ(v)) 
                {
                    push(NIL_VAL);
                    break;
                }
                push(meta(v));
                break;
            }        
            case OpCode::OP_SET_LOCAL: 
            {
                uint16_t slot = read_short();
                //unused: Value val = 
                peek(0);
                stack[frame->argBaseIndex+slot] = peek(0);
                break;
            }        
            case OpCode::OP_GET_UPVALUE: 
            {
                uint16_t slot = read_short();
                Value val = *(frame->closure->upvalues[slot]->location);
                push(val);
                break;
            }        
            case OpCode::OP_SET_UPVALUE: 
            {
                uint16_t slot = read_short();
                Value* val = frame->closure->upvalues[slot]->location;
                *val = peek(0);
                break;
            }        
            case OpCode::OP_GET_GLOBAL: 
            {
                ObjString* name = read_string();
                if(globals.count(name->toString()) == 0)
                {
                    return runtimeError("Undefined variable '%s'.", name->toString().c_str());
                }
                Value value = globals[name->toString()];
                push(value);
                break;
            }         
            case OpCode::OP_GET_GLOBAL_ADDR: 
            {
                ObjString* name = read_string();
                if(globals.count(name->toString()) == 0)
                {
                    return runtimeError("Undefined variable '%s'.", name->toString().c_str());
                }
                Value value = globals[name->toString()].pointer(*this);
                push(value);
                break;
            }         
            case OpCode::OP_DEFINE_GLOBAL: 
            {
                ObjString* name = read_string();
                globals[name->toString()] = peek(0);
                pop();
                break;
            }
            case OpCode::OP_SET_GLOBAL: 
            {
                ObjString* name = read_string();
                if(globals.count(name->toString()) == 0)
                {
                    return runtimeError("Undefined variable '%s'.", name->toString().c_str());
                }
                globals[name->toString()] = peek(0);
                break;
            }        
            case OpCode::OP_GET_SUPER: 
            {
                ObjString* name = read_string();
                ObjClass* superclass = as<ObjClass>(pop());

                ObjInstance* instance = as<ObjInstance>(peek(0));
                if(!instance)
                {
                    return runtimeError("Super called for non-instance '%s'.", name->toString().c_str());
                }

                Value bound = superclass->bindMethod(instance,name->toString());
                if(IS_NIL(bound))
                {
                    return runtimeError("Binding Super failed - method does not exits in superclass.");
                }
                pop(); // instance
                push(bound);
                break;
            }        
            case OpCode::OP_EQUAL: 
            {
                Value b = pop();
                Value a = pop();
                push(a.isEqual(b));
                break;
            }        
            case OpCode::OP_ISA: 
            {
                Value b = pop();
                Value a = pop(); // a isa b

                auto classA = ::as<ObjClass>(a);
                auto classB = ::as<ObjClass>(b);

                if(!classA)
                {
                    auto objA = ::as<ObjInstance>(a);
                    if(objA) classA = objA->klass;
                }
                if(!classB)
                {
                    auto objB = ::as<ObjInstance>(b);
                    if(objB) classB = objB->klass;
                }
                if(classA && classB)
                {
                    std::string aName = classA->name->toString();
                    std::string bName = classB->name->toString();

                    if(aName == bName)
                    {
                        push(true);
                        break;
                    }
                    ObjClass* base = classA->super;
                    bool found = false;
                    while(base)
                    {
                        std::string aName2 = base->name->toString();
                        if(aName2 == bName)
                        {
                            push(true);
                            found = true;
                            break;
                        }
                        base = base->super;
                    }
                    if(found) break;
                }
                push(false);
                break;
            }        
			case OpCode::OP_GREATER: num_op<op_greater>(*this); break;
			case OpCode::OP_LESS:    num_op<op_less>(*this); break;
            case OpCode::OP_ADD: 
            {
                VM& vm = *this;
                if(isa<ObjString>(peek(0)) && isa<ObjString>(peek(1)))
                {
                    concatenate();
                } 
                else if (IS_INT(peek(0)) && IS_INT(peek(1))) 
                {
                    ptrdiff_t b = pop().as.integer;
                    ptrdiff_t a = pop().as.integer;
                    push(a + b);
                } 
                else if(IS_NUM_OPERANDS)
                {
                    double b = IS_INT(peek(0)) ? pop().as.integer : pop().as.number;
                    double a = IS_INT(peek(0)) ? pop().as.integer : pop().as.number;
                    push(a + b);
                } 
                else 
                {
                    concatenate();
                }
                break;
            }        
            case OpCode::OP_SUBTRACT: num_op<op_minus>(*this); break;
            case OpCode::OP_MULTIPLY: num_op<op_multiply>(*this); break;
            case OpCode::OP_DIVIDE:
            {
                if(peek(0).to_int() == 0)
                {
                    return runtimeError("Division by zero."); 
                }
				num_op<op_divide>(*this); break;
            }
            case OpCode::OP_SHIFT_RIGHT: int_op<op_shift_right>(*this); break;
            case OpCode::OP_SHIFT_LEFT: int_op<op_shift_left>(*this); break;
            case OpCode::OP_MODULO: int_op<op_modulo>(*this); break;
            case OpCode::OP_BIN_AND:  int_op<op_bin_and>(*this); break;
            case OpCode::OP_BIN_OR: int_op<op_bin_or>(*this); break;
            case OpCode::OP_NOT: 
            {
                push(pop().isFalsey());
                break;
            }
            case OpCode::OP_NEGATE: 
            {
                if ( (!IS_NUMBER(peek(0))) && (!IS_INT(peek(0))) && (!IS_BOOL(peek(0))) ) 
                {
                    return runtimeError("Operand must be a number or bool.");
                }
                if (IS_INT(peek(0)) )             
                {
                    push(ptrdiff_t(-pop().as.integer));
                }
                else if (IS_BOOL(peek(0)) )             
                {
                    push(bool(!pop().as.boolean));
                }
                else
                {
                    push(double(-pop().as.number));
                }
                break;
            }

            case OpCode::OP_BIN_NEGATE: 
            {
                unary_op<op_bin_negate>(*this);
                break;
            }

            case OpCode::OP_PRINT: 
            {
                Value v = pop();
                v.print();
                printf("\n");
                break;
            }        
            case OpCode::OP_JUMP: 
            {
                uint16_t offset = read_short();
                frame->ip += offset;
                break;
            }        
            case OpCode::OP_JUMP_IF_FALSE: 
            {
                uint16_t offset = read_short();
                if (peek(0).isFalsey()) frame->ip += offset;
                break;
            }        
            case OpCode::OP_LOOP: 
            {
                uint16_t offset = read_short();
                frame->ip -= offset;
                break;
            }        
            case OpCode::OP_TRY_BEGIN:
            {
                int offset = read_short();
                int finalizer = read_short();
                ptrdiff_t cur = frame->ip - &frame->closure->function->chunk.code[0];
                ptrdiff_t try_offset = offset == 0xFFFF ? 0 : offset + cur;
                ptrdiff_t fin_offset = finalizer == 0xFFFF ? 0 : finalizer + cur;
                exHandlers.push_back(ExceptionHandler{ frames.size()-1, try_offset, fin_offset });
                break; 
            }
            case OpCode::OP_TRY_END:
            {
                exHandlers.pop_back();
                break;
            }
            case OpCode::OP_THROW:
            {
                bool unhandledEx = doThrow();
                if(unhandledEx)
                {
                    return runtimeError("unhandled exception\n");
                }
                frame = &frames.back();
                break;
            }
            case OpCode::OP_FINALLY:
            {
                if(!pendingEx.empty())
                {
                    push(pendingEx.back());
                    pendingEx.clear();
                    bool unHandledEx = doThrow();
                    if(unHandledEx)
                    {
                        return runtimeError("unhandled exception\n");
                    }
                    frame = &frames.back();
                }
                if(!pendingRet.empty())
                {
                    push(pendingRet.back());
                    pendingRet.clear();
                    Value r;
                    bool bailOut = doReturn(r);
                    frame = &frames.back();
                    if(bailOut) return r;
                }
                break;
            }
            case OpCode::OP_RETURN: 
            {
                Value r = NIL_VAL;

                bool bailOut = doReturn(r);

                if(bailOut)
                {
                    return r;
                } 

                frame = &frames.back();

                break;
            }
            case OpCode::OP_CALL: 
            {
                int argCount = read_byte();
                Value callee = peek(argCount);
                if(!IS_OBJ(callee))
                {
                    return runtimeError("non-callable called as function. '%s'.", callee.to_string().c_str());
                }
                callee.as.obj->callValue(argCount);

                if(frames.empty()) return peek(0);
                frame = &frames.back();

                break;
            }        
            case OpCode::OP_INVOKE: 
            {
                ObjString* method = read_string();
                int argCount = read_byte();

                Value receiver = peek(argCount);
                if (!IS_OBJ(receiver)) 
                {
                    frame->exitCode = InterpretResult::INTERPRET_RUNTIME_ERROR;
                    return NIL_VAL;
                }

                auto m = as<Methodable>(receiver);
                if(m)
                {
                    bool r = m->invokeMethod(method->toString(), argCount);
                    if(!r)
                    {
                        return runtimeError("invoke method %s failed.", method->toString().c_str());
                    }
                }
                if(frames.empty()) return peek(0);
                frame = &frames.back();
                break;
            }           
            case OpCode::OP_CLOSURE: 
            {
                ObjFunction* function = as<ObjFunction>(read_constant());
                ObjClosure* closure = new ObjClosure(*this,function);
                push(closure);

                for (size_t i = 0; i < closure->upvalues.size(); i++) 
                {
                    uint8_t isLocal = read_byte();
                    uint16_t index = read_short();
                    if (isLocal) 
                    {
                        closure->upvalues[i] = captureUpvalue(&stack[frame->argBaseIndex + index]);
                    } 
                    else 
                    {
                        closure->upvalues[i] = frame->closure->upvalues[index];
                    }                
                }            
                break;
            }      
            case OpCode::OP_CLOSE_UPVALUE:
                closeUpvalues( &stack[stack.size()-1] );
                pop();
                break;       
            case OpCode::OP_CLASS:
                push(new ObjClass(*this,read_string()));
                break;       
            case OpCode::OP_DUP:
            {
                push(peek(0));
                break;
            }
            case OpCode::OP_DECORATOR:
            {
                Value target = pop();
                Value proxy = pop();

                auto p = new ObjDecorator(*this,proxy.as.obj,target.as.obj);
                push(p);
                break;
            }
            case OpCode::OP_SET_META:
            {
                Value val = pop(); // ??? peek(0) ?
                Value meta = read_constant();
                auto obj = ::as<Obj>(val);
                if(obj)
                {
                    obj->metadata = meta;
                }
                break;      
            } 
            case OpCode::OP_GET_PROPERTY: 
            {
                ObjString* name = read_string();
                Propertyable* prop = as<Propertyable>(peek(0));

                if (!prop) 
                {
                    return runtimeError("Only instances have properties.");
                }            

                Value val = prop->getProperty(name->toString());
                pop(); //instance
                push(val);

                break;
            }                 
            case OpCode::OP_SET_PROPERTY: 
            {
                ObjClass* clazz = as<ObjClass>(peek(1));
                if(clazz)
                {
                    clazz->setProperty(read_string()->toString(),peek(0));
                    Value value = pop();
                    pop();
                    push(value);
                    break;
                }
                ObjInstance* instance = as<ObjInstance>(peek(1));
                if (!instance) 
                {
                    return runtimeError("Only instances have fields.");
                }

                ObjRecord* rec = as<ObjRecord>(instance);
                if(rec)
                {
                    rec->setProperty(read_string()->toString(),  peek(0));
                    Value value = pop();
                    pop();
                    push(value);
                    break;
                }

                ObjMap* map = dynamic_cast<ObjMap*>(instance);
                if(map)
                {
                    map->setProperty(read_string()->toString(),peek(0));
                    Value value = pop();
                    pop();
                    push(value);
                    break;
                }

                instance->setProperty(read_string()->toString(),peek(0));
                Value value = pop();
                pop();
                push(value);
                break;
            }        
            case OpCode::OP_INHERIT: 
            {
                Value superclass = peek(1);
                if (!isa<ObjClass>(superclass)) 
                {
                    return runtimeError("Superclass must be a class.");
                }

                auto subclass = as<ObjClass>(peek(0));
                subclass->inherit(as<ObjClass>(superclass));
                pop(); // Subclass.

                break;
            }        
            case OpCode::OP_METHOD:
                defineMethod(read_string());
                break;        
            case OpCode::OP_STATIC_METHOD:
                defineStaticMethod(read_string());
                break;        
            case OpCode::OP_SETTER:
                defineSetter(read_string());
                break;        
            case OpCode::OP_GETTER:
                defineGetter(read_string());
                break;        
            case OpCode::OP_ARRAY_GET:
            {
                Value index = pop();
                Value that = peek(0);

                Indexable* array = as<Indexable>(that);
                if(array)
                {
                    Value val = array->index(index.as.integer);
                    pop();
                    push(val);
                    break;
                }
                pop();
                push(NIL_VAL);
                break;
            }
            case OpCode::OP_ARRAY_SLICE:
            {
                Value slice = pop();
                Value index = pop();
                Value that = peek(0);

                Indexable* array = as<Indexable>(that);
                if(array)
                {
                    Value val = array->slice(index.as.integer,slice.as.integer);
                    pop();
                    push(val);
                    break;
                }
                pop();
                push(NIL_VAL);
                break;
            }
            case OpCode::OP_ARRAY_SET:
            {
                Value value = pop();
                Value index = pop();
                Value that = pop();

                Indexable* array = as<Indexable>(that);
                if(array)
                {
                    array->index(index.as.integer, value);
                }
                push(NIL_VAL);
                break;
            }
            case OpCode::OP_ARRAY_INIT:
            {
                uint16_t n = read_short();
                ObjArray* array = new ObjArray(*this);
                std::vector<Value> v;
                for( int i = 0; i < n; i++)
                {
                    v.push_back(pop());
                }
                std::reverse(v.begin(),v.end());
                for( auto& it : v)
                {
                    array->add(it);
                }
                push(array);
                break;
            }
            case OpCode::OP_DELETE: 
            {
                Value that = pop();
                Value key = pop();

                if(IS_OBJ(that))
                {
                    auto map = ::as<Propertyable>(that);
                    if(map)
                    {
                        map->deleteProperty(key.to_string());
                        break;
                    }
                }
                break;
            }
            case OpCode::OP_MAP_INIT:
            {
                uint16_t n = read_short();
                ObjMap* map = new ObjMap(*this);
                std::vector<Value> keys;
                std::vector<Value> values;
                for( int i = 0; i < n; i++)
                {
                    values.push_back(pop());
                    keys.push_back(pop());
                }
                for( size_t i = 0; i < keys.size(); i++)
                {
                    map->item( keys[i].to_string() ,values[i]);
                }
                push(map);
                break;
            }
            case OpCode::OP_MAP_GET:
            {
                Value key = pop();
                Value that = peek(0);

                Propertyable* map = as<Propertyable>(that);
                if(map)
                {
                    Value val = map->getProperty(key.to_string());
                    pop();
                    push(val);
                    break;
                }
                pop();
                push(NIL_VAL);
                break;
            }
            case OpCode::OP_MAP_SET:
            {
                Value value = peek(0);
                Value key = peek(1);
                Value that = peek(2);

                Propertyable* map = as<Propertyable>(that);
                if(map)
                {
                    map->setProperty(key.to_string(), value);
                }
                pop();
                pop();
                break;
            }
        }
        if (allocations > nextGC) 
        {
            gc.collectGarbage();
        }    
        else
        {
    #ifdef DEBUG_STRESS_GC
            GC::collectGarbage();
    #endif            
        }

    }
  }
  catch(RuntimeException& ex)
  {
	return runtimeError(ex.what());
  }
#undef READ_BYTE
#undef READ_SHORT
#undef READ_CONSTANT
#undef BINARY_OP
#undef READ_STRING
}


bool VM::call(ObjClosure* closure, int argCount) 
{
    if (argCount < closure->function->arity()) 
    {
        runtimeError("Expected %d arguments but got %d.", closure->function->arity(), argCount);
        exit(64);
    }

    if (frames.size() > 256) 
    {
        runtimeError("Stack overflow.");
        exit(27);
    }    

    auto f =  CallFrame{ 
        closure, 
        &closure->function->chunk.code[0],
        (int)(stack.size() - argCount - 1)
    };

    if(argCount > closure->function->arity())
    {
        for( int i = closure->function->arity(); i <argCount; i++ )
        {
            int index = (int) stack.size() -argCount + i;
            f.varargs.push_back(stack[index]);
        }
        for( int i = closure->function->arity(); i <argCount; i++ )
        {
            pop();
        }
    }

    frames.push_back( f );

    return true;
}

// return true to exit run loop
bool VM::doReturn(Value& result)
{
    bool bailOut = false;
    CallFrame* frame = &frames.back();

    if(!pendingEx.empty())
    {
        // cannot return, rethrow
        push(pendingEx.back());
        pendingEx.pop_back();
        bool r = doThrow();
        return r;
    }

    // check for finalizers to run

    if(!exHandlers.empty() && exHandlers.back().frameIndex == frames.size()-1)
    {
        pendingRet.clear();
        pendingRet.push_back(pop());
        exHandlers.back().offset = 0;
        frame->ip = &frame->closure->function->chunk.code[exHandlers.back().finalizer];
        exHandlers.back().finalizer = 0;
        return bailOut;
    }

    if(frame->returnToCallerOnReturn)
    {
        result =  peek(0);
        bailOut = true;
    }

    int n = unwind();
    if(n == 0)
    {
        bailOut = true;
    }
    pendingRet.clear();

    return bailOut;
}

// return true if unhandled exception
bool VM::doThrow()
{
    CallFrame* frame = &frames.back();

    if(exHandlers.empty())
    {
        Value ex = pop();
        ex.print();
        return true;
    }
    while( exHandlers.back().frameIndex != frames.size()-1)
    {
        int n = unwind();
        if( n == 0)
        {
            Value ex = pop();
            ex.print();
            return true;
        }

        frame = &frames.back();

        if(frame->returnToCallerOnReturn)
        {
            Value ex = pop();
            pendingEx.clear();
            pendingRet.clear();
            pendingEx.push_back(ex);
            return true;
        }
    }

    Value ex = pop();
    pendingEx.clear();
    pendingRet.clear();
    pendingEx.push_back(ex);


    if(! exHandlers.empty() && exHandlers.back().frameIndex == frames.size()-1)
    {
        ptrdiff_t offset = exHandlers.back().offset;
        ptrdiff_t finalizer = exHandlers.back().finalizer;

        if(offset)
        {
            push(pendingEx.back());
            pendingEx.clear();
            frame->ip = &frame->closure->function->chunk.code[offset];
            exHandlers.back().offset = 0;

            return false;
        }
        if(finalizer)
        {
            frame->ip = &frame->closure->function->chunk.code[finalizer];
            exHandlers.back().finalizer = 0;

            return false;
        }
        return false;
    }
    return true;
}


ObjUpvalue* VM::captureUpvalue(Value* local) 
{
    auto upvalue = openUpvalues.begin();

    while( upvalue != openUpvalues.end() && (*upvalue)->location > local)
    {
        upvalue++;
    }

    if (upvalue != openUpvalues.end() && (*upvalue)->location == local) 
    {
        return *upvalue;
    }    
    ObjUpvalue* createdUpvalue = new ObjUpvalue(*this,local);
    openUpvalues.insert(upvalue,createdUpvalue);
    return createdUpvalue;
}

void VM::closeUpvalues(Value* last) 
{
    while( !openUpvalues.empty() && openUpvalues.front()->location >= last)
    {
        ObjUpvalue* upvalue = openUpvalues.front();
        upvalue->closed = *(upvalue->location);
        upvalue->location = &upvalue->closed;
        openUpvalues.pop_front();
    }
}

void VM::defineMethod(ObjString* name) 
{
    Value method = peek(0);
    ObjClass* klass = as<ObjClass>(peek(1));
    klass->defineMethod(name,method);
    pop();
}

void VM::defineStaticMethod(ObjString* name) 
{
    Value method = peek(0);
    ObjClass* klass = as<ObjClass>(peek(1));
    klass->defineStaticMethod(name,method);
    pop();
}

void VM::defineSetter(ObjString* name) 
{
    Value method = peek(0);
    ObjClass* klass = as<ObjClass>(peek(1));
    klass->defineSetter(name,method);
    pop();
}

void VM::defineGetter(ObjString* name) 
{
    Value method = peek(0);
    ObjClass* klass = as<ObjClass>(peek(1));
    klass->defineGetter(name,method);
    pop();
}

void VM::resetStack()
{
    stack.clear();
}

void VM::push(Value value)
{   
    stack.push_back(value);
}

Value VM::pop()
{
    if(stack.empty()) return NIL_VAL;
    Value result = stack.back();
    stack.pop_back();
    return result;
}

Value VM::peek(int distance) 
{
    if(stack.empty()) return NIL_VAL;

    ptrdiff_t index = stack.size() -1 -distance;
    return stack[index];
}

void VM::defineNative(const char* name, NativeFn function) 
{
    push(new ObjString(*this,name, (int)strlen(name)));
    push(new ObjNativeFun(*this,function));

    globals[stack[0].to_string()] = stack[1];
    
    pop();
    pop();
}

void VM::defineGlobal(const char* name, Value value) 
{
    push(new ObjString(*this, name, (int)strlen(name)));
    push(value);

    globals[stack[0].to_string()] = stack[1];
    
    pop();
    pop();
}

Value VM::runtimeError( const char* format, ...) 
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    int s = (int)frames.size();
    for (int i = s - 1; i >= 0; i--) 
    {
        CallFrame* frame = &frames[i];
        ObjFunction* function = frame->closure->function;
        size_t instruction = frame->ip - &function->chunk.code[0] - 1;
        fprintf(stderr, "[line %d] in ", function->chunk.lines[instruction]);
        if (function->name() == NULL) 
        {
            fprintf(stderr, "script\n");
        } 
        else {
            fprintf(stderr, "%s()\n", function->name()->toString().c_str());
        }
    }    

    if(!frames.empty())
    {
        CallFrame* frame = &frames.back();
        frame->exitCode = InterpretResult::INTERPRET_RUNTIME_ERROR;
    }

    resetStack();
    return NIL_VAL;
}

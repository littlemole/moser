#ifndef moc_vm_h
#define moc_vm_h

#include "gc.h"
#include "compiler.h"

#include <list>
#include <set>

/*
    the MOSER Virtual Machine
*/

class CallFrame 
{
public:

    CallFrame() {};
    CallFrame(ObjClosure* c, uint8_t* p, int idx)
    : closure(c), ip(p), argBaseIndex(idx)
    {}

    ObjClosure* closure = nullptr;
    uint8_t* ip = nullptr;
    std::vector<Value> varargs;

	Value* basePointer = nullptr;
//    int argBaseIndex = 0;

    Value& arg(VM& vm, int i);
    
    InterpretResult exitCode = InterpretResult::INTERPRET_OK;
	bool returnToCallerOnReturn = false;

};

struct ExceptionHandler 
{
    size_t frameIndex = 0;
    ptrdiff_t offset = 0;
    ptrdiff_t finalizer = 0;
};

enum CO_INIT {
    CO_INIT_NONE  = 0,
    CO_INIT_COM   = 1,
    CO_INIT_OLE   = 2,
    CO_INIT_WINRT = 4
};

class VM
{
//friend class Obj;
//friend class GC;
public:
    VM();
    ~VM();

    GC gc;
    Compiler* compiler = nullptr;

    std::unordered_map<std::string,Value> globals;
    std::vector<std::string> cliArgs;

	std::vector<Value> stack;

private:

	std::vector<CallFrame> frames;
	std::vector<ExceptionHandler> exHandlers;
	std::list<Obj*> objects;
	std::list<ObjUpvalue*> openUpvalues;
	std::vector<Obj*>grayStack;
	std::vector<Value> pendingEx;
    std::vector<Value> pendingRet;
public:

    InterpretResult interpret(const std::string& source);
    InterpretResult compile(const std::string& source, bool persist = false);
    InterpretResult execute(const std::string& bytes);
    InterpretResult debug(const std::string& path);

	void markRoots(); 

	bool hasException();
	void printPendingException();
	void sweep();
	void finalize();

    void push(Value value);
    Value pop();
    Value& peek(int distance);
	void poke(int distance, const Value& v);

    Value eval(const std::string& src);

    bool call(ObjClosure* closure, int argCount);
    Value runtimeError( const char* format, ...);

    Value run();

    template<class ... Args>
    void make_obj(const std::string& name, Args ... args)
    {
        if (globals.count(name) == 0)
        {
            runtimeError("Undefined variable '%s'.", name.c_str());
            return;
        }
        Value clazz = globals[name];
        if(!IS_OBJ(clazz)) return;

        Obj* obj = clazz.as.obj;
        std::vector<Value> values{ args... };
        for(auto& v : values)
        {
            push(v);
        }
        GC::Lock lock(*this);
        obj->callValue(values.size());
        // obj now on top of stack
    }
    void freeObjects();

    template<class ... Args>
    Value execute( ObjClosure* f, Args ... args )
    {
        std::vector<Value> values{ args ... };
        for(auto& v : values)
        {
            push(v);
        }

        call(f, (int)values.size());
		frames.back().returnToCallerOnReturn = true;
        Value r = run();
        return r;
    }

    void defineNative(const char* name, NativeFn function);
    void defineGlobal(const char* name, Value value);    


	inline void inc_allocations(Obj* obj)
	{
		allocations++;
		objects.push_back(obj);
	}

	inline void decc_allocations()
	{
		allocations--;
	}

	inline void setNextGC(long n)
	{
		nextGC = allocations * n;
	}

	inline void push_obj_to_greystack(Obj* obj)
	{
		grayStack.push_back(obj);
	}

	inline void traceReferences() 
	{
		while(!grayStack.empty())
		{
			Obj* object = grayStack.back();
			grayStack.pop_back();
			gc.blackenObject(object);
		}
	}	

	inline CallFrame& top_frame() {
		return frames.back();
	}

	inline size_t stack_size() {
		return stack.size();
	}

	std::vector<std::string> include_path;

#ifdef _WIN32
    CO_INIT coinit = CO_INIT_NONE;
#endif

private:

    size_t allocations = 0;
    size_t nextGC = 10;

    bool debugSession = false;
    bool stepping = false;

    std::map<std::string,std::set<int>> breakpoints;

    void step();

    ObjUpvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);
    
    void defineMethod(ObjString* name);    
    void defineStaticMethod(ObjString* name);    
    void defineGetter(ObjString* name);    
    void defineSetter(ObjString* name);    

    void resetStack();

    void concatenate();
    int unwind();

    bool doReturn(Value& result);
    bool doThrow();

    inline uint8_t read_byte()
    {
        CallFrame* frame = &frames.back();
        return (*frame->ip++);
    }

    inline uint16_t read_short()
    {
        CallFrame* frame = &frames.back();
        frame->ip += 2;
        return (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]);
    }

    inline Value& read_constant()
    {
        CallFrame* frame = &frames.back();
        return  (frame->closure->function->chunk.constants[read_short()]);
    }

    inline ObjString* read_string()
    {
        const Value& v = read_constant();
        Obj* obj = v.as.obj;
        return  as<ObjString>(obj);
    }


};


#endif
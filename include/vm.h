#ifndef moc_vm_h
#define moc_vm_h

#include "gc.h"
#include "compiler.h"

#include <list>
#include <set>

/*
    the MOSER Virtual Machine
*/

class CallFrame;

struct ExceptionHandler 
{
	CallFrame* frame = nullptr;
    ptrdiff_t offset = 0;
    ptrdiff_t finalizer = 0;
};

class CallFrame 
{
friend class VM;
friend class ValueOrPtr;
public:

    CallFrame();
    CallFrame(ObjClosure* c, int argc, uint8_t* p);
	CallFrame(CallFrame&& rhs);

	CallFrame(const CallFrame& rhs) = delete;

	InterpretResult exitCode = InterpretResult::INTERPRET_OK;

    Value& arg( int i);
	Value arguments(VM& vm) ;
	void poke(int distance, const Value& v);

	bool returnToCallerOnReturn = false;

private:
	Obj* future_result = nullptr;
	Obj* future_prending = nullptr;
    ObjClosure* closure = nullptr;
	int argCount = 0;
    uint8_t* ip = nullptr;

    std::vector<Value> varargs;
	std::vector<Value> stack;
	std::vector<Value> pendingEx;
	std::vector<Value> pendingRet;
	std::vector<ExceptionHandler> exHandlers;
};


enum CO_INIT {
    CO_INIT_NONE  = 0,
    CO_INIT_COM   = 1,
    CO_INIT_OLE   = 2,
    CO_INIT_WINRT = 4
};

class VM
{
public:
    VM();
    ~VM();

    GC gc;
    Compiler* compiler = nullptr;

    std::unordered_map<std::string,Value> globals;
    std::vector<std::string> cliArgs;
	std::vector<std::string> include_path;

	InterpretResult interpret(const std::string& source);
	InterpretResult compile(const std::string& source, bool persist = false);
	InterpretResult execute(const std::string& bytes);
	InterpretResult debug(const std::string& path);

	inline CallFrame& top_frame() {
		return *(frames.back());
	}

	bool hasException();
	void printPendingException();

    void push(Value value);
    Value pop();
    Value& peek(int distance);

    Value run();
    int unwind();
    bool doThrow();
    bool doReturn(Value& result);

    Value runtimeError( const char* format, ...);

    Value eval(const std::string& src);

    bool call(ObjClosure* closure, int argCount);

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

		push(clazz);
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

    template<class ... Args>
    Value execute( ObjClosure* f, Args ... args )
    {
        std::vector<Value> values{ args ... };
        for(auto& v : values)
        {
            push(v);
        }

        call(f, (int)values.size());
		frames.back()->returnToCallerOnReturn = true;
        Value r = run();
        return r;
    }

	inline void co_resume(ObjCoro* coro, Value result, bool isSuccess)
	{
		pendingCoroutines.erase(coro->frame);
		frames.push_back(coro->frame);
		push(result);

		if(!isSuccess)
		{
			doThrow();
		}
	}

	// compiler interface

    void defineNative(const char* name, NativeFn function);
    void defineGlobal(const char* name, Value value);    


	// gc interface
	void markRoots(); 
	void sweep();
	void finalize();

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


#ifdef _WIN32
    CO_INIT coinit = CO_INIT_NONE;
#endif

private:

	std::list<ObjUpvalue*> openUpvalues;
	std::vector<Value> stack;
	std::list<Obj*> objects;
	std::vector<Obj*>grayStack;
	std::set<CallFrame*> pendingCoroutines;
	std::vector<CallFrame*> frames;

    size_t allocations = 0;
    size_t nextGC = 10;

    bool debugSession = false;
    bool stepping = false;

    std::map<std::string,std::set<int>> breakpoints;

    void step();

    ObjUpvalue* captureUpvalue(int index);
    void closeUpvalues(int index);
    
    void defineMethod(ObjString* name);    
    void defineStaticMethod(ObjString* name);    
    void defineGetter(ObjString* name);    
    void defineSetter(ObjString* name);    

//    void resetStack();

    void concatenate();

    inline uint8_t read_byte()
    {
        CallFrame* frame = frames.back();
        return (*frame->ip++);
    }

    inline uint16_t read_short()
    {
        CallFrame* frame = frames.back();
        frame->ip += 2;
        return (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]);
    }

    inline Value& read_constant()
    {
        CallFrame* frame = frames.back();
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
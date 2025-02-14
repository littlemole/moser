#ifndef moc_gc_h
#define moc_gc_h

#include "value.h"
#include "common.h"

#include <map>
#include <unordered_map>

// forwards
class VM;

// the MOSER GC

class GC
{
public:

    GC(VM& vm);

    bool shutdown();
    void shutdown(bool);
    bool locked();
    void lock(bool);
    void collectGarbage();
	void blackenObject(Obj* object);
	void traceReferences();

    void markValue(Value& value);
    void markObject(Obj* object);
    void markRoots();
    void markCompilerRoots();
    void markMap( std::unordered_map<std::string,Value>& map);
    void markArray(std::vector<Value>& array);
    void finalize();

    void pin(Obj*);
    void unpin(Obj*);

    class Lock
    {
    public:
        Lock(VM&);
        ~Lock();
    private:
        VM& vm;
    };

private:
    VM&  vm;
    bool shutdown_ = false;
    ptrdiff_t locked_ = 0;
    std::map<Obj*, ptrdiff_t> pinned_;
};

#endif
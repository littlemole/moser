#include "gc.h"
#include "vm.h"
#include "compiler.h"
#include "foreign.h"


#define GC_HEAP_GROW_FACTOR 2

extern Compiler* currentCompiler;


GC::GC(VM& v) : vm(v) {}

bool GC::shutdown() { return shutdown_; } 
void GC::shutdown(bool b) { shutdown_ = b; }

bool GC::locked() { return locked_ > 0; }
void GC::lock(bool b) { locked_ += b ? 1 : -1; }

GC::Lock::Lock(VM& v) : vm(v)
{
    vm.gc.lock(true);
}

GC::Lock::~Lock()
{
    vm.gc.lock(false);
}


void GC::pin(Obj* ptr)
{
    if(pinned_.count(ptr) == 0)
    {
        pinned_[ptr] = 0;        
    }
    pinned_[ptr] = pinned_[ptr] + 1; 
}

void GC::unpin(Obj* ptr)
{
    if(!pinned_.count(ptr))
    {
        return;
    }

    pinned_[ptr] = pinned_[ptr] - 1; 

    if(pinned_[ptr] > 0) return;

    pinned_.erase(ptr);
}


void GC::markObject(Obj* object) 
{
    if (object == nullptr) return;
    if (object->isMarked) return;    
    object->isMarked = true;

    markValue(object->metadata);

    vm.grayStack.push_back(object);
/*
#ifdef DEBUG_LOG_GC
    printf("%p mark ", (void*)object);
    Value(object).print();
    printf("\n");
#endif    
*/
}

void GC::markValue(Value& value) 
{
    if (IS_OBJ(value)) markObject(value.as.obj);
}

void GC::markCompilerRoots() 
{
    Compiler* compiler = currentCompiler;
    while (compiler != nullptr) 
    {
        markObject((Obj*)compiler->function);
        markObject(compiler->metadata.as.obj);
        compiler = compiler->enclosing;
    }
}

void GC::markMap( std::unordered_map<std::string,Value>& map)
{
    for( auto& it : map )
    {
        markValue(it.second);
    }
}

void GC::markRoots() 
{
    for ( auto& it : vm.stack )
    {
        markValue(it);
    }

    markMap(vm.globals);

    markCompilerRoots();

    for (size_t i = 0; i < vm.frames.size(); i++) 
    {
        markObject((Obj*)vm.frames[i].closure);
        markArray(vm.frames[i].varargs);
    }

    for( auto& it : vm.openUpvalues)
    {
        markObject( it );
    }    

    markArray(vm.pendingEx);
    markArray(vm.pendingRet);

    
    for(auto& p : pinned_)
    {
        markObject(p.first);
    }
}

void GC::markArray(std::vector<Value>& array) 
{
    for (size_t i = 0; i < array.size(); i++) 
    {
        markValue(array[i]);
    }
}

static void blackenObject(Obj* object) 
{
    /*
#ifdef DEBUG_LOG_GC
    printf("%p blacken ", (void*)object);
    Value(object).print();
    printf("\n");
#endif    
*/
    object->mark_gc();
    return;

}

static void traceReferences(VM& vm) 
{
    while(!vm.grayStack.empty())
    {
        Obj* object = vm.grayStack.back();
        vm.grayStack.pop_back();
        blackenObject(object);
    }
}

static void sweep(VM& vm) 
{
    auto object = vm.objects.begin();
    while ( object != vm.objects.end()) 
    {
        if ( (*object)->isMarked) 
        {
            (*object)->isMarked = false;
            object++;
        } 
        else 
        {
            Obj* unreached = *object;
            decltype(object) tmp = object;
            if(object != vm.objects.begin())
            {
                object--;

                vm.objects.erase(tmp);
                if(object != vm.objects.end())
                    object++;

                delete unreached;
            }
            else
            {
                vm.objects.erase(tmp);
                object = vm.objects.begin();
                delete unreached;
            }
        }
    }
}

void GC::finalize() 
{
    auto object = vm.objects.begin();
    while ( object != vm.objects.end()) 
    {
        if ( (*object)->isMarked) 
        {
            object++;
        } 
        else 
        {
            Obj* unreached = *object;

#ifdef DEBUG_LOG_GC
    printf("finalize ");
    Value(unreached).print();
#endif

            unreached->finalize();
            object++;
        }
    }
}

void GC::collectGarbage()
{
    if(locked_) return;

    GC::Lock guard(vm);

#ifdef DEBUG_LOG_GC
    printf("-- gc begin\n");
    size_t before = vm.allocations;
#endif

    markRoots();
    traceReferences(vm);
    finalize();
    sweep(vm);

    vm.nextGC = vm.allocations * GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
    printf("-- gc end\n");

    printf("   collected %zu objects (from %zu to %zu) next at %zu\n",
         before - vm.allocations, before, vm.allocations,
         vm.nextGC);
#endif

}
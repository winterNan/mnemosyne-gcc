#ifndef _MNEMOSYNE_HEAP_HEAP_HH
#define _MNEMOSYNE_HEAP_HEAP_HH

#include <alps/layers/pointer.hh>
#include <alps/layers/slabheap.hh>
#include <alps/layers/extentheap.hh>
#include <alps/layers/hybridheap.hh>

#include <mnemosyne.h>
#include <mtm.h>
#include <mtm_i.h>
#include <itm.h>

class Context {
public:
    Context(bool _do_v = true, bool _do_nv = true)
        : do_v(_do_v),
          do_nv(_do_nv)
    { 
        if (_ITM_inTransaction) {
            td = _ITM_getTransaction();
        } else {
            td = NULL;
        }
    }

    void load(uint8_t* src, uint8_t *dest, size_t size)
    {
        if (td) {
            // FIXME: for multithreading we need a version of _ITM_memcpy that 
            // performs just versioning without isolation
            _ITM_memcpyRtWt(dest, src, size);
        } else {
            memcpy(dest, src, size);
        }
    }

    void store(uint8_t* src, uint8_t *dest, size_t size)
    {
        if (td) {
            // FIXME: for multithreading we need a version of _ITM_memcpy that 
            // performs just versioning without isolation
            _ITM_memcpyRtWt(dest, src, size);
        } else {
            memcpy(dest, src, size);
        }
    }

    bool do_v;
    bool do_nv;

    _ITM_transaction * td;
};

typedef alps::SlabHeap<Context, alps::TPtr, alps::PPtr> SlabHeap_t;
typedef alps::ExtentHeap<Context, alps::TPtr, alps::PPtr> ExtentHeap_t;
typedef alps::HybridHeap<Context, alps::TPtr, alps::PPtr, SlabHeap_t, ExtentHeap_t> HybridHeap_t;

class Heap {
public:

    int init();

    void* pmalloc(size_t sz);
    void pmalloc_undo(void* ptr);
    void pfree_prepare(void* ptr);
    void pfree_commit(void* ptr);

private:
    ExtentHeap_t* exheap_;
    SlabHeap_t* slheap_;
    HybridHeap_t* hheap_;
};

#endif // _MNEMOSYNE_HEAP_HEAP_HH

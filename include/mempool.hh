#ifndef MEMPOOL_HH
#define MEMPOOL_HH

#include <cstdint>
#include <boost/intrusive/set.hpp>
#include <boost/intrusive/list.hpp>
#include <osv/mutex.h>
#include <arch.hh>

namespace memory {

const size_t page_size = 4096;

extern size_t phys_mem_size;

void* alloc_phys_contiguous_aligned(size_t sz, size_t align);
void free_phys_contiguous_aligned(void* p);

void* alloc_page();
void free_page(void* page);
void* alloc_huge_page(size_t bytes);
void free_huge_page(void *page, size_t bytes);
void setup_free_memory(void* start, size_t bytes);

void debug_memory_pool(size_t *total, size_t *contig);

namespace bi = boost::intrusive;

class pool {
public:
    explicit pool(unsigned size);
    ~pool();
    void* alloc();
    void free(void* object);
    unsigned get_size();
    static pool* from_object(void* object);
private:
    struct page_header;
    struct free_object;
private:
    void add_page();
    static page_header* to_header(free_object* object);

    // should get called with the preemption lock taken
    void free_same_cpu(free_object* obj, unsigned cpu_id);
    void free_different_cpu(free_object* obj, unsigned obj_cpu);
private:
    unsigned _size;

    struct page_header {
        pool* owner;
        unsigned cpu_id;
        unsigned nalloc;
        bi::list_member_hook<> free_link;
        free_object* local_free;  // free objects in this page
    };

    typedef bi::list<page_header,
                     bi::member_hook<page_header,
                                     bi::list_member_hook<>,
                                     &page_header::free_link>,
                     bi::constant_time_size<false>
                    > CACHELINE_ALIGNED free_list_type;
    // maintain a list of free pages percpu
    free_list_type _free[64];
public:
    static const size_t max_object_size;
    static const size_t min_object_size;
};

struct pool::free_object {
    free_object* next;
};

class malloc_pool : public pool {
public:
    malloc_pool();
private:
    static size_t compute_object_size(unsigned pos);
};

struct page_range {
    explicit page_range(size_t size);
    size_t size;
    boost::intrusive::set_member_hook<> member_hook;
};

void free_initial_memory_range(void* addr, size_t size);
void enable_debug_allocator();

extern bool tracker_enabled;

}

#endif

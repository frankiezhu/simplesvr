/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#ifndef __MEM_POOL_HEADER__
#define __MEM_POOL_HEADER__

#include <list>
using namespace std;

typedef enum mem_type
{
    MEM_TYPE_4K,
    MEM_TYPE_16K,
    MEM_TYPE_OTHER,
    MEM_TYPE_MAX
} mem_type_t;

#define MEM_MAGIC 0x13
struct mem_head
{
    unsigned char magic;
    unsigned char size;
    unsigned short magic2;
};

class MemPool
{
public:
    MemPool();
    ~MemPool();

    void *get(int size);
    void put(void *ptr);
    int init();

private:
    int read_mem_size(void *ptr);
    mem_type_t get_type(int size);
    void *alloc_mem(int size);
    void init_mem_head(void *ptr, int size);
    void free_mem_ptr(void *ptr);
    void *get_other_mem(int size);
    void put_other_mem(void *ptr, int size);

    void destroy();

private:
    list<void *> m_arr_pools[MEM_TYPE_MAX];
};

MemPool *glb_mempool();

#endif

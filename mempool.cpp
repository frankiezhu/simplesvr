/**
 ***All Rights Reserved
 *
 *author frankiezhu
 *date 2013-03-22
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include "mempool.h"
#include "log.h"

const static int default_mem_size[MEM_TYPE_MAX - 1] = {4 * 1024, 16 * 1024};
const static int default_mem_cnt[MEM_TYPE_MAX - 1] = {4 * 1024, 1 * 512};

MemPool::MemPool()
{
}

MemPool::~MemPool()
{
    this->destroy();
}

mem_type_t MemPool::get_type(int size)
{
    if (size > default_mem_size[MEM_TYPE_16K])
    {
        return MEM_TYPE_OTHER;
    }
    else if (size <= default_mem_size[MEM_TYPE_4K])
    {
        return MEM_TYPE_4K;
    }
    else
    {
        return MEM_TYPE_16K;
    }
}

int MemPool::read_mem_size(void *ptr)
{
    struct mem_head *head = (struct mem_head *)((char *)ptr - sizeof(struct mem_head));
    if (head->magic != MEM_MAGIC && head->magic2 != MEM_MAGIC)
    {
        logerr("memory corrupt, abort\n");
        raise(SIGABRT);
    }
    return head->size;
}

void *MemPool::alloc_mem(int size)
{
    void *mem = malloc(size + sizeof(struct mem_head));
    if (!mem)
    {
        logerr("malloc failed!\n");
        return NULL;
    }
    init_mem_head(mem, size);
    mem = (char *)mem + sizeof(struct mem_head);
    return mem;
}

void MemPool::init_mem_head(void *ptr, int size)
{
    struct mem_head *head = (struct mem_head *)(ptr);
    head->magic = MEM_MAGIC;
    head->magic2 = MEM_MAGIC;
    head->size = size;
}

void MemPool::free_mem_ptr(void *ptr)
{
    char *real_head = (char *)ptr - sizeof(struct mem_head);
    free(real_head);
}

void *MemPool::get_other_mem(int size)
{
    void *ptr = NULL;
    list<void *>::iterator it;
    for (it = m_arr_pools[MEM_TYPE_OTHER].begin();
            it != m_arr_pools[MEM_TYPE_OTHER].end(); ++it)
    {
        ptr = *it;
        if (read_mem_size(ptr) > size)
        {
            m_arr_pools[MEM_TYPE_OTHER].erase(it);
            return ptr;
        }
    }
    ptr = alloc_mem(size);
    return ptr;
}

void MemPool::put_other_mem(void *ptr, int size)
{
    list<void *>::iterator it;
    for (it = m_arr_pools[MEM_TYPE_OTHER].begin();
            it != m_arr_pools[MEM_TYPE_OTHER].end(); ++it)
    {
        if (read_mem_size(*it) >= size)
        {
            break;
        }
    }
    m_arr_pools[MEM_TYPE_OTHER].insert(it, ptr);
}


void *MemPool::get(int size)
{
    mem_type_t type = get_type(size);
    if (type == MEM_TYPE_OTHER)
    {
        return get_other_mem(size);
    }

    if (!m_arr_pools[type].empty())
    {
        void *ptr = m_arr_pools[type].back();
        m_arr_pools[type].pop_back();
        return ptr;
    }

    return alloc_mem(size);
}

void MemPool::put(void *ptr)
{
    int size = read_mem_size(ptr);
    mem_type_t type = get_type(size);
    if (type == MEM_TYPE_OTHER)
    {
        return put_other_mem(ptr, size);
    }
    m_arr_pools[type].push_back(ptr);
}

void MemPool::destroy()
{
    for (int i = 0; i < MEM_TYPE_MAX; i++)
    {
        while (!m_arr_pools[i].empty())
        {
            void *ptr = m_arr_pools[i].back();
            m_arr_pools[i].pop_back();
            free_mem_ptr(ptr);
        }
    }
    return;
}

int MemPool::init()
{
    for (int i = 0; i < MEM_TYPE_MAX - 1; i++)
    {
        int init_nr = default_mem_cnt[i];
        int obj_size = default_mem_size[i];
        int all_size = obj_size + sizeof(struct mem_head);
        int j = 0;
        while (j < init_nr)
        {
            char *ptr = (char *)malloc(all_size);
            if (!ptr)
            {
                logerr("alloc mem failed!\n");
                return -1;
            }
            init_mem_head(ptr, obj_size);
            ptr += sizeof(struct mem_head);
            put(ptr);
            ++j;
        }
    }
    return 0;
}


MemPool *glb_mempool()
{
    static MemPool s_instance;
    return &s_instance;
}





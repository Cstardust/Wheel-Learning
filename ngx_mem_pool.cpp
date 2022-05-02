#include"ngx_mem_pool.h"
#include <new>
#include<iostream>
//  ����size��С���ڴ��  ��ÿ��С�ڴ�block�Ĵ�С��Ϊsize��
ngx_pool_t* ngx_mem_pool::ngx_create_pool(size_t size)
{
    size = std::max((size_t)NGX_MIN_POOL_SIZE, size);
    pool_ = static_cast<ngx_pool_t*>(malloc(size));
    if (pool_ == nullptr) {
        return nullptr;
    }

    pool_->d.last = (u_char*)pool_ + sizeof(ngx_pool_t);
    pool_->d.end = (u_char*)pool_ + size;
    pool_->d.next = nullptr;
    pool_->d.failed = 0;

    size = size - sizeof(ngx_pool_t);
    pool_->max = (size < NGX_MAX_ALLOC_FROM_POOL) ? size : NGX_MAX_ALLOC_FROM_POOL;

    pool_->current = pool_;
    pool_->large = nullptr;
    pool_->cleanup = nullptr;
    return pool_;
}


//  �����ֽڶ��룬���ڴ������size��С���ڴ档�²��п��ܴӲ�ͳ�������ڴ�
void* ngx_mem_pool::ngx_palloc(size_t size)
{
    if (size <= pool_->max) {
        return ngx_palloc_small(size, 1);
    }
    return ngx_palloc_large(size);
}


//  ���Դ��ڴ�����ó�size��С�ڴ档�ڴ�ز�����Ӳ���ϵͳ���١�align=1��ζ����Ҫ�ڴ����
void* ngx_mem_pool::ngx_palloc_small(size_t size, ngx_uint_t align)
{
    u_char* m;
    ngx_pool_t* p;
    p = pool_->current;

    do {
        m = p->d.last;

        if (align) {
            m = static_cast<u_char*>(ngx_align_ptr(m, NGX_ALIGNMENT));  //  ??
        }

        if ((size_t)(p->d.end - m) >= size) {
            p->d.last = m + size;

            return m;
        }

        p = p->d.next;

    } while (p);

    return ngx_palloc_block(size);
}


//  �Ӳ���ϵͳmalloc�����µ�С���ڴ�ء�ngx_palloc_small����ngx_palloc_block��ngx_palloc_block�ײ����ngx_memalign����unixƽ̨��ngx_memalign����ngx_alloc�������Ƕ�malloc��ǳ��װ��
void* ngx_mem_pool :: ngx_palloc_block(size_t size)
{
    u_char* m;
    size_t       psize;
    ngx_pool_t* p, * new_m;

    psize = (size_t)(pool_->d.end - (u_char*)pool_);

    m = static_cast<u_char*>(malloc(psize));   //  ngx_alloc�ײ����malloc
    if (m == nullptr) {
        return nullptr;
    }

    new_m = (ngx_pool_t*)m;

    new_m->d.end = m + psize;
    new_m->d.next = NULL;
    new_m->d.failed = 0;

    m += sizeof(ngx_pool_data_t);
    m = static_cast<u_char*>(ngx_align_ptr(m, NGX_ALIGNMENT));
    new_m->d.last = m + size;

    for (p = pool_->current; p->d.next; p = p->d.next) {
        if (p->d.failed++ > 4) {
            pool_->current = p->d.next;
        }
    }

    p->d.next = new_m;

    return m;
}


//  �Ӳ���ϵͳmalloc���ٴ���ڴ棬���ص�ĳ�����еĴ��ͷ��Ϣ�¡������ٴ��ڴ������һ���������ڴ�blockͷ��Ϣ��
void* ngx_mem_pool::ngx_palloc_large(size_t size)
{
    void* p;
    ngx_uint_t         n;
    ngx_pool_large_t* large;

    p = malloc(size);
    if (p == nullptr) {
        return nullptr;
    }

    n = 0;

    for (large = pool_->large; large; large = large->next) {
        if (large->alloc == nullptr) {
            large->alloc = p;
            return p;
        }

        if (n++ > 3) {
            break;
        }
    }

    large = static_cast<ngx_pool_large_t*>(ngx_palloc_small(sizeof(ngx_pool_large_t), 1));
    if (large == nullptr) {
        free(p);
        return nullptr;
    }

    large->alloc = p;
    large->next = pool_->large;
    pool_->large = large;

    return p;
}



//  �Ӳ�ͳmalloc����ڴ档ngx_palloc_large����ngx_alloc��ngx_alloc����malloc
// void* ngx_mem_pool::ngx_alloc(size_t size)
// {
//     void* p = malloc(size);
//     return p;
// }


//  �ͷŴ���ڴ�block��ngx���ṩ�ͷ�С���ڴ�Ľӿڡ�ԭ�������
void ngx_mem_pool::ngx_pfree(void* p)
{
    ngx_pool_large_t* l;
    for (l = pool_->large; l; l = l->next) {
        if (p == l->alloc) {
            free(l->alloc);
            l->alloc = nullptr;
            return ;
        }
    }
}




//  �����ֽڶ��룬���ڴ������size��С���ڴ�
void* ngx_mem_pool::ngx_pnalloc(size_t size)
{
    if (size <= pool_->max) {
        return ngx_palloc_small(size, 0);
    }
    return ngx_palloc_large(size);
}

//  ����ngx_palloc������ʼ��Ϊ0.
void* ngx_mem_pool :: ngx_pcalloc(size_t size)
{
    //  ���ڴ�������ڴ�
    void *p = ngx_palloc(size);
    if (p) {    //  ��0����
        ngx_memzero(p, size);
    }
    return p;
}


//  �ڴ����ú���
void ngx_mem_pool::ngx_reset_pool()
{
    //  �Լ��ӵģ��ͷ��ⲿ��Դ
    for (ngx_pool_cleanup_t* c = pool_->cleanup; c; c = c->next)
    {
        if (c->handler&&c->data) {
            c->handler(c->data);
            c->handler = nullptr;
            c->data = nullptr;
        }
    }

    //  �ͷŴ���ڴ�
    for (ngx_pool_large_t* l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    //  ����С���ڴ�
    ngx_pool_t* p = pool_;
    p->d.last = (u_char*)p + sizeof(ngx_pool_t);
    p->d.failed = 0;
    for (p = p->d.next; p; p = p->d.next)
    {
        p->d.last = (u_char*)p + sizeof(ngx_pool_data_t);
        p->d.failed = 0;
    }
    
    //for (p = pool_; p; p = p->d.next) {
    //    p->d.last = (u_char*)p + sizeof(ngx_pool_t);
    //    p->d.failed = 0;
    //}
    //  ����pool_��Ա
    pool_->current = pool_;
    pool_->large = nullptr;
}


//  �����ڴ��
void ngx_mem_pool::ngx_destroy_pool()
{
    ngx_pool_t* p, * n;
    ngx_pool_large_t* l;
    ngx_pool_cleanup_t* c;

    //  �ͷ��ⲿ��Դ
    for (c = pool_->cleanup; c; c = c->next) {
        if (c->handler) {
            c->handler(c->data);
        }
    }

    //  �ͷŴ���ڴ�
    for (l = pool_->large; l; l = l->next) {
        if (l->alloc) {
            free(l->alloc);
        }
    }

    //  �ͷ�С���ڴ�
    for (p = pool_, n = pool_->d.next; /* void */; p = n, n = n->d.next) {
        free(p);
        if (n == nullptr) {
            break;
        }
    }
}

//  �����ⲿ��Դ��Ϣͷ�����ӻص�������������
ngx_pool_cleanup_t* ngx_mem_pool::ngx_pool_cleanup_add(size_t size)
{
    ngx_pool_cleanup_t* c;

    c = static_cast<ngx_pool_cleanup_t*>(ngx_palloc(sizeof(ngx_pool_cleanup_t)));
    if (c == nullptr) {
        return nullptr;
    }

    if (size) {
        c->data = ngx_palloc(size);
        if (c->data == nullptr) {
            return nullptr;
        }
    }
    else {
        c->data = nullptr;
    }

    c->handler = nullptr;
    c->next = pool_->cleanup;

    pool_->cleanup = c;

    return c;
}


ngx_mem_pool::ngx_mem_pool(size_t size)
{
    pool_ = ngx_create_pool(size);
    if (pool_ == nullptr) {
        throw std::bad_alloc();
    }
}

ngx_mem_pool::~ngx_mem_pool()
{
    std::cout << "~ngx_mem_pool" << std::endl;
    ngx_destroy_pool();
}
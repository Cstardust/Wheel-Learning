
#include<cstdio>
#include<cstdlib>
#include<iostream>
#include"ngx_mem_pool.h"
#include <new>

typedef struct Data stData;
struct Data
{
    //  ���ָ��
    char* ptr;
    FILE* pfile;
    char* ptr2;
};


typedef struct yData yData;
struct yData
{
    char* ptr;
    int x;
    int y;
};



void self_handler(void* p)
{
    printf("self_handler\n");
    stData* q = (stData*)p;
    printf("free ptr mem!\n");
    free(q->ptr);
    printf("free ptr mem!\n");
    free(q->ptr2);
    printf("close file!\n");
    fclose(q->pfile);
}


void self_handler_02(void* p)
{
    char* q = (char*)p;
    printf("self_handler_02\n");
    printf("free ptr mem!\n");
    free(q);
}



int main()
{
    //  1. ngx_create_pool ���ڴ��
        //  ��һ���ڴ�Block��������������ngx_pool_t
        // ngx_pool_t::max = min(512 - sizeof(ngx_pool_t) , 4095)     
        ngx_mem_pool mem_pool(512);
    //  2. С���ڴ��Լ��ⲿ��Դ
        //  ���ڴ������С���ڴ�
        stData* p1 = static_cast<stData*>(mem_pool.ngx_palloc(sizeof(stData))); // ��С���ڴ�ط����
        if (p1 == nullptr)
        {
            printf("ngx_palloc %ld bytes fail\n", sizeof(stData));
            return -1;
        }
        //  С���ڴ汣���ָ��������ⲿ��Դ
        p1->ptr = static_cast<char*>(malloc(12));
        strcpy(p1->ptr, "hello world");
        p1->pfile = fopen("data.txt", "w");
        p1->ptr2 = static_cast<char*>(malloc(20));
        strcpy(p1->ptr2, "goodbye world");
        //  Ԥ�ûص����������ͷ��ⲿ��Դ
        ngx_pool_cleanup_t* c1 = mem_pool.ngx_pool_cleanup_add(sizeof(stData));     //  �����ڴ棬����handler����
        c1->handler = self_handler;
        memcpy(c1->data, p1, sizeof(stData));                                      //  �û�ֻ���𿽱��������ⲿ��Դ����һ�µ�c1->data����p1ָ��ָ����������ֽڿ�����c->dataָ����ڴ�


    //  3. ����ڴ��Լ��ⲿ��Դ
        //  ���ڴ���������ڴ�
        yData* p2 = static_cast<yData*>(mem_pool.ngx_palloc(512)); // �Ӵ���ڴ�ط����
        if (p2 == nullptr)
        {
            printf("ngx_palloc 512 bytes fail...");
            return -1;
        }
        //  �ⲿ��Դ
        p2->ptr = static_cast<char*>(malloc(12));
        strcpy(p2->ptr, "hello world");

        //  Ԥ�ûص����������ͷ��ⲿ��Դ
        ngx_pool_cleanup_t* c2 = mem_pool.ngx_pool_cleanup_add(0);                   //  �������ڴ棬ֱ����c->dataָ��Ҫ�ͷŵ��ڴ�
        c2->handler = self_handler_02;
        c2->data = p2->ptr;

        //  �����ڴ��
        //  mem_pool.ngx_reset_pool();
        //  std::cout << "reset over" << std::endl;

        //  �ͷ��ڴ��
        //   ~mem_pool() ����mem_pool.ngx_destroy_pool(); // 1.�������е�Ԥ�õ��������� 2.�ͷŴ���ڴ� 3.�ͷ�С���ڴ�������ڴ�

        return 0;
}
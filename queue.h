/**
 * @file queue.h
 * @brief 数据结构封装
 *
 * 1.可以进行重复初始化\n
 * 2.queue_element结构都根据自己的需要进行修改(一般都不需要)\n
 *  	typedef 自定义的结构 queue_element;
 * 3.
 * 4.
 *
 * @author tangfu - abctangfuqiang2008@163.com
 * @version 0.1
 * @date 2011-10-17
 */
#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "macro_helper.h"
#include <pthread.h>
#include <semaphore.h>
#include "atomic.h"

//extern queue_element;
typedef struct queue_element_t queue_element;
typedef struct queue_s queue_array;

#define QUEUE_OP_SUCCESS		0
#define QUEUE_OP_ERROR 		-1
#define QUEUE_FULL 			-2
#define QUEUE_EMPTY 			-3
typedef enum QUEUE_ACTION_TYPE_S {QUEUE_UNBLOCK = 0, QUEUE_BLOCK } QUEUE_TYPE;

struct queue_s {
        int ( *init )( queue_array *, int , int );
        void ( *reset )( queue_array * );
        int ( *resize )( queue_array *, int );
        int ( *get_size )( queue_array * );
        int ( *get_current_len )( queue_array * );
        int ( *in_queue )( queue_array *, void * , QUEUE_TYPE );
        int ( *out_queue )( queue_array *, void * , QUEUE_TYPE );
        int ( *is_empty )( queue_array * );
        int ( *is_full )( queue_array * );

        pthread_rwlock_t lock;

        sem_t empty;
        sem_t resource;
        sem_t reader;
        sem_t writer;

        volatile int size;
        int element_size;
        void *element;

        volatile int  in_index;
        volatile int out_index;
        volatile int init_flag;
        atomic_t cur_count;

        volatile int used_max;		//只在入队时使用，入队操作全部互斥，因此不用原子操作
        volatile int drop_count;		//由于队列满而丢弃点入队操作，不用原子操作理由通上
};

queue_array *create_queue();
void queue_destroy( queue_array * );
#endif /* __QUEUE_H__  */

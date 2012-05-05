#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//////////////////////////////////////////queue_array//////////////////////////////////////////
static int queue_array_init( queue_array *this, int size , int element_size );
static int queue_array_resize( queue_array *this, int newsize );
void queue_destroy( queue_array *this );
static int queue_in_queue_array( queue_array *this, void *data, QUEUE_TYPE type );
static int queue_out_queue_array( queue_array *this, void *data, QUEUE_TYPE type );
static void queue_array_reset( queue_array *this );
static inline int queue_array_getsize( queue_array *this );
static inline int queue_array_getcurlen( queue_array *this );
static inline int queue_array_is_empty( queue_array *this );
static inline int queue_array_is_full( queue_array *this );


queue_array *create_queue()
{
        queue_array *this = malloc_safe( queue_array );

        if( this == NULL )
                return NULL;

        this->init = queue_array_init;
        this->reset = queue_array_reset;
        this->resize = queue_array_resize;
        this->get_size = queue_array_getsize;
        this->get_current_len = queue_array_getcurlen;
        this->in_queue = queue_in_queue_array;
        this->out_queue = queue_out_queue_array;
        this->is_empty = queue_array_is_empty;
        this->is_full = queue_array_is_full;
        pthread_rwlockattr_t attr;
        pthread_rwlockattr_init( &attr );
        pthread_rwlockattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
        pthread_rwlock_init( &this->lock , &attr );
        pthread_rwlockattr_destroy( &attr );
        return this;
}

static int queue_array_init( queue_array *this, int size , int element_size )
{
        if( this == NULL || size < 3 )
                return -1;

        pthread_rwlock_wrlock( &this->lock );

        if( element_size < 0 || size < 0 ) {
                fprintf( stderr, "assigned size is illegal\n" );
                pthread_rwlock_unlock( &this->lock );
                return 0;
        }

        if( this->element != NULL )
                free( this->element );

        if( ( this->element = malloc_array_safe( size * element_size, void ) ) == NULL ) {
                fprintf( stderr, "static queue_array init failed\n" );
                pthread_rwlock_unlock( &this->lock );
                return -1;
        }

        sem_init( &this->empty, 0, size );
        sem_init( &this->resource, 0, 0 );
        sem_init( &this->reader, 0, 1 );
        sem_init( &this->writer, 0, 1 );
        this->element_size = element_size;
        this->size = size;
        this->in_index = 0;
        this->out_index = 0;
        this->used_max = 0;
        this->drop_count = 0;
        this->init_flag = 1;
        pthread_rwlock_unlock( &this->lock );
        return 0;
}

static int queue_array_resize( queue_array *this, int newsize )
{
        if( this == NULL )
                return -1;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 0 ) {
                fprintf( stderr, "static queue_array has not initd yet\n" );
                pthread_rwlock_unlock( &this->lock );
                return -1;
        }

        if( this->size == newsize ) {
                pthread_rwlock_unlock( &this->lock );
                return 0;
        }

        queue_element *p;

        if( ( p = malloc_array_safe( newsize * this->element_size, void ) ) == NULL ) {
                fprintf( stderr, "static queue_array resize failed\n" );
                pthread_rwlock_unlock( &this->lock );
                return -1;
        }

        free_safe( this->element );
        this->element = p;
        this->size = newsize;
        atomic_set( &this->cur_count, 0 );
        this->in_index = 0;
        this->out_index = 0;
        this->used_max = 0;
        sem_destroy( &this->empty );
        sem_destroy( &this->resource );
        sem_destroy( &this->reader );
        sem_destroy( &this->writer );
        sem_init( &this->empty, 0, newsize );
        sem_init( &this->resource, 0, 0 );
        sem_init( &this->reader, 0, 0 );
        sem_init( &this->writer, 0, 1 );
        pthread_rwlock_unlock( &this->lock );
        return 0;
}

void queue_destroy( queue_array *this )
{
        if( this == NULL )
                return;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 0 ) {
                fprintf( stderr, "static queue_array has not initd yet\n" );
                pthread_rwlock_unlock( &this->lock );
                pthread_rwlock_destroy( &this->lock );
                return;
        }

        sem_destroy( &this->empty );
        sem_destroy( &this->resource );
        sem_destroy( &this->reader );
        sem_destroy( &this->writer );
        free_safe( this->element );
        this->init_flag = 0;
        atomic_set( &this->cur_count, 0 );
        this->in_index = 0;
        this->out_index = 0;
        this->size = 0;
        this->used_max = 0;
        this->drop_count = 0;
        pthread_rwlock_unlock( &this->lock );
        pthread_rwlock_destroy( &this->lock );
        free_safe( this );
}

static int queue_in_queue_array( queue_array *this, void *data , QUEUE_TYPE type )
{
        if( this == NULL )
                return QUEUE_OP_ERROR;

        if( data == NULL )
                return QUEUE_OP_ERROR;

        int cur;
        pthread_rwlock_rdlock( &this->lock );

        if( this->init_flag == 0 ) {
                fprintf( stderr, "static queue_array has not initd yet\n" );
                pthread_rwlock_unlock( &this->lock );
                return QUEUE_OP_ERROR;
        }

        switch( type ) {
                case QUEUE_BLOCK:
                        pthread_rwlock_unlock( &this->lock );
                        sem_wait( &this->empty );
                        pthread_rwlock_rdlock( &this->lock );
                        break;
                case QUEUE_UNBLOCK:
                default:

                        if( sem_trywait( &this->empty ) != 0 ) {
                                pthread_rwlock_unlock( &this->lock );
                                ++this->drop_count;
                                return QUEUE_FULL;
                        }

                        break;
        }

        sem_wait( &this->writer );
        memcpy( ( char * )( this->element ) + ( this->in_index * this->element_size ), data, this->element_size );

        if( this->in_index == this->size - 1 )
                this->in_index = 0;
        else
                ++this->in_index;

        atomic_inc( &this->cur_count );
        cur = atomic_read( &this->cur_count );

        if( this->used_max < cur )
                this->used_max = cur;

        sem_post( &this->resource );
        sem_post( &this->writer );
        pthread_rwlock_unlock( &this->lock );
        return QUEUE_OP_SUCCESS;
}

static int queue_out_queue_array( queue_array *this, void *data, QUEUE_TYPE type )
{
        if( this == NULL )
                return QUEUE_OP_ERROR;

        if( data == NULL )
                return QUEUE_OP_ERROR;

        pthread_rwlock_rdlock( &this->lock );

        if( this->init_flag == 0 ) {
                fprintf( stderr, "static queue_array has not initd yet\n" );
                pthread_rwlock_unlock( &this->lock );
                return QUEUE_OP_ERROR;
        }

        switch( type ) {
                case QUEUE_BLOCK:
                        pthread_rwlock_unlock( &this->lock );
                        sem_wait( &this->resource );
                        pthread_rwlock_rdlock( &this->lock );
                        break;
                case QUEUE_UNBLOCK:
                default:

                        if( sem_trywait( &this->resource ) != 0 ) {
                                pthread_rwlock_unlock( &this->lock );
                                return QUEUE_EMPTY;//队列空
                        }

                        break;
        }

        sem_wait( &this->reader );
        memcpy( data, ( char * )( this->element ) + this->out_index * this->element_size,  this->element_size );

        if( this->out_index == this->size - 1 )
                this->out_index = 0;
        else
                ++this->out_index;

        atomic_dec( &this->cur_count );
        sem_post( &this->empty );
        sem_post( &this->reader );
        pthread_rwlock_unlock( &this->lock );
        return QUEUE_OP_SUCCESS;
}

static void queue_array_reset( queue_array *this )
{
        if( this == NULL )
                return;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 0 ) {
                fprintf( stderr, "static queue_array has not initd yet\n" );
                pthread_rwlock_unlock( &this->lock );
                return;
        }

        atomic_set( &this->cur_count, 0 );
        this->in_index = 0;
        this->out_index = 0;
        this->used_max = 0;
        this->drop_count = 0;
        sem_destroy( &this->empty );
        sem_destroy( &this->resource );
        sem_destroy( &this->reader );
        sem_destroy( &this->writer );
        sem_init( &this->empty, 0, this->size );
        sem_init( &this->resource, 0, 0 );
        sem_init( &this->reader, 0, 0 );
        sem_init( &this->writer, 0, 1 );
        pthread_rwlock_unlock( &this->lock );
}

static inline int queue_array_getsize( queue_array *this )
{
        int ret;
        pthread_rwlock_rdlock( &this->lock );

        if( this->init_flag == 1 )
                ret = this->size;
        else
                ret = 0;

        pthread_rwlock_unlock( &this->lock );
        return ret;
}
static inline int queue_array_getcurlen( queue_array *this )
{
        int ret;
        pthread_rwlock_rdlock( &this->lock );
        ret = atomic_read( &this->cur_count );
        pthread_rwlock_unlock( &this->lock );
        return ret;
}

static inline int queue_array_is_empty( queue_array *this )
{
        pthread_rwlock_rdlock( &this->lock );

        if( atomic_read( &this->cur_count ) == 0 ) {
                pthread_rwlock_unlock( &this->lock );
                return 1;
        } else {
                pthread_rwlock_unlock( &this->lock );
                return 0;
        }

        pthread_rwlock_unlock( &this->lock );
}

static inline int queue_array_is_full( queue_array *this )
{
        pthread_rwlock_rdlock( &this->lock );

        if( atomic_read( &this->cur_count ) == this->size ) {
                pthread_rwlock_unlock( &this->lock );
                return 1;
        } else {
                pthread_rwlock_unlock( &this->lock );
                return 0;
        }

        pthread_rwlock_unlock( &this->lock );
}


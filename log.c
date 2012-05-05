#include "log.h"
#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>


#define convert_level(m) (m >= DEBUG ? 1 : 0)
///////////////////////////queue///////////////////////////
struct queue_element_t {
        log_mode mode;
        log_level level;
        struct timeval timestamp;
        char category[CATEGORY_LEN];
        char msg[LOG_LEN];
};

const char *log_level_str[] = {"FATAL", "ERROR", "INFO", "DEBUG"};
const char *unknown = "UNKNOWN";

struct log_lib_t {
        queue_array *data;
        volatile int log_flag;		//log t enable or shutdown, just do not add_log and get_log
        volatile int init_flag;
        volatile int start_flag;
        pthread_rwlock_t lock;
        volatile int64_t total;		//recored num
        pthread_t id;
        FILE *log_fp[2];
        /* FILE *debug_fp; */
        int sock;
        char *render_buffer;
};


//////////////////////////////////////////////
static inline const char *level2str( log_level level );
static void log_recored( log_t *this,  queue_element *job );
static void catch_signal( int i );
static void *entry( void *p );
///////////////////////////////////////////////////////////////////

log_t *log_create()
{
        log_t *temp = malloc( sizeof( log_t ) );

        if( temp == NULL )
                return NULL;

        memset( temp, 0, sizeof( log_t ) );
        pthread_rwlockattr_t attr;
        pthread_rwlockattr_init( &attr );
        pthread_rwlockattr_setpshared( &attr, PTHREAD_PROCESS_SHARED );
        pthread_rwlock_init( &temp->lock, &attr );
        pthread_rwlockattr_destroy( &attr );
        temp->data = create_queue();

        if( temp->data == NULL ) {
                pthread_rwlock_destroy( &temp->lock );
                free( temp );
                return NULL;
        }

        return temp;
}

void log_destroy( log_t *this )
{
        if( this == NULL )
                return;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 0 ) {
                return ;
        }

        if( this->id != 0 ) {
                pthread_kill( this->id, LOG_STOP_SIGNAL );
                this->id = 0;
        }

        queue_destroy( this->data );

        if( this->log_fp[0] != NULL )
                fclose( this->log_fp[0] );

        if( this->log_fp[1] != NULL )
                fclose( this->log_fp[1] );

        free( this->render_buffer );
        pthread_rwlock_unlock( &this->lock );
        pthread_rwlock_destroy( &this->lock );
        free_safe( this );
}

LOG_BOOL log_init( log_t *this )
{
        if( this == NULL )
                return LOG_FALSE;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 1 ) {
                fprintf( stderr, "log has already init \n" );
                pthread_rwlock_unlock( &this->lock );
                return LOG_TRUE;
        }

        this->data = create_queue();
        this->render_buffer = malloc( RENDER_BUF_LEN );

        if( this->render_buffer  ==  NULL ) {
                fprintf( stderr, "log init failed\n" );
                pthread_rwlock_unlock( &this->lock );
                return LOG_FALSE;
        }

        if( this->data->init( this->data, LOG_BUFFER_NUM, sizeof( struct queue_element_t ) ) != 0 ) {
                fprintf( stderr, "log init failed\n" );
                free( this->render_buffer );
                pthread_rwlock_unlock( &this->lock );
                return LOG_FALSE;
        }

        this->init_flag = 1;
        this->log_flag = 1;
        this->start_flag = 0;
        this->total = 0;

        if( this->log_fp[0] != NULL )
                fclose( this->log_fp[0] );

        this->log_fp[0] = NULL;

        if( this->log_fp[1] != NULL )
                fclose( this->log_fp[1] );

        this->log_fp[1] = NULL;

        if( this->sock != 0 )
                close( this->sock );

        this->sock = 0;
        pthread_rwlock_unlock( &this->lock );
        return LOG_TRUE;
}

inline void log_disable( log_t *this )
{
        if( this == NULL )
                return;

        pthread_rwlock_wrlock( &this->lock );
        this->log_flag = 0;
        pthread_rwlock_unlock( &this->lock );
}


inline void log_enable( log_t *this )
{
        if( this == NULL )
                return;

        pthread_rwlock_wrlock( &this->lock );
        this->log_flag = 0;
        pthread_rwlock_unlock( &this->lock );
}


void log_stop( log_t *this )
{
        if( this == NULL )
                return ;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 0 ) {
                return ;
        }

        if( this->id != 0 ) {
                pthread_kill( this->id, LOG_STOP_SIGNAL );
                this->id = 0;
        }

        this->start_flag = 0;
        this->log_flag = 0;
        pthread_rwlock_unlock( &this->lock );
}


void log_print_status( log_t * this, FILE *stream )
{
        if( this == NULL || stream == NULL )
                return ;

        pthread_rwlock_rdlock( &this->lock );
        fprintf( stream, "[log]\n\tlog_buffer_num=%d\n\tlog_total=%lld\n\tused_max_buffer=%d\n\tdrop_log_num=%d\n", LOG_BUFFER_NUM, this->total, this->data->used_max, this->data->drop_count );
        pthread_rwlock_unlock( &this->lock );
}

LOG_BOOL log_write( log_t *this, log_mode mode, log_level level, char *category, char *fmt, ... )
{
        queue_element temp;

        if( this == NULL )
                return LOG_FALSE;

        pthread_rwlock_rdlock( &this->lock );

        if( this->log_flag  ==  0 ) {
                pthread_rwlock_unlock( &this->lock );
                return LOG_FALSE;
        }

        gettimeofday( &temp.timestamp, NULL );

        if( category != NULL )
                memcpy( temp.category, category, CATEGORY_LEN );
        else
                strcpy( temp.category, "main" );

        va_list va;
        va_start( va , fmt );
        vsnprintf( temp.msg, LOG_LEN, fmt, va );
        va_end( va );
        temp.mode = mode;
        temp.level = level;
        this->data->in_queue( this->data, &temp, QUEUE_UNBLOCK );
        ++this->total;
        pthread_rwlock_unlock( &this->lock );
        return LOG_TRUE;
}

//static LOG_BOOL log_dispatch( log_t *this, dispatch_type type, callback_do_type dotype, void ( *wrap )( log * ) );
LOG_BOOL log_dispatch( log_t *this, dispatch_type type )
{
        if( this  ==  NULL )
                return LOG_FALSE;

        pthread_rwlock_wrlock( &this->lock );

        if( this->init_flag == 0 || this->start_flag == 1 ) {
                return LOG_FALSE;
        }

        pthread_attr_t attr;
        pthread_attr_init( &attr );

        switch( type ) {
                case DISPATCH_UNBLOCK:
                        pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_DETACHED );

                        if( pthread_create( ( pthread_t * )( &this->id ), &attr, entry, ( void * )this ) != 0 ) {
                                this->start_flag = 0;
                                this->id = 0;
                                pthread_rwlock_unlock( &this->lock );
                                return LOG_FALSE;
                        }

                        break;
                case DISPATCH_BLOCK:

                        if( pthread_create( ( pthread_t * )( &this->id ), NULL, entry, ( void * )this ) != 0 ) {
                                this->start_flag = 0;
                                this->id = 0;
                                pthread_rwlock_unlock( &this->lock );
                                return LOG_FALSE;
                        }

                        if( pthread_join( this->id, NULL ) != 0 ) {
                                perror( "block failed" );
                        }

                        break;
                default:
                        break;
        }

        pthread_rwlock_unlock( &this->lock );
        return LOG_TRUE;
}


static void *entry( void *p )
{
        if( p  ==  NULL )
                return NULL;

        log_t *this = ( log_t * )p;

        if( this->render_buffer  == NULL ) {
                fprintf( stderr, "malloc render_buffer failed\n" );
        }

        queue_element job;
        sigset_t sigmask;
        int ret;
        sigfillset( &sigmask );
        sigdelset( &sigmask, LOG_STOP_SIGNAL );
        pthread_sigmask( SIG_SETMASK, &sigmask, NULL );
        signal( LOG_STOP_SIGNAL, catch_signal );

        while( 1 ) {		//对回调函数进行封装，屏蔽所有线程池调用细节
                ret = this->data->out_queue( this->data, &job , QUEUE_BLOCK );

                if( ret == QUEUE_OP_SUCCESS ) {
                        log_recored( this, &job );
                } else if( ret == QUEUE_OP_ERROR ) {
                        fprintf( stderr, "log-dispatch : get log error\n" );
                }
        }

        return NULL;
}

static void catch_signal( int i )
{
        pthread_exit( 0 );
}

LOG_BOOL log_set_file( log_t *this, char *log_file, char *debug_file )
{
        FILE *fp;

        if( log_file == NULL )
                return LOG_FALSE;

        pthread_rwlock_wrlock( &this->lock );
        fp = fopen( log_file, "a+" );

        if( fp == NULL ) {
                pthread_rwlock_unlock( &this->lock );
                return LOG_FALSE;
        }

        if( this->log_fp[0] != NULL )
                fclose( this->log_fp[0] );

        this->log_fp[0] = fp;
        setlinebuf( this->log_fp[0] );

        if( debug_file != NULL ) {
                fp = fopen( debug_file, "a+" );

                if( fp == NULL ) {
                        pthread_rwlock_unlock( &this->lock );
                        return LOG_FALSE;
                }

                if( this->log_fp[1] != NULL )
                        fclose( this->log_fp[1] );

                this->log_fp[1] = fp;
                setlinebuf( this->log_fp[1] );
        }

        pthread_rwlock_unlock( &this->lock );
        return LOG_TRUE;
}


LOG_BOOL log_set_socket( log_t *this, char *ip, char *port, sock_type type )
{
        if( this == NULL	|| ip  ==  NULL || port  == NULL )
                return LOG_FALSE;

        if( type != UDP  &&  type != TCP )
                return LOG_FALSE;

        int flags, client_sock;
        struct addrinfo *result, hints, *rp = NULL;
        /* result = get_addrinfo( ip, port, IP_ANY, type, IP_PROTOCOL ); */

        if( result == NULL )
                return LOG_FALSE;

        memset( &hints, 0, sizeof( struct addrinfo ) );
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = type;
        hints.ai_protocol = IPPROTO_IP;
        hints.ai_flags = AI_NUMERICHOST;
        //hints.ai_flags = 0;

        if( ( flags = getaddrinfo( ip, port, ( const struct addrinfo * )&hints, &result ) ) != 0 ) {
                fprintf( stderr, "getaddrinfo: %s\n", gai_strerror( flags ) );
                return LOG_FALSE;
        }

        for( rp = result; rp != NULL; rp = rp->ai_next ) {
                client_sock =
                        socket( rp->ai_family, rp->ai_socktype,
                                rp->ai_protocol );

                if( client_sock == -1 )
                        continue;

                flags = fcntl( client_sock, F_GETFL, 0 );	//if get failed, socket just be block,so no need to do the error
                fcntl( client_sock, F_SETFL, flags | O_NONBLOCK );

                if( connect( client_sock, rp->ai_addr, rp->ai_addrlen ) != -1 ) {
                        fcntl( client_sock, F_SETFL, flags );
                        break;
                } else {
                        fd_set set;
                        FD_ZERO( &set );
                        FD_SET( client_sock, &set );
                        struct timeval tv;
                        tv.tv_usec = 0;
                        tv.tv_sec = 3;
                        int error = 0, len = 0;

                        if( select( client_sock + 1, NULL, &set, NULL, &tv ) > 0 ) {
                                getsockopt( client_sock, SOL_SOCKET, SO_ERROR,
                                            &error, ( socklen_t * ) & len );

                                if( error != 0 )
                                        close( client_sock );
                                else {
                                        fcntl( client_sock, F_SETFL, flags );
                                        break;
                                }
                        } else	//timeout or select failed      means connect failed
                                close( client_sock );
                }
        }

        freeaddrinfo( result );

        if( rp == NULL )
                return LOG_FALSE;
        else {
                pthread_rwlock_wrlock( &this->lock );
                this->sock = client_sock;
                pthread_rwlock_unlock( &this->lock );
                return LOG_TRUE;
        }
}

inline const char *level2str( log_level level )
{
        if( level > DEBUG )
                return unknown;
        else
                return log_level_str[level];
}

static void log_recored( log_t *this,  queue_element *job )
{
        int i = convert_level( job->level );
        struct tm tm;
        gmtime_r( &job->timestamp.tv_sec, &tm );
        pthread_rwlock_rdlock( &this->lock );

        if( job->level != DEBUG ) {
                snprintf( this->render_buffer, RENDER_BUF_LEN ,  "[%04d/%02d/%02d %02d:%02d:%02d.%03ld][%-5s][%s] - %s\n",
                          tm.tm_year + 1900,  tm.tm_mon + 1,  tm.tm_mday,
                          tm.tm_hour,  tm.tm_min,  tm.tm_sec,
                          job->timestamp.tv_usec / 1000,
                          level2str( job->level ),
                          job->category, job->msg );
        } else {
                snprintf( this->render_buffer, RENDER_BUF_LEN ,  "[%04d/%02d/%02d %02d:%02d:%02d.%03ld][%-5s][%s] - %s (%s,%d:%s)\n",
                          tm.tm_year + 1900,  tm.tm_mon + 1,  tm.tm_mday,
                          tm.tm_hour,  tm.tm_min,  tm.tm_sec,
                          job->timestamp.tv_usec / 1000,
                          level2str( job->level ),
                          job->category, job->msg, __FILE__, __LINE__, __FUNCTION__ );
        }

        switch( job->mode ) {
                case TO_FILE:

                        if( this->log_fp[i] != NULL )
                                fprintf( this->log_fp[i], "%s", this->render_buffer );
                        else
                                fprintf( stderr, "%s", this->render_buffer );

                        break;
                case TO_CONSOLE_AND_FILE:
                        fprintf( stderr, "%s", this->render_buffer );

                        if( this->log_fp[i] != NULL )
                                fprintf( this->log_fp[i], "%s", this->render_buffer );
                        else
                                fprintf( stderr, "%s", this->render_buffer );

                        break;
                case TO_SOCKET:

                        if( this->sock  == 0 )
                                return;

                        int total = 0, len, length;
                        length = strlen( this->render_buffer );
                        len = length;

                        while( len > 0 ) {
                                len = send( this->sock, this->render_buffer + total, len, 0 );

                                if( len <= 0 )
                                        break;

                                total  +=  len;
                                len = length - total;
                        }

                        break;
                case TO_CONSOLE:
                default:
                        fprintf( stderr, "%s\n", this->render_buffer );
                        break;
        }

        pthread_rwlock_unlock( &this->lock );
}

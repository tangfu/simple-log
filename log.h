/**
 * @file log.h
 * @brief 日志库
 *
 * 1.日志禁用接口是禁用日志写入接口，但不影响缓存中未写完的数据\n
 * 2.debug级别的日志默认写入debug_file文件中，如果没有设定那么写入log_file中\n
 * 3.每条日志不需要添加换行符，会自动添加\n
 * 4.Debug输出受限于是否定义ENABLE_DEBUG，所有调试信息在DEBUG标志关闭的情况下都无法输出，除非使用原始日志接口,需要在包含之前定义\n
 * 5.调试信息带有文件名，行号，函数名等调试数据\n
 * 6.使用非阻塞的添加方式，记录日志的记录总数和丢弃数\n
 * 7.由于系统使用了pthread,链接时使用-lpthread\n
 * 8.debug级别的日志使用行缓冲，其他级别暂时也使用行缓冲，尽量减少日志丢失的可能性\n
 * 9.提供FATAL，ERROR，DEBUG，INFO四种级别\n
 * 10.日志信息的输出设备支持三种:文件，终端，SOCKET。SOCKET同时支持使用UDP或者TCP进行连接, 使用nc -u -l 5468和nc -l 5468可以进行本机测试\n
 *
 * @author tangfu - abctangfuqiang2008@163.com
 * @version 0.1
 * @date 2012-03-22
 */

#ifndef __LOG_H__
#define __LOG_H__


#ifndef LOG_BOOL
#define LOG_BOOL int
#define LOG_TRUE 1
#define LOG_FALSE 0
#endif

#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <time.h>
#include <sys/socket.h>


typedef enum log_mode_s { TO_CONSOLE = 0x01, TO_FILE = 0x02, TO_SOCKET = 0x04, TO_CONSOLE_AND_FILE = 0x03} log_mode;
typedef enum log_level_s {FATAL = 0, ERROR, INFO, DEBUG } log_level;
typedef enum log_policy_s {LOG_DELAY = 0, LOG_DIRECT} log_policy;
typedef enum dispatch_type_s {DISPATCH_UNBLOCK = 0, DISPATCH_BLOCK} dispatch_type;
typedef enum sock_type_s {TCP = SOCK_STREAM, UDP = SOCK_DGRAM} sock_type;

#define LOG_STOP_SIGNAL SIGRTMAX-5
#define LOG_SOCKET_PORT_DEFAULT "5468"


#define LOG_FILE 0
#define DEBUG_FILE 1


#define LOG_BUFFER_NUM		50
#define CATEGORY_LEN			64
#define LOG_LEN				128
#define RENDER_BUF_LEN		512


/////////////////////////////////////////////LOG_INTERFACE/////////////////////////////////////////////////////
typedef struct log_lib_t log_t;

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief	log_create 创建日志库对象
 *
 * @return	日志库对象
 */
log_t *log_create();
/**
 * @brief	log_init 初始化日志对象
 *
 * @param	this	日志库对象指针
 *
 * @return	日志错误码
 */
LOG_BOOL log_init( log_t *this );
/**
 * @brief	log_set_file	为日志指定输出文件和调试文件
 *
 * @param	this			日志对象
 * @param	log_file		日志默认输出文件
 * @param	debug_file		默认调试文件
 *
 * @return	日志错误码
 */
LOG_BOOL log_set_file( log_t *this, char *log_file,  char *debug_file );
/**
 * @brief	log_set_socket	为日志指定输出套接字
 *
 * @param	this			日志对象
 * @param	ip				输出目标主机IP
 * @param	port			目标主机端口
 * @param	type			协议类型TCP or UDP
 *
 * @return	日志错误码
 */
LOG_BOOL log_set_socket( log_t *this, char *ip, char *port, sock_type type );
/**
 * @brief	log_destroy 销毁日志
 *
 * @param	this		日志对象指针
 */
void log_destroy( log_t *this );
/**
 * @brief	log_disable 禁用日志，调用后日志无法输入和输出
 *
 * @param	this		日志对象指针
 */
void log_disable( log_t *this );
/**
 * @brief	log_enable	开启日志，重新开启日志功能
 *
 * @param	this		日志对象指针
 */
void log_enable( log_t *this );
/**
 * @brief	log_stop	停止日志调度
 *
 * @param	this		日志对象指针
 */
void log_stop( log_t *this );
/**
 * @brief	log_write	日志写入的接口
 *
 * @param	this		日志对象指针
 * @param	mode		日志输出模式
 * @param	level		日志级别
 * @param	category	日志分类
 * @param	fmt			日志消息的格式
 * @param	...			变参...
 *
 * @return				日志错误码
 */
LOG_BOOL log_write( log_t *this, log_mode mode, log_level level, char *category, char *fmt, ... );
/**
 * @brief	log_print_status	打印日志对象当前状态
 *
 * @param	this				日志对象指针
 * @param	stream				输出流
 */
void log_print_status( log_t *this , FILE *stream );
/**
 * @brief	log_dispatch	日志对象调度接口
 *
 * @param	this			日志对象指针
 * @param	type			阻塞方式，包括以阻塞方式调度或者以非阻塞方式调度
 *
 * @return
 */
LOG_BOOL log_dispatch( log_t *this, dispatch_type type );

#ifdef __cplusplus
}
#endif


/**
 *	@attention
 *		开启ENABLE_DEBUG标记后，调试级别的日志信息才能正常输出，否则被注释掉了不会有任何效果
 */
#ifdef ENABLE_DEBUG
#define LOG_DEBUG_TO_CONSOLE(this, fmt, arg... ) log_write(this, TO_CONSOLE, DEBUG, NULL, fmt, ##arg)
#define LOG_DEBUG(this, fmt, arg... ) log_write(this, TO_CONSOLE_AND_FILE, DEBUG, NULL, fmt, ##arg)
#define LOG_DEBUG_TO_FILE(this, fmt, arg... ) log_write(this, TO_FILE, DEBUG, NULL, fmt, ##arg)
#define LOG_DEBUG_TO_SOCKET(this, fmt, arg... ) log_write(this, TO_SOCKET, DEBUG, NULL, fmt, ##arg)
#else
#define LOG_DEBUG_TO_CONSOLE(this, fmt, arg... ) {}
#define LOG_DEBUG(this, fmt, arg... ) {}
#define LOG_DEBUG_TO_FILE(this, fmt, arg... ) {}
#define LOG_DEBUG_TO_SOCKET(this, fmt, arg...) {}
#endif


/**
 *	@brief	TO_CONSOLE_AND_FILE
 *
 *	将日志输入出到文件和终端的简化接口
 *
 */
#define LOG_FATAL(this, fmt, arg... ) log_write(this, TO_CONSOLE_AND_FILE, FATAL, NULL, fmt, ##arg)
#define LOG_ERROR(this, fmt, arg... ) log_write(this, TO_CONSOLE_AND_FILE, ERROR, NULL, fmt, ##arg)
#define LOG_INFO(this, fmt, arg... ) log_write(this, TO_CONSOLE_AND_FILE, INFO, NULL, fmt, ##arg)

/**
 *	@brief	TO_CONSOLE
 *
 *	将日志输入出到终端的简化接口
 *
 */
#define LOG_FATAL_TO_CONSOLE(this, fmt, arg... ) log_write(this, TO_CONSOLE, FATAL, NULL, fmt, ##arg)
#define LOG_ERROR_TO_CONSOLE(this, fmt, arg... ) log_write(this, TO_CONSOLE, ERROR, NULL, fmt, ##arg)
#define LOG_INFO_TO_CONSOLE(this, fmt, arg... ) log_write(this, TO_CONSOLE, INFO, NULL, fmt, ##arg)

/**
 *	@brief	TO_FILE
 *
 *	将日志输出到文件的简化接口
 *
 */
#define LOG_FATAL_TO_FILE(this, fmt, arg... ) log_write(this, TO_FILE, FATAL, NULL, fmt, ##arg)
#define LOG_ERROR_TO_FILE(this, fmt, arg... ) log_write(this, TO_FILE, ERROR, NULL, fmt, ##arg)
#define LOG_INFO_TO_FILE(this, fmt, arg... ) log_write(this, TO_FILE, INFO, NULL, fmt, ##arg)

/**
 *	@brief	TO_SOCKET
 *
 *	将日志输出到套机字的简化接口
 *
 */
#define LOG_FATAL_TO_SOCKET(this, fmt, arg... ) log_write(this, TO_SOCKET, FATAL, NULL, fmt, ##arg)
#define LOG_ERROR_TO_SOCKET(this, fmt, arg... ) log_write(this, TO_SOCKET, ERROR, NULL, fmt, ##arg)
#define LOG_INFO_TO_SOCKET(this, fmt, arg... ) log_write(this, TO_SOCKET, INFO, NULL, fmt, ##arg)



#endif	/* __LOG_H__ */


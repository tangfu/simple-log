/**
 * @file macro_helper.h
 * @brief   通用的宏集合
 *
 * @author tangfu - abctangfuqiang2008@163.com
 * @version 1.0
 * @date 2011-03-27
 *
 * @attention
 *	C standard macro :
 *		__DATE__		"mm dd yy"(string)
 *		__FILE__		current src filename(string)
 *		__LINE__		current line no(integer)
 *		__TIME__		"hh：mm：ss"(string)
 *		__func__		current func name(string)
 */


#ifndef __MACRO_HELPER_H__
#define	__MACRO_HELPER_H__

/**
 * 设置强制内联，windows和linux都能使用
 */
#if !defined(forceinline)
#ifdef _MSC_VER_ // for MSVC
#define forceinline __forceinline
#elif defined __GNUC__ // for gcc on Linux/Apple OS X
#define forceinline __inline__ __attribute__((always_inline))
#else
#define forceinline
#endif
#endif

/**
 * 分支预测功能，
 */

#define likely(x) __builtin_expect((x),1)
#define unlikely(x) __builtin_expect((x),0)

/**
 * @brief   基本的数学运算
 *
 *  between	判断c是否处于a，b之间，是返回1，否则返回0
 *  is_even	判断n是否是偶数
 *  square	x的平方
 *  cube	x的立方
 *  max		取x，y的最大值
 *  min		取x，y的最小值
 *  swap	交换a和b的值
 *  mod		取X对于M的余数
 *  get_interger_sign	x为正数返回0，负数返回-1
 *  is_opposite_sign	x,y符号相反返回1，符号相同返回0
 *  is_power_2		x是2的幂返回1，否则返回0
 *
 */
#define between(c, a, b)	((c > a && c < b) ? 1 : 0)
#define	is_even(n)		((n)%2 == 0)
#define	square(x)		(x)*(x)
#define	cube(x)			(x)*(x)*(x)
#define	max(x,y)		{typeof(x) x_ = (x);typeof(y) y_ = (y);(x_>y_) ? x_ : y_);}
#define	min(x,y)		{typeof(x) x_ = (x);typeof(y) y_ = (y);(x_<y_) ? x_ : y_);}

#define check_zero(divisor)  \
       if (divisor == 0)  \
       printf("*** Attempt to divide by zero on line %d  "  \
       "of file %s  ***\n",__LINE__, __FILE__)

#define swap(a,b) swap_xor(a,b)

#define swap1(a,b) { typeof(a) _t = a; a = b; b = _t;}
#define swap2(a, b)  { a = a + b;b = a - b; a = a - b;  }
#define swap_xor(a,b) { a ^= b;b ^= a;a ^= b; }

//M必须是2的N次方  MOD(X, M) = X - (X >> M) << M
#define mod(X, M) = X & (M-1)


//Compute the sign of an integer
// 负数返回-1，正数返回0
#define get_integer_sign(x) (-(x<0))

//Detect if two integers have opposite signs
//符号相反返回1，符号相同返回0
#define is_opposite_sign(x,y) ((x ^ y) < 0)

//Determining if an integer is a power of 2
//是2的幂返回1，否则返回0
#define is_power_2(x) ( v && !(x & (x-1)) )

/*****************************bitmap******************************/
/**
 *  @brief	位图操作
 *	bitmap_bytes	    计算cnt个字节需要占用多少个位图字节
 *	set_bit		    设置位图的id位置为1
 *	unset_bit	    设置位图的id位置为0
 *	test_bit	    测试位图的id位是否为1
 *
 *  @attention
 *	位图id从1开始
 *
 */

///计算位图所占字节数
#define bitmap_bytes(cnt)	((cnt - 1) >> 3) + 1
///设置位图的id位(设为1)
#define set_bit(pointer, id)	*(pointer + ((id - 1) >> 3) ) |= (128U >> ((id - 1) & 7))
///取消位图的id位的设置(设为0)
#define unset_bit(pointer, id)	*(pointer + ((id - 1) >> 3)) &= ~(128U >> ((id - 1) & 7))
///测试位图的id位是否设置(是否为1)
#define test_bit(pointer, id) (*(pointer + ((id - 1) >> 3)) & (128U >> ((id - 1) & 7)) != 0)
///安全设置位图
#define set_bit_safe(pointer, id)	do{  \
    if(pointer != NULL)\
	*(pointer + ((id - 1) >> 3) ) |= (128U >> ((id - 1) & 7));\
    } while(0);
///安全取消位图
#define unset_bit_safe(pointer, id)	do{  \
    if(pointer != NULL)\
	*(pointer + ((id - 1) >> 3)) &= ~(128U >> ((id - 1) & 7));\
    } while(0);


/**
 * @brief   统计接口执行时间
 *  1.使用gettimeofday来计时
 *	initcalc_g	    初始化
 *	calcstart_g	    开始计时
 *	calc_g_func	    调用函数接口
 *	calcend_g	    终止计时
 *	calc_g		    计算时间
 *	calc_output_g	    输出时间
 *
 *  2.使用clock_gettime来计时
 *	initcalc_c	    初始化
 *	calcstart_c	    开始计时
 *	calc_c_func	    调用函数接口
 *	calcend_c	    终止计时
 *	calc_c		    计算时间
 *	calc_output_c	    输出时间
 */

#ifndef __CALC_TIME_H__
#define __CALC_TIME_H__


#include <sys/time.h>
#include <time.h>



/*gettimeofday*/

#define initcalc_g() \
	struct timeval tpstart_g;\
	struct timeval tpend_g;\
	double timeuse_g;


#define calc_g_func(func) {\
	gettimeofday(&tpstart_g, NULL); \
	func; \
	gettimeofday(&tpend_g, NULL); \
	timeuse_g = 1000000 * (tpend_g.tv_sec - tpstart_g.tv_sec) + (tpend_g.tv_usec - tpstart_g.tv_usec); \
	timeuse_g /= 1000;\
}

#define calcstart_g() {\
	gettimeofday(&tpstart_g, NULL);\
	}

#define calcend_g() {\
	gettimeofday(&tpend_g, NULL);\
	}

#define calc_g() {\
	timeuse_g = 1000000 * (tpend_g.tv_sec - tpstart_g.tv_sec) + (tpend_g.tv_usec - tpstart_g.tv_usec); \
	timeuse_g /= 1000;\
}

#define calcoutput_g() {\
	printf("[%.5Fms] ", timeuse_g);\
	}

#define calcresult_g timeuse_g


/*clock_gettime*/


#define initcalc_c() \
	struct timespec tpstart_c;\
	struct timespec tpend_c;\
	double timeuse_c;


#define calc_c_func(func) {\
	clock_gettime(CLOCK_REALTIME, &tpstart_c); \
	func; \
	clock_gettime(CLOCK_REALTIME, &tpend_c); \
	timeuse_c = 1000000000 * (tpend_c.tv_sec - tpstart_c.tv_sec) + (tpend_c.tv_nsec - tpstart_c.tv_nsec); \
	timeuse_c /= 1000;\
}

#define calcstart_c() {\
	clock_gettime(CLOCK_REALTIME, &tpstart_c);\
	}

#define calcend_c() {\
	clock_gettime(CLOCK_REALTIME, &tpend_c);\
	}

#define calc_c() {\
	timeuse_c = 1000000000 * (tpend_c.tv_sec - tpstart_c.tv_sec) + (tpend_c.tv_nsec - tpstart_c.tv_nsec); \
	timeuse_c /= 1000;\
}

#define calcoutput_c() {\
	printf("[%.5Fus] ", timeuse_c);\
	}

#define calcresult_c timeuse_c

#endif /* __CALC_TIME_H__ */



/**
 *  @brief  字符串相关
 *	upcase	    将字符c转换为大写字符
 *	lowcase	    将字符c转换未小写字符
 *	array_size  计算数组a的长度
 */
#define	upcase(c)   (((c) >= 'a' && (c) <= 'z') ? ((c) - 0x20) : (c))
#define	lowcase(c)  (((c) >= 'A' && (c) <= 'Z') ? ((c) + 0x20) : (c))
#define	array_size(a)	(sizeof(a) / sizeof((a)[0]))



/**
 *  @brief malloc宏合集
 *	free_safe	    安全释放内存，同时设置指针未NULL
 *	malloc_safe	    指定malloc一个type结构体
 *	malloc_num_safe	    指定分配num字节的空间
 *	malloc_array_safe   指定分配number个type结构体
 *
 */
#define free_safe(x) {free(x);(x) = NULL;}
#define malloc_safe(type) ((type *)malloc(sizeof(type)))
#define malloc_num_safe(num, type) ((type *)malloc(num))
#define malloc_array_safe(number, type) ((type *)malloc((number) * sizeof(type)))



/**
 * @brief   简单的异常处理宏
 */
#define JOIN(x,y) JOIN_AGAIN(x,y)
#define JOIN_AGAIN(x,y)	x ## y
#define static_assert(e) \
	typedef char JOIN(assertion_failed_at_line, __LINE__) [(e) ? 1 : -1] [(e) ? 1 : -1]




/****************************show*****************************/
#ifdef __COLOR_H__
#define Err(fmt, arg...)	fprintf(stderr,__BRIGHT_COLOR_FG_RED__"[ ERROR ] : fmt"__COLOR_RESET__, ##arg)
#define Warn(fmt, arg...)	fprintf(stdout,__COLOR_FG_YELLOW__"[ WARNING ] : fmt"__COLOR_RESET__, ##arg)
#else
#define Err(fmt, arg...)	fprintf(stderr,""\033[31;1m"[ ERROR ] : fmt"\033[0m"", ##arg)
#define Warn(fmt, arg...)	fprintf(stdout,""\033[33m"[ WARNING ] : fmt"\033[0m"", ##arg)
#endif


#endif/* __MACRO_HELPER_H__  */

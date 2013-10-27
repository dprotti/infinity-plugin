/*	cputest.h
 *
 *      Cpu detection code, extracted from mmx.h ((c)1997-99 by H. Dietz
 *      and R. Fisher). Converted to C and improved by Fabrice Bellard
 */
#ifndef _CPUTEST_H_
#define _CPUTEST_H_


#define MM_MMX    0x0001        /* standard MMX */
#define MM_3DNOW  0x0004        /* AMD 3DNOW */
#define MM_MMXEXT 0x0002        /* SSE integer functions or AMD MMX ext */
#define MM_SSE    0x0008        /* SSE functions */
#define MM_SSE2   0x0010        /* PIV SSE2 functions */

/*	should be defined by architectures supporting
 *      one or more MultiMedia extension			*/
int mm_support(void);

/*	return the result of mm_support and show the results
 *      to stdout						*/
int mm_support_check_and_show(void);

/*	Function to test if mmx instructions are supported...
 *      Returns 1 if MMX instructions are supported, 0 otherwise */
int mmx_ok(void);

extern unsigned int mm_flags;

/*static inline void emms(void)
 * {
 *  __asm __volatile ("emms;":::"memory");
 * }*/


#endif /* _CPUTEST_H_ */

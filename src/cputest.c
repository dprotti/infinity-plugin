#include "config.h"
#include "cputest.h"
#include "gettext.h"
#include "glib.h"


/* ebx saving is necessary for PIC. gcc seems unable to see it alone */
#if MMX_DETECTION
#define cpuid(index, eax, ebx, ecx, edx) \
	__asm __volatile \
		("movl %%ebx, %%esi\n\t" \
		"cpuid\n\t" \
		"xchgl %%ebx, %%esi" \
		: "=a" (eax), "=S" (ebx), \
		"=c" (ecx), "=d" (edx) \
		: "0" (index));
#endif


/* Function to test if multimedia instructions are supported...  */
int mm_support(void)
{
#if MMX_DETECTION
	int rval;
	int eax, ebx, ecx, edx;

	__asm__ __volatile__ (
	        /* See if CPUID instruction is supported ... */
	        /* ... Get copies of EFLAGS into eax and ecx */
		"pushf\n\t"
		"popl %0\n\t"
		"movl %0, %1\n\t"

	        /* ... Toggle the ID bit in one copy and store */
	        /*     to the EFLAGS reg */
		"xorl $0x200000, %0\n\t"
		"push %0\n\t"
		"popf\n\t"

	        /* ... Get the (hopefully modified) EFLAGS */
		"pushf\n\t"
		"popl %0\n\t"
		: "=a" (eax), "=c" (ecx)
		:
		: "cc"
		);

	if (eax == ecx)
		return 0; /* CPUID not supported */

	cpuid(0, eax, ebx, ecx, edx);

	if (ebx == 0x756e6547 &&
	    edx == 0x49656e69 &&
	    ecx == 0x6c65746e) {
		/* intel */
inteltest:
		cpuid(1, eax, ebx, ecx, edx);
		if ((edx & 0x00800000) == 0)
			return 0;
		rval = MM_MMX;
		if (edx & 0x02000000)
			rval |= MM_MMXEXT | MM_SSE;
		if (edx & 0x04000000)
			rval |= MM_SSE2;
		return rval;
	} else if (ebx == 0x68747541 &&
		   edx == 0x69746e65 &&
		   ecx == 0x444d4163) {
		/* AMD */
		cpuid(0x80000000, eax, ebx, ecx, edx);
		if ((guint32)eax < 0x80000001)
			goto inteltest;
		cpuid(0x80000001, eax, ebx, ecx, edx);
		if ((edx & 0x00800000) == 0)
			return 0;
		rval = MM_MMX;
		if (edx & 0x80000000)
			rval |= MM_3DNOW;
		if (edx & 0x00400000)
			rval |= MM_MMXEXT;
		return rval;
	} else if (ebx == 0x746e6543 &&
		   edx == 0x48727561 &&
		   ecx == 0x736c7561) { /*  "CentaurHauls" */
		/* VIA C3 */
		cpuid(0x80000000, eax, ebx, ecx, edx);
		if ((guint32)eax < 0x80000001)
			goto inteltest;
		cpuid(0x80000001, eax, ebx, ecx, edx);
		rval = 0;
		if (edx & (1 << 31))
			rval |= MM_3DNOW;
		if (edx & (1 << 23))
			rval |= MM_MMX;
		if (edx & (1 << 24))
			rval |= MM_MMXEXT;
		return rval;
	} else if (ebx == 0x69727943 &&
		   edx == 0x736e4978 &&
		   ecx == 0x64616574) {
		/* Cyrix Section */
		/* See if extended CPUID level 80000001 is supported */
		/* The value of CPUID/80000001 for the 6x86MX is undefined
		 * according to the Cyrix CPU Detection Guide (Preliminary
		 * Rev. 1.01 table 1), so we'll check the value of eax for
		 * CPUID/0 to see if standard CPUID level 2 is supported.
		 * According to the table, the only CPU which supports level
		 * 2 is also the only one which supports extended CPUID levels.
		 */
		if (eax != 2)
			goto inteltest;
		cpuid(0x80000001, eax, ebx, ecx, edx);
		if ((eax & 0x00800000) == 0)
			return 0;
		rval = MM_MMX;
		if (eax & 0x01000000)
			rval |= MM_MMXEXT;
		return rval;
	} else {
		return 0;
	}
#else   /* not MMX_DETECTION */
	return 0;
#endif
}


int mm_support_check_and_show()
{
	int r;
	gchar *msg, *tmp;

	r = mm_support();
	if (r & 0) {
		g_message("Infinity: There is not MMX support\n");
		return r;
	}
	msg = g_strdup("Infinity: Looking for Multimedia Extensions Support...");
	if (r & MM_MMX) {
		tmp = g_strconcat(msg, " MMX", NULL);
		g_free(msg);
		msg = tmp;
	}
	if (r & MM_3DNOW) {
		tmp = g_strconcat(msg, " 3DNOW", NULL);
		g_free(msg);
		msg = tmp;
	}
	if (r & MM_MMXEXT) {
		tmp = g_strconcat(msg, " MMX2", NULL);
		g_free(msg);
		msg = tmp;
	}
/*
 * for now this extensions are not used
 * if (r & MM_SSE) {
 * tmp = g_strconcat (msg, " SSE", 0);
 * g_free (msg);
 * msg = tmp;
 * }
 * if (r & MM_SSE2) {
 * tmp = g_strconcat (msg, " SSE2", 0);
 * g_free (msg);
 * msg = tmp;
 * }
 */
	tmp = g_strconcat(msg, " detected", NULL);
	g_free(msg);
	msg = tmp;
	g_message("%s", msg);
	g_free(msg);

	return r;
}


int mmx_ok(void)
{
	return mm_support() & 0x1;
}

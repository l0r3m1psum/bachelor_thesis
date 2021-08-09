#define CPUID(func, ax, bx, cx, dx)                  \
	asm volatile (                                   \
		"cpuid" :                                    \
		"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : \
		"a" (func)                                   \
	)

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
/* https://softpixel.com/~cwright/programming/simd/cpuid.php
 * https://wiki.osdev.org/Inline_Assembly
 * https://stackoverflow.com/questions/14283171/how-to-receive-l1-l2-l3-cache-size-using-cpuid-instruction-in-x86
 * https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.pdf#page=292
 */
int
main(void) {
	uint32_t a, b, c, d;
	{
		CPUID(0, a, b, c, d);
		char buf[5] = {0};
		memcpy(buf, &b, sizeof b);
		printf("%s", buf);
		memcpy(buf, &d, sizeof d);
		printf("%s", buf);
		memcpy(buf, &c, sizeof c);
		printf("%s", buf);
		putchar('\n');
	}

	{
		CPUID(1, a, b, c, d);
		uint32_t res = 0;
		asm volatile ("mov %%ebx, %0" : "=r" (res));
		/* Extracting %bh */
		res >>= 8;
		res &= 0x0000000f;
		printf("cache line size: %d\n", res*8);
	}

	{
		asm volatile (
			"mov $1, %%ecx\n"
			"mov $4, %%eax\n"
			"cpuid" :
			"=a" (a), "=b" (b), "=c" (c), "=d" (d)
		);
		uint32_t res = 0, S = 0;
		asm volatile ("mov %%ebx, %0\nmov %%ecx, %1" : "=r" (res), "=r" (S));
#define L_maks 0x000007ff /* [11:0] */
#define P_mask 0x001ff800 /* [21:12] */
#define W_mask 0xffe00000 /* [31:22] */
		uint32_t
			L =  res & L_maks,
			P =  (res & P_mask) >> 12,
			W =  (res & W_mask) >> 22;
		static_assert(L_maks | P_mask | W_mask == 0xffffffff, "bad masks");
#undef L_maks
#undef P_maks
#undef W_maks
		printf("System Coherency Line Size %d\n", L+1);
		printf("Physical Line partitions %d\n", P+1);
		printf("Ways of associativity %d\n", W+1);
		printf("Number of Sets %d\n", S+1);

	}
	return 0;
}

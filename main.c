#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>

#include <omp.h>
#include <time.h>

#define L 65536
int T[L] = {0};
void init(int *T){
    int i,j;
    for ( i=0 ; i<L ; ++i ){
        for ( j=i^0xffff ; j ; j>>=1 ){
            T[i] = ((j&1) ? T[i]+1 : 0);
        }
    }
}

// Query Table T
int clz_T(uint32_t x, int *T){
    return T[x>>16] + ( (T[x>>16]==16) ? T[x&0xffff] : 0 );
}

static double diff_in_second(struct timespec t1, struct timespec t2)
{
    struct timespec diff;
    if (t2.tv_nsec-t1.tv_nsec < 0) {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec - 1;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec + 1000000000;
    } else {
        diff.tv_sec  = t2.tv_sec - t1.tv_sec;
        diff.tv_nsec = t2.tv_nsec - t1.tv_nsec;
    }
    return (diff.tv_sec + diff.tv_nsec / 1000000000.0);
}

int recursive_clz(uint32_t x, int c)
{
    if (!x) return 32;
    if (c == 0) return 0;
    /* shift upper half down, rest is filled up with 0s */
	uint16_t upper = (x >> c); 
	// mask upper half away
	uint16_t lower = (x & (0xFFFF >> (16 - c)));
	return upper ? recursive_clz(upper, c / 2) : c + recursive_clz(lower, c / 2);
}

int iter_clz(uint32_t x) {
    int n = 32, c = 16;
    do {
        uint32_t y = x >> c;
        if (y) { n -= c; x = y; }
        c >>= 1;
    } while (c);
    return (n - x);
}

int bin_search_clz(uint32_t x) {
    if (x == 0) return 32;
    int n = 0;
    if (x <= 0x0000FFFF) { n += 16; x <<= 16; }
    if (x <= 0x00FFFFFF) { n += 8; x <<= 8; }
    if (x <= 0x0FFFFFFF) { n += 4; x <<= 4; }
    if (x <= 0x3FFFFFFF) { n += 2; x <<= 2; }
    if (x <= 0x7FFFFFFF) { n += 1; x <<= 1; }
    return n;
}

int byte_shift_clz(uint32_t x) {
    if (x == 0) return 32;
    int n = 1;
    if ((x >> 16) == 0) { n += 16; x <<= 16; }
    if ((x >> 24) == 0) { n += 8; x <<= 8; }
    if ((x >> 28) == 0) { n += 4; x <<= 4; }
    if ((x >> 30) == 0) { n += 2; x <<= 2; }
    n = n - (x >> 31);
    return n;
}

bool test_clz(void *test_func)
{
    unsigned (*clz)(uint32_t, int) = test_func;
    assert(clz(0, 16) == 32);
    for(uint32_t i = 1; i < UINT_MAX; i++)
    {
        bool result = (clz(i, 16) == __builtin_clz(i));
        assert(result);
        if (!result)
            break;
    }
    return 1;
}

int main()
{
    struct timespec start, end, perstart, perend;
    FILE *output;
    output = fopen("output.txt", "w");

    clock_gettime(CLOCK_REALTIME, &perstart);
    for(uint32_t i = 0; i < UINT_MAX; i++)
    {
        int a = clz_T(i, T);
        if (i % 1000000 == 0)
        {
            clock_gettime(CLOCK_REALTIME, &perend);
            fprintf(output, "%u %lf\n", i, diff_in_second(perstart, perend));
            clock_gettime(CLOCK_REALTIME, &perstart);
        }
    }

    fclose(output);

    clock_gettime(CLOCK_REALTIME, &end);

    printf("execution time: %lf sec\n", diff_in_second(start, end));
}

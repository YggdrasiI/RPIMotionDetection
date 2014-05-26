/* Fast implementations of sqrt(a*a + b*b) for a,b in [-127,128]
 * See misc/testIntegerSqrt.c for performance test.
 *
 * The fastest algorithm (3) is inaccurate for high absolute values,
 * which should be no problem. 
 *
 * Select algorithm with NORM2_USE_ALGO
 *
 * Algorithm 1 - Use normal c function for integer sqrt.
 * Algorithm 2 - Use assembler optimized function for integer sqrt.
 * Algorithm 3 - Use lookup map for low values and approximation for higher values.
 *
 * TODO: There is still some space for optimazions because all imv vector components
 * are multiple of 2. Thus, the array sizes can be shrinked.
 *
 */

#ifndef NORM2_H
#define NORM2_H

#define NORM2_USE_ALGO 3

/* Fill arrays on startup */
void norm2_init_arrays();
unsigned int norm2(const signed char a, const signed char b);

#endif

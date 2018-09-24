/*
 * randgen.c   : a portable random generator 
 * 
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * $Log: genrand.h,v $
 * Revision 1.3  2005/06/22 03:01:50  arthchan2003
 * Added  keyword
 *
 * Revision 1.3  2005/03/30 01:22:48  archan
 * Fixed mistakes in last updates. Add
 *
 * 
 * 18-Nov-04 ARCHAN (archan@cs.cmu.edu) at Carnegie Mellon University
 *              First incorporated from the Mersenne Twister Random
 *              Number Generator package. It was chosen because it is
 *              in BSD-license and its performance is quite
 *              reasonable. Of course if you look at the inventors's
 *              page.  This random generator can actually gives
 *              19937-bits period.  This is already far from we need. 
 *              This will possibly good enough for the next 10 years. 
 *
 *              I also downgrade the code a little bit to avoid Sphinx's
 *              developers misused it. 
 */

#define S3_RAND_MAX_INT32 0x7fffffff
#include <stdio.h>

/** \file genrand.h
 *\brief High performance prortable random generator created by Takuji
 *Nishimura and Makoto Matsumoto.  
 * 
 * A high performance which applied Mersene twister primes to generate
 * random number. If probably seeded, the random generator can achieve 
 * 19937-bits period.  For technical detail.  Please take a look at 
 * (FIXME! Need to search for the web site.) http://www.
 */

/**
 * Macros to simplify calling of random generator function.
 *
 */
#define s3_rand_seed(s) genrand_seed(s);
#define s3_rand_int31()  genrand_int31()
#define s3_rand_real() genrand_real3()
#define s3_rand_res53()  genrand_res53()

/**
 *Initialize the seed of the random generator. 
 */
void genrand_seed(unsigned long s);

/**
 *generates a random number on [0,0x7fffffff]-interval 
 */
long genrand_int31(void);

/**
 *generates a random number on (0,1)-real-interval 
 */
double genrand_real3(void);

/**
 *generates a random number on [0,1) with 53-bit resolution
 */
double genrand_res53(void);
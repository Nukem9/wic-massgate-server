#pragma once

/*******************************************************************************
 *						phpass0.3/PasswordHash
 *
 * c++ implementation of phpass to generate password hashes that can be used with a phpBB3 forum
 * http://cvsweb.openwall.com/cgi/cvsweb.cgi/projects/phpass/
 *
 * TODO: find a better md5 implementation
 * - i had trouble implementing md5.c and md5.h from http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 * 
 * -quoted from http://www.openwall.com/phpass/
 *	"A cut-down version of phpass (supporting the portable hashes only) has 
 *	 been integrated into phpBB3 (although they have changed the hash 
 *	 type identifier string from "$P$" to "$H$", the hashes are otherwise 
 *	 compatible with those of genuine phpass)."
 *
 * - phpass c requires MD5 functions from openssl
 * - mersenne twister prng is used instead of microtime()/getmypid()/rand() function
 *
 *******************************************************************************
 *			-''Mersenne Twister'' random number generator MT19937-
 *
 * taken from http://www.mcs.anl.gov/~kazutomo/hugepage-old/twister.c
 * -changed seedMT(4357U) to seedMT((uint32)GetTickCount());
 * -forcing tickcount to odd number
 * -converted to class
 *
 *******************************************************************************
 *					-OpenSSL v1.0.2h-
 *
 * Note: only the headers required for MD5 functionality are included
 *
 * OpenSSL windows binaries Main Page:
 * https://wiki.openssl.org/index.php/Binaries
 *
 * OpenSSL/MD5 headers and import library are from Win32 OpenSSL v1.0.2h (20.3 MB Installer) located on the Shining Light Website
 * https://slproweb.com/products/Win32OpenSSL.html (32 bit)
 *	- md5.h, e_os2.h, opensslconf.h
 *	- libeay32.lib
 * the dynamic library 'libeay32.dll' included with the project is not from the above package as it is dependant on the VC++ 2013 runtimes
 *
 * instead, libeay32.dll is taken from https://indy.fulgan.com/SSL/
 *	-version 1.0.2h, does not require any visual studio runtimes
 *	 https://indy.fulgan.com/SSL/openssl-1.0.2h-i386-win32.zip
 *
 * Note:mysql connector/c 6.1.6 uses openssl version 1.0.1k
 * - not that it really matters, both 1.0.1k and 1.0.2h are heartbleed free
 *
 *******************************************************************************/

// http://www.math.keio.ac.jp/~matumoto/ver980409.html

// This is the ``Mersenne Twister'' random number generator MT19937, which
// generates pseudorandom integers uniformly distributed in 0..(2^32 - 1)
// starting from any odd seed in 0..(2^32 - 1).  This version is a recode
// by Shawn Cokus (Cokus@math.washington.edu) on March 8, 1998 of a version by
// Takuji Nishimura (who had suggestions from Topher Cooper and Marc Rieffel in
// July-August 1997).
//
// Effectiveness of the recoding (on Goedel2.math.washington.edu, a DEC Alpha
// running OSF/1) using GCC -O3 as a compiler: before recoding: 51.6 sec. to
// generate 300 million random numbers; after recoding: 24.0 sec. for the same
// (i.e., 46.5% of original time), so speed is now about 12.5 million random
// number generations per second on this machine.
//
// According to the URL <http://www.math.keio.ac.jp/~matumoto/emt.html>
// (and paraphrasing a bit in places), the Mersenne Twister is ``designed
// with consideration of the flaws of various existing generators,'' has
// a period of 2^19937 - 1, gives a sequence that is 623-dimensionally
// equidistributed, and ``has passed many stringent tests, including the
// die-hard test of G. Marsaglia and the load test of P. Hellekalek and
// S. Wegenkittl.''  It is efficient in memory usage (typically using 2506
// to 5012 bytes of static data, depending on data type sizes, and the code
// is quite short as well).  It generates random numbers in batches of 624
// at a time, so the caching and pipelining of modern systems is exploited.
// It is also divide- and mod-free.
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Library General Public License as published by
// the Free Software Foundation (either version 2 of the License or, at your
// option, any later version).  This library is distributed in the hope that
// it will be useful, but WITHOUT ANY WARRANTY, without even the implied
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
// the GNU Library General Public License for more details.  You should have
// received a copy of the GNU Library General Public License along with this
// library; if not, write to the Free Software Foundation, Inc., 59 Temple
// Place, Suite 330, Boston, MA 02111-1307, USA.
//
// The code as Shawn received it included the following notice:
//
//   Copyright (C) 1997 Makoto Matsumoto and Takuji Nishimura.  When
//   you use this, send an e-mail to <matumoto@math.keio.ac.jp> with
//   an appropriate reference to your work.
//
// It would be nice to CC: <Cokus@math.washington.edu> when you write.
//
//
// uint32 must be an unsigned integer type capable of holding at least 32
// bits; exactly 32 should be fastest, but 64 is better on an Alpha with
// GCC at -O3 optimization so try your options and see what's best for you
//

typedef unsigned long uint32;

#define N              (624)                 // length of state vector
#define M              (397)                 // a period parameter
#define K              (0x9908B0DFU)         // a magic constant
#define hiBit(u)       ((u) & 0x80000000U)   // mask all but highest   bit of u
#define loBit(u)       ((u) & 0x00000001U)   // mask all but lowest    bit of u
#define loBits(u)      ((u) & 0x7FFFFFFFU)   // mask     the highest   bit of u
#define mixBits(u, v)  (hiBit(u)|loBits(v))  // move hi bit of u to hi bit of v

class MTwist
{
private:
	uint32	state[N+1];	// state vector + 1 extra to not violate ANSI C
	uint32	*next;		// next random value is computed from here
	int		left;		// can *next++ this many times before reloading

public:
	MTwist() : left(-1)
	{
		// you can seed with any uint32, but the best are odds in 0..(2^32 - 1)
		uint32 tickcount = GetTickCount();
		if ((tickcount % 2) == 0)
			tickcount--;

		seedMT(tickcount);
	}

	void seedMT(uint32 seed);
	uint32 reloadMT();
	uint32 randomMT();
};

class PasswordHash
{
private:
	MTwist twister;

	char *itoa64;
	int iteration_count_log2;
	bool portable_hashes;
	uint32 random_state;

public:
	void get_random_bytes(char *dst, int count);
	void encode64(char *dst, char *src, int count);
	void gensalt_private(char* output, char *input);
	void crypt_private(char *dst, char *password, char *setting);
	void HashPassword(char *dst, char *input);
	bool CheckPassword(char *password, char *stored_hash);

	PasswordHash(int iteration_count_log2, bool portable_hashes)
	{
		this->itoa64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		if (iteration_count_log2 < 4 || iteration_count_log2 > 31)
			iteration_count_log2 = 8;
		this->iteration_count_log2 = iteration_count_log2;

		this->portable_hashes = portable_hashes;

		this->random_state = twister.randomMT();
	}
};
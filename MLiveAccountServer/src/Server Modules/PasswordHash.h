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

class PasswordHash
{
private:
	MC_MTwister twister;

	char *itoa64;
	int iteration_count_log2;
	bool portable_hashes;
	uint32 random_state;

public:
	void get_random_bytes	(char *dst, int count);
	void encode64			(char *dst, char *src, int count);
	void gensalt_private	(char *output, char *input);
	void crypt_private		(char *dst, wchar_t *password, char *setting);
	void HashPassword		(char *dst, wchar_t *input);
	bool CheckPassword		(wchar_t *password, char *stored_hash);

	PasswordHash(int iteration_count_log2, bool portable_hashes) : twister()
	{
		this->itoa64 = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

		if (iteration_count_log2 < 4 || iteration_count_log2 > 31)
			iteration_count_log2 = 8;
		this->iteration_count_log2 = iteration_count_log2;

		this->portable_hashes = portable_hashes;

		this->random_state = twister.Random();
	}
};
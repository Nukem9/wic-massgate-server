#pragma once

/*******************************************************************************
 *						phpass0.3/PasswordHash
 *
 * c++ implementation of phpass to generate password hashes that can be used with a php forum
 * http://www.openwall.com/phpass/
 * http://www.openwall.com/crypt/
 * http://cvsweb.openwall.com/cgi/cvsweb.cgi/projects/phpass/
 * http://cvsweb.openwall.com/cgi/cvsweb.cgi/Owl/packages/glibc/crypt_blowfish/
 *
 * - mersenne twister prng is used instead of microtime()/getmypid()/rand() function
 * - added a variation of phps hash_equals function
 *
 *******************************************************************************/

#define PHP_VERSION					5
#define CRYPT_BLOWFISH				1
#define CRYPT_EXT_DES				0
#define CRYPT_OUTPUT_SIZE			(7 + 22 + 31 + 1)
#define CRYPT_GENSALT_OUTPUT_SIZE	(7 + 22 + 1)

class PasswordHash
{
private:
	MC_MTwister twister;

	char *itoa64;
	int iteration_count_log2;
	bool portable_hashes;
	uint32 random_state;

	void get_random_bytes	(char *dst, int count);
	void encode64			(char *dst, char *src, int count);
	void gensalt_private	(char *output, char *input);
	void crypt_private		(char *dst, char *password, char *setting);
	void gensalt_extended	(char *output, char *input);
	void gensalt_blowfish	(char *output, char *input);
	void crypt_blowfish		(char *dst, char *password, char *setting);
	int hash_equals			(const void *stored_hash, const void *user_hash, const size_t size);

public:
	void HashPassword		(char *dst, char *input);
	bool CheckPassword		(char *password, char *stored_hash);

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
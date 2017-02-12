#include "../stdafx.h"

#define XXTEA_ROUND (((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((z ^ *((T *)this->m_Keys + (e ^ (p & keyPartSize)))) + (sum ^ y)))

MMG_BlockTEA::MMG_BlockTEA()
{
	this->m_Indentifier = CIPHER_BLOCKTEA;
	memset(this->m_Keys, 0, sizeof(this->m_Keys));
}

MMG_BlockTEA::MMG_BlockTEA(ulong *keys)
{
	this->m_Indentifier = CIPHER_BLOCKTEA;
	memcpy(this->m_Keys, keys, sizeof(this->m_Keys));
}

MMG_BlockTEA::MMG_BlockTEA(ulong key1, ulong key2, ulong key3, ulong key4)
{
	this->m_Indentifier = CIPHER_BLOCKTEA;
	this->m_Keys[0] = key1;
	this->m_Keys[1] = key2;
	this->m_Keys[2] = key3;
	this->m_Keys[3] = key4;
}

MMG_BlockTEA::~MMG_BlockTEA()
{
	memset(this->m_Keys, 0, sizeof(this->m_Keys));
}

template <typename T>
void MMG_BlockTEA::Encrypter(T *aData, sizeptr_t aDataLength)
{
	static T delta = (T)(0x9E3779B9 >> (32 - (sizeof(T) * 8)));

	if (aDataLength > 1)
	{
		T rounds	= (T)(6 + (52 / aDataLength));
		T sum		= 0;

		T			e = 0;
		sizeptr_t	p = 0;
		T			y = 0;
		T			z = aData[aDataLength - 1];

		// Size of each element in the key array
		T keyPartSize = (sizeof(this->m_Keys) / sizeof(T)) - 1;

		for (; rounds; rounds--)
		{
			sum += delta;
			e = (sum >> 2) & 3;

			for (p = 0; p < aDataLength - 1; p++)
			{
				y = aData[p + 1]; 
				z = aData[p] += XXTEA_ROUND;
			}

			y = aData[0];
			z = aData[aDataLength - 1] += XXTEA_ROUND;
		}
	}
}

template<typename T>
void MMG_BlockTEA::Decrypter(T *aData, sizeptr_t aDataLength)
{
	static T delta = (T)(0x9E3779B9 >> (32 - (sizeof(T) * 8)));

	if (aDataLength > 1)
	{
		T rounds	= (T)(6 + (52 / aDataLength));
		T sum		= rounds * delta;

		T			e = 0;
		sizeptr_t	p = 0;
		T			y = aData[0];
		T			z = aData[aDataLength - 1];

		// Size of each element in the key array
		T keyPartSize = (sizeof(this->m_Keys) / sizeof(T)) - 1;

		do
		{
			e = (sum >> 2) & 3;

			for (p = aDataLength - 1; p > 0; p--)
			{
				z = aData[p - 1];
				y = aData[p] -= XXTEA_ROUND;
			}

			z = aData[aDataLength - 1];
			y = aData[0] -= XXTEA_ROUND;
		} while ((sum -= delta) != 0);
	}
}

void MMG_BlockTEA::Encrypt(char *aData, sizeptr_t aDataLength)
{
	// First pass, use sizeof(ulong) byte blocks
	// Calculate the length in array elements of 'ulong' (rounded down to nearest multiple of 2)
	Encrypter<ulong>((ulong *)aData, (aDataLength / sizeof(ulong)) & 0xFFFFFE);

	// Second pass, use sizeof(uchar) byte blocks
	// Start at where the last encryption routine stops
	Encrypter<uchar>((uchar *)&aData[aDataLength & 0xFFFFFFF8], aDataLength % 8);
}

void MMG_BlockTEA::Decrypt(char *aData, sizeptr_t aDataLength)
{
	// First pass, use sizeof(ulong) byte blocks
	// Calculate the length in array elements of 'ulong' (rounded down to nearest multiple of 2)
	Decrypter<ulong>((ulong *)aData, (aDataLength / sizeof(ulong)) & 0xFFFFFE);

	// Second pass, use sizeof(uchar) byte blocks
	// Start at where the last encryption routine stops
	Decrypter<uchar>((uchar *)&aData[aDataLength & 0xFFFFFFF8], aDataLength % 8);
}
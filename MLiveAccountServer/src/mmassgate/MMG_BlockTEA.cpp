#include "../stdafx.h"

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

DWORD _S1_8 = 0;
DWORD delta = 0;
void test3(MMG_BlockTEA *aCipher, uint *aData, uint aDataLength)
{
	unsigned int v3; // ebx@3
	unsigned int *v4; // edi@3
	unsigned int v5; // esi@3
	unsigned int v6; // ecx@3
	unsigned int v7; // eax@4
	unsigned int v8; // ecx@5
	unsigned int v9; // edx@5
	unsigned int v10; // ebx@6
	int v11; // eax@6
	int v12; // ecx@6
	int v13; // ecx@6
	int v14; // esi@6
	unsigned int q; // [sp+4h] [bp-8h]@5
	int v17; // [sp+8h] [bp-4h]@5
	int e; // [sp+10h] [bp+4h]@5

	if ( !(_S1_8 & 1) )
	{
		_S1_8 |= 1u;
		delta = 0x9E3779B9u;
	}

	if ( aDataLength > 1 )
	{
		v3 = aDataLength;
		v4 = aData;
		v6 = 0;
		v5 = aData[aDataLength - 1];

		v7 = 52 / aDataLength + 6;
		if ( 52 / aDataLength != -6 )
		{
			do
			{
				v8 = delta + v6;
				v17 = v7 - 1;
				e = (v8 >> 2) & 3;
				v9 = 0;
				q = v8;
				if ( v3 != 1 )
				{
					do
					{
						v10 = v4[v9 + 1];
						v11 = (16 * v5 ^ (v4[v9 + 1] >> 3)) + ((v5 >> 5) ^ 4 * v10);
						v12 = e ^ v9++ & 3;
						v13 = v5 ^ aCipher->m_Keys[v12];
						v14 = v10 ^ q;
						v3 = aDataLength;
						v4[v9 - 1] += (v14 + v13) ^ v11;
						v5 = v4[v9 - 1];
					}
					while ( v9 < aDataLength - 1 );
				}
				v6 = q;
				v7 = v17;
				v4[v3 - 1] += ((*v4 ^ q) + (v5 ^ aCipher->m_Keys[e ^ v9 & 3])) ^ ((16 * v5 ^ (*v4 >> 3)) + ((v5 >> 5) ^ 4 * *v4));
				v5 = v4[v3 - 1];
			}
			while ( v17 );
		}
	}
}

DWORD _S2_4 = 0;
unsigned char delta_0 = 0;
void test4(MMG_BlockTEA *aCipher, char *aData, uint aDataLength)
{
	int v3; // esi@3
	unsigned __int8 v4; // cl@3
	unsigned __int8 v5; // bl@3
	char v6; // al@4
	int v7; // esi@6
	char v8; // [sp+5h] [bp-9h]@5
	int v9; // [sp+6h] [bp-8h]@3
	unsigned __int8 p; // [sp+16h] [bp+8h]@5

	if ( !(_S2_4 & 1) )
	{
		_S2_4 |= 1u;
		delta_0 = 0x9E;
	}
	v3 = (unsigned __int8)aDataLength;
	v4 = aData[(unsigned __int8)aDataLength - 1];
	v5 = 0;
	v9 = (unsigned __int8)aDataLength;
	if ( (unsigned __int8)aDataLength > 1u )
	{
	v6 = 52 / (unsigned __int8)aDataLength + 6;
	if ( 52 / (unsigned __int8)aDataLength != -6 )
	{
		do
		{
		v5 += delta_0;
		v8 = v6 - 1;
		p = 0;
		if ( v3 - 1 > 0 )
		{
			v7 = 0;
			do
			{
			aData[v7] += ((aData[v7 + 1] ^ v5) + (v4 ^ *((BYTE *)aCipher->m_Keys + ((v5 >> 2) & 3 ^ v7 & 0xF)))) ^ ((16 * v4 ^ ((unsigned __int8)aData[v7 + 1] >> 3)) + ((v4 >> 5) ^ (unsigned __int8)(4 * aData[v7 + 1])));
			v4 = aData[v7];
			++p;
			v7 = p;
			}
			while ( p < v9 - 1 );
			v3 = v9;
		}
		aData[v3 - 1] += ((*aData ^ v5) + (v4 ^ *((BYTE *)aCipher->m_Keys + ((v5 >> 2) & 3 ^ p & 0xF)))) ^ ((16 * v4 ^ ((unsigned __int8)*aData >> 3)) + ((v4 >> 5) ^ (unsigned __int8)(4 * *aData)));
		--v6;
		v4 = aData[v3 - 1];
		}
		while ( v8 );
	}
	}
}

void MMG_BlockTEA::Encrypt(uint *aData, uint aDataLength)
{
	test3(this, aData, (aDataLength >> 2) & 0xFFFFFE);

	char *derp = (char *)aData;
	char *data_pointer = &derp[aDataLength & 0xFFFFFFF8];

	test4(this, data_pointer, aDataLength & 0x7);
}

DWORD _S3_4 = 0;
DWORD delta_1 = 0;
void test1(MMG_BlockTEA *aCipher, uint *aData, uint aDataLength)
{
	unsigned int v3; // ebx@3
	unsigned int *v4; // edi@3
	unsigned int v5; // esi@3
	unsigned int v6; // eax@4
	int v7; // edx@5
	unsigned int v8; // ebx@8
	int v9; // ecx@8
	int v10; // ebp@8
	unsigned int i; // [sp+4h] [bp-4h]@4
	int e; // [sp+Ch] [bp+4h]@5

	if ( !(_S3_4 & 1) )
	{
		_S3_4 |= 1u;
		delta_1 = 0x9E3779B9u;
	}

	v3 = aDataLength;
	v4 = aData;
	v5 = *aData;
	if ( aDataLength > 1 )
	{
		v6 = delta_1 * (0x34 / aDataLength + 6);
		for ( i = delta_1 * (0x34 / aDataLength + 6); v6; i = v6 )
		{
			v7 = v3 - 1;
			e = (v6 >> 2) & 3;

			if ( v3 != 1 )
			{
				do
				{
					v6 = i;
					v4[v7] -= ((v5 ^ i) + (v4[v7 - 1] ^ aCipher->m_Keys[e ^ v7 & 3])) ^ ((16 * v4[v7 - 1] ^ (v5 >> 3))
																							+ ((v4[v7 - 1] >> 5) ^ 4 * v5));
					--v7;
					v5 = v4[v7 + 1];
				} while ( v7 );
				v3 = aDataLength;
			}

			v8 = v4[v3 - 1];
			v9 = (v8 >> 5) ^ 4 * v5;
			v10 = 16 * v8 ^ (v5 >> 3);
			v3 = aDataLength;
			*v4 -= ((v5 ^ v6) + (v4[aDataLength - 1] ^ aCipher->m_Keys[e ^ v7 & 3])) ^ (v10 + v9);
			v6 -= delta_1;
			v5 = *v4;
		}
	}
}

DWORD _S4_0 = 0;
unsigned char delta_2 = 0;
void test2(MMG_BlockTEA *aCipher, char *aData, uint aDataLength)
{
	char v3; // bl@3
	char *v4; // edi@3
	unsigned __int8 v5; // cl@3
	unsigned __int8 v6; // al@4
	unsigned __int8 i; // dl@5
	unsigned __int8 v8; // al@7
	int v9; // esi@8
	int v11; // [sp+Ah] [bp-4h]@4
	unsigned __int8 p; // [sp+12h] [bp+4h]@7
	unsigned __int8 sum; // [sp+16h] [bp+8h]@4

	if ( !(_S4_0 & 1) )
	{
		_S4_0 |= 1u;
		delta_2 = 0x9Eu;
	}
	v3 = aDataLength;
	v4 = aData;
	v5 = *aData;
	if((unsigned __int8)aDataLength > 1)
	{
		v11 = (unsigned __int8)aDataLength;
		sum = delta_2 * (52 / (unsigned __int8)aDataLength + 6);
		v6 = sum;
		if(sum)
		{
			for(i = v3 - 1; ; i = v3 - 1)
			{
				v8 = (v6 >> 2) & 3;
				p = i;

				if(i)
				{
					v9 = i;
					do
					{
						v4[v9] -= ((v5 ^ sum) + (v4[v9 - 1] ^ *((BYTE *)aCipher->m_Keys + (v8 ^ v9 & 0xF)))) ^ ((16 * v4[v9 - 1] ^ (v5 >> 3)) + (((unsigned __int8)v4[v9 - 1] >> 5) ^ (unsigned __int8)(4 * v5)));
						v5 = v4[v9--];
						--p;
					} while ( p );
				}

				*v4 -= ((v5 ^ sum) + (v4[v11 - 1] ^ *((BYTE *)aCipher->m_Keys + (v8 ^ p & 0xF)))) ^ ((16 * v4[v11 - 1] ^ (v5 >> 3)) + (((unsigned __int8)v4[v11 - 1] >> 5) ^ (unsigned __int8)(4 * v5)));
				v6 = sum - delta_2;
				v5 = *v4;
				sum = v6;
				if(!v6)
					break;
			}
		}
	}
}

void MMG_BlockTEA::Decrypt(uint *aData, uint aDataLength)
{
	test1(this, aData, (aDataLength / 4) & 0xFFFFFE);

	char *derp = (char *)aData;
	char *data_pointer = &derp[aDataLength & 0xFFFFFFF8/*round to the nearest multiple of 8*/];

	test2(this, data_pointer, aDataLength & 0x7 /*modulo-8 remainder*/);
}

uint MMG_BlockTEA::GetDelta()
{
	return 0x9E3779B9;
}
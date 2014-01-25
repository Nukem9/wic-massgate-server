#pragma once

class MP_Pack
{
public:

private:

public:
	static sizeptr_t PackZip	(voidptr_t aSource, voidptr_t aDestination, sizeptr_t aSourceLength, sizeptr_t aDestLength, sizeptr_t *numBytesConsumed);
	static sizeptr_t UnpackZip	(voidptr_t aSource, voidptr_t aDestination, sizeptr_t aSourceLength, sizeptr_t aDestLength, sizeptr_t *numBytesConsumed);

private:
};
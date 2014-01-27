#include "../stdafx.h"

sizeptr_t MP_Pack::PackZip(voidptr_t aSource, voidptr_t aDestination, sizeptr_t aSourceLength, sizeptr_t aDestLength, sizeptr_t *numBytesConsumed)
{
	z_stream stream;
	stream.next_in		= (Bytef *)aSource;
	stream.avail_in		= aSourceLength;
	stream.next_out		= (Bytef *)aDestination;
	stream.avail_out	= aDestLength;

	stream.zalloc		= Z_NULL;
	stream.zfree		= Z_NULL;
	stream.opaque		= Z_NULL;

	if (deflateInit(&stream, Z_BEST_SPEED))
		return 0;

	if (deflate(&stream, Z_FINISH) != Z_STREAM_END)
	{
		deflateEnd(&stream);

		if (numBytesConsumed)
			*numBytesConsumed = aSourceLength;

		return 0;
	}

	if (deflateEnd(&stream) != Z_OK)
		return 0;

	if (numBytesConsumed)
		*numBytesConsumed = stream.total_in;

	return stream.total_out;
}

sizeptr_t MP_Pack::UnpackZip(voidptr_t aSource, voidptr_t aDestination, sizeptr_t aSourceLength, sizeptr_t aDestLength, sizeptr_t *numBytesConsumed)
{
	z_stream stream;
	stream.next_in		= (Bytef *)aSource;
	stream.avail_in		= aSourceLength;
	stream.next_out		= (Bytef *)aDestination;
	stream.avail_out	= aDestLength;

	stream.zalloc		= Z_NULL;
	stream.zfree		= Z_NULL;
	stream.opaque		= Z_NULL;

	if (inflateInit(&stream))
		return 0;

	if (inflate(&stream, Z_FINISH) != Z_STREAM_END)
	{
		inflateEnd(&stream);

		if (numBytesConsumed)
			*numBytesConsumed = aSourceLength;

		return 0;
	}

	if (inflateEnd(&stream) != Z_OK)
		return 0;

	if (numBytesConsumed)
		*numBytesConsumed = stream.total_in;

	return stream.total_out;
}
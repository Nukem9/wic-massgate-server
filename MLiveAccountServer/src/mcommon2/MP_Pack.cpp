#include "../stdafx.h"

uint MP_Pack::UnpackZip(PVOID aSource, PVOID aDestination, unsigned int aSourceLength, unsigned int aDestLength, uint *numBytesConsumed)
{
	z_stream stream;
	stream.next_in		= (Bytef *)aSource;
	stream.avail_in		= aSourceLength;
	stream.next_out		= (Bytef *)aDestination;
	stream.avail_out	= aDestLength;

	stream.zalloc		= Z_NULL;
	stream.zfree		= Z_NULL;
	stream.opaque		= Z_NULL;

	if(inflateInit(&stream))
		return 0;

	if(inflate(&stream, Z_FINISH) != Z_STREAM_END)
	{
		inflateEnd(&stream);

		if(numBytesConsumed)
			*numBytesConsumed = aSourceLength;

		return 0;
	}

	if(inflateEnd(&stream) != Z_OK)
		return 0;

	if(numBytesConsumed)
		*numBytesConsumed = stream.total_in;

	return stream.total_out;
}
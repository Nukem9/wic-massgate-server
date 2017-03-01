#pragma once

class MMG_BitStream
{
public:
	enum StreamStatus
	{
		NoChange,
		OperationOk,
		EndOfStream,
	};

	enum StreamOffset
	{
		StartPosition,
		CurrentPosition,
		EndPosition,
	};

protected:
	uint						m_Position;
	uint						m_MaxLength;
	MMG_BitStream::StreamStatus m_Status;

public:
	MMG_BitStream(uint theRawBufferLengthInBits)
	{
		this->m_Position	= 0;
		this->m_MaxLength	= theRawBufferLengthInBits;
		this->m_Status		= StreamStatus::NoChange;
	}

private:
};

template<typename T>
class MMG_BitReader : public MMG_BitStream
{
public:

private:
	const static int TypeSizeInBits = (sizeof(T) * 8);

	unsigned int	m_BufferLength;
	const T			*m_ReadBuffer;

public:
	MMG_BitReader(const T *theRawBuffer, uint theRawBufferLengthInBits) : MMG_BitStream(theRawBufferLengthInBits)
	{
		this->m_BufferLength	= theRawBufferLengthInBits;
		this->m_ReadBuffer		= theRawBuffer;
		
		// Buffer bit length must divide evenly into bytes
		assert(theRawBufferLengthInBits / 8 >= sizeof(T));

		// Length must be a multiple of (sizeof(T) * 8)
		assert((theRawBufferLengthInBits & (~(sizeof(T) * 8 - 1))) == theRawBufferLengthInBits);
	}

	T ReadBits(T theNumBits)
	{
		if (theNumBits == 1)
		{
			// Read a single bit
			T value		= this->m_ReadBuffer[this->m_Position / TypeSizeInBits];
			uint mask	= TypeMask(this->m_Position);
			
			this->m_Position += 1;

			if (this->m_Position == this->m_MaxLength)
				this->m_Status = StreamStatus::EndOfStream;

			return (value & (1 << mask)) > 0;
		}
		else
		{
			assert(theNumBits <= TypeSizeInBits);

			if ((this->m_Position / TypeSizeInBits) == ((this->m_Position + theNumBits - 1) / TypeSizeInBits))
			{
				// Read X bits now as they fit into a single array index
				T value		= this->m_ReadBuffer[this->m_Position / TypeSizeInBits];
				uint mask	= TypeMask(this->m_Position);

				this->m_Position += theNumBits;

				if (this->m_Position == this->m_MaxLength)
					this->m_Status = StreamStatus::EndOfStream;

				return ((((1 << theNumBits) - 1) << mask) & value) >> mask;
			}
			else
			{
				// Read multiple because it spans across array indexes
				uint pos		= this->m_Position + theNumBits;
				uint firstHalf	= theNumBits - TypeMask(pos);
				uint secondHalf = TypeMask(pos);

				return this->ReadBits(firstHalf) | (this->ReadBits(secondHalf) << firstHalf);
			}
		}

		__debugbreak();
		__assume(0);
	}

private:
	template<typename U>
	inline U TypeMask(U Value)
	{
		return Value & (TypeSizeInBits - 1);
	}
};

template<typename T>
class MMG_BitWriter : public MMG_BitStream
{
public:

private:
	const static int TypeSizeInBits = (sizeof(T) * 8);

	uint	m_BufferLength;
	T		*m_DestBuffer;

public:
	MMG_BitWriter(T *theDestBuffer, uint theDestBufferLengthInBits) : MMG_BitStream(theDestBufferLengthInBits)
	{
		this->m_BufferLength	= theDestBufferLengthInBits;
		this->m_DestBuffer		= theDestBuffer;
	}

	void WriteBits(T theValue, T theNumBits)
	{
		if (theNumBits == 1)
		{
			T bitSetMask = 1 << TypeMask(this->m_Position);

			if (theValue & 1)
				this->m_DestBuffer[this->m_Position / TypeSizeInBits] |= bitSetMask;
			else
				this->m_DestBuffer[this->m_Position / TypeSizeInBits] &= ~bitSetMask;

			this->m_Position += 1;
		}
		else
		{
			uint v5 = 0;
			uint v6 = 0;
			uint v8 = 0;
			uint pos = 0;

			while (1)
			{
				v5	= this->m_Position;
				v6	= theNumBits;
				v8	= this->m_Position / TypeSizeInBits;
				pos = theNumBits + this->m_Position;

				if (v8 == ((pos - 1) / TypeSizeInBits))
					break;

				theNumBits	= TypeMask(pos);
				T firstHalf	= v6 - TypeMask(pos);

				this->WriteBits(theValue, firstHalf);

				theValue >>= firstHalf;

				if (theNumBits == 1)
					return this->WriteBits(theValue, 1);
			}

			this->m_DestBuffer[v8]	= (theValue << TypeMask(v5)) | this->m_DestBuffer[v8] & ~(((1 << theNumBits) - 1) << TypeMask(v5));
			this->m_Position		+= theNumBits;
		}

		if (this->m_Position == this->m_MaxLength)
			this->m_Status = StreamStatus::EndOfStream;
	}

private:
	template<typename U>
	inline U TypeMask(U Value)
	{
		return Value & (TypeSizeInBits - 1);
	}
};
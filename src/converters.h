#pragma once

#include <cstdint>
#include "bspan.h"

namespace waavs {
	//
// Note:  For the various 'as_xxx' routines, there is no size
// error checking.  It is assumed that whatever is calling the
// function will do appropriate error checking beforehand.
// This is a little unsafe, but allows the caller to decide where
// they want to do the error checking, and therefore control the
// performance characteristics better.

	// Read a single byte
	static uint8_t as_u8(const void *data) noexcept
	{
		uint8_t result = *((uint8_t*)data);

		return result;
	}

	// Read a unsigned 16 bit value
	// assuming stream is in little-endian format
	// and machine is also little-endian
	static uint16_t as_u16_le(const void *data) noexcept
	{
		uint16_t r = *((uint16_t*)data);

		return r;

		//return ((uint8_t *)fStart)[0] | (((uint8_t *)fStart)[1] << 8);
	}

	// Read a unsigned 32-bit value
	// assuming stream is in little endian format
	uint32_t as_u32_le(const void *data) noexcept
	{
		uint32_t r = *((uint32_t*)data);

		return r;

		//return ((uint8_t *)fStart)[0] | (((uint8_t *)fStart)[1] << 8) | (((uint8_t *)fStart)[2] << 16) |(((uint8_t *)fStart)[3] << 24);
	}

	// Read a unsigned 64-bit value
	// assuming stream is in little endian format
	uint64_t as_u64_le(const void *data) noexcept
	{
		uint64_t r = *((uint64_t*)data);

		return r;
		//return ((uint8_t *)fStart)[0] | (((uint8_t *)fStart)[1] << 8) | (((uint8_t *)fStart)[2] << 16) | (((uint8_t *)fStart)[3] << 24) |
		//    (((uint8_t *)fStart)[4] << 32) | (((uint8_t *)fStart)[5] << 40) | (((uint8_t *)fStart)[6] << 48) | (((uint8_t *)fStart)[7] << 56);
	}

	//=============================================
	// BIG ENDIAN
	//=============================================
	// Read a unsigned 16 bit value
	// assuming stream is in big endian format
	uint16_t as_u16_be(const void *data) noexcept
	{
		uint16_t r = *((uint16_t*)data);
		return bswap16(r);
	}

	// Read a unsigned 32-bit value
	// assuming stream is in big endian format
	uint32_t as_u32_be(const void *data) noexcept
	{
		uint32_t r = *((uint32_t*)data);
		return bswap32(r);
	}

	// Read a unsigned 64-bit value
	// assuming stream is in big endian format
	uint64_t as_u64_be(const void *data) noexcept
	{
		uint64_t r = *((uint64_t*)data);
		return bswap64(r);
	}
}

namespace waavs {
	static bool read_u8(ByteSpan& bs, uint8_t& value)  noexcept
	{
		if (bs.size() < 1)
			return false;

		value = as_u8(bs.data());
		bs.skip(1);

		return true;
	}

	static bool read_u16(ByteSpan& bs, uint16_t& value, bool bsIsLE = true)  noexcept
	{
		if (bs.size() < 2)
			return false;

		if (bsIsLE)
			value = as_u16_le(bs.data());
		else
			value = as_u16_be(bs.data());

		bs.skip(2);

		return true;
	}

	static bool read_u32(ByteSpan& bs, uint32_t& value, bool bsIsLE = true)  noexcept
	{
		if (bs.size() < 4)
			return false;

		if (bsIsLE)
			value = as_u32_le(bs.data());
		else
			value = as_u32_be(bs.data());

		bs.skip(4);

		return true;
	}

	static bool read_u64(ByteSpan& bs, uint64_t& value, bool bsIsLE = true)   noexcept
	{
		if (bs.size() < 8)
			return false;

		if (bsIsLE)
			value = as_u64_le(bs.data());
		else
			value = as_u64_be(bs.data());

		bs.skip(8);

		return true;
	}
}

namespace waavs {
	// Read le unsigned 32-bit value
	static bool read_u32_le(ByteSpan &bs, uint32_t& value) noexcept
	{
		if (bs.size() < 4)
			return false;

		value = as_u32_le(bs.data());
		bs.skip(4);

		return true;
	}

	// Read be unsigned 32-bit value
	bool read_u32_be(ByteSpan &bs, uint32_t& value) noexcept
	{
		if (bs.size() < 4)
			return false;

		value = as_u32_be(bs.data());
		bs.skip(4);

		return true;
	}

	// Read le unsigned 64-bit value
	static bool read_u64_le(ByteSpan &bs, uint64_t& value) noexcept
	{
		if (bs.size() < 8)
			return false;

		value = as_u64_le(bs.data());
		bs.skip(8);
		return true;
	}
}

namespace waavs {
	static bool read_i32_be(ByteSpan &bs, int32_t& value) noexcept
	{
		return read_u32_be(bs, (uint32_t&)value);
	}

	bool read_i32_le(ByteSpan &bs, int32_t& value) noexcept
	{
		return read_u32_le(bs, (uint32_t&)value);
	}
}


namespace waavs {
	bool read_f64_le(ByteSpan &bs, double& value) noexcept
	{
		return read_u64_le(bs, (uint64_t&)value);
	}
}
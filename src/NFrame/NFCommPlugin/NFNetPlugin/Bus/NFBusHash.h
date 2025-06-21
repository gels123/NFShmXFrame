// -------------------------------------------------------------------------
//    @FileName         :    NFBusHash.h
//    @Author           :    Yi.Gao
//    @Date             :   2022-09-18
//    @Module           :    NFBusPlugin
//    @Desc             :
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFHash.hpp"

template <bool IsHash64>
struct HashFactor;

template <>
struct HashFactor<false>
{
	static uint32_t Hash(uint32_t seed, const void* s, size_t l)
	{
		return NFHash::murmur_hash3_x86_32(s, static_cast<int>(l), seed);
		// CRC算法性能太差，包长度也不太可能超过2GB
		// return atbus::detail::crc32(crc, static_cast<const unsigned char *>(s), l);
	}
};

template <>
struct HashFactor<true>
{
	static uint64_t Hash(uint64_t seed, const void* s, size_t l)
	{
		return NFHash::murmur_hash3_x86_32(s, static_cast<int>(l), static_cast<uint32_t>(seed));
		// CRC算法性能太差，包长度也不太可能超过2GB
		// return atbus::detail::crc64(crc, static_cast<const unsigned char *>(s), l);
	}
};

// -------------------------------------------------------------------------
//    @FileName         :    NFPacketParseMgr.cpp
//    @Author           :    gaoyi
//    @Date             :    24-8-1
//    @Email            :    445267987@qq.com
//    @Module           :    NFPacketParseMgr
//
// -------------------------------------------------------------------------

#include "NFPacketParseMgr.h"

#include <NFComm/NFPluginModule/NFCheck.h>

#include "InternalPacketParse.h"
#include "NFComm/NFPluginModule/NFIPacketParse.h"

std::vector<NFIPacketParse*> NFPacketParseMgr::m_pPacketParse = {CreatePacketParse(PACKET_PARSE_TYPE_INTERNAL), CreatePacketParse(PACKET_PARSE_TYPE_INTERNAL), CreatePacketParse(PACKET_PARSE_TYPE_INTERNAL), CreatePacketParse(PACKET_PARSE_TYPE_INTERNAL), CreatePacketParse(PACKET_PARSE_TYPE_INTERNAL)};

void NFPacketParseMgr::DeletePacketParse(const NFIPacketParse* pPacketParse)
{
	if (pPacketParse)
	{
		NF_SAFE_DELETE(pPacketParse);
	}
}

void NFPacketParseMgr::ReleasePacketParse()
{
	for (size_t i = 0; i < m_pPacketParse.size(); i++)
	{
		DeletePacketParse(m_pPacketParse[i]);
	}
	m_pPacketParse.clear();
}

int NFPacketParseMgr::ResetPacketParse(uint32_t parseType, NFIPacketParse* pPacketParse)
{
	CHECK_NULL(0, pPacketParse);
	CHECK_EXPR(parseType < m_pPacketParse.size(), -1, "parseType:{}", parseType);
	if (m_pPacketParse[parseType])
	{
		DeletePacketParse(m_pPacketParse[parseType]);
		m_pPacketParse[parseType] = nullptr;
	}
	CHECK_EXPR(m_pPacketParse[parseType] == NULL, -1, "parseType:{} exist", parseType);
	m_pPacketParse[parseType] = pPacketParse;
	return 0;
}

NFIPacketParse* NFPacketParseMgr::CreatePacketParse(int parseType)
{
	if (parseType == PACKET_PARSE_TYPE_INTERNAL)
	{
		return NF_NEW InternalPacketParse();
	}
	else
	{
		return NF_NEW InternalPacketParse();
	}
}

int NFPacketParseMgr::DeCode(uint32_t packetType, const char* strData, uint32_t unLen, char*& outData, uint32_t& outLen, uint32_t& allLen, NFDataPackage& recvPackage)
{
	CHECK_EXPR(packetType < m_pPacketParse.size(), -1, "packetType:{}", packetType);
	CHECK_NULL(0, m_pPacketParse[packetType]);
	return m_pPacketParse[packetType]->DeCodeImpl(strData, unLen, outData, outLen, allLen, recvPackage);
}

int NFPacketParseMgr::EnCode(uint32_t packetType, const NFDataPackage& recvPackage, const char* strData, uint32_t unLen, NFBuffer& buffer, uint64_t nSendBusLinkId)
{
	CHECK_EXPR(packetType < m_pPacketParse.size(), -1, "packetType:{}", packetType);
	CHECK_NULL(0, m_pPacketParse[packetType]);
	return m_pPacketParse[packetType]->EnCodeImpl(recvPackage, strData, unLen, buffer, nSendBusLinkId);
}

// 使用 lzf 算法 压缩、解压
int NFPacketParseMgr::Compress(uint32_t packetType, const char* inBuffer, int inLen, void* outBuffer, unsigned int outSize)
{
	CHECK_EXPR(packetType < m_pPacketParse.size(), -1, "packetType:{}", packetType);
	CHECK_NULL(0, m_pPacketParse[packetType]);
	return m_pPacketParse[packetType]->CompressImpl(inBuffer, inLen, outBuffer, outSize);
}

int NFPacketParseMgr::Decompress(uint32_t packetType, const char* inBuffer, int inLen, void* outBuffer, int outSize)
{
	CHECK_EXPR(packetType < m_pPacketParse.size(), -1, "packetType:{}", packetType);
	CHECK_NULL(0, m_pPacketParse[packetType]);
	return m_pPacketParse[packetType]->DecompressImpl(inBuffer, inLen, outBuffer, outSize);
}

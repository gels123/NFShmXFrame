// -------------------------------------------------------------------------
//    @FileName         :    InternalPacketParse.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFNetPlugin
//
// -------------------------------------------------------------------------
#pragma once

#include <NFComm/NFPluginModule/NFIPacketParse.h>

class InternalPacketParse : public NFIPacketParse
{
public:
	InternalPacketParse();
	////////////////////////////////////////////////////////////////////
	int DeCodeImpl(const char* strData, uint32_t unLen, char*& outData, uint32_t& outLen, uint32_t& allLen, NFDataPackage& recvPackage) override;
	int EnCodeImpl(const NFDataPackage& recvPackage, const char* strData, uint32_t unLen, NFBuffer& buffer, uint64_t nSendBusLinkId = 0) override;
};

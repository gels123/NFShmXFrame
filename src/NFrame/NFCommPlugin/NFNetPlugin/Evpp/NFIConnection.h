// -------------------------------------------------------------------------
//    @FileName         :    NFIConnection.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFNetPlugin
//
//
//                    .::::.
//                  .::::::::.
//                 :::::::::::  FUCK YOU
//             ..:::::::::::'
//           '::::::::::::'
//             .::::::::::
//        '::::::::::::::..
//             ..::::::::::::.
//           ``::::::::::::::::
//            ::::``:::::::::'        .:::.
//           ::::'   ':::::'       .::::::::.
//         .::::'      ::::     .:::::::'::::.
//        .:::'       :::::  .:::::::::' ':::::.
//       .::'        :::::.:::::::::'      ':::::.
//      .::'         ::::::::::::::'         ``::::.
//  ...:::           ::::::::::::'              ``::.
// ```` ':.          ':::::::::'                  ::::..
//                    '.:::::'                    ':'````..
//
// -------------------------------------------------------------------------
#pragma once

#include <evpp/tcp_conn.h>

#include "NFComm/NFPluginModule/NFIModule.h"
#include "NFComm/NFPluginModule/NFNetDefine.h"

class NFIConnection : public NFIModule
{
public:
	NFIConnection(NFIPluginManager* p, NF_SERVER_TYPE serverType, const NFMessageFlag& flag): NFIModule(p), m_connectionType(0), m_unLinkId(0), m_serverType(serverType), m_flag(flag)
	{

	}

	~NFIConnection() override
	{

	}

	virtual void SetConnCallback(const evpp::ConnectionCallback& back)
	{
		m_connCallback = back;
	}

	virtual void SetMessageCallback(const evpp::MessageCallback& back)
	{
		m_messageCallback = back;
	}

	virtual uint64_t GetBusId() const { return m_flag.mBusId; }

	virtual uint64_t GetBusLength() const { return m_flag.mBusLength; }

	virtual uint32_t GetPacketParseType() const { return m_flag.mPacketParseType; }

	virtual bool IsSecurity() const { return m_flag.mSecurity; }

	virtual uint32_t GetConnectionType() { return m_connectionType; }

	virtual void SetConnectionType(uint32_t type) { m_connectionType = type; }

	virtual void SetLinkId(uint64_t id) { m_unLinkId = id; }
	virtual uint64_t GetLinkId() const { return m_unLinkId; }

	virtual bool IsActivityConnect() const { return m_flag.bActivityConnect; }

protected:
	evpp::ConnectionCallback m_connCallback;

	evpp::MessageCallback m_messageCallback;

	uint32_t m_connectionType;

	uint64_t m_unLinkId;

	NF_SERVER_TYPE m_serverType;

	NFMessageFlag m_flag;
};

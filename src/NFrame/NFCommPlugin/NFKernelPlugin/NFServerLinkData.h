// -------------------------------------------------------------------------
//    @FileName         :    NFCMsgModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFPluginModule/NFIMessageModule.h"
#include "NFComm/NFPluginModule/NFINetModule.h"
#include "NFComm/NFPluginModule/NFIHttpHandle.h"
#include "NFComm/NFCore/NFCommMapEx.hpp"
#include <stdint.h>

struct NetRpcService
{
	NetRpcService()
	{
		m_pTarget = NULL;
		m_pRpcService = NULL;
		m_createCo = false;
		m_iCount = 0;
		m_iAllUseTime = 0;
		m_iMinTime = 1000000000;
		m_iMaxTime = 0;
	}

	NetRpcService(NFIDynamicModule* pTarget, NFIRpcService* pRpcService, bool createCo): m_pTarget(pTarget),
																						 m_pRpcService(pRpcService),
																						 m_createCo(createCo)
	{
		m_iCount = 0;
		m_iAllUseTime = 0;
		m_iMinTime = 1000000000;
		m_iMaxTime = 0;
	}

	NetRpcService(const NetRpcService& functor)
	{
		if (this != &functor)
		{
			m_pTarget = functor.m_pTarget;
			m_pRpcService = functor.m_pRpcService;
			m_createCo = functor.m_createCo;
			m_iCount = functor.m_iCount;;
			m_iAllUseTime = functor.m_iAllUseTime;
			m_iMinTime = functor.m_iMinTime;
			m_iMaxTime = functor.m_iMaxTime;
		}
	}

	NetRpcService& operator=(const NetRpcService& functor)
	{
		if (this != &functor)
		{
			m_pTarget = functor.m_pTarget;
			m_pRpcService = functor.m_pRpcService;
			m_createCo = functor.m_createCo;
			m_iCount = functor.m_iCount;;
			m_iAllUseTime = functor.m_iAllUseTime;
			m_iMinTime = functor.m_iMinTime;
			m_iMaxTime = functor.m_iMaxTime;
		}

		return *this;
	}

	NFIDynamicModule* m_pTarget;
	NFIRpcService* m_pRpcService;
	bool m_createCo;
	uint64_t m_iCount;
	uint64_t m_iAllUseTime;
	uint64_t m_iMinTime;
	uint64_t m_iMaxTime;
};

struct NetReceiveFunctor
{
	NetReceiveFunctor()
	{
		m_pTarget = NULL;
		m_pFunctor = NULL;
		m_iCount = 0;
		m_iAllUseTime = 0;
		m_iMinTime = 1000000000;
		m_iMaxTime = 0;
		m_createCo = false;
	}

	NetReceiveFunctor(NFIDynamicModule* pTarget, const NET_RECEIVE_FUNCTOR& functor, bool createCo): m_pTarget(pTarget),
																									 m_pFunctor(functor),
																									 m_createCo(createCo)
	{
		m_iCount = 0;
		m_iAllUseTime = 0;
		m_iMinTime = 1000000000;
		m_iMaxTime = 0;
	}

	NetReceiveFunctor(const NetReceiveFunctor& functor)
	{
		if (this != &functor)
		{
			m_pTarget = functor.m_pTarget;
			m_pFunctor = functor.m_pFunctor;
			m_iCount = functor.m_iCount;;
			m_iAllUseTime = functor.m_iAllUseTime;
			m_iMinTime = functor.m_iMinTime;
			m_iMaxTime = functor.m_iMaxTime;
			m_createCo = functor.m_createCo;
		}
	}

	NetReceiveFunctor& operator=(const NetReceiveFunctor& functor)
	{
		if (this != &functor)
		{
			m_pTarget = functor.m_pTarget;
			m_pFunctor = functor.m_pFunctor;
			m_iCount = functor.m_iCount;;
			m_iAllUseTime = functor.m_iAllUseTime;
			m_iMinTime = functor.m_iMinTime;
			m_iMaxTime = functor.m_iMaxTime;
			m_createCo = functor.m_createCo;
		}

		return *this;
	}

	NFIDynamicModule* m_pTarget;
	NET_RECEIVE_FUNCTOR m_pFunctor;
	uint64_t m_iCount;
	uint64_t m_iAllUseTime;
	uint64_t m_iMinTime;
	uint64_t m_iMaxTime;
	bool m_createCo;
};

struct NetEventFunctor
{
	NetEventFunctor()
	{
		m_pTarget = NULL;
		m_pFunctor = NULL;
		m_createCo = false;
	}

	NetEventFunctor(NFIDynamicModule* pTarget, const NET_EVENT_FUNCTOR& functor, bool createCo): m_pTarget(pTarget),
																								 m_pFunctor(functor),
																								 m_createCo(createCo)
	{
	}

	NetEventFunctor(const NetEventFunctor& functor)
	{
		if (this != &functor)
		{
			m_pTarget = functor.m_pTarget;
			m_pFunctor = functor.m_pFunctor;
		}
	}

	NetEventFunctor& operator=(const NetEventFunctor& functor)
	{
		if (this != &functor)
		{
			m_pTarget = functor.m_pTarget;
			m_pFunctor = functor.m_pFunctor;
		}

		return *this;
	}

	NFIDynamicModule* m_pTarget;
	NET_EVENT_FUNCTOR m_pFunctor;
	bool m_createCo;
};

struct CallBack
{
	//call back
	CallBack()
	{
		mxReceiveCallBack.resize(NF_MODULE_MAX);
		for (int i = 0; i < (int)mxReceiveCallBack.size(); i++)
		{
			if (i <= NF_MODULE_CLIENT)
			{
				mxReceiveCallBack[i].resize(NF_NET_MAX_MSG_ID);
			}
			else
			{
				mxReceiveCallBack[i].resize(NF_NET_OTHER_MAX_MSG_ID);
			}
		}

		mxRpcCallBack.resize(NF_MODULE_MAX);
		for (int i = 0; i < (int)mxRpcCallBack.size(); i++)
		{
			if (i <= NF_MODULE_CLIENT)
			{
				mxRpcCallBack[i].resize(NF_NET_MAX_MSG_ID);
			}
			else
			{
				mxRpcCallBack[i].resize(NF_NET_OTHER_MAX_MSG_ID);
			}
		}
	}

	virtual ~CallBack()
	{
	}

	std::vector<std::vector<NetReceiveFunctor>> mxReceiveCallBack;
	std::unordered_map<uint64_t, NetEventFunctor> mxEventCallBack;
	std::unordered_map<uint64_t, NetReceiveFunctor> mxOtherMsgCallBackList;
	NetReceiveFunctor mxAllMsgCallBackList;
	std::unordered_map<uint32_t, std::unordered_map<std::string, HTTP_RECEIVE_FUNCTOR>> mxHttpMsgCBMap; //uint32_t => NFHttpType
	std::unordered_map<uint32_t, std::vector<HTTP_RECEIVE_FUNCTOR>> mxHttpOtherMsgCBMap;                //uint32_t => NFHttpType
	std::unordered_map<std::string, HTTP_FILTER_FUNCTOR> mxHttpMsgFliterMap;
	std::vector<std::vector<NetRpcService>> mxRpcCallBack;
};

struct ServerLinkData
{
	ServerLinkData()
	{
		mServerType = NF_ST_NONE;
		m_serverLinkId = 0;
		m_clientLinkId = 0;
		mServerList.resize(NF_ST_MAX);
		mServerListMap.resize(NF_ST_MAX);
		mCrossServerListMap.resize(NF_ST_MAX);
		mNoCrossServerListMap.resize(NF_ST_MAX);
	}

	NFCommMapEx<uint32_t, NFServerData> mServerMap;
	std::vector<std::vector<NF_SHARE_PTR<NFServerData>>> mServerList;
	std::vector<NFConsistentCommMapEx<uint32_t, NFServerData>> mServerListMap;
	std::vector<NFConsistentCommMapEx<uint32_t, NFServerData>> mCrossServerListMap;
	std::vector<NFConsistentCommMapEx<uint32_t, NFServerData>> mNoCrossServerListMap;
	NFCommMapEx<std::string, NFConsistentCommMapEx<uint32_t, NFServerData>> mDBStoreServerMap;
	std::map<uint64_t, uint32_t> mLinkIdToBusIdMap; //linkid -- busid
	NFServerData m_routeData;
	NFServerData m_masterServerData;
	NF_SERVER_TYPE mServerType;
	uint64_t m_serverLinkId;
	uint64_t m_clientLinkId;

	uint64_t GetServerLinkId() const;

	void SetServerLinkId(uint64_t linkId);

	uint64_t GetClientLinkId() const;

	void SetClientLinkId(uint64_t linkId);

	NF_SHARE_PTR<NFServerData> GetServerByServerId(uint32_t busId);

	NF_SHARE_PTR<NFServerData> GetServerByUnlinkId(uint64_t unlinkId);

	NF_SHARE_PTR<NFServerData> CreateServerByServerId(uint32_t busId, NF_SERVER_TYPE busServerType, const NFrame::ServerInfoReport& data);

	virtual void CloseServer(NF_SERVER_TYPE destServer, uint32_t busId, uint64_t usLinkId);

	virtual void CreateLinkToServer(uint32_t busId, uint64_t linkId);

	void DelServerLink(uint64_t linkId);

	NFServerData* GetRouteData() { return &m_routeData; }

	const NFServerData* GetRouteData() const { return &m_routeData; }

	NFServerData* GetMasterData() { return &m_masterServerData; }

	const NFServerData* GetMasterData() const { return &m_masterServerData; }

	void CloseAllLink(NFIMessageModule* pMessageModule);

	virtual int SendMsgToMasterServer(NFIMessageModule* pMessageModule, uint32_t nModuleId, uint32_t nMsgId, const google::protobuf::Message& xData, uint64_t valueId = 0);

	std::vector<NF_SHARE_PTR<NFServerData>> GetServerByServerType(NF_SERVER_TYPE serverTypes);

	NF_SHARE_PTR<NFServerData> GetFirstServerByServerType(NF_SERVER_TYPE serverTypes);

	NF_SHARE_PTR<NFServerData> GetFirstServerByServerType(NF_SERVER_TYPE serverTypes, bool crossServer);

	NF_SHARE_PTR<NFServerData> GetRandomServerByServerType(NF_SERVER_TYPE serverTypes);

	NF_SHARE_PTR<NFServerData> GetRandomServerByServerType(NF_SERVER_TYPE serverTypes, bool crossServer);

	NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE serverTypes, uint64_t value);

	NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE serverTypes, uint64_t value, bool crossServer);

	NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE serverTypes, const std::string& value);

	NF_SHARE_PTR<NFServerData> GetSuitServerByServerType(NF_SERVER_TYPE serverTypes, const std::string& value, bool crossServer);

	NF_SHARE_PTR<NFServerData> GetFirstDbServer(const std::string& dbName);
	NF_SHARE_PTR<NFServerData> GeRandomDbServer(const std::string& dbName);
	NF_SHARE_PTR<NFServerData> GetSuitDbServer(const std::string& dbName, uint64_t value);
	NF_SHARE_PTR<NFServerData> GetSuitDbServer(const std::string& dbName, const std::string& value);

	std::vector<NF_SHARE_PTR<NFServerData>> GetAllServer();

	std::vector<NF_SHARE_PTR<NFServerData>> GetAllServer(NF_SERVER_TYPE serverTypes);

	std::vector<NF_SHARE_PTR<NFServerData>> GetAllServer(NF_SERVER_TYPE serverTypes, bool isCrossServer);

	std::vector<std::string> GetDBNames();
};

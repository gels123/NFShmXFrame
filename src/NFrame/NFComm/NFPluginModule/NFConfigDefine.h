// -------------------------------------------------------------------------
//    @FileName         :    NFConfigDefine.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFPluginModule
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFPluginModule/NFILuaLoader.h"
#include "NFComm/NFCore/NFServerIDUtil.h"
#include "NFComm/NFCore/NFTime.h"
#include "NFComm/NFPluginModule/NFServerDefine.h"
#include "NFComm/NFKernelMessage/FrameComm.nanopb.h"
#include "NFComm/NFPluginModule/NFProtobufCommon.h"
#include <unordered_map>

#define DEFINE_LUA_STRING_LOAD_PLUGIN            "LoadPlugin"
#define DEFINE_LUA_STRING_FRAME_PLUGINS         "FramePlugins"
#define DEFINE_LUA_STRING_SERVER_PLUGINS        "ServerPlugins"
#define DEFINE_LUA_STRING_WORK_PLUGINS          "WorkPlugins"

#define DEFINE_LUA_STRING_LOG_LEVEL                "LogLevel"                    //log等级配置
#define DEFINE_LUA_STRING_LOG_FLUSH_LEVEL        "LogFlushLevel"                //log刷新等级配置

#define DEFINE_LUA_STRING_LOG_INFO                "LogInfo"            //log配置

#define DEFINE_LUA_STRING_EXTERNAL_DATA         "ExternalData"

enum EnumLogInfoLuaString
{
    LOG_INFO_LOG_ID = 0,
    LOG_INFO_DISPLAY = 1,
    LOG_INFO_LEVEL = 2,
    LOG_INFO_LOG_NAME = 3,
    LOG_INFO_LOG_GUID = 4,
};

typedef pbPluginConfig NFPluginConfig;

struct NFLogConfig
{
    NFLogConfig()
    {
        Clear();
    }
    
    void Clear()
    {
        mLineConfigList.clear();
    }
    
    std::vector<LogInfoConfig> mLineConfigList;
};

struct NFServerConfig : public pbNFServerConfig
{
public:
    NFServerConfig()
    {
        m_openZeroTime = 0;
    }
    
    void Parse()
    {
        for (int i = 0; i < (int) MysqlConfig.TBConfList.size(); i++)
        {
            struct pbTableConfig &tableConfig = MysqlConfig.TBConfList[i];
            mTBConfMap.emplace(tableConfig.TableName, tableConfig);
        }

        m_openZeroTime = NFTime::GetZeroTime(ServerOpenTime);
    }

    //获取开服时间
    uint64_t GetOpenTime() const { return ServerOpenTime; }
    uint64_t GetServerOpenTime() const { return ServerOpenTime; }
    
    //获取开服当天零点时间
    uint64_t GetOpenDayZeroTime() { return m_openZeroTime; }
    
    std::string GetServerId() const { return ServerId; }
    
    std::string GetServerName() const { return ServerName; }
    
    uint32_t GetBusId() const { return BusId; }
    
    uint32_t GetZoneId() const { return NFServerIDUtil::GetZoneID(BusId); }
    
    uint32_t GetWorldId() const { return NFServerIDUtil::GetWorldID(BusId); }
    
    std::string GetDefaultDBName() const { return DefaultDBName; }
    
    std::string GetCrossDBName() const { return CrossDBName; }
    
    uint32_t GetMaxOnlinePlayerNum() const { return MaxOnlinePlayerNum; }
    
    uint32_t GetHeartBeatTimeout() const { return HeartBeatTimeout; }
    
    uint32_t GetClientKeepAliveTimeout() const { return ClientKeepAliveTimeout; }
    
    bool IsCrossServer() const { return CrossServer; }

    template<typename T>
    int GetExternData(T& data)
    {
        auto pb = data.ToPb();
        bool flag = NFProtobufCommon::LuaToProtoMessage(m_externData, &pb);
        if (!flag)
        {
            return -1;
        }

        data.FromPb(pb);
        return 0;
    }

    std::unordered_map<std::string, pbTableConfig> mTBConfMap;
    uint64_t m_openZeroTime;            //开服那天的0点时间
    LuaIntf::LuaRef m_externData;
};

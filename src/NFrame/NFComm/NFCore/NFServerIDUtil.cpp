// -------------------------------------------------------------------------
//    @FileName         :    NFServerIDUtil.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFServerIDUtil
//
// -------------------------------------------------------------------------

#include "NFServerIDUtil.h"
#include "NFStringUtility.h"
#include "NFCommon.h"
#include "NFComm/NFCore/NFSocketLibFunction.h"

uint32_t NFServerIDUtil::GetWorldID(uint32_t busId)
{
    uint32_t dwSvrID = ntohl(busId);
    uint32_t worldId = dwSvrID >> (ZONE_ID_BITS + SERVER_TYPE_BITS + INST_ID_BITS);
    return worldId;
}


uint32_t NFServerIDUtil::GetZoneID(uint32_t busId)
{
    uint32_t dwSvrID = ntohl(busId);
    uint32_t zoneId = dwSvrID << WORLD_ID_BITS;
    zoneId >>= (WORLD_ID_BITS + SERVER_TYPE_BITS + INST_ID_BITS);
    return zoneId;
}

int NFServerIDUtil::GetZoneIDFromZoneAreaID(int iZoneAreaID)
{
    return iZoneAreaID % 10000;
}

uint32_t NFServerIDUtil::GetServerType(uint32_t busId)
{
    uint32_t dwSvrID = ntohl(busId);
    uint32_t serverType = dwSvrID << (WORLD_ID_BITS + ZONE_ID_BITS);
    serverType >>= (WORLD_ID_BITS + ZONE_ID_BITS + INST_ID_BITS);
    return serverType;
}


uint32_t NFServerIDUtil::GetInstID(uint32_t busId)
{
    uint32_t dwSvrID = ntohl(busId);
    uint32_t instId = dwSvrID << (WORLD_ID_BITS + ZONE_ID_BITS + SERVER_TYPE_BITS);
    instId >>= (WORLD_ID_BITS + ZONE_ID_BITS + SERVER_TYPE_BITS);
    return instId;
}

uint32_t NFServerIDUtil::GetZoneAreaID(uint32_t busId)
{
    return GetWorldID(busId) * AREA_DIGIT_BASE + GetZoneID(busId);
}

uint32_t NFServerIDUtil::MakeProcID(int world, int zone, int serverType, int inst)
{
    return htonl(MakeProcIDImpl(world, zone, serverType, inst));
}

uint32_t NFServerIDUtil::MakeProcIDImpl(int world, int zone, int serverType, int inst)
{
    uint32_t procId = world << (ZONE_ID_BITS + SERVER_TYPE_BITS + INST_ID_BITS)
        | zone << (SERVER_TYPE_BITS + INST_ID_BITS)
        | serverType << (INST_ID_BITS)
        | inst;
    return procId;
}

uint32_t NFServerIDUtil::GetBusID(const std::string& busname)
{
    std::vector<uint32_t> vec;
    NFStringUtility::SplitStringToUint32(busname, ".", vec);
    uint8_t world = 0;
    uint16_t zone = 0;
    uint8_t servertype = 0;
    uint8_t inst = 0;
    if (vec.size() >= 1)
    {
        world = vec[0];
    }

    if (vec.size() >= 2)
    {
        zone = vec[1];
    }

    if (vec.size() >= 3)
    {
        servertype = vec[2];
    }

    if (vec.size() >= 4)
    {
        inst = vec[3];
    }

    return MakeProcID(world, zone, servertype, inst);
}

uint32_t NFServerIDUtil::GetShmObjKey(const std::string& busname)
{
    std::vector<uint32_t> vec;
    NFStringUtility::SplitStringToUint32(busname, ".", vec);
    int world = 0;
    int zone = 0;
    int serverType = 0;
    int inst = 0;
    if (vec.size() >= 1)
    {
        world = vec[0];
    }

    if (vec.size() >= 2)
    {
        zone = vec[1];
    }

    if (vec.size() >= 3)
    {
        serverType = vec[2];
    }

    if (vec.size() >= 4)
    {
        inst = vec[3];
    }

    return MakeProcIDImpl(world, zone, serverType, inst);
}

std::string NFServerIDUtil::GetBusNameFromBusID(uint32_t busId)
{
    uint32_t worldId = GetWorldID(busId);
    uint32_t zoneId = GetZoneID(busId);
    uint32_t serverId = GetServerType(busId);
    uint32_t instId = GetInstID(busId);
    std::string str = NF_FORMAT("{}.{}.{}.{}", worldId, zoneId, serverId, instId);
    return str;
}

std::string NFServerIDUtil::GetBusNameFromBusID(const std::string& busId)
{
    return GetBusNameFromBusID(NFCommon::strto<uint32_t>(busId));
}

bool NFServerIDUtil::MakeAddress(const std::string& addrStr, NFChannelAddress& addr)
{
    addr.mAddress = addrStr;

    // 获取协议
    size_t scheme_end = addr.mAddress.find_first_of("://");
    if (addr.mAddress.npos == scheme_end)
    {
        return false;
    }

    addr.mScheme = addr.mAddress.substr(0, scheme_end);
    NFStringUtility::ToLower(addr.mScheme);

    size_t port_end = addr.mAddress.find_last_of(":");
    addr.mPort = 0;
    if (addr.mAddress.npos != port_end && port_end >= scheme_end + 3)
    {
        addr.mPort = NFCommon::strto<int>(addr.mAddress.c_str() + port_end + 1);
    }

    // 截取域名
    addr.mHost = addr.mAddress.substr(scheme_end + 3, (port_end == addr.mAddress.npos) ? port_end : port_end - scheme_end - 3);

    return true;
}

void NFServerIDUtil::MakeAddress(const std::string& scheme, const std::string& host, int port, NFChannelAddress& addr)
{
    addr.mScheme = scheme;
    addr.mHost = host;
    addr.mPort = port;
    addr.mAddress.reserve(addr.mScheme.size() + addr.mHost.size() + 4 + 8);
    addr.mAddress = addr.mScheme + "://" + addr.mHost;

    if (port > 0)
    {
        std::string portStr = NFCommon::tostr(port);
        addr.mAddress += ":";
        addr.mAddress += portStr;
    }
}

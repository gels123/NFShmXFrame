// -------------------------------------------------------------------------
//    @FileName         :    EnetObject.h
//    @Author           :    gaoyi
//    @Date             :    2025-03-13
//    @Email			:    445267987@qq.com
//    @Module           :    EnetObject
//
// -------------------------------------------------------------------------

#pragma once

#include <cstdint>
#include <enet/enet.h>
#include "NFComm/NFPluginModule/NFNetDefine.h"

class NFEnetMessage;

class EnetObject
{
public:
    friend NFEnetMessage;

    /**
     * @brief	构造函数
     */
    explicit EnetObject(ENetPeer* pConn);

    /**
     * @brief	析构函数
     */
    ~EnetObject();

    /**
     * @brief
     *
     * @return std::string
     */
    std::string GetStrIp() const;

    uint32_t GetPort() const;

    /**
     * @brief
     *
     * @param  val
     * @return void
     */
    void SetStrIp(const std::string& val);

    void SetPort(uint32_t port);

    /**
     * @brief	获得唯一ID
     *
     * @return
     */
    uint64_t GetLinkId() const;

    /**
    * @brief	获得唯一ID
    *
    * @return
    */
    void SetLinkId(uint64_t linkId);

    /**
     * @brief
     *
     * @return bool
     */
    bool GetNeedRemove() const;

    /**
     * @brief
     *
     * @param  val
     * @return void
     */
    void SetNeedRemove(bool val);

    /**
     * @brief 关闭对象禁止对象的读写功能
     *
     * @return void
     */
    void CloseObject() const;

    /**
    * @brief 是否是服务器端
    *
    * @return void
    */
    void SetIsServer(bool b);

    bool IsServer() const;

    /**
    * @brief 设置解析方法
    *
    * @return void
    */
    void SetPacketParseType(uint32_t packetType) { m_packetParseType = packetType; }

    void SetConnPtr(ENetPeer* pConn) { m_connPtr = pConn; }

    void SetLastHeartBeatTime(uint64_t updateTime) { m_lastHeartBeatTime = updateTime; }

    uint64_t GetLastHeartBeatTime() const { return m_lastHeartBeatTime; }

    void SetSecurity(bool security) { m_security = security; }

    bool IsSecurity() const { return m_security; }

    bool IsDisConnect() const;
    bool IsConnect() const;

protected:
    /**
     * @brief	代表客户端连接的唯一ID
     */
    uint64_t m_usLinkId;

    /**
     * @brief	连接代表的对方的IP
     */
    std::string m_strIp;
    uint32_t m_port;

    /**
     * @brief 是否需要删除, 这个链接不在起作用，将在下一次循环中被删除
     */
    bool m_needRemove;

    /**
    * @brief is server
    */
    bool m_isServer; //是否是服务器端

    /**
    * @brief 解码消息类型
    */
    uint32_t m_packetParseType;

    /**
    * @brief 来自EVPP的链接代理
    */
    ENetPeer* m_connPtr;

    /**
    * @brief 心跳包更新时间
    */
    uint64_t m_lastHeartBeatTime;

    bool m_security;
};

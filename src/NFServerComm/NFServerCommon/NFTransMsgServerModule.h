// -------------------------------------------------------------------------
//    @FileName         :    NFTransMsgServerModule.h
//    @Author           :    gaoyi
//    @Date             :    22-10-28
//    @Email			:    445267987@qq.com
//    @Module           :    NFTransMsgServerModule
//
// -------------------------------------------------------------------------

#include "NFComm/NFPluginModule/NFIDynamicModule.h"

/**
 * @brief ����������NFProxyServer,NFProxyAgentServer,NFRouteAgentServer��ʵ����������NFMasterServer�ȹ���
 */
class NFTransMsgServerModule : public NFIDynamicModule
{
public:
    NFTransMsgServerModule(NFIPluginManager *p, NF_SERVER_TYPES serverType) : NFIDynamicModule(p), m_serverType(serverType)
    {
        m_connectMasterServer = true;
    }

    virtual ~NFTransMsgServerModule()
    {

    }

    /**
     * @brief ����ʱ��
     * @param nTimerID
     */
    virtual void OnTimer(uint32_t nTimerID) override;

    /**
     * @brief ��Ӧע���¼�
     * @param nEventID
     * @param nSrcID
     * @param bySrcType
     * @param message
     * @return
     */
    virtual int OnExecute(uint32_t nEventID, uint64_t nSrcID, uint32_t bySrcType, const google::protobuf::Message& message) override;
public:
    int BindServer();
    int ConnectMasterServer();
public:
    //////////////////////////////////////////////////////////Server������//////////////////////////////////////////////////////////////////
    /**
     * @brief Server�����������¼�������������/bus�������¼�
     * @param nEvent
     * @param unLinkId
     * @return
     */
    virtual int OnServerSocketEvent(eMsgType nEvent, uint64_t unLinkId);

    /**
     * @brief ����Serverδע�����Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerOtherMessage(uint64_t unLinkId, NFDataPackage &packet);

    /**
     * @brief ����Server�����������ӵ���
     * @param unLinkId
     * @return
     */
    virtual int OnHandleServerDisconnect(uint64_t unLinkId);
    //////////////////////////////////////////////////////////Server������//////////////////////////////////////////////////////////////////
public:
    //////////////////////////////////////////////////////////NFMasterServer������//////////////////////////////////////////////////////////////////
    /**
     * @brief ����Master������
     * @param xData
     * @return
     */
    virtual int ConnectMasterServer(const proto_ff::ServerInfoReport &xData);

    /**
     * @brief ע��Master������
     * @param serverState
     * @return
     */
    virtual int RegisterMasterServer(uint32_t serverState);

    /**
     * @brief ����Master�����������¼�
     * @param nEvent
     * @param unLinkId
     * @return
     */
    virtual int OnMasterSocketEvent(eMsgType nEvent, uint64_t unLinkId);

    /**
     * @brief ����Master������δע����Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleMasterOtherMessage(uint64_t unLinkId, NFDataPackage &packet);

    /**
     * @brief ��������MasterServer�������������ı���
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerReportFromMasterServer(uint64_t unLinkId, NFDataPackage &packet);

    /**
     * @brief ��������MasterServer�������������ı���
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleOtherServerReportFromMasterServer(const proto_ff::ServerInfoReport &xData);

    /**
     * @brief ÿ��һ��ʱ����Master����������������Ϣ
     * @return
     */
    virtual int ServerReportToMasterServer();
    //////////////////////////////////////////////////////////NFMasterServer������//////////////////////////////////////////////////////////////////
public:
    bool IsConnectMasterServer() const;

    void SetConnectMasterServer(bool connectMasterServer);
private:
    NF_SERVER_TYPES m_serverType;
    bool m_connectMasterServer;
};


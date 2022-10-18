// -------------------------------------------------------------------------
//    @FileName         :    NFIWorldPlayerModule.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFIWorldPlayerModule
//
// -------------------------------------------------------------------------

#pragma once

#include <ServerInternal.pb.h>
#include "NFComm/NFPluginModule/NFIDynamicModule.h"
#include "NFWorldPlayerMgr.h"

class NFCWorldPlayerModule : public NFIDynamicModule
{
public:
    explicit NFCWorldPlayerModule(NFIPluginManager *p);

    virtual ~NFCWorldPlayerModule();

    virtual bool Awake() override;

    virtual bool Execute() override;

    virtual bool OnDynamicPlugin() override;

public:
    /**
     * @brief ����������Ϣ���Ƿ�Ӧ�ô���
     * @param unLinkId
     * @param packet
     * @return
     */
    int OnCheckWorldServerMsg(uint64_t unLinkId, NFDataPackage &packet);

    /**
     * @brief �����¼Э��
     * @param unLinkId
     * @param packet
     * @return
     */
    int OnHandleClientCenterLogin(uint64_t unLinkId, NFDataPackage &packet);
public:
    /**
     * @brief logic msg
     */

    /**
     * @brief ��ȡ��ɫ�б�
     * @param unLinkId
     * @param packet
     * @return
     */
    int OnHandleLogicGetRoleListRsp(uint64_t unLinkId, NFDataPackage &packet);

    /**
     * @brief
     * @param charDBBaseInfo
     * @param charSimpleInfo
     * @return
     */
    int CharDBToCharSimpleDB(const proto_ff::tbRoleInfo& charDBBaseInfo, proto_ff::CharacterDBSimpleInfo& charSimpleInfo);
public:
    /**
     * @brief �����֪ͨ���ظı��߼���
     * @param pPlayer
     * @param ctype
     * @param logicId �߼���Id
     * @param force
     * @param flag
     * @return
     */
    int GateChangeLogic(NFWorldPlayer* pPlayer, proto_ff::NotifyGateChangeLogic_cType ctype, uint32_t logicId, bool force = false, proto_ff::LOGOUT_FLAG flag = proto_ff::LOGOUT_FLAG_NORMAL);

    /**
     * @brief
     * @param pPlayer
     * @param logicId
     * @param type
     * @return
     */
    int NotifyLogicLeave(NFWorldPlayer* pPlayer, uint32_t logicId, proto_ff::LOGOUT_TYPE type = proto_ff::LOGOUT_TYPE_NONE);
private:
    /**
     * @brief NFILuaModule
     */
    NFILuaModule m_luaModule;
};

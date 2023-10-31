// -------------------------------------------------------------------------
//    @FileName         :    NFItemMgr.h
//    @Author           :    gaoyi
//    @Date             :    23-10-31
//    @Email			:    445267987@qq.com
//    @Module           :    NFItemMgr
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFShmObjTemplate.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "NFGameCommon/NFComTypeDefine.h"
#include "NFItem.h"

class NFItemMgr : public NFShmObjTemplate<NFItemMgr, EOT_SERVER_COMMON_ITEM_MGR_ID, NFShmObj>
{
public:
    NFItemMgr();
    
    virtual ~NFItemMgr();
    
    int CreateInit();
    int ResumeInit();
public:
    ///////////////////////////////// ��Դ�� ������Ʒ �ӿ� /////////////////////////////////////////
    //�Ƿ�����Դ�� ������Ʒ
    bool IsVirItem(uint64_t itemId);
    //�Ƿ�����Դ�� ������Ʒ, ����ǣ��� attrId ������Ϊ ������Ʒ��Ӧ������ID
    bool IsVirItem(uint64_t itemId, uint32_t &attrId);
    //���˳�������Ʒ������
    void FilterVirItem(LIST_ITEM &lstItem, LIST_ITEM &lstOutItem, MAP_UINT32_INT64 &mapAttr);
    //���˳�������Ʒ������
    void FilterVirItem(LIST_ITEM &lstItem, LIST_ITEM &lstOutItem);
    //���˳�������Ʒ������
    void FilterVirItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecOutProtoItemsEx, MAP_UINT32_INT64 &mapAttr);
    //���˳�������Ʒ������
    void FilterVirItem(VEC_ITEM_PROTO_EX &vecProtoItemsEx, VEC_ITEM_PROTO_EX &vecOutProtoItemsEx);
    bool IsDynExpItem(uint64_t itemId);
public:

};
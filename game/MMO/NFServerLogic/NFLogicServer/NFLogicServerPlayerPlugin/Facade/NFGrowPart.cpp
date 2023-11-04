// -------------------------------------------------------------------------
//    @FileName         :    NFGrowPart.cpp
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFGrowPart
//
// -------------------------------------------------------------------------

#include "NFGrowPart.h"
#include "DescStoreEx/NFGrowDescEx.h"
#include "proto_svr_event.pb.h"
#include "NFLogicCommon/NFEventDefine.h"
#include "NFLogicCommon/NFPackageDefine.h"
#include "Package/NFPackagePart.h"

NFGrowPart::GrowPartEntry*  NFGrowPart::GrowPartData::GetPartEntry(int64_t id)
{
    auto iter = entryMap.find(id);
    return iter != entryMap.end() ? &(iter->second) : nullptr;
}

void NFGrowPart::GrowPartData::SetPartEntry(NFGrowPart::GrowPartEntry& entry)
{
    entryMap[entry.id] = entry;
}

NFGrowPart::NFGrowPart()
{
    if (EN_OBJ_MODE_INIT == NFShmMgr::Instance()->GetCreateMode())
    {
        CreateInit();
    }
    else
    {
        ResumeInit();
    }
}

NFGrowPart::~NFGrowPart()
{
}

int NFGrowPart::CreateInit()
{
    return 0;
}

int NFGrowPart::ResumeInit()
{
    return 0;
}

int NFGrowPart::Init(NFPlayer *pMaster, uint32_t partType, const proto_ff::RoleDBData &dbData)
{
    CHECK_NULL(pMaster);
    m_pMaster = pMaster;
    m_playerId = pMaster->GetCid();
    m_partType = partType;
    
    initParts();
    
    LoadFromDB(dbData);
    InitConfig(dbData);
    return 0;
}

int NFGrowPart::UnInit()
{
    return NFPart::UnInit();
}

int NFGrowPart::LoadFromDB(const proto_ff::RoleDBData &dbData)
{
    const proto_ff::GrowDBData &growDB = dbData.grow();
    int32_t partSize = growDB.parts_size();
    
    for (int32_t i = 0; i < partSize; i++)
    {
        const proto_ff::GrowPartDBData &partdb = growDB.parts(i);
        int32_t partEntrySize = partdb.entrys_size();
        GrowPartData tp;
        tp.type = partdb.type();
        tp.curId = partdb.curid();
        for (int32_t j = 0; j < partEntrySize; j++)
        {
            GrowPartEntry entry;
            entry.id = partdb.entrys(j).id();
            entry.lv = partdb.entrys(j).lv();
            tp.entryMap[entry.id] = entry;
        }
        m_partsMap[tp.type] = tp;
    }
    
    calcAttr(false);
    
    return 0;
}

int NFGrowPart::InitConfig(const proto_ff::RoleDBData &data)
{
    return NFPart::InitConfig(data);
}

int NFGrowPart::SaveDB(proto_ff::RoleDBData &dbData)
{
    proto_ff::GrowDBData *pGrow = dbData.mutable_grow();
    CHECK_NULL(pGrow);
    for (auto &e: m_partsMap)
    {
        const GrowPartData &part = e.second;
        proto_ff::GrowPartDBData *pPartProto = pGrow->add_parts();
        
        pPartProto->set_type(part.type);
        pPartProto->set_curid(part.curId);
        
        for (auto &ie: part.entryMap)
        {
            proto_ff::GrowPartEntryDBInfo *pPartEntry = pPartProto->add_entrys();
            CHECK_NULL(pPartEntry);
            pPartEntry->set_id(ie.second.id);
            pPartEntry->set_lv(ie.second.lv);
        }
    }
    
    return 0;
}

int NFGrowPart::OnLogin()
{
    SendAllGrowData();
    return 0;
}

int NFGrowPart::FillFacadeProto(proto_ff::RoleFacadeProto &outproto)
{
    for (auto& e : m_partsMap)
    {
        proto_ff::Attr64* at = outproto.add_growfacade();
        if (at)
        {
            at->set_id(e.second.type);
            at->set_value(e.second.curId);
        }
    }
    return 0;
}

void NFGrowPart::FillHeadProto(proto_ff::RoleHeadPicProto& proto)
{
    proto.set_frame(0);
    proto.set_head(0);
    GrowPartData* pPart = getPart(proto_ff::GrowType_proto_frame);
    if (nullptr != pPart) proto.set_frame(pPart->curId);
    pPart = getPart(proto_ff::GrowType_head);
    if (nullptr != pPart) proto.set_head(pPart->curId);
}

int NFGrowPart::RegisterMessage()
{
    RegisterClientMessage(proto_ff::CLIENT_GROW_LVUP_REQ);
    RegisterClientMessage(proto_ff::CLIENT_GROW_FACADE_DRESS_REQ);
    RegisterClientMessage(proto_ff::CLIENT_GROW_FACADE_UNDRESS_REQ);
    return 0;
}

int NFGrowPart::OnHandleClientMessage(uint32_t msgId, NFDataPackage &packet)
{
    switch (msgId)
    {
        case proto_ff::CLIENT_GROW_LVUP_REQ:
            HanlderLvUpReq(msgId, packet);
            break;
        case proto_ff::CLIENT_GROW_FACADE_DRESS_REQ:
            HanlderDressReq(msgId, packet);
            break;
        case proto_ff::CLIENT_GROW_FACADE_UNDRESS_REQ:
            HanlderUnDressReq(msgId, packet);
            break;
        default:
            break;
    }
    return 0;
}

int NFGrowPart::OnHandleServerMessage(uint32_t msgId, NFDataPackage &packet)
{
    return NFPart::OnHandleServerMessage(msgId, packet);
}

int NFGrowPart::HanlderLvUpReq(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::GrowLvUpReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    int64_t id = req.id();
    
    GrowInfoCfg* pGrowCfg = NFGrowDescEx::Instance()->GetGrowCfg(id);
    CHECK_EXPR(pGrowCfg, -1, "GrowPart::HanlderLvUpReq id:{} not find", id);
    
    GrowPartData* pPart = getPart(pGrowCfg->type);
    CHECK_EXPR(pPart, -1, "GrowPart::HanlderLvUpReq id:{} not find type", id);
    
    GrowPartEntry* pPartEntry = pPart->GetPartEntry(id);
    if (pPartEntry)
    {
        OnLvupPartEntry(id);
    }
    else
    {
        OnActivePartEntry(id);
    }
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

int NFGrowPart::HanlderDressReq(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::GrowFacadeDressReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    int64_t id = req.id();
    
    GrowInfoCfg* pGrowCfg = NFGrowDescEx::Instance()->GetGrowCfg(id);
    CHECK_EXPR(pGrowCfg, -1, "GrowPart::HanlderDressReq id:{} not find", id);
    int32_t type = pGrowCfg->type;
    GrowPartData* pPart = getPart(pGrowCfg->type);
    CHECK_EXPR(pPart, -1, "GrowPart::HanlderDressReq id:%d type:{} not find type", id,type);
    
    GrowPartEntry* pPartEntry = pPart->GetPartEntry(id);
    
    proto_ff::GrowFacadeDressRsp rsp;
    rsp.set_id(id);
    rsp.set_ret(proto_ff::RET_SUCCESS);
    if (!pPartEntry)
    {
        rsp.set_ret(proto_ff::RET_GROW_PART_NOT_ACTIVE);
        m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_FACADE_DRESS_RSP, rsp);
        return -1;
    }
    
    pPart->curId = id;
    
    MarkDirty();
    m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_FACADE_DRESS_RSP, rsp);
    SendGrowPartData(type);
    
    //���ɲ��������¼�
    proto_ff::GrowPartDressEvent event;
    event.set_id(id);
    event.set_type(type);
    event.set_curlv(pPartEntry->lv);
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_GROW_PART_DRESS, CREATURE_PLAYER, m_pMaster->Cid(), event);
    //ͬ�����
    m_pMaster->SyncFacade();
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

int NFGrowPart::HanlderUnDressReq(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::GrowFacadeUnDressReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    int64_t id = req.id();
    
    GrowInfoCfg* pGrowCfg = NFGrowDescEx::Instance()->GetGrowCfg(id);
    CHECK_EXPR(pGrowCfg, -1, "GrowPart::HanlderUnDressReq id:{} not find", id);
    int32_t type = pGrowCfg->type;
    GrowPartData* pPart = getPart(pGrowCfg->type);
    CHECK_EXPR(pPart, -1, "GrowPart::HanlderUnDressReq id:{} type:{} not find type", id, type);
    
    GrowPartEntry* pPartEntry = pPart->GetPartEntry(id);
    
    proto_ff::GrowFacadeUnDressRsp rsp;
    rsp.set_id(id);
    rsp.set_ret(proto_ff::RET_SUCCESS);
    if (!pPartEntry)
    {
        rsp.set_ret(proto_ff::RET_FAIL);
        m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_FACADE_UNDRESS_RSP, rsp);
        return -1;
    }
    
    pPart->curId = 0;
    
    MarkDirty();
    m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_FACADE_UNDRESS_RSP, rsp);
    SendGrowPartData(type);
    //ͬ�����
    m_pMaster->SyncFacade();
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

int NFGrowPart::OnActivePartEntry(int64_t id)
{
    GrowInfoCfg* pGrowCfg = NFGrowDescEx::Instance()->GetGrowCfg(id);
    CHECK_EXPR(pGrowCfg, -1, "GrowPart::OnActivePartEntry id:{} not find", id);
    
    int32_t type = pGrowCfg->type;
    
    GrowPartData* pPart = getPart(type);
    CHECK_EXPR(pPart, -1, "GrowPart::OnActivePartEntry id:{} not find type", id);
    
    GrowPartEntry* pPartEntry = pPart->GetPartEntry(id);
    CHECK_EXPR((pPartEntry == nullptr), -1,"GrowPart::OnActivePartEntry id:{} not null",id);
    
    proto_ff::GrowLvUpRsp rsp;
    rsp.set_id(id);
    rsp.set_ret(proto_ff::RET_SUCCESS);
    int32_t prof = m_pMaster->GetAttr(proto_ff::A_PROF);
    CHECK_EXPR(pGrowCfg->LimitProf(prof) == false, -1, "GrowPart::OnActivePartEntry prof:{} limit", prof);
    
    SCommonSource sourceParam;
    sourceParam.src = S_grow;
    sourceParam.param1 = id;
    sourceParam.param2 = type;
    
    //���ͼ����¼�
    LIST_ITEM lstItem;
    SItem item;
    item.nItemID = pGrowCfg->activationItem;
    item.nNum = pGrowCfg->activationNum;
    item.byBind = EBindState_all;
    lstItem.push_back(item);
    NFPackagePart* pPackagePart = dynamic_cast<NFPackagePart*>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_NULL(pPackagePart);
    
    if (!pPackagePart->RemoveItem(lstItem, sourceParam))
    {
        rsp.set_ret(proto_ff::RET_PACKAGE_ITEM_NUM_LACK);
        m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_LVUP_RSP, rsp);
        return -1;
    }
    GrowPartEntry t;
    t.id = id;
    t.lv = 0;
    pPart->SetPartEntry(t);
    
    MarkDirty();
    m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_LVUP_RSP, rsp);
    SendGrowPartData(type);
    
    //��������
    calcAttr(true);
    
    //���ɲ��������¼�
    proto_ff::GrowPartActiveEvent event;
    event.set_id(id);
    event.set_type(type);
    event.set_curlv(0);
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_GROW_PART_ACTIVE, CREATURE_PLAYER, m_pMaster->Cid(), event);
    return 0;
}

int NFGrowPart::OnLvupPartEntry(int64_t id)
{
    GrowInfoCfg* pGrowCfg = NFGrowDescEx::Instance()->GetGrowCfg(id);
    CHECK_EXPR(pGrowCfg, -1, "GrowPart::OnActivePartEntry id:{} not find", id);
    
    int32_t type = pGrowCfg->type;
    
    GrowPartData* pPart = getPart(type);
    CHECK_EXPR(pPart, -1, "GrowPart::OnActivePartEntry id:{} not find type", id);
    
    GrowPartEntry* pPartEntry = pPart->GetPartEntry(id);
    CHECK_EXPR((pPartEntry == nullptr), -1,"GrowPart::OnActivePartEntry id:{} not null",id);
    
    proto_ff::GrowLvUpRsp rsp;
    rsp.set_id(id);
    rsp.set_ret(proto_ff::RET_SUCCESS);
    
    uint32_t curLv = pPartEntry->lv;
    
    int32_t prof = m_pMaster->GetAttr(proto_ff::A_PROF);
    CHECK_EXPR(pGrowCfg->LimitProf(prof) == false, -1, "GrowPart::OnLvupPartEntry prof:{} limit", prof);
    
    if (curLv >= pGrowCfg->lvMap.size())
    {
        rsp.set_ret(proto_ff::RET_GROW_PART_ENTRY_LEVEL_FULL);
        m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_LVUP_RSP, rsp);
        return -1;
    }
    
    int32_t needNum = pGrowCfg->GetStarLvNum(curLv + 1);
    if (needNum <= 0)
    {
        rsp.set_ret(proto_ff::RET_FAIL);
        m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_LVUP_RSP, rsp);
        return -1;
    }
    
    SCommonSource sourceParam;
    sourceParam.src = S_grow;
    sourceParam.param1 = id;
    sourceParam.param2 = type;
    
    //���ͼ����¼�
    LIST_ITEM lstItem;
    SItem item;
    item.nItemID = pGrowCfg->startItem;
    item.nNum = needNum;
    item.byBind = EBindState_all;
    lstItem.push_back(item);
    NFPackagePart* pPackagePart = dynamic_cast<NFPackagePart*>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_NULL(pPackagePart);
    
    if (!pPackagePart->RemoveItem(lstItem, sourceParam))
    {
        rsp.set_ret(proto_ff::RET_PACKAGE_ITEM_NUM_LACK);
        m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_LVUP_RSP, rsp);
        return -1;
    }
    
    pPartEntry->lv++;
    
    MarkDirty();
    m_pMaster->SendMsgToClient(proto_ff::CLIENT_GROW_LVUP_RSP, rsp);
    SendGrowPartData(type);
    
    //��������
    calcAttr(true);
    
    //���ɲ��������¼�
    proto_ff::GrowPartLvUpEvent event;
    event.set_id(id);
    event.set_type(type);
    event.set_curlv(curLv+1);
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_GROW_PART_LVUP, CREATURE_PLAYER, m_pMaster->Cid(), event);
    return 0;
}

int NFGrowPart::FillProto(const NFGrowPart::GrowPartData &part, proto_ff::GrowSubInfo &proto)
{
    proto.set_type(part.type);
    
    for (auto& e : part.entryMap)
    {
        const GrowPartEntry& entry = e.second;
        proto_ff::GrowStarInfo* pStarProto = proto.add_subs();
        pStarProto->set_id(entry.id);
        pStarProto->set_lv(entry.lv);
        pStarProto->set_state(part.curId == entry.id ? 1 : 0);
    }
    return 0;
}

NFGrowPart::GrowPartData *NFGrowPart::getPart(int32_t type)
{
    auto iter = m_partsMap.find(type);
    return iter != m_partsMap.end() ? &(iter->second) : nullptr;
}

int NFGrowPart::SendAllGrowData()
{
    proto_ff::NotifyGrowRsp rsp;
    for (auto& e : m_partsMap)
    {
        const GrowPartData& part = e.second;
        proto_ff::GrowSubInfo* pSub = rsp.add_subs();
        CHECK_NULL(pSub);
        FillProto(part, *pSub);
    }
    m_pMaster->SendMsgToClient(proto_ff::NOTIFY_GROW_RSP, rsp);
    return 0;
}

int NFGrowPart::SendGrowPartData(int32_t type)
{
    GrowPartData* pPart = getPart(type);
    CHECK_NULL(pPart);
    
    proto_ff::NotifyGrowRsp rsp;
    proto_ff::GrowSubInfo* pSubProto = rsp.add_subs();
    CHECK_NULL(pSubProto);
    
    FillProto(*pPart, *pSubProto);
    m_pMaster->SendMsgToClient(proto_ff::NOTIFY_GROW_RSP, rsp);
    return 0;
}

void NFGrowPart::calcAttr(bool sync)
{
    MAP_INT32_INT64 allAttr;
    MAP_INT32_INT64 partAttr;
    
    for (auto& e : m_partsMap)
    {
        calcAttr(e.first, partAttr);
        MergeAttr(partAttr, allAttr);
        partAttr.clear();
    }
    PrintAttr(0, allAttr);
    
    for (auto& e : allAttr)
    {
        m_pMaster->SetAttrGroupCache(proto_ff::EAttrGroup_Grow, e.first, e.second);
    }
    
    if (sync)
    {
        m_pMaster->SynAttrToClient();
    }
}

void NFGrowPart::calcAttr(int32_t type, MAP_INT32_INT64 &outAttr)
{
    GrowPartData* pPart = getPart(type);
    CHECK_EXPR_NOT_RET(pPart, " GrowPart::calcAttr type:{} is null", type);
    
    // �����аٷֱ�,������С��������������ֹ��ʧ����
    MAP_INT32_FLOAT floatAttr;
    for (auto& e : pPart->entryMap)
    {
        int64_t id = e.second.id;
        int32_t lv = e.second.lv;
        auto pGrowCfg = NFGrowDescEx::Instance()->GetGrowCfg(id);
        CHECK_EXPR_NOT_RET(pGrowCfg, "");
        if (lv != 0)
        {
            for (auto& ie : pGrowCfg->attrsMap)
            {
                int32_t attrId = ie.first;
                int32_t value = ie.second;
                floatAttr[attrId] += value + ((pGrowCfg->starBar * lv) / F_TEN_THOUSAND) * value;
            }
        }
        //�ӳɼ�������
        for (auto& te : pGrowCfg->activeAttrMap)
        {
            int32_t attrId = te.first;
            int32_t value = te.second;
            floatAttr[attrId] += value;
        }
        
    }
    
    //���ÿ��ģ��ӳ����ԣ�����ȡ��
    for (auto& e : floatAttr)
    {
        outAttr[e.first] = (int32_t)floor(e.second);
    }
    
    PrintAttr(type, outAttr);
}

void NFGrowPart::MergeAttr(MAP_INT32_INT64 &src, MAP_INT32_INT64 &dst)
{
    for (auto& e : src)
    {
        dst[e.first] += e.second;
    }
}

void NFGrowPart::PrintAttr(int32_t type, MAP_INT32_INT64 &attr)
{

}

void NFGrowPart::initParts()
{
    for (int32_t i = proto_ff::GrowType_MIN; i < proto_ff::GrowType_head; i++)
    {
        GrowPartData part;
        part.type = i;
        m_partsMap[i] = part;
    }
}

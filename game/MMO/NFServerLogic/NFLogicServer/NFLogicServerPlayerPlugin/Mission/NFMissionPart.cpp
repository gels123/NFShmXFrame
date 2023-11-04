// -------------------------------------------------------------------------
//    @FileName         :    NFMissionPart.cpp
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFMissionPart
//
// -------------------------------------------------------------------------

#include "NFMissionPart.h"
#include "NFLogicCommon/NFEventDefine.h"
#include "proto_svr_event.pb.h"
#include "Package/NFPackagePart.h"
#include "NFComm/NFCore/NFRandom.hpp"

NFMissionPart::NFMissionPart()
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

NFMissionPart::~NFMissionPart()
{
}

int NFMissionPart::CreateInit()
{
    _aryDyIdAlloc.resize(_aryDyIdAlloc.max_size());
    for (int i = 0; i < (int) _aryDyIdAlloc.size(); i++)
    {
        _aryDyIdAlloc[i] = false;
    }
    return 0;
}

int NFMissionPart::ResumeInit()
{
    return 0;
}

int NFMissionPart::Init(NFPlayer *pMaster, uint32_t partType, const proto_ff::RoleDBData &dbData)
{
    NFPart::Init(pMaster, partType, dbData);
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_ITEM_CHANGE, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_PASS_DUPLICATE, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_ARENA_JOIN, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_ADD_FRIEND, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_STONE_INLAY, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_EQUP_STREN, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_LEVELUP, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_GUILD_CHANGE, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_WING_ADVANCE, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_TREASURE_ADVANCE, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    Subscribe(NF_ST_LOGIC_SERVER, EVENT_PARTNER_ADVANCE, CREATURE_PLAYER, m_pMaster->Cid(), "MissionPart");
    return 0;
}

int NFMissionPart::UnInit()
{
    return NFPart::UnInit();
}

int NFMissionPart::RegisterMessage()
{
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_QUERY_MiSSIONLIST);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_NPC_ACCEPTMISSION);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_ABANDONMISSION);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_SUBMITMISSION);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_TALKWITHNPC);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_SUBMITMISSIONGOODS);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_MISSION_ONEKEY_FINISH);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_MISSION_ONCE_FINISH);
    RegisterClientMessage(proto_ff::CLIENT_TO_LOGIC_MISSION_RECV_SPECIAL_REWARD);
    return 0;
}

int NFMissionPart::LoadFromDB(const proto_ff::RoleDBData &dbData)
{
    if (dbData.has_task())
    {
        const proto_ff::CharacterDBTaskData &missionDBData = dbData.task();
        
        //�ѽ������б�
        uint32_t nAccept = missionDBData.missiontrack_size();
        for (uint32_t i = 0; i < nAccept; ++i)
        {
            const proto_ff::CharacterDBMissionTrack &missionDBTrack = missionDBData.missiontrack(i);
            if (_playerTrackMissionMap.size() >= _playerTrackMissionMap.max_size())
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_playerTrackMissionMap Space Not Enough, need size:{}", nAccept);
                continue;
            }
            MissionTrack *pMissionTrack = &_playerTrackMissionMap[missionDBTrack.dynamicid()];
            if (nullptr != pMissionTrack)
            {
                if (!pMissionTrack->CopyFromMissionProto(missionDBTrack))
                {
                    continue;
                }
                uint64_t dynamicId = pMissionTrack->dynamicId;
                //������ȵȼ�
                int32_t progressLev = 1;
                MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(pMissionTrack->missionId);
                if (nullptr != pMissionInfo)
                {
                    progressLev = pMissionInfo->progressLev;
                }
                //�������������Ƿ��Ѿ����,�������ֹ������������˵���״̬û����Ϊ���״̬
                bool completeFlag = true;
                
                for (auto iterChk = pMissionTrack->items.begin(); iterChk != pMissionTrack->items.end(); ++iterChk)
                {
                    ItemInfo &itemInfo = (*iterChk);
                    if (itemInfo.currentValue >= itemInfo.finalValue)
                    {
                        if (!itemInfo.completedFlag)
                        {
                            itemInfo.currentValue = itemInfo.finalValue;
                            itemInfo.completedFlag = true;
                            MarkDirty();
                        }
                    }
                    else
                    {
                        completeFlag = false;
                    }
                }
                //û����ɵ��������Ҫע���¼��Լ�����������
                if (!completeFlag)
                {
                    //ע���¼�
                    for (auto iter = pMissionTrack->items.begin(); iter != pMissionTrack->items.end(); ++iter)
                    {
                        ItemInfo &itemInfo = (*iter);
                        if (!itemInfo.completedFlag)
                        {
                            //���ѽ�����ע���¼�
                            int32_t relevent = MISSION_COND_TYPE_TO_EVENT(itemInfo.type);
                            RegisterEvent(relevent, dynamicId, progressLev);
                        }
                    }
                    //�������
                    OnAddMissionDrop(pMissionTrack, progressLev);
                }
                else
                {
                    if (pMissionTrack->status != MISSION_E_COMPLETION)
                    {
                        pMissionTrack->status = MISSION_E_COMPLETION;
                    }
                }
                //
                if (ValidDyMissionId(dynamicId))
                {
                    _aryDyIdAlloc[dynamicId] = 1;
                }
            }
        }
        
        //�Ѿ��ύ�������б�
        for (uint32_t j = 0; j < (uint32_t) missionDBData.already_submit_size(); ++j)
        {
            if (_setAlreadySubmit.size() >= _setAlreadySubmit.max_size())
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_setAlreadySubmit Space Not Enough, need size:{}",
                           missionDBData.already_submit_size());
                continue;
            }
            
            _setAlreadySubmit.insert(missionDBData.already_submit(j));
        }
        
        //��̬���������Ϣ
        uint32_t nDyCnt = missionDBData.dyinfo_size();
        for (uint32_t n = 0; n < nDyCnt; ++n)
        {
            const proto_ff::CharacterDBDyMissionInfo &dyInfo = missionDBData.dyinfo(n);
            if (_mapDyMissionTrack.size() >= _mapDyMissionTrack.max_size())
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapDyMissionTrack Space Not Enough, need size:{}", nDyCnt);
                continue;
            }
            DyMissionTrack &track = _mapDyMissionTrack[dyInfo.mission_type()];
            track.kind = dyInfo.mission_type();
            track.lastFresh = dyInfo.lastfresh();
            track.acceptNum = dyInfo.accept_num();
            track.bountyParam.ten_status = dyInfo.bounty_param().ten_state();
            track.bountyParam.twenty_status = dyInfo.bounty_param().twenty_state();
        }
        
        //����ύ�������б�
        uint32_t nRecentSubmit = missionDBData.recent_submit_size();
        for (uint32_t m = 0; m < nRecentSubmit; ++m)
        {
            const proto_ff::CharacterDBRecentSubmitMission &recent = missionDBData.recent_submit(m);
            auto iter = _mapRecentSubmit.find(recent.mission_type());
            if (iter != _mapRecentSubmit.end())
            {
                if (iter->second.size() >= iter->second.max_size())
                {
                    NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapRecentSubmit mission_type Space Not Enough....");
                    continue;
                }
                iter->second.insert(recent.mission_id());
            }
            else
            {
                if (_mapRecentSubmit.size() >= _mapRecentSubmit.max_size())
                {
                    NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapRecentSubmit Space Not Enough....");
                    continue;
                }
                _mapRecentSubmit[recent.mission_type()].insert(recent.mission_id());
            }
        }
    }
    return 0;
}

int NFMissionPart::SaveDB(proto_ff::RoleDBData &dbData)
{
    proto_ff::CharacterDBTaskData *pTaskData = dbData.mutable_task();
    pTaskData->Clear();
    
    //��̬��������
    for (auto iterDy = _mapDyMissionTrack.begin(); iterDy != _mapDyMissionTrack.end(); ++iterDy)
    {
        DyMissionTrack &info = iterDy->second;
        proto_ff::CharacterDBDyMissionInfo *proto = pTaskData->add_dyinfo();
        if (nullptr != proto)
        {
            proto->set_mission_type(info.kind);
            proto->set_accept_num(info.acceptNum);
            proto->set_lastfresh(info.lastFresh);
            auto pBounty = proto->mutable_bounty_param();
            pBounty->set_ten_state(info.bountyParam.ten_status);
            pBounty->set_twenty_state(info.bountyParam.twenty_status);
        }
    }
    //�ѽ������б�
    for (PlayerTrackMissionMap::iterator it = _playerTrackMissionMap.begin(); it != _playerTrackMissionMap.end(); ++it)
    {
        auto pSingleMissionTrack = pTaskData->mutable_missiontrack()->Add();
        if (nullptr != pSingleMissionTrack)
        {
            it->second.SetMissionTrackProto(*pSingleMissionTrack);
        }
    }
    
    //�Ѿ��ύ����
    for (auto iterAlready = _setAlreadySubmit.begin(); iterAlready != _setAlreadySubmit.end(); ++iterAlready)
    {
        pTaskData->add_already_submit((*iterAlready));
    }
    
    //����ύ������
    for (auto iterRecent = _mapRecentSubmit.begin(); iterRecent != _mapRecentSubmit.end(); ++iterRecent)
    {
        auto &setMission = iterRecent->second;
        for (auto iterMission = setMission.begin(); iterMission != setMission.end(); ++iterMission)
        {
            proto_ff::CharacterDBRecentSubmitMission *pRecentProto = pTaskData->add_recent_submit();
            if (nullptr != pRecentProto)
            {
                pRecentProto->set_mission_type(iterRecent->first);
                pRecentProto->set_mission_id((*iterMission));
            }
        }
    }
    return 0;
}

int NFMissionPart::OnLogin()
{
    CheckTrunkMission();
    
    SendMissionInfo();
    return 0;
}

int NFMissionPart::OnHandleClientMessage(uint32_t msgId, NFDataPackage &packet)
{
    switch (msgId)
    {
        case proto_ff::CLIENT_TO_LOGIC_QUERY_MiSSIONLIST: //�����б�
        {
            SendMissionList(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_NPC_ACCEPTMISSION: //NPC��ȡ����
        {
            HandleNpcAcceptMission(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_ABANDONMISSION: //��������
        {
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_SUBMITMISSION: //�ύ����
        {
            HandleSubmitMission(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_TALKWITHNPC: //NPC�Ի�
        {
            HandleTalkWithNpc(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_SUBMITMISSIONGOODS: //�ύ��Ʒ
        {
            HandleSubmitMissionGoods(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_MISSION_ONEKEY_FINISH: //һ�����ŵ��ð��
        {
            HandleOnekeyFinishMission(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_MISSION_ONCE_FINISH: //�������
        {
            HandleOnceFinishMission(msgId, packet);
            break;
        }
        case proto_ff::CLIENT_TO_LOGIC_MISSION_RECV_SPECIAL_REWARD://��ȡ���⽱��
        {
            HandleMissionRecvSpecialReward(msgId, packet);
            break;
        }
        default:
            break;
    }
    return 0;
}

int NFMissionPart::OnHandleServerMessage(uint32_t msgId, NFDataPackage &packet)
{
    return NFPart::OnHandleServerMessage(msgId, packet);
}

void NFMissionPart::CheckTrunkMission(bool notify)
{
    if ((_mapRecentSubmit.size() <= 0 && _playerTrackMissionMap.size() <= 0) ||
        ((!HaveRecentSubmit(MISSION_TYPE_ID_TRUNK)) && (MissionNumByType(MISSION_TYPE_ID_TRUNK) <= 0)))
    {
        //����ύ�����б�Ϊ��,�����ѽ�ȡ�����б�ҲΪ��
        //����ύ�����б�Ϊ�գ���������ύ�����б���û���������񣬲����ѽ�ȡ�����б�ҲΪ����Ҳû����������
        
        auto &mapFirst = TaskDescEx::Instance()->GetFirstMission();
        for (auto iterFirt = mapFirst.begin(); iterFirt != mapFirst.end(); ++iterFirt)
        {
            auto &setMission = iterFirt->second;
            for (auto iterMission = setMission.begin(); iterMission != setMission.end(); ++iterMission)
            {
                uint64_t missionid = (*iterMission);
                MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionid);
                if (nullptr == pMissionInfo)
                {
                    NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "CheckTrunkMission 11 nullptr == pMissionInfo cid:{}, missionid:{} ",
                               m_pMaster->Cid(),
                               missionid);
                    continue;
                }
                if (HaveAccept(missionid))
                {
                    continue;
                }
                if (HaveSubmited(missionid))
                {
                    continue;
                }
                int32_t ret = OnAccept(missionid, notify);
                if (proto_ff::RET_SUCCESS != ret)
                {
                    if (MISSION_TYPE_ID_TRUNK == pMissionInfo->kind)
                    {
                        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                                   "CheckTrunkMission...OnAcceptType failed...cid:{} ,kind:{},mission:{},ret:{} ",
                                   m_pMaster->Cid(),
                                   pMissionInfo->kind, missionid, ret);
                    }
                }
                else
                {
                    NFLogDebug(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                               "MissionPart::CheckTrunkMission...accept mission...cid:{},kind:{}, mission:{},ret:{} ", m_pMaster->Cid(),
                               pMissionInfo->kind, missionid, ret);
                }
            }
        }
    }
    else
    {
        for (auto iterRecent = _mapRecentSubmit.begin(); iterRecent != _mapRecentSubmit.end(); ++iterRecent)
        {
            auto &setMission = iterRecent->second;
            auto iterMission = setMission.begin();
            for (; iterMission != setMission.end(); ++iterMission)
            {
                uint64_t missionid = (*iterMission);
                MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionid);
                if (nullptr == pMissionInfo)
                {
                    NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "CheckTrunkMission nullptr == pMissionInfo cid:{}, missionid:{} ",
                               m_pMaster->Cid(),
                               missionid);
                    continue;
                }
                
                uint64_t backMissionid = pMissionInfo->backTaskId;
                if (backMissionid > 0)
                {
                    MissionInfo *pBackMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(backMissionid);
                    if (nullptr == pBackMissionInfo)
                    {
                        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                                   "CheckTrunkMission nullptr == pBackMissionInfo cid:{}, missionid:{},backMissionid:{} ",
                                   m_pMaster->Cid(), missionid, backMissionid);
                        continue;
                    }
                    
                    //
                    if (HaveAccept(backMissionid))
                    {
                        //LogErrFmtPrint("[logic] MissionPart::CheckTrunkMission backMissionid have accept cid:%lu, missionid:%lu,backMissionid:%lu ", m_pMaster->GetCid(), missionid, backMissionid);
                        continue;
                    }
                    
                    if (HaveSubmited(backMissionid))
                    {
                        continue;
                    }
                    
                    if (MISSION_TYPE_ID_TRUNK == pBackMissionInfo->kind)
                    {
                        if (MissionNumByType(MISSION_TYPE_ID_TRUNK) > 0)
                        {
                            continue;
                        }
                    }
                    
                    int32_t ret = OnAccept(backMissionid, notify);
                    if (proto_ff::RET_SUCCESS != ret)
                    {
                        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                                   "CheckTrunkMission...OnAcceptType failed...cid:{},mission:{},backMissionid:{},ret:{} ",
                                   m_pMaster->Cid(), missionid, backMissionid, ret);
                    }
                    else
                    {
                        NFLogDebug(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                                   "MissionPart::CheckTrunkMission...accept mission...cid:{},kind:{}, mission:{},backMissionid:{},ret:{} ",
                                   m_pMaster->Cid(), pBackMissionInfo->kind, missionid, backMissionid, ret);
                    }
                }
            }
        }
    }
}

int NFMissionPart::HandleNpcAcceptMission(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::CGNpcAcceptMissionReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    
    uint64_t cid = m_pMaster->Cid();
    int32_t missionType = req.missiontype();
    //
    int32_t ret = proto_ff::RET_MISSION_NOT_EXIST;
    ret = AcceptMissionByType(missionType, true);
    if (proto_ff::RET_SUCCESS != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "HandleNpcAcceptMission....AcceptMissionByType failed...cid:{},missionType:{},ret:{} ", cid,
                   missionType, ret);
    }
    
    proto_ff::CGNpcAcceptMissionRsp rsp;
    rsp.set_missiontype(missionType);
    rsp.set_retcode(ret);
    m_pMaster->SendMsgToClient(proto_ff::LOGIC_TO_CLIENT_NPC_ACCEPTMISSION, rsp);
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

int NFMissionPart::HandleSubmitMission(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::CGSubmitMissionReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    
    proto_ff::GCSubmitMissionRsp rsp;
    
    uint64_t dymissionId = req.dynamicid();
    uint32_t index = req.index();
    
    //
    MissionInfo *pMissionCfgInfo = nullptr;
    const DyMissionInfo *pDyMissionCfgInfo = nullptr;
    int32_t ret = proto_ff::RET_MISSION_NOT_EXIST;
    PlayerTrackMissionMap::iterator iter = _playerTrackMissionMap.find(dymissionId);
    if (iter != _playerTrackMissionMap.end())
    {
        uint64_t missionId = iter->second.missionId;
        pMissionCfgInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionId);
        if (nullptr != pMissionCfgInfo)
        {
            ret = OnSubmit(missionId, index);
        }
        else
        {
            pDyMissionCfgInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(missionId);
            if (nullptr != pDyMissionCfgInfo)
            {
                ret = OnSubmitDy(dymissionId);
            }
        }
    }
    
    if (proto_ff::RET_MISSION_NOT_EXIST != ret && proto_ff::RET_SUCCESS != ret) //���������⴦���ɹ������񲻴��ڵ�ʱ�򲻷��ظ�ǰ�ˣ�ֻ�����������ʱ��ŷ��ظ�ǰ��
    {
        rsp.set_dynamicid(dymissionId);
        rsp.set_ret(ret);
        m_pMaster->SendMsgToClient(proto_ff::LOGIC_TO_CLIENT_SUBMITMISSION, rsp);
    }
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

int NFMissionPart::HandleTalkWithNpc(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::CGTalkWithNpcReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    
    uint64_t dymissionId = req.dynamicid();
    uint64_t itemId = req.npcid();
    uint32_t type = req.type();
    
    PlayerTrackMissionMap::iterator it = _playerTrackMissionMap.find(dymissionId);
    if (it == _playerTrackMissionMap.end())
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] CGTalkWithNpcReq but dymissionId = %lu not in playerTrackMissionMap...cid:%lu",
                   dymissionId, m_pMaster->Cid());
        return -1;
    }
    
    //ֻ�жԻ��͵���ĳ�������ͨ����Э������ύ
    if (type != MISSION_FINISH_TYPE_KILL_MONS_AREA
        && type != MISSION_FINISH_TYPE_TAKL
        && type != MISSION_FINISH_TYPE_AREA
        && type != MISSION_FINISH_TYPE_FINISH_ACTION)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] CGTalkWithNpcReq...type error... cid:%lu, dymissionId:%lu, itemid:%lu , type:%d..",
                   m_pMaster->Cid(), dymissionId, itemId, type);
        return -1;
    }
    
    if (MISSION_FINISH_TYPE_KILL_MONS_AREA == type)
    {
        //��������ˢ��
        //OnAreaFreshMonster(dymissionId);
        return 0;
    }
    
    //NPC��̸,������������ʹ����Ϊ��ť�����������������
    uint32_t eventType = MISSION_COND_TYPE_TO_EVENT(type);
    ExecuteData executeData(eventType, itemId, 1);
    OnEvent(eventType, executeData);
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

int NFMissionPart::HandleSubmitMissionGoods(uint32_t msgId, NFDataPackage &packet)
{
    return 0;
}

int NFMissionPart::HandleOnekeyFinishMission(uint32_t msgId, NFDataPackage &packet)
{
    return 0;
}

int NFMissionPart::HandleOnceFinishMission(uint32_t msgId, NFDataPackage &packet)
{
    return 0;
}

int NFMissionPart::HandleMissionRecvSpecialReward(uint32_t msgId, NFDataPackage &packet)
{
    return 0;
}

int NFMissionPart::SendMissionList(uint32_t msgId, NFDataPackage &packet)
{
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- begin -- ");
    proto_ff::CGQueryMissionListReq req;
    CLIENT_MSG_PROCESS_WITH_PRINTF(packet, req);
    
    SendMissionInfo();
    
    NFLogTrace(NF_LOG_SYSTEMLOG, 0, "--- end -- ");
    return 0;
}

void NFMissionPart::SendMissionInfo()
{
    proto_ff::GCQueryMissionListRsp rsp;
    
    //�ѽ������б�
    for (auto missionIte = _playerTrackMissionMap.begin(); missionIte != _playerTrackMissionMap.end(); ++missionIte)
    {
        proto_ff::CMissionTrack *missionTrack = rsp.add_acceptedlist();
        if (nullptr != missionTrack)
        {
            missionIte->second.SetMissionTrackProto(*missionTrack);
        }
    }
    
    //��̬�����������Ѿ���ȡ�Ĵ���
    proto_ff::DyMissionCntAllProto *pAllProto = rsp.mutable_dy_count();
    if (nullptr != pAllProto)
    {
        for (auto iter = _mapDyMissionTrack.begin(); iter != _mapDyMissionTrack.end(); ++iter)
        {
            DyMissionTrack *pDyTrack = &iter->second;
            if (nullptr != pDyTrack)
            {
                proto_ff::DyMissionCntProto *proto = pAllProto->add_count();
                if (nullptr != proto)
                {
                    proto->set_missiontype(iter->first);
                    proto->set_accept_num(pDyTrack->acceptNum);
                    if (iter->first == MISSION_TYPE_ID_BOUNTY)
                    {
                        auto pBounty = proto->mutable_bounty_param();
                        pBounty->set_ten_state(pDyTrack->bountyParam.ten_status);
                        pBounty->set_twenty_state(pDyTrack->bountyParam.twenty_status);
                    }
                }
            }
        }
    }
    
    m_pMaster->SendMsgToClient(proto_ff::LOGIC_TO_CLIENT_QUERY_MiSSIONLIST, rsp);
}

int32_t NFMissionPart::OnAccept(uint64_t missionId, bool notify)
{
    //��ȡ����
    MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionId);
    if (nullptr == pMissionInfo)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "OnAcceptType Have Not MissionID={} Config, Please Check Mission Config", missionId);
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    int32_t ret = CanAccept(missionId);
    if (proto_ff::RET_SUCCESS != ret)
    {
        return ret;
    }
    
    if (_playerTrackMissionMap.size() >= _playerTrackMissionMap.max_size())
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_playerTrackMissionMap Space Noet Enough.......");
        return proto_ff::RET_FAIL;
    }
    
    //��ӵ��ɽ��б�����
    MissionTrack *pMissionTrack = &_playerTrackMissionMap[missionId];
    if (nullptr == pMissionTrack)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnAcceptType MissionID={}, Allcol MissionTrackObj is nullptr", missionId);
        return proto_ff::RET_FAIL;
    }
    
    //������������Ϣ ����������
    
    //��ȡ�����ȡ�м����
    AddReward(pMissionInfo->missionId, pMissionInfo->kind, pMissionInfo->receAdd);
    //�������������Ϣ
    pMissionTrack->missionId = missionId;
    pMissionTrack->dynamicId = missionId; //��֧������̬����ID������IDһ��
    pMissionTrack->missionType = pMissionInfo->kind;
    pMissionTrack->acceptMissionTime = NFTime::Now().UnixSec();
    pMissionTrack->status = MISSION_E_ACCEPTED;
    if (proto_ff::RET_SUCCESS != OnExtractCond(pMissionInfo, pMissionTrack))
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnAcceptType MissionID={}, OnExtractCond failed...", missionId);
        return proto_ff::RET_FAIL;
    }
    
    MarkDirty();
    
    //֪ͨ��ȡ״̬
    if (notify)
    {
        UpdateMissionProgress(missionId);
    }
    
    //��ȡ�����¼�
    proto_ff::AcceptTaskEvent acceptEvent;
    acceptEvent.set_cid(m_pMaster->Cid());
    acceptEvent.set_taskid(missionId);
    acceptEvent.set_tasktype(pMissionInfo->kind);
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_ACCEPT_TASK, CREATURE_PLAYER, m_pMaster->Cid(), acceptEvent);
    
    //�ж������Ƿ����
    bool isCompletedFlag = true;
    for (uint32_t i = 0; i < pMissionTrack->items.size(); i++)
    {
        if (!pMissionTrack->items[i].completedFlag)
        {
            isCompletedFlag = false;
            break;
        }
    }
    
    //����Ѿ����
    if (isCompletedFlag)
    {
        //����������
        pMissionTrack->status = MISSION_E_COMPLETION;
    }
    else //û����ɵ������Ż�ע���¼�
    {
        //���������䴦��
        OnAddMissionDrop(pMissionTrack, pMissionInfo->progressLev);
        //ע���¼�
        for (uint32_t i = 0; i < pMissionTrack->items.size(); i++)
        {
            if (!pMissionTrack->items[i].completedFlag)
            {
                int32_t relEvent = MISSION_COND_TYPE_TO_EVENT(pMissionTrack->items[i].type);
                RegisterEvent(relEvent, pMissionTrack->dynamicId, pMissionInfo->progressLev);
            }
        }
    }
    
    //��������ѽ�ȡ�������Ҫ �ٴ�֪ͨ�ͻ���
    if (isCompletedFlag && notify)
    {
        UpdateMissionProgress(missionId);
    }
    
    OnAcceptType(missionId, pMissionInfo->kind);
    
    //��־
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::OnAcceptType(uint64_t missionId, uint32_t kind)
{

}

int32_t NFMissionPart::CanAccept(uint64_t missionId)
{
    MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionId);
    if (nullptr == pMissionInfo)
    {
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    //�Ƿ��ȡ��������
    if (HaveAccept(pMissionInfo->missionId))
    {
        return proto_ff::RET_MISSION_HAVE_ACCEPT;
    }
    
    //�Ƿ��Ѿ��ύ
    if (HaveSubmited(pMissionInfo->missionId))
    {
        return proto_ff::RET_MISSION_ALREADY_SUBMIT;
    }
    
    //����������֤���Ƿ��Ѿ���ȡ����������
    if (MISSION_TYPE_ID_TRUNK == pMissionInfo->kind)
    {
        if (MissionNumByType(MISSION_TYPE_ID_TRUNK) > 0)
        {
            return proto_ff::RET_MISSION_HAVE_TYPE_ACCEPT;
        }
    }
    
    SCanAcceptParam param;
    param.missionId = pMissionInfo->missionId;
    int32_t ret = CanMatchAcceptCond(pMissionInfo);
    if (proto_ff::RET_SUCCESS != ret)
    {
        return ret;
    }
    
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::CanMatchAcceptCond(MissionInfo *pMissionInfo)
{
    CHECK_EXPR(pMissionInfo, proto_ff::RET_FAIL, "pMissionInfo == NULL");
    const AcceptInfo &cond = pMissionInfo->accept;
    //ǰ���½��жϣ����ϵ��
    if (cond.setPreOrMission.size() > 0)
    {
        bool haveSubmit = false;
        for (auto iterOr = cond.setPreOrMission.begin(); iterOr != cond.setPreOrMission.end(); ++iterOr)
        {
            uint64_t id = (*iterOr);
            if (HaveSubmited(id))
            {
                haveSubmit = true;
                break;
            }
        }
        if (!haveSubmit)
        {
            return proto_ff::RET_MISSION_GROUP_LIMIT;
        }
    }
    
    //�ȼ�
    if (cond.minLevel > 0 && m_pMaster->GetAttr(proto_ff::A_LEVEL) < cond.minLevel)
    {
        return proto_ff::RET_MISSION_LEVEL_ERROR;
    }
    
    //ְҵ
    if (cond.profession > 0 && m_pMaster->GetAttr(proto_ff::A_PROF) != cond.profession)
    {
        return proto_ff::RET_MISSION_PROFESSION_LIMIT;
    }
    
    //ǰ���½��жϣ����ϵ��
    if (cond.setPreAndMission.size() > 0)
    {
        bool haveSubmit = true;
        for (auto iterAnd = cond.setPreAndMission.begin(); iterAnd != cond.setPreAndMission.end(); ++iterAnd)
        {
            const uint64_t id = (*iterAnd);
            if (!HaveSubmited(id))
            {
                haveSubmit = false;
                break;
            }
        }
        if (!haveSubmit)
        {
            return proto_ff::RET_MISSION_GROUP_LIMIT;
        }
    }
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::OnAcceptDy(uint64_t missionId, bool notify)
{
    int32_t ret = CanAcceptDy(missionId);
    if (proto_ff::RET_SUCCESS != ret)
    {
        return ret;
    }
    
    //��ȡ����
    const DyMissionInfo *pMissionInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(missionId);
    if (nullptr == pMissionInfo)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnAcceptDyType nullptr == pMissionInfo  cid:{}, missionid:{}", m_pMaster->Cid(),
                   missionId);
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    //���ɶ�̬����ID
    uint64_t dymissionId = AllocNewDyMisssionId();
    if (!ValidDyMissionId(dymissionId))
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnAcceptDyType AllocNewDyMisssionId failed... cid:{},missionid:{},dymissionId:{}",
                   m_pMaster->Cid(), missionId, dymissionId);
        return proto_ff::RET_FAIL;
    }
    
    if (_playerTrackMissionMap.size() >= _playerTrackMissionMap.max_size())
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                   "[logic] OnAcceptDyType _playerTrackMissionMap Space Not Enough.. cid:{},missionid:{},dymissionId:{}", m_pMaster->Cid(), missionId,
                   dymissionId);
        FreeDyMissionId(dymissionId);
        return proto_ff::RET_FAIL;
    }
    
    //
    MissionTrack *pMissionTrack = &_playerTrackMissionMap[dymissionId];
    if (nullptr == pMissionTrack)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnAcceptDyType Allcol MissionTrackObj is nullptr cid:{},missionid:{},dymissionId:{}",
                   m_pMaster->Cid(), missionId, dymissionId);
        FreeDyMissionId(dymissionId);
        return proto_ff::RET_FAIL;
    }
    
    //�������������Ϣ
    pMissionTrack->missionId = missionId;
    pMissionTrack->dynamicId = dymissionId;
    pMissionTrack->acceptMissionTime = NFTime::Now().UnixSec();
    pMissionTrack->missionType = pMissionInfo->kind;
    pMissionTrack->status = MISSION_E_ACCEPTED;
    //������������Ϣ ����������
    if (proto_ff::RET_SUCCESS != OnExtractDyCond(pMissionInfo, pMissionTrack))
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnAcceptDyType OnExtractCond failed...cid:{},missionid:{},dymissionId:{}",
                   m_pMaster->Cid(), missionId, dymissionId);
        return proto_ff::RET_FAIL;
    }
    
    MarkDirty();
    
    //��ȡ���Խ���
    OnExtractDyAttrReward(pMissionTrack);
    //
    //��ȡ����,��Ҫ�ŵ������������ǰ�棬��ǰ�˴������
    OnAcceptDyType(dymissionId, pMissionInfo->kind);
    //���Ҫ֪ͨ�ͻ���
    if (notify)
    {
        UpdateMissionProgress(dymissionId);
    }
    
    //��ȡ�����¼�
    //��ȡ�����¼�
    proto_ff::AcceptTaskEvent acceptEvent;
    acceptEvent.set_cid(m_pMaster->Cid());
    acceptEvent.set_taskid(missionId);
    acceptEvent.set_tasktype(pMissionInfo->kind);
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_ACCEPT_TASK, CREATURE_PLAYER, m_pMaster->Cid(), acceptEvent);
    
    //�ж������Ƿ����
    bool isCompletedFlag = true;
    for (uint32_t i = 0; i < pMissionTrack->items.size(); i++)
    {
        if (!pMissionTrack->items[i].completedFlag)
        {
            isCompletedFlag = false;
            break;
        }
    }
    
    //����Ѿ����
    if (isCompletedFlag)
    {
        //����������
        pMissionTrack->status = MISSION_E_COMPLETION;
    }
    else //û����ɵ������Ż�ע���¼�
    {
        //���������䴦��
        OnAddMissionDrop(pMissionTrack, 1);
        //ע���¼�
        for (uint32_t i = 0; i < pMissionTrack->items.size(); i++)
        {
            if (!pMissionTrack->items[i].completedFlag)
            {
                int32_t relEvent = MISSION_COND_TYPE_TO_EVENT(pMissionTrack->items[i].type);
                RegisterEvent(relEvent, pMissionTrack->dynamicId, 1);
            }
        }
    }
    //�����ȡ����ɣ���Ҫ�ٴ�֪ͨ�ͻ���
    if (isCompletedFlag && notify)
    {
        UpdateMissionProgress(dymissionId);
    }
    //
    if (isCompletedFlag)
    {
        //�������
        OnFinishDy(dymissionId, pMissionInfo->kind);
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::OnAcceptDyType(uint64_t dymissionId, int32_t missionType)
{
    NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] MissionPart::OnAcceptDyType...cid:{}, missionType:{} ", m_pMaster->Cid(), missionType);
    OnAddAcceptDyCount(missionType, 1);
}

int32_t NFMissionPart::CanAcceptDy(uint64_t dyCfgId)
{
    const DyMissionInfo *pDyMissionInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(dyCfgId);
    return CanAcceptDy(pDyMissionInfo);
}

int32_t NFMissionPart::CanAcceptDy(const DyMissionInfo *pDyMissionInfo)
{
    if (nullptr == pDyMissionInfo)
    {
        return proto_ff::RET_FAIL;
    }
    
    uint32_t level = m_pMaster->GetAttr(proto_ff::A_LEVEL);
    if (MissionNumByType(pDyMissionInfo->kind) > 0)
    {
        return proto_ff::RET_MISSION_PERIOD_ONCE_EXIST_NUM_LIMIT;
    }
    
    if (level < pDyMissionInfo->minLev || level > pDyMissionInfo->maxLev)
    {
        return proto_ff::RET_MISSION_LEVEL_ERROR;
    }
    
    int32_t maxCount = DyMaxCount(pDyMissionInfo->kind, level);
    if (maxCount != MISSION_INFINITE_COUNT)
    {
        if ((int32_t) GetDyMissionAceeptCnt(pDyMissionInfo->kind) >= maxCount)
        {
            return proto_ff::RET_MISSION_PERIOD_FINISH_NUM_LIMIT;
        }
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::OnAddAcceptDyCount(int32_t missionType, uint32_t count)
{
    //���¶�̬�������
    DyMissionTrack *pDyTrack = GetDyMissionTrack(missionType);
    if (nullptr == pDyTrack)
    {
        if (_mapDyMissionTrack.size() >= _mapDyMissionTrack.max_size())
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapDyMissionTrack Space Not Enough");
            return;
        }
        
        DyMissionTrack &track = _mapDyMissionTrack[missionType];
        track.kind = missionType;
        track.acceptNum = 0;
        track.lastFresh = NFTime::Now().UnixSec();
        
        pDyTrack = GetDyMissionTrack(missionType);
        if (nullptr == pDyTrack)
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapDyMissionTrack can't find mission type:{}", missionType);
            return;
        }
    }
    
    pDyTrack->acceptNum += count;
    
    SET_UINT32 setMissionType;
    setMissionType.insert(missionType);
    NotifyDyAcceptCount(setMissionType);
    MarkDirty();
}

int32_t NFMissionPart::AcceptMissionByType(int32_t missionType, bool notify)
{
    int32_t ret = CanAcceptMissionByType(missionType);
    if (proto_ff::RET_SUCCESS == ret)
    {
        uint64_t missionId = DyRandMissionId(missionType);
        const DyMissionInfo *pDyMissionCfgInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(missionId);
        if (nullptr == pDyMissionCfgInfo)
        {
            ret = proto_ff::RET_MISSION_NOT_EXIST;
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                       "AcceptMissionByType...can not find mission config....cid:{} ,missionType:{}, missionid:{}, notify:{}", m_pMaster->Cid(),
                       missionType, missionId, (int32_t) notify);
        }
        else
        {
            ret = OnAcceptDy(missionId, notify);
            if (proto_ff::RET_SUCCESS != ret)
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                           "AcceptMissionByType...OnAcceptDyType failed....cid:{} ,missionType:{}, missionid:{}, notify:{}, ret:{} ",
                           m_pMaster->Cid(),
                           missionType, missionId, (int32_t) notify, ret);
            }
        }
    }
    
    return ret;
}

int32_t NFMissionPart::CanAcceptMissionByType(int32_t missionType)
{
    if (MISSION_TYPE_ID_GUILD == missionType)
    {
        if (m_pMaster->GetAttr(proto_ff::A_FACTION_ID) <= 0)
        {
            return proto_ff::RET_FACTION_NOT_JOIN;
        }
    }
    int32_t level = m_pMaster->GetAttr(proto_ff::A_LEVEL);
    int32_t maxCount = DyMaxCount(missionType, level);
    int32_t acceptNum = GetDyMissionAceeptCnt(missionType);
    if (maxCount == MISSION_INFINITE_COUNT || acceptNum < maxCount)
    {
        if (MissionNumByType(missionType) <= 0)
        {
            return proto_ff::RET_SUCCESS;
        }
        else
        {
            return proto_ff::RET_MISSION_HAVE_TYPE_ACCEPT;
        }
    }
    return proto_ff::RET_MISSION_PERIOD_FINISH_NUM_LIMIT;
}

int32_t NFMissionPart::ClearMissionByType(int32_t missionType, bool notify)
{
    MAP_UINT64_UINT64 mapDelMission;
    mapDelMission.clear();
    MAP_UINT64_UINT64 mapDelMissionEx;
    mapDelMissionEx.clear();
    int32_t delCount = 0;
    PlayerTrackMissionMap::iterator iter = _playerTrackMissionMap.begin();
    for (; iter != _playerTrackMissionMap.end(); ++iter)
    {
        uint64_t missionId = iter->second.missionId;
        uint64_t dymissionId = iter->second.dynamicId;
        const DyMissionInfo *pDyMissionCfgInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(missionId);
        if (nullptr != pDyMissionCfgInfo)
        {
            if ((uint32_t) missionType == pDyMissionCfgInfo->kind)
            {
                mapDelMission[dymissionId] = missionId;
            }
        }
        else
        {
            MissionInfo *pMissionCfgInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionId);
            if (nullptr != pMissionCfgInfo)
            {
                if ((uint32_t) missionType == pMissionCfgInfo->kind)
                {
                    mapDelMissionEx[dymissionId] = missionId;
                    if (MISSION_TYPE_ID_TRUNK == pMissionCfgInfo->kind)
                    {
                        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "ClearMissionByType..remove trunk mission..cid:{}, missionid:{} ",
                                   m_pMaster->Cid(), missionId);
                    }
                }
            }
        }
    }
    //
    for (auto iterDel = mapDelMission.begin(); iterDel != mapDelMission.end(); ++iterDel)
    {
        uint64_t dymissionId = iterDel->first;
        uint64_t missionId = iterDel->second;
        int32_t ret = RemoveDyMission(dymissionId, notify);
        if (proto_ff::RET_SUCCESS != ret)
        {
            //��������ӡ��־������Ƴ�ʧ�ܣ��ܿ��ܵ�������ˢ����Ӫ�������Ķ�̬ID���ظ���
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                       "MissionPart::ClearMissionByType...RemoveDyMission failed....cid:%lu,missionType:{}, missionId:{}, dymissionid:{},ret:{} ",
                       m_pMaster->Cid(),
                       missionType, missionId, dymissionId, ret);
        }
        else
        {
            delCount += 1;
        }
    }
    
    //
    for (auto iterDelEx = mapDelMissionEx.begin(); iterDelEx != mapDelMissionEx.end(); ++iterDelEx)
    {
        uint64_t dymissionId = iterDelEx->first;
        uint64_t missionId = iterDelEx->second;
        MissionInfo *pMissionCfgInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionId);
        if (nullptr != pMissionCfgInfo)
        {
            int32_t ret = RemoveMission(pMissionCfgInfo);
            if (proto_ff::RET_SUCCESS != ret)
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                           "MissionPart::ClearMissionByType...RemoveMission failed....cid:{}, missionId:{}, dymissionid:{},ret:{} ",
                           m_pMaster->Cid(),
                           missionId, dymissionId, ret);
            }
            else
            {
                delCount += 1;
            }
        }
        else
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                       "MissionPart::ClearMissionByType...pMissionCfgInfo is nullptr....cid:{}, missionId:{}, dymissionid:{} ", m_pMaster->Cid(),
                       missionId, dymissionId);
        }
    }
    
    return delCount;
}

int32_t NFMissionPart::DyMaxCount(int32_t missionType, int32_t level)
{
    int32_t count = 0;
    auto pTaskDynamicCfg = TaskdynamicTaskdynamicDesc::Instance()->GetResDescPtr();
    for (auto iter = pTaskDynamicCfg->begin(); iter != pTaskDynamicCfg->end(); ++iter)
    {
        auto &info = iter->second;
        if (missionType == info.m_taskType && level >= info.m_minlv && level <= info.m_maxlv)
        {
            count = info.m_receCount;
            break;
        }
    }
    return count;
}

uint32_t NFMissionPart::GetDyMissionAceeptCnt(int32_t missionType)
{
    DyMissionTrack *pDyTrack = GetDyMissionTrack(missionType);
    if (nullptr != pDyTrack)
    {
        return pDyTrack->acceptNum;
    }
    return 0;
}

void NFMissionPart::OnFinishDy(uint64_t dymissionId, uint32_t dymissionType)
{
    if (dymissionType == MISSION_TYPE_ID_BOUNTY)
    {
        DyMissionTrack *pDyTrack = GetDyMissionTrack(MISSION_TYPE_ID_BOUNTY);
        if (pDyTrack)
        {
            SMissionReward missionReward;
            OnAddDyMissionReward(dymissionType, dymissionId, missionReward);
            
            if (pDyTrack->acceptNum >= 10 && pDyTrack->bountyParam.ten_status == 0)
            {
                pDyTrack->bountyParam.ten_status = 1;
            }
            else if (pDyTrack->acceptNum >= 20 && pDyTrack->bountyParam.twenty_status == 0)
            {
                pDyTrack->bountyParam.twenty_status = 1;
            }
            
            MarkDirty();
            
            SET_UINT32 setMissionType;
            setMissionType.insert(dymissionType);
            NotifyDyAcceptCount(setMissionType);
        }
    }
}

DyMissionTrack *NFMissionPart::GetDyMissionTrack(int32_t missionType)
{
    PlayerDyMissionTrackMap::iterator iter = _mapDyMissionTrack.find(missionType);
    return (iter != _mapDyMissionTrack.end()) ? &iter->second : nullptr;
}

int32_t NFMissionPart::OnAddDyMissionReward(int32_t missionType, uint64_t missionId, SMissionReward &missionReward)
{
    int32_t level = m_pMaster->GetAttr(proto_ff::A_LEVEL);
    const TASK_REWARD *pReward = TaskDescEx::Instance()->GetDyMissionReward(missionType, level);
    if (nullptr == pReward)
    {
        return proto_ff::RET_FAIL;
    }
    
    NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
    if (nullptr == pPackage)
    {
        return proto_ff::RET_FAIL;
    }
    
    SCommonSource sourceParam;
    sourceParam.src = S_MISSION;
    sourceParam.param1 = missionId;
    
    uint32_t profression = m_pMaster->GetAttr(proto_ff::A_PROF);
    const TASK_REWARD &reward = *pReward;
    
    LIST_ITEM lstItemA;
    lstItemA.clear();
    //
    uint32_t isize = (uint32_t) reward.size();
    for (uint32_t i = 0; i < isize; ++i)
    {
        uint32_t rewardType = reward[i].type;
        
        if (reward[i].profession != 0 && profression != reward[i].profession)
        {
            continue;
        }
        if (MISSION_AWARD_GOODS == rewardType || MISSION_AWARD_EQUIP == rewardType)
        {
            pPackage->AddItem(reward[i].id, reward[i].value, sourceParam, reward[i].bind);
            
            //
            SItem item;
            item.nItemID = reward[i].id;
            item.nNum = reward[i].value;
            item.byBind = reward[i].bind;
            lstItemA.push_back(item);
        }
        else if (MISSION_AWARD_SKILL == rewardType)
        {
/*            SkillPart *pSkillPart = dynamic_cast<SkillPart *>(pPlayer->GetPart(PART_SKILL));
            if (nullptr != pSkillPart)
            {
                SCommonSource source;
                if (!pSkillPart->AddSkill(reward[i].id, 1, source, true))
                {
                    LogErrFmtPrint("[logic] MissionManager::OnAddDyMissionReward...AddSkill failed...cid:%lu,missionType:%d, level:%d,missionId:%lu ",
                                   pPlayer->Cid(), missionType, level, missionId);
                }
                else
                {
                    missionReward.setSkill.insert(reward[i].id);
                }
            }*/
        }
        else if (MISSION_AWARD_ATTR == rewardType) //���Խ���
        {
            m_pMaster->AddAttr(reward[i].id, reward[i].value, &sourceParam, true);
            missionReward.mapAttr[reward[i].id] += reward[i].value;
        }
        else if (MISSION_AWARD_USE_CONTRI == rewardType)
        {
            missionReward.useContri += reward[i].id;
        }
        else if (MISSION_AWARD_UNION_EXP == rewardType)
        {
            missionReward.unionExp += reward[i].id;
        }
    }
    //
    NFItemMgr::MergeItem(lstItemA, missionReward.lstItem);
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::NotifyDyAcceptCount(SET_UINT32 &setMissionType)
{
    proto_ff::GCUpdateDyAcceptCount notify;
    proto_ff::DyMissionCntAllProto *proto = notify.mutable_dy_count();
    if (nullptr != proto)
    {
        SET_UINT32::iterator iter = setMissionType.begin();
        for (; iter != setMissionType.end(); ++iter)
        {
            uint32_t missionType = (*iter);
            uint32_t acceptNum = 0;
            DyMissionTrack *pDyTrack = GetDyMissionTrack(missionType);
            if (nullptr != pDyTrack)
            {
                acceptNum = pDyTrack->acceptNum;
            }
            
            proto_ff::DyMissionCntProto *protoA = proto->add_count();
            if (nullptr != protoA)
            {
                protoA->set_missiontype(missionType);
                protoA->set_accept_num(acceptNum);
                if (missionType == MISSION_TYPE_ID_BOUNTY)
                {
                    auto pBounty = protoA->mutable_bounty_param();
                    pBounty->set_ten_state(pDyTrack->bountyParam.ten_status);
                    pBounty->set_twenty_state(pDyTrack->bountyParam.twenty_status);
                }
            }
        }
        
        m_pMaster->SendMsgToClient(proto_ff::LOGIC_TO_CLIENT_UPDDATE_DY_ACCEPT_NUM, notify);
    }
}

bool NFMissionPart::ValidDyMissionId(uint64_t dyMissionId)
{
    if (dyMissionId >= 1 && dyMissionId <= MISSION_MAX_DYNAMIC_ALLOC)
    {
        return true;
    }
    return false;
}

uint64_t NFMissionPart::AllocNewDyMisssionId()
{
    for (uint32_t i = 1; i <= MISSION_MAX_DYNAMIC_ALLOC; ++i)
    {
        if (!_aryDyIdAlloc[i])
        {
            _aryDyIdAlloc[i] = true;
            return i;
        }
    }
    return 0;
}

void NFMissionPart::FreeDyMissionId(uint64_t dyMissionId)
{
    if (dyMissionId >= 1 && dyMissionId <= MISSION_MAX_DYNAMIC_ALLOC)
    {
        _aryDyIdAlloc[dyMissionId] = false;
    }
}

uint64_t NFMissionPart::DyRandMissionId(int32_t missionType)
{
    auto *pMissionLst = TaskDescEx::Instance()->GetDyMissionLstByType(missionType);
    if (nullptr == pMissionLst)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "DyRandMissionId...nullptr == pMissionLst...missionType:{} ,cid:{} ", missionType,
                   m_pMaster->Cid());
        return proto_ff::RET_FAIL;
    }
    
    uint32_t level = m_pMaster->GetAttr(proto_ff::A_LEVEL);
    //��ȡ��������������ID
    VEC_UINT64 vecMission;
    vecMission.clear();
    for (auto iterSet = pMissionLst->begin(); iterSet != pMissionLst->end(); ++iterSet)
    {
        const uint64_t missionId = (*iterSet);
        const DyMissionInfo *pDyMissionCfgInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(missionId);
        if (nullptr == pDyMissionCfgInfo || level < pDyMissionCfgInfo->minLev || level > pDyMissionCfgInfo->maxLev)
        {
            continue;
        }
        vecMission.push_back(missionId);
    }
    int32_t iszie = vecMission.size();
    if (iszie <= 0)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "DyRandMissionId....iszie <= 0....missionType:{}, cid:{} ", missionType, m_pMaster->Cid());
        return proto_ff::RET_FAIL;
    }
    int32_t rnd = NFRandInt(0, iszie);
    uint64_t resmissionId = vecMission.at(rnd);
    //
    return resmissionId;
}

bool NFMissionPart::HaveRecentSubmit(int32_t missionType)
{
    auto iter = _mapRecentSubmit.find(missionType);
    return (iter != _mapRecentSubmit.end()) ? true : false;
}

int32_t NFMissionPart::MissionNumByType(int32_t missionType)
{
    int32_t num = 0;
    auto iter = _playerTrackMissionMap.begin();
    for (; iter != _playerTrackMissionMap.end(); ++iter)
    {
        if ((uint32_t) missionType == iter->second.missionType)
        {
            num++;
        }
    }
    return num;
}

bool NFMissionPart::HaveAccept(const uint64_t &missionId)
{
    if (_playerTrackMissionMap.find(missionId) != _playerTrackMissionMap.end())
    {
        return true;
    }
    
    return false;
}

bool NFMissionPart::HaveSubmited(uint64_t missionId)
{
    auto iter = _setAlreadySubmit.find(missionId);
    return (iter != _setAlreadySubmit.end()) ? true : false;
}

int32_t NFMissionPart::OnExtractCond(MissionInfo *pMissionInfo, MissionTrack *pMissionTrack)
{
    CHECK_EXPR(pMissionInfo, proto_ff::RET_FAIL, "pMissionInfo == Null");
    CHECK_EXPR(pMissionTrack, proto_ff::RET_FAIL, "pMissionTrack == Null");
    
    const InterExecute &execute = pMissionInfo->execute;
    for (auto iter = execute.items.begin(); iter != execute.items.end(); ++iter)
    {
        const InterItemPair &inter = (*iter);
        //�������
        ItemInfo condItem(inter.itemId, 0, inter.itemCount, false, inter.itemType, inter.parma1, inter.parma2, inter.parma3);
        //����Ԥ����
        OnPreUpdateProgress(condItem);
        //��ӵ������б���
        if (pMissionTrack->items.size() >= pMissionTrack->items.max_size())
        {
            NFLogError(NF_LOG_SYSTEMLOG, 0, "pMissionTrack->items Space Not Enough");
            return proto_ff::RET_FAIL;
        }
        pMissionTrack->items.push_back(condItem);
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::OnPreUpdateProgress(ItemInfo &cond)
{
    if (cond.completedFlag)
    {
        return;
    }
    
    int64_t count = 0;
    int32_t event = MISSION_COND_TYPE_TO_EVENT(cond.type);
    if (M_EVENT_COLL_COLLECT_ITEM == event) //�ռ���Ʒ
    {
        //�ռ���Ʒ		��ʽ��3 = ��Ʒid = ������Ʒ��Ŀ = ����id = �����id = ����id
        NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
        if (nullptr != pPackage)
        {
            int64_t bindNum = 0;
            int64_t unbindNum = 0;
            count = pPackage->GetItemNum(cond.itemId, unbindNum, bindNum);
        }
    }
    else if (M_EVENT_EMBLEM_INLAY == event)
    {
    }
    else if (M_EVENT_LADDER == event) //���±������(���ݸ���)
    {
    }
    else if (M_EVENT_SLOT_STREN == event)
    {
    }
    else if (M_EVENT_INBUFF == event) //����ĳ��BUFF
    {
    }
    else if (M_EVENT_LEVEL == event) //�ȼ��ﵽ���ټ�
    {
        //����ָ���ȼ�	��ʽ: 14=����ȼ���ֵ=Ŀ��ֵ=0=0=0
        //������xx�� ,ǰ����ʾ 0/1,
        if ((uint64_t) m_pMaster->GetAttr(proto_ff::A_LEVEL) >= cond.itemId)
        {
            count = cond.itemId;
        }
    }
    else if (M_EVENT_JOIN_GUILD == event)
    {
    
    }
    else if (M_EVENT_TREASURE_LEV == event)
    {
    }
    else if (M_EVENT_PARTNER_RANKLEV == event)
    {
    }
    else if (M_EVENT_WING_LEV == event)
    {
    }
    else if (M_EVENT_SKILL_LEVEL == event)
    {
    }
    else if (M_EVENT_APTITUDE == event)
    {
    }
    else if (M_EVENT_FETCH_OPERATE == event)
    {
    }
    else
    {
        return;
    }
    
    if (count > 0)
    {
        ExecuteData data(event, cond.itemId, count);
        bool notify = false;
        OnUpdateCondProcess(data, cond, notify);
    }
}

void NFMissionPart::OnUpdateCondProcess(const ExecuteData &data, ItemInfo &cond, bool &notify)
{
    int32_t relevent = MISSION_COND_TYPE_TO_EVENT(cond.type);
    if ((uint32_t) relevent != data.type || cond.completedFlag)
    {
        return;
    }
    if (MISSION_FINISH_TYPE_SUBMIT_SPEC_EQUIP == cond.type) //�ύָ��Ʒ�׺�Ʒ�ʵ�װ�����⴦����
    {
    }
    else if (M_EVENT_INFINITE_HUNT == relevent)
    {
    }
    else if (M_EVENT_LADDER == relevent)
    {
    }
    else if (M_EVENT_APTITUDE == relevent)
    {
    
    }
    else if (M_EVENT_SLOT_STREN == relevent)
    {
    }
    else if (M_EVENT_TREASURE_LEV == relevent)
    {
    }
    else if (M_EVENT_PARTNER_RANKLEV == relevent)
    {
    }
    else if (M_EVENT_WING_LEV == relevent)
    {
    }
    else if (cond.itemId != data.id)
    {
        return;
    }
    
    //
    if (M_EVENT_LEVEL == relevent
        || M_EVENT_TREASURE_LEV == relevent            //���ߵȼ�
        || M_EVENT_PARTNER_RANKLEV == relevent        //����Ƚ�
        || M_EVENT_WING_LEV == relevent                //���ȼ�
        )
    {
        //������xx�� ,ǰ����ʾ 0/1,
        //���ߵȼ� ǰ����ʾ 0/1,	��ʽ 11501=ָ�����ߵȼ�=1=linkid=0=0
        //����Ƚ� ǰ����ʾ 0/1,	��ʽ 11601=ָ������Ƚ�=1=linkid=0=0
        //���ȼ� ǰ����ʾ 0/1,	��ʽ 11701=ָ�����ȼ�=1=linkid=0=0
        if (data.id >= cond.itemId)
        {
            cond.currentValue = cond.finalValue;
            cond.completedFlag = true;
            notify = true;
        }
    }
    else if (M_EVENT_INFINITE_HUNT == relevent
        || M_EVENT_LADDER == relevent
        || M_EVENT_APTITUDE == relevent
        )
    {
        //���λ���������Ծ��� ǰ����ʾ 0/1
        //���±������(����) ǰ����ʾ 0/1
        //תְ���ʵȼ� ǰ����ʾ 0/1
        if (data.id >= cond.itemId)
        {
            cond.currentValue = cond.finalValue;
            cond.completedFlag = true;
            notify = true;
        }
    }
    else if (M_EVENT_SLOT_STREN == relevent)
    {
        //װ����λǿ���ȼ�	��ʽ 11301=0=װ����λ����=�ȼ�=ui���=0
        if (MISSION_FINISH_TYPE_SLOT_STREN == cond.type)
        {
/*            if (nullptr != pPlayer)
            {
                EquipPart *pPackage = dynamic_cast<EquipPart *>(pPlayer->GetPart(PART_EQUIP));
                if (nullptr != pPackage)
                {
                    int32_t count = pPackage->GetStrongLvNum(cond.parma1);
                    if (count > cond.currentValue)
                    {
                        cond.currentValue = count;
                        notify = true;
                        if (cond.currentValue >= cond.finalValue)
                        {
                            cond.currentValue = cond.finalValue;
                            cond.completedFlag = true;
                        }
                    }
                }
            }*/
        }
    }
    else
    {
        //�ύ��Ʒ����������Ʒ�۳���Դ�����Ǵ�������߿۳���
        if (MISSION_FINISH_TYPE_SUBMIT_ITEM == cond.type || MISSION_FINISH_TYPE_SUBMIT_EQUIP == cond.type ||
            MISSION_FINISH_TYPE_SUBMIT_SPEC_EQUIP == cond.type)
        {
            if (S_MissionSubmitItem != data.source)
            {
                return;
            }
            
            if (MISSION_FINISH_TYPE_SUBMIT_SPEC_EQUIP == cond.type) //�ύָ��Ʒ�ʵ�װ��ʱ����Ҫ��֤
            {
                auto *pEquipCfg = EquipEquipDesc::Instance()->GetDesc(data.id);
                if (nullptr != pEquipCfg && pEquipCfg->m_quality >= (int32_t) cond.parma1 && pEquipCfg->m_wearQuality >= (int32_t) cond.itemId)
                {
                    //Ʒ�״��ڵ�����������ֵ��Ʒ�ʴ��ڵ�����������ֵ
                }
                else
                {
                    return;
                }
            }
        }
        else if (MISSION_FINISH_TYPE_KILL_MONS_WILD_BOSS == cond.type)
        {
            //Ұ��BOSS��Ҫ�ж���Դ
/*            if ((int32_t)EMonsCreateType::MapBoss != data.source)
            {
                return;
            }*/
        }
        else if (MISSION_FINISH_TYPE_KILL_MONS_BARREN_BOSS == cond.type)
        {
            //��ԨBOSS��Ҫ�ж���Դ
/*			if (EMonsCreateType_BarrenBoss != data.source)
			{
				return;
			}*/
        }
        
        
        int32_t currentValue = cond.currentValue + data.count;
        if (currentValue >= 0)
        {
            notify = true;
            if (currentValue >= cond.finalValue)
            {
                cond.currentValue = cond.finalValue;
                cond.completedFlag = true;
            }
            else
            {
                cond.currentValue = currentValue;
                cond.completedFlag = false;
            }
        }
    }
}

int32_t NFMissionPart::OnExtractDyCond(const DyMissionInfo *pDyMissionInfo, MissionTrack *pMissionTrack, bool preUpdate)
{
    CHECK_EXPR(pDyMissionInfo, proto_ff::RET_FAIL, "pDyMissionInfo == NULL");
    CHECK_EXPR(pMissionTrack, proto_ff::RET_FAIL, "pMissionTrack == NULL");
    
    //��ȡһ����̬����ID
    if (pDyMissionInfo->setComplete.size() < 1)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnExtractDyCond pDyMissionInfo->setComplete.size() < 1  missionid:{} ",
                   pDyMissionInfo->missionId);
        return proto_ff::RET_FAIL;
    }
    
    VEC_INT64 vecCond;
    vecCond.clear();
    
    for (auto iter = pDyMissionInfo->setComplete.begin(); iter != pDyMissionInfo->setComplete.end(); ++iter)
    {
        vecCond.push_back((*iter));
    }
    
    int32_t isize = vecCond.size();
    int32_t rndidx = NFRandInt(0, isize);// 0 - (isize-1)
    int64_t condId = vecCond.at(rndidx);
    
    const DyConditionInfo *pDyConditionInfo = TaskDescEx::Instance()->GetDyConditionCfgInfo(condId);
    if (nullptr == pDyConditionInfo)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnExtractDyCond nullptr == pDyConditionInfo missionid:{}, condid:{} ",
                   pDyMissionInfo->missionId, condId);
        return proto_ff::RET_FAIL;
    }
    
    //��̬������������
    ItemInfo condItem;
    uint64_t textId;
    int32_t ret = OnGeneralCond(pDyMissionInfo, pDyConditionInfo, condItem, textId);
    if (proto_ff::RET_SUCCESS != ret)
    {
        return ret;
    }
    
    if (preUpdate)
    {
        //����Ԥ����
        OnPreUpdateProgress(condItem);
    }
    
    //��ӵ������б���
    pMissionTrack->items.push_back(condItem);
    pMissionTrack->textId = textId;
    
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::OnGeneralCond(const DyMissionInfo *pDyMissionInfo, const DyConditionInfo *pDyConditionInfo, ItemInfo &cond, uint64_t &textId)
{
    CHECK_EXPR(pDyMissionInfo, proto_ff::RET_FAIL, "pDyMissionInfo == NULL");
    CHECK_EXPR(pDyConditionInfo, proto_ff::RET_FAIL, "pDyConditionInfo == NULL");
    
    //������ֵ
    cond.type = pDyConditionInfo->cond.itemType;
    cond.itemId = pDyConditionInfo->cond.itemId;
    cond.finalValue = pDyConditionInfo->cond.itemCount;
    cond.parma1 = pDyConditionInfo->cond.parma1;
    cond.parma2 = pDyConditionInfo->cond.parma2;
    cond.parma3 = pDyConditionInfo->cond.parma3;
    
    //���textid
    textId = TaskDescEx::Instance()->GetDyTextId(pDyMissionInfo->kind, cond.type);
    if (textId <= 0)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                   "[logic] MissionManager::OnGeneralCond...can not find textid..... missionid:{} ,condid:{}, kind:{}, condType:{}",
                   pDyMissionInfo->missionId, pDyConditionInfo->condId, pDyMissionInfo->kind, cond.type);
    }
    
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::OnExtractDyAttrReward(MissionTrack *pNewMissinTrack)
{
    return 0;
}

int32_t NFMissionPart::AddReward(uint64_t missionId, int32_t kind, TASK_REWARD &reward, float ration)
{
    SCommonSource sourceParam;
    sourceParam.src = S_MISSION;
    sourceParam.param1 = missionId;
    //
    
    LIST_ITEM addItems;
    addItems.clear();
    if (!CanAddReward(missionId, kind, reward, addItems))
    {
        return proto_ff::RET_PACKAGE_COMMON_SPACE_NOT_ENOUGH;
    }
    
    for (uint32_t i = 0; i < reward.size(); ++i)
    {
        uint32_t rewardType = reward[i].type;
        uint32_t profression = m_pMaster->GetAttr(proto_ff::A_PROF);
        if (reward[i].profession != 0 && profression != reward[i].profession)
        {
            continue;
        }
        
        if (MISSION_AWARD_SKILL == rewardType)
        {
/*            SkillPart *pSkillPart = dynamic_cast<SkillPart *>(pPlayer->GetPart(PART_SKILL));
            if (nullptr != pSkillPart)
            {
                SCommonSource source;
                if (!pSkillPart->AddSkill(reward[i].id, 1, source, true))
                {
                    LogErrFmtPrint("[logic] MissionManager::AddReward...AddSkill failed...cid:%lu, pre:%s,missionid:%lu", pPlayer->Cid(),
                                   param.strPre.c_str(), param.missionId);
                }
            }*/
        }
        else if (MISSION_AWARD_ATTR == rewardType) //���Խ���
        {
            int64_t value = ceil(reward[i].value * ration);
            //
            m_pMaster->AddAttr(reward[i].id, value, &sourceParam, true);
        }
        else if (MISSION_AWARD_UNION_EXP == rewardType)
        {
        }
        else if (MISSION_AWARD_USE_CONTRI == rewardType)
        {
        }
    }
    
    //�����Ʒ
    if (addItems.size() > 0)
    {
        NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
        NF_ASSERT(pPackage);
        //�����жϹ�����������ֱ�������Ʒ
        pPackage->AddItem(addItems, sourceParam);
    }
    
    return proto_ff::RET_SUCCESS;
}

bool NFMissionPart::CanAddReward(uint64_t missionId, int32_t kind, TASK_REWARD &reward, LIST_ITEM &lstOutItem)
{
    LIST_ITEM addItems;
    addItems.clear();
    for (uint32_t i = 0; i < reward.size(); ++i)
    {
        uint32_t rewardType = reward[i].type;
        uint32_t profression = m_pMaster->GetAttr(proto_ff::A_PROF);
        
        if (reward[i].profession != 0 && profression != reward[i].profession)
        {
            continue;
        }
        
        if (MISSION_AWARD_GOODS == rewardType || MISSION_AWARD_EQUIP == rewardType) //װ�����߽���
        {
            SItem item;
            item.nItemID = reward[i].id;
            item.nNum = reward[i].value;
            item.byBind = reward[i].bind;
            addItems.push_back(item);
        }
        else if (MISSION_AWARD_SKILL == rewardType)
        {
/*             const SkillCfg *pSkill = g_GetSkillMgr()->GetCfg(reward[i].id);
           if (nullptr == pSkill)
            {
                LogErrFmtPrint("[logic] MissionManager::CanAddReward nullptr == pSkill  cid:%lu, pre:%s,missionid:%lu", pPlayer->Cid(),
                               param.strPre.c_str(), param.missionId);
                return false;
            }
            SkillPart *pSkillPart = dynamic_cast<SkillPart *>(pPlayer->GetPart(PART_SKILL));
            if (nullptr == pSkillPart)
            {
                LogErrFmtPrint("[logic] MissionManager::CanAddReward nullptr == pSkillPart  cid:%lu, pre:%s,missionid:%lu", pPlayer->Cid(),
                               param.strPre.c_str(), param.missionId);
                return false;
            }*/
        }
    }
    if (addItems.size() > 0)
    {
        NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
        NF_ASSERT(pPackage);
        
        if (!pPackage->CanAddItem(addItems))
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "missionId:{} add items failed.......", missionId);
            return false;
        }
        
        lstOutItem.insert(lstOutItem.end(), addItems.begin(), addItems.end());
    }
    return true;
}

int32_t NFMissionPart::RemoveMission(MissionInfo *pMissionInfo)
{
    CHECK_EXPR(pMissionInfo, proto_ff::RET_FAIL, "pMissionInfo == NULL");
    
    auto iter = _playerTrackMissionMap.find(pMissionInfo->missionId);
    if (iter == _playerTrackMissionMap.end())
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] RemoveMission missionId={} can not find mission data , charId={}",
                   pMissionInfo->missionId, m_pMaster->Cid());
        return proto_ff::RET_FAIL;
    }
    
    return RemoveMission(&iter->second, pMissionInfo);
}

int32_t NFMissionPart::RemoveMission(MissionTrack *pMissinTrack, MissionInfo *pMissionInfo)
{
    CHECK_EXPR(pMissinTrack, proto_ff::RET_FAIL, "pMissinTrack == NULL");
    CHECK_EXPR(pMissionInfo, proto_ff::RET_FAIL, "pMissionInfo == NULL");
    
    //�Ƴ��������
    OnDelMissionDrop(pMissinTrack);
    //�Ƴ��м���Ʒ
    RemoveReward(pMissinTrack);
    //�Ƴ���������
    _playerTrackMissionMap.erase(pMissinTrack->dynamicId);
    
    //�Ƴ���ȡ����ʱ���ŵ���Ʒ
    RemoveReward(pMissionInfo->missionId, pMissionInfo->receAdd);
    //�Ƴ������¼�����
    RemoveEvent(pMissionInfo->missionId);
    //֪ͨɾ������
    NotifyDelMission(pMissionInfo->missionId);
    //
    MarkDirty();
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::RemoveDyMission(uint64_t dymissionId, bool notify)
{
    if (!ValidDyMissionId(dymissionId))
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] RemoveDyMission  dymissionId:{} ValidDyMissionId failed, charId={}", dymissionId,
                   m_pMaster->Cid());
        return proto_ff::RET_FAIL;
    }
    
    auto iter = _playerTrackMissionMap.find(dymissionId);
    if (iter == _playerTrackMissionMap.end())
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] RemoveDyMission dymissionId:{} can not find mission data , charId={}", dymissionId,
                   m_pMaster->Cid());
        return proto_ff::RET_FAIL;
    }
    
    uint64_t missionId = iter->second.missionId;
    const DyMissionInfo *pDyMissionCfgInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(missionId);
    if (nullptr == pDyMissionCfgInfo)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                   "[logic] RemoveDyMission dymissionId:{}, missionid:{}, can not find mission config , charId={}", dymissionId, missionId,
                   m_pMaster->Cid());
        return proto_ff::RET_FAIL;
    }
    
    //�Ƴ��������
    OnDelMissionDrop(&iter->second);
    
    //�Ƴ�������Ʒ
    OnDelMissionItem(&iter->second);
    
    //�Ƴ���������
    _playerTrackMissionMap.erase(iter);
    MarkDirty();
    
    //���ն�̬����ID,�������Ӫ��������ͽ����񣬶�̬ID����Ӫ���ͽ�ɽ��б��������
    FreeDyMissionId(dymissionId);
    
    //�Ƴ������¼�����
    RemoveEvent(dymissionId);
    
    //֪ͨɾ������
    if (notify)
    {
        NotifyDelMission(dymissionId);
    }
    
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::OnUpdateProgress(uint64_t missionId, const ExecuteData &data)
{
    //�Ȳ����ѽ��б����Ƿ��и�����
    auto iter = _playerTrackMissionMap.find(missionId);
    if (iter == _playerTrackMissionMap.end())
    {
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    if (MISSION_E_ACCEPTED != iter->second.status)
    {
        return proto_ff::RET_MISSION_STATE_NOT_MATCH;
    }
    
    SET_UINT64 setAreaMonsDy;
    setAreaMonsDy.clear();
    bool notify = false;
    //����ִ�е�Ԫ��ʼִ�� ���������
    for (uint32_t i = 0; i < iter->second.items.size(); i++)
    {
        ItemInfo &cond = iter->second.items[i];
        OnUpdateCondProcess(data, cond, notify);
        if (notify)
        {
            MarkDirty();
            //
            if (MISSION_FINISH_TYPE_KILL_MONS_AREA == cond.type && cond.currentValue < cond.finalValue)
            {
                setAreaMonsDy.insert(missionId);
            }
        }
    }
    
    //�ж������Ƿ����
    bool isCompletedFlag = true;
    for (uint32_t i = 0; i < iter->second.items.size(); i++)
    {
        if (!iter->second.items[i].completedFlag)
        {
            isCompletedFlag = false;
            break;
        }
    }
    
    //��������
    if (isCompletedFlag)
    {
        //�����Ѿ����
        iter->second.status = MISSION_E_COMPLETION;
        //
        MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(iter->second.missionId);
        if (nullptr != pMissionInfo)
        {
        }
        else
        {
            auto pDyMissionInfo = TaskDescEx::Instance()->GetDyMissionCfgInfo(iter->second.missionId);
            if (pDyMissionInfo)
            {
                //�������(��̬����)
                OnFinishDy(iter->second.dynamicId, pDyMissionInfo->kind);
            }
        }
    }
    
    if (notify)
    {
        UpdateMissionProgress(missionId);
    }
    
    if (setAreaMonsDy.size() > 0)
    {
        //OnCheckAreaFreshMonster(setAreaMonsDy);
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::UpdateMissionProgress(uint64_t missionId)
{
    auto missionIte = _playerTrackMissionMap.find(missionId);
    if (missionIte != _playerTrackMissionMap.end())
    {
        proto_ff::GCUpdateMissionStatusNotify notify;
        proto_ff::CMissionTrack *pMissionTrack = notify.add_updatelist();
        if (nullptr != pMissionTrack)
        {
            missionIte->second.SetMissionTrackProto(*pMissionTrack);
            m_pMaster->SendMsgToClient(proto_ff::LOGIC_TO_CLIENT_UPDATEMISSIONSTATUSNOTIFY, notify);
        }
    }
}

void NFMissionPart::NotifyDelMission(uint64_t dymissionId)
{
    proto_ff::GCDeleteMissionNotify notify;
    NFLogDebug(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] Delete old DyMissionID={}, cid:{} ", dymissionId, m_pMaster->Cid());
    notify.add_dynamicid(dymissionId);
    m_pMaster->SendMsgToClient(proto_ff::LOGIC_TO_CLIENT_GCDELETEMISSIONNOTIFY, notify);
}

int32_t NFMissionPart::OnDelMissionItem(MissionTrack *pMissionTrack)
{
    CHECK_EXPR(pMissionTrack, proto_ff::RET_FAIL, "pMissionTrack == NULL");
    
    NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_EXPR(pPackage, proto_ff::RET_FAIL, "pPackage == NULL");
    
    for (auto iter = pMissionTrack->items.begin(); iter != pMissionTrack->items.end(); ++iter)
    {
        ItemInfo &cond = (*iter);
        if (MISSION_FINISH_TYPE_COLLECT_ITEM == cond.type
            || MISSION_FINISH_TYPE_COLLECT_CLIENT == cond.type
            )
        {
            if (cond.currentValue > 0)
            {
                SCommonSource sourceParam;
                sourceParam.src = S_MISSION;
                sourceParam.param1 = pMissionTrack->missionId;
                pPackage->RemoveItem(cond.itemId, cond.currentValue, sourceParam);
            }
        }
    }
    
    return proto_ff::RET_SUCCESS;
}

int32_t NFMissionPart::OnAddMissionDrop(MissionTrack *pMissionTrack, int32_t progressLev)
{
    CHECK_EXPR(pMissionTrack, proto_ff::RET_FAIL, "pMissionTrack == NULL");
    
    uint64_t missionId = pMissionTrack->missionId;
    uint64_t dymissionId = pMissionTrack->dynamicId;
    
    for (auto iter = pMissionTrack->items.begin(); iter != pMissionTrack->items.end(); ++iter)
    {
        ItemInfo &cond = (*iter);
        if (MISSION_FINISH_TYPE_COLLECT_KILL_MONS == cond.type
            || MISSION_FINISH_TYPE_COLLECT_ITEM == cond.type
            )
        {
            //�ռ���Ʒ		��ʽ 301=��Ʒid=��Ʒ����=����ID=����id=׷������ID�����������㹻���������������
            //����ռ�		��ʽ 302=��Ʒid=��Ʒ����=����ID=����id=׷��·��id
            if (!AddMissionDrop(dymissionId, cond.parma1, 0, cond.parma2, progressLev))
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                           "OnAddMissionDrop...pMissionPart->AddMissionDrop failed....cid:{}, missionid:{}, dymissionid:{}, condtype:{},itemid:{},finalvalue:{},param1:{},param2:{},param3:{} ",
                           m_pMaster->Cid(), missionId, dymissionId, cond.type, cond.itemId, cond.finalValue, cond.parma1, cond.parma2, cond.parma3);
            }
        }
    }
    return proto_ff::RET_SUCCESS;
}

bool NFMissionPart::AddMissionDrop(uint64_t dymissionId, uint64_t monsId, uint64_t dropId, uint64_t boxId, int32_t progressLev)
{
    auto iterMons = _mapMissionAllDrop.find(monsId);
    if (iterMons != _mapMissionAllDrop.end())
    {
        MissionDropMap &mapDrop = iterMons->second;
        auto iterMission = mapDrop.find(dymissionId);
        if (iterMission != mapDrop.end())
        {
            return false;
        }
        else
        {
            MissionDrop stDrop;
            stDrop.dropId = dropId;
            stDrop.boxId = boxId;
            stDrop.progressLv = progressLev;
            if (mapDrop.size() >= mapDrop.max_size())
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapMissionAllDrop mapDrop Space Not Enough");
                return false;
            }
            mapDrop[dymissionId] = stDrop;
        }
    }
    else
    {
        MissionDrop stDrop;
        stDrop.dropId = dropId;
        stDrop.boxId = boxId;
        stDrop.progressLv = progressLev;
        if (_mapMissionAllDrop.size() >= _mapMissionAllDrop.max_size())
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapMissionAllDrop Space Not Enough");
            return false;
        }
        _mapMissionAllDrop[monsId][dymissionId] = stDrop;
    }
    return true;
}

bool NFMissionPart::DelMissionDrop(uint64_t dymissionId, uint64_t monsId)
{
    MissionAllDropMap::iterator iterMons = _mapMissionAllDrop.find(monsId);
    if (iterMons != _mapMissionAllDrop.end())
    {
        MissionDropMap &mapDrop = iterMons->second;
        MissionDropMap::iterator iterMission = mapDrop.find(dymissionId);
        if (iterMission != mapDrop.end())
        {
            mapDrop.erase(iterMission);
            if (mapDrop.size() <= 0)
            {
                _mapMissionAllDrop.erase(iterMons);
            }
            //
            return true;
        }
    }
    return false;
}

MissionDropMap *NFMissionPart::GetMissionDrop(uint64_t monsterId)
{
    MissionAllDropMap::iterator iterMons = _mapMissionAllDrop.find(monsterId);
    if (iterMons != _mapMissionAllDrop.end())
    {
        return &iterMons->second;
    }
    
    return NULL;
}

int32_t NFMissionPart::OnDelMissionDrop(MissionTrack *pMissionTrack)
{
    CHECK_EXPR(pMissionTrack, proto_ff::RET_FAIL, "pMissionTrack == NULL");
    
    uint64_t missionId = pMissionTrack->missionId;
    uint64_t dymissionId = pMissionTrack->dynamicId;
    for (auto iter = pMissionTrack->items.begin(); iter != pMissionTrack->items.end(); ++iter)
    {
        ItemInfo &cond = (*iter);
        if (MISSION_FINISH_TYPE_COLLECT_KILL_MONS == cond.type)
        {
            //����ռ�		��ʽ 302=��Ʒid=��Ʒ����=����ID=����id=׷��·��id
            if (!DelMissionDrop(dymissionId, cond.parma1))
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                           "OnDelMissionDrop...pMissionPart->DelMissionDrop failed....cid:{}, missionid:{}, dymissionid:{}, condtype:{},itemid:{},finalvalue:{},param1:{},param2:{},param3:{} ",
                           m_pMaster->Cid(), missionId, dymissionId, cond.type, cond.itemId, cond.finalValue, cond.parma1, cond.parma2, cond.parma3);
            }
        }
    }
    return proto_ff::RET_SUCCESS;
}

int NFMissionPart::RemoveReward(MissionTrack *pMissionTrack)
{
    CHECK_EXPR(pMissionTrack, proto_ff::RET_FAIL, "pMissionTrack == NULL");
    
    NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_EXPR(pPackage, proto_ff::RET_FAIL, "pPackage == NULL");
    
    SCommonSource param;
    param.src = S_MISSION;
    param.param1 = pMissionTrack->missionId;
    for (auto iter = pMissionTrack->items.begin(); iter != pMissionTrack->items.end(); ++iter)
    {
        ItemInfo &cond = (*iter);
        if (MISSION_FINISH_TYPE_COLLECT_ITEM == cond.type
            || MISSION_FINISH_TYPE_COLLECT_KILL_MONS == cond.type
            || MISSION_FINISH_TYPE_COLLECT_CLIENT == cond.type
            )
        {
            pPackage->RemoveItem(cond.itemId, cond.finalValue, param);
        }
    }
    
    return proto_ff::RET_SUCCESS;
}

int NFMissionPart::RemoveReward(uint64_t missionId, TASK_REWARD &reward)
{
    SCommonSource sourceParam;
    sourceParam.src = S_MISSION;
    sourceParam.param1 = missionId;
    
    ORDER_MAP_INT32_INT32 costAttrs;
    NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_EXPR(pPackage, proto_ff::RET_FAIL, "pPackage == NULL");
    
    for (uint32_t i = 0; i < reward.size(); ++i)
    {
        uint32_t rewardType = reward[i].type;
        if (rewardType == MISSION_AWARD_GOODS || rewardType == MISSION_AWARD_EQUIP) //����
        {
            if (nullptr != pPackage)
            {
                pPackage->RemoveItem(reward[i].id, reward[i].value, sourceParam, reward[i].bind);
            }
        }
        else if (rewardType == MISSION_AWARD_ATTR) //����
        {
            int32_t id = reward[i].id;
            int64_t costValue = -reward[i].value;
            m_pMaster->AddAttr(id, costValue, &sourceParam, true);
        }
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::RegisterEvent(uint32_t eventType, uint64_t missionId, int32_t progressLev)
{
    auto iter = _eventTabal.find(eventType);
    if (iter != _eventTabal.end())
    {
        auto &mapLevMission = iter->second;
        auto iterLv = mapLevMission.find(progressLev);
        if (iterLv != mapLevMission.end())
        {
            if (iterLv->second.size() >= iterLv->second.max_size())
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_eventTabal iterLv->second Space Not Enough");
            }
            iterLv->second.insert(missionId);
        }
        else
        {
            if (iterLv->second.size() >= iterLv->second.max_size())
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_eventTabal mapLevMission Space Not Enough");
            }
            mapLevMission[progressLev].insert(missionId);
        }
    }
    else
    {
        if (_eventTabal.size() >= _eventTabal.max_size())
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_eventTabal Space Not Enough");
        }
        _eventTabal[eventType][progressLev].insert(missionId);
    }
}

void NFMissionPart::RemoveEvent(uint64_t missionId)
{
    auto iter = _eventTabal.begin();
    while (iter != _eventTabal.end())
    {
        auto &mapLevMission = iter->second;
        auto iterLv = mapLevMission.begin();
        while (iterLv != mapLevMission.end())
        {
            auto &setMission = iterLv->second;
            auto iterMission = setMission.find(missionId);
            if (iterMission != setMission.end())
            {
                setMission.erase(iterMission);
                if (setMission.size() <= 0)
                {
                    iterLv = mapLevMission.erase(iterLv);
                    continue;
                }
            }
            ++iterLv;
        }
        if (mapLevMission.size() <= 0)
        {
            iter = _eventTabal.erase(iter);
            continue;
        }
        ++iter;
    }
}

void NFMissionPart::OnEvent(uint32_t eventType, const ExecuteData &data, uint64_t dynamicId)
{
    int32_t level = m_pMaster->GetAttr(proto_ff::A_LEVEL);
    EventTabal::iterator iter = _eventTabal.find(eventType);
    if (_eventTabal.end() != iter)
    {
        auto &mapLvMission = iter->second;
        auto iterLv = mapLvMission.begin();
        for (; iterLv != mapLvMission.end(); ++iterLv)
        {
            auto &setMission = iterLv->second;
            if (iterLv->first <= level)
            {
                if (dynamicId > 0)
                {
                    auto iterMission = setMission.find(dynamicId);
                    if (iterMission != setMission.end())
                    {
                        OnUpdateProgress((*iterMission), data);
                        break;
                    }
                }
                else
                {
                    auto iterMission = setMission.begin();
                    for (; iterMission != setMission.end(); ++iterMission)
                    {
                        OnUpdateProgress((*iterMission), data);
                    }
                }
            }
        }
    }
}

int32_t NFMissionPart::OnSubmit(uint64_t missionId, uint32_t selidx)
{
    NFPackagePart *pIpackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_EXPR(pIpackage, proto_ff::RET_FAIL, "NFPackagePart == NULL");
    
    MissionInfo *pMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(missionId);
    if (nullptr == pMissionInfo)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "[logic] OnSubmit Have Not MissionID={} Config, Please Check Mission Config", missionId);
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    //�����ύ�������Ƿ����ѽ��б��д���
    auto iter = _playerTrackMissionMap.find(missionId);
    if (_playerTrackMissionMap.end() == iter)
    {
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    MissionTrack *pSubmitMissionTrack = &iter->second;
    if (nullptr == pSubmitMissionTrack)
    {
        return proto_ff::RET_FAIL;
    }
    
    //�ж������Ƿ����
    if (MISSION_E_COMPLETION != pSubmitMissionTrack->status)
    {
        return proto_ff::RET_MISSION_STATE_NOT_MATCH;
    }
    
    uint64_t subdynamicId = pSubmitMissionTrack->dynamicId;
    //�̶������ź��棬���ܸ��轱��֮��ᴥ��ˢ�¿ɽ��б�
    
    LIST_ITEM lstOutItem;
    lstOutItem.clear();
    if (!CanAddReward(pMissionInfo->missionId, pMissionInfo->kind, pMissionInfo->subAward, lstOutItem))
    {
        //�����ռ䲻��
        return proto_ff::RET_PACKAGE_COMMON_SPACE_NOT_ENOUGH;
    }
    
    //�Ƴ�����
    int32_t ret = RemoveMission(pSubmitMissionTrack, pMissionInfo);
    if (proto_ff::RET_SUCCESS != ret)
    {
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                   "MissionManager::OnSubmit...RemoveMission failed..cid:{},dynamic:{},missionid:{},backmission:{},ret:{}", m_pMaster->Cid(),
                   subdynamicId, pMissionInfo->missionId, pMissionInfo->backTaskId, ret);
        return ret;
    }
    
    //
    int32_t retReward = AddReward(pMissionInfo->missionId, pMissionInfo->kind, pMissionInfo->subAward);
    if (proto_ff::RET_SUCCESS != retReward)
    {
        //�����ǲ��ᵽ�����ӡ��־������涨λ����
        NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                   "MissionManager::OnSubmit...AddReward failed..cid:{},dynamic:{},missionid:{},backmission:{},retReward:{}", m_pMaster->Cid(),
                   subdynamicId, pMissionInfo->missionId, pMissionInfo->backTaskId, retReward);
    }
    
    //�ύ����
    uint64_t premissionid = 0;
    if (pMissionInfo->setPreTask.size() > 0)
    {
        premissionid = (*pMissionInfo->setPreTask.begin());
    }
    
    OnSubmit(missionId, premissionid, pMissionInfo->kind);
    
    //������񴥷��¼�
    uint32_t missionType = pMissionInfo->kind;
    proto_ff::FinishTaskEvent taskEvent;
    taskEvent.set_taskid(missionId);
    taskEvent.set_tasktype(missionType);
    taskEvent.set_cid(m_pMaster->Cid());
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_FINISH_TASK, CREATURE_PLAYER, m_pMaster->Cid(), taskEvent);
    //
    NFLogDebug(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
               "MissionManager::OnSubmit...success cid:{},dynamic:{},missionid:{},premission:{},backmission:{},ret:{}", m_pMaster->Cid(),
               subdynamicId, pMissionInfo->missionId, premissionid, pMissionInfo->backTaskId, ret);
    //
    if (pMissionInfo->backTaskId > 0)
    {
        const MissionInfo *pBackMissionInfo = TaskDescEx::Instance()->GetMissionCfgInfo(pMissionInfo->backTaskId);
        if (nullptr != pBackMissionInfo)
        {
            //����к��������Զ���ȡ
            int32_t retaccept = OnAccept(pMissionInfo->backTaskId, true);
            if (proto_ff::RET_SUCCESS != retaccept)
            {
                NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                           "MissionManager::OnSubmit...can not accept backtask...cid:{},missionid:{},backmission:{},retaccept:{}",
                           m_pMaster->Cid(), pMissionInfo->missionId, pMissionInfo->backTaskId, retaccept);
            }
            else
            {
                NFLogDebug(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                           "MissionManager::OnSubmit...accept backtask success...cid:{},missionid:{},backmission:{} ", m_pMaster->Cid(),
                           pMissionInfo->missionId, pMissionInfo->backTaskId);
            }
        }
        else
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                       "MissionManager::OnSubmit...can not find backtask cfg...cid:{},missionid:{},backmission:{} ", m_pMaster->Cid(),
                       pMissionInfo->missionId, pMissionInfo->backTaskId);
        }
    }
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::OnSubmit(uint64_t missionId, uint64_t premissionId, uint32_t kind)
{
    auto iter = _mapRecentSubmit.find(kind);
    if (iter != _mapRecentSubmit.end())
    {
        iter->second.erase(premissionId);
        if (iter->second.size() >= iter->second.max_size())
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapRecentSubmit iter->second Space Not Enough");
            return;
        }
        iter->second.insert(missionId);
    }
    else
    {
        if (_mapRecentSubmit.size() >= _mapRecentSubmit.max_size())
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(), "_mapRecentSubmit iter->second Space Not Enough");
            return;
        }
        _mapRecentSubmit[kind].insert(missionId);
    }
    _setAlreadySubmit.insert(missionId);
    MarkDirty();
    
    //
    CheckPreAcceptMission(missionId);
}

void NFMissionPart::CheckPreAcceptMission(uint64_t missionId, bool notify)
{
    auto pSetPreAccept = TaskDescEx::Instance()->GetPreAcceptMission(missionId);
    if (nullptr == pSetPreAccept)
    {
        return;
    }
    
    for (auto iter = pSetPreAccept->begin(); iter != pSetPreAccept->end(); ++iter)
    {
        uint64_t acceptId = (*iter);
        MissionInfo *pMissionCfg = TaskDescEx::Instance()->GetMissionCfgInfo(acceptId);
        if (nullptr == pMissionCfg)
        {
            continue;
        }
        
        if (HaveAccept(acceptId))
        {
            continue;
        }
        
        if (HaveSubmited(acceptId))
        {
            continue;
        }
        
        if (proto_ff::RET_SUCCESS != CanMatchAcceptCond(pMissionCfg))
        {
            continue;
        }
        
        int32_t ret = OnAccept(acceptId, notify);
        if (proto_ff::RET_SUCCESS != ret)
        {
            NFLogError(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                       "MissionPart::CheckPreAcceptMission...OnAccept failed...cid:{},mission:{},acceptId:{},ret:{} ", m_pMaster->Cid(),
                       missionId, acceptId, ret);
        }
        else
        {
            NFLogDebug(NF_LOG_SYSTEMLOG, m_pMaster->Cid(),
                       "MissionPart::CheckPreAcceptMission...accept mission...cid:{},mission:{},acceptId:{},ret:{}", m_pMaster->Cid(),
                       missionId, acceptId, ret);
        }
    }
}

int32_t NFMissionPart::OnSubmitDy(uint64_t dymissionId)
{
    NFPackagePart *pPackage = dynamic_cast<NFPackagePart *>(m_pMaster->GetPart(PART_PACKAGE));
    CHECK_EXPR(pPackage, proto_ff::RET_FAIL, "pPackage == NULL");
    
    //�����ύ�������Ƿ����ѽ��б��д���
    auto iter = _playerTrackMissionMap.find(dymissionId);
    if (_playerTrackMissionMap.end() == iter)
    {
        return proto_ff::RET_MISSION_NOT_EXIST;
    }
    
    //�ж������Ƿ����
    if (MISSION_E_COMPLETION != iter->second.status)
    {
        return proto_ff::RET_MISSION_STATE_NOT_MATCH;
    }
    
    uint64_t missionId = iter->second.missionId;
    uint32_t missionType = iter->second.missionType;
    
    //�Ƴ�����
    int32_t ret = RemoveDyMission(dymissionId, true);
    if (proto_ff::RET_SUCCESS != ret)
    {
        return ret;
    }
    
    if (missionType != MISSION_TYPE_ID_BOUNTY)
    {
        //������
        SMissionReward missionReward;
        OnAddDyMissionReward(missionType, missionId, missionReward);
    }
    
    //������񴥷��¼�
    proto_ff::FinishTaskEvent taskEvent;
    taskEvent.set_taskid(missionId);
    taskEvent.set_tasktype(missionType);
    taskEvent.set_cid(m_pMaster->Cid());
    FireExecute(NF_ST_LOGIC_SERVER, EVENT_FINISH_TASK, CREATURE_PLAYER, m_pMaster->Cid(), taskEvent);
    
    //�����ύ����,Ҫ�ŵ��������棬����������о���������������
    OnSubmitDy(dymissionId, missionType);
    
    return proto_ff::RET_SUCCESS;
}

void NFMissionPart::OnSubmitDy(uint64_t dymissionId, int32_t missionType)
{
    if (MISSION_TYPE_ID_LOOP == missionType)
    {
        OnFinishLoopMission(1);
    }
    
    if (MISSION_TYPE_ID_GUILD == missionType || MISSION_TYPE_ID_LOOP == missionType || MISSION_TYPE_ID_BOUNTY == missionType)
    {
        AcceptMissionByType(missionType, true);
    }
}

void NFMissionPart::OnFinishLoopMission(int32_t count)
{
    ExecuteData allExecuteData(M_EVENT_LOOPMISSIONNUM, 0, count);
    OnEvent(M_EVENT_LOOPMISSIONNUM, allExecuteData);
}











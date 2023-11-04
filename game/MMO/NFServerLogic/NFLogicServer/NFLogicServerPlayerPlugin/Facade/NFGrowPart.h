// -------------------------------------------------------------------------
//    @FileName         :    NFGrowPart.h
//    @Author           :    gaoyi
//    @Date             :    23-10-20
//    @Email			:    445267987@qq.com
//    @Module           :    NFGrowPart
//
// -------------------------------------------------------------------------

#pragma once


#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFShmCore/NFShmObj.h"
#include "NFComm/NFShmCore/NFShmMgr.h"
#include "Part/NFPart.h"
#include "NFLogicCommon/NFLogicShmTypeDefines.h"
#include "NFComm/NFShmCore/NFISharedMemModule.h"
#include "Grow.pb.h"

class NFGrowPart : public NFShmObjTemplate<NFGrowPart, EOT_LOGIC_PART_ID+PART_GROW, NFPart>
{
public:
    //����Ԫ��
    struct GrowPartEntry
    {
        GrowPartEntry()
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
        
        int CreateInit()
        {
            id = 0;
            lv = 0;
            return 0;
        }
        
        int ResumeInit()
        {
            return 0;
        }
        
        int64_t id = 0;
        int32_t lv = 0;
    };
    
    typedef NFShmHashMap<int64_t, GrowPartEntry, 10> GrowPartEntryMap;
    
    //���ɲ���
    struct GrowPartData
    {
        GrowPartData()
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
        
        int CreateInit()
        {
            type = 0;
            curId = 0;
            return 0;
        }
        
        int ResumeInit()
        {
            return 0;
        }
        
        int32_t type;
        GrowPartEntryMap entryMap;
        int64_t curId;
        
        GrowPartEntry *GetPartEntry(int64_t id);
        
        void SetPartEntry(GrowPartEntry &entry);
    };
    
    typedef NFShmHashMap<int32_t, GrowPartData, proto_ff::GrowType_MAX> GrowPartMap;
public:
    NFGrowPart();

    virtual ~NFGrowPart();

    int CreateInit();

    int ResumeInit();

public:
    //******************part���ýӿ�******************
    /**
     * @brief ��ʼ��part
     * @param pMaster
     * @param partType
     * @param dbData
     * @param bCreatePlayer
     * @return
     */
    virtual int Init(NFPlayer *pMaster, uint32_t partType, const proto_ff::RoleDBData &dbData);
    
    /**
     * @brief �ͷ�part��Դ
     * @return
     */
    virtual int UnInit();
public:
    /**
     * @brief �����ݿ��м�������
     * @param data
     * @return
     */
    virtual int LoadFromDB(const proto_ff::RoleDBData& data);
    
    /**
     * @brief �������г�ʼ������
     * @return
     */
    virtual int InitConfig(const proto_ff::RoleDBData& data);
    
    /**
     * @brief �洢DB�������
     * @param proto
     * @return
     */
    virtual int SaveDB(proto_ff::RoleDBData &dbData);
    
    /**
     * @brief ��½���
     * @return
     */
    virtual int OnLogin();
    virtual int OnLogin(proto_ff::PlayerInfoRsp& playerInfo) { return 0; }
    
    /**
     * @brief �ǳ����
     * @return
     */
    virtual int OnLogout() { return 0; }
    
    /**
     * @brief �������
     * @return
     */
    virtual int OnDisconnect() { return 0; }
    
    /**
     * @brief �������
     * @return
     */
    virtual int OnReconnect() { return 0; }
    
    ////////////////////////////////// ÿ����� ÿ��һ��� ˢ�½ӿ� ///////////////////////////////////
    /**
     * @brief ÿ����� ˢ�½ӿ�
     * @return
     */
    virtual int DailyZeroUpdate() { return 0; }
    
    /**
     * @brief ÿ����� ˢ�½ӿ�
     * @return
     */
    virtual int WeekZeroUpdate() { return 0; }
    
    /**
     * @brief ÿ��ˢ�½ӿ�
     * @return
     */
    virtual int MonthZeroUpdate() { return 0; };
    
    /**
     * ���������Ϣ
     * @param outproto
     */
    virtual int FillFacadeProto(proto_ff::RoleFacadeProto& outproto);
    //���ͷ������
    void FillHeadProto(proto_ff::RoleHeadPicProto& proto);
public:
    /**
     * @brief update
     */
    virtual int Update() { return 0; }

public:
    /**
     * @brief ע��Ҫ�������Ϣ
     * @return
     */
    virtual int RegisterMessage();
public:
    /**
     * @brief ����ͻ�����Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleClientMessage(uint32_t msgId, NFDataPackage &packet);
    
    /**
     * @brief �������Է���������Ϣ
     * @param unLinkId
     * @param packet
     * @return
     */
    virtual int OnHandleServerMessage(uint32_t msgId, NFDataPackage &packet);
public:
    int HanlderLvUpReq(uint32_t msgId, NFDataPackage &packet);        //����
    int HanlderDressReq(uint32_t msgId, NFDataPackage &packet);        //����
    int HanlderUnDressReq(uint32_t msgId, NFDataPackage &packet);        //ж��
public:
    int OnActivePartEntry(int64_t id);                            //������
    int OnLvupPartEntry(int64_t id);                            //��������
public:
    int FillProto(const GrowPartData &part, proto_ff::GrowSubInfo &proto);
    
    GrowPartData *getPart(int32_t type);

public:
    int SendAllGrowData();                                        //����ȫ������
    int SendGrowPartData(int32_t type);                        //����ĳ����������
public:
    void calcAttr(bool sync);
    
    void calcAttr(int32_t type, MAP_INT32_INT64 &outAttr);        //�������ɲ�������
    void MergeAttr(MAP_INT32_INT64 &src, MAP_INT32_INT64 &dst);
    
    void PrintAttr(int32_t type, MAP_INT32_INT64 &attr);

private:
    void initParts();

protected:
    GrowPartMap m_partsMap;                    //�������ɵĲ���
};
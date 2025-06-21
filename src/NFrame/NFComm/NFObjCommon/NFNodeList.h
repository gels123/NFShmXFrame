// -------------------------------------------------------------------------
//    @FileName         :    NFNodeList.h
//    @Author           :    gaoyi
//    @Date             :    22-11-14
//    @Email			:    445267987@qq.com
//    @Module           :    NFNodeList
//
// -------------------------------------------------------------------------

#pragma once

#include "NFRawObject.h"
#include "NFComm/NFCore/NFPlatform.h"
#include "NFComm/NFObjCommon/NFObject.h"
#include "NFComm/NFObjCommon/NFShmMgr.h"
#include "NFComm/NFPluginModule/NFIMemMngModule.h"
#include "NFComm/NFPluginModule/NFCheck.h"

// 链表对象类
// 此对象需要继承NFShmObj类
// 方便共享内存对象的链表管理
class NFIListNodeObj
{
public:
    virtual ~NFIListNodeObj()
    {
    }

    NFIListNodeObj()
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
        m_iPrevNode = INVALID_ID;
        m_iNextNode = INVALID_ID;
        m_iListCheckId = INVALID_ID;
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    int GetPrevNode() const
    {
        return m_iPrevNode;
    }

    void SetPrevNode(int iNode)
    {
        m_iPrevNode = iNode;
    }

    int GetNextNode() const
    {
        return m_iNextNode;
    }

    void SetNextNode(int iNode)
    {
        m_iNextNode = iNode;
    }

    int GetListCheckId() const
    {
        return m_iListCheckId;
    }

    void SetListCheckId(int iListCheckId)
    {
        m_iListCheckId = iListCheckId;
    }

    static int GetListCheckIdSeq();

public:
    static int m_iListCheckIdSeq;
    int m_iListCheckId;
    int m_iPrevNode;
    int m_iNextNode;
};

template <class NodeObjType>
class NFListNodeObjWithObjectId final : public NFIListNodeObj
{
public:
    ~NFListNodeObjWithObjectId() override
    {
    }

    static NodeObjType* GetObjByListNodeId(int iListNodeId)
    {
        return NodeObjType::GetObjByObjId(iListNodeId);
    }

    int GetListNodeId()
    {
        return dynamic_cast<NodeObjType*>(this)->GetObjId();
    }
};

template <class NodeObjType>
class NFListNodeObjWithGlobalId : public NFIListNodeObj
{
public:
    ~NFListNodeObjWithGlobalId() override
    {
    }

    static NodeObjType* GetObjByListNodeId(int iListNodeId)
    {
        return NodeObjType::GetObjByGlobalId(iListNodeId, true);
    }

    int GetListNodeId()
    {
        return dynamic_cast<NodeObjType*>(this)->GetGlobalId();
    }
};

//双向列表
template <class NodeObjType>
class NFNodeObjList final : public NFRawObject
{
public:
    NFNodeObjList()
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
        InitNodeList();
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    void InitNodeList()
    {
        m_iNodeCount = 0;
        m_iHeadNode = INVALID_ID;
        m_iTailNode = INVALID_ID;
        m_iListCheckId = NFIListNodeObj::GetListCheckIdSeq();
    }

    bool IsEmpty() const
    {
        return m_iNodeCount <= 0;
    }

    bool Empty() const
    {
        return IsEmpty();
    }

    int GetNodeCount() const
    {
        return m_iNodeCount;
    }

    int GetHeadNodeId() const
    {
        return m_iHeadNode;
    }

    int GetTailNodeId() const
    {
        return m_iTailNode;
    }

    int GetLastCheckId() const
    {
        return m_iListCheckId;
    }

    NodeObjType* GetHeadNodeObj()
    {
        if (m_iHeadNode != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(m_iHeadNode);
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed, m_iHeadNode:{}", m_iHeadNode);
            return pObj;
        }

        return nullptr;
    }

    NodeObjType* GetTailNodeObj()
    {
        if (m_iTailNode != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(m_iTailNode);
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed, m_iTailNode:{}", m_iTailNode);
            return pObj;
        }

        return nullptr;
    }

    NodeObjType* GetPrevNodeObj(NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, NULL, "");
        if (pNode->GetPrevNode() != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(pNode->GetPrevNode());
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed");
            return pObj;
        }

        return nullptr;
    }

    NodeObjType* GetNextNodeObj(NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, NULL, "");
        if (pNode->GetNextNode() != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(pNode->GetNextNode());
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed");
            return pObj;
        }

        return nullptr;
    }

    int AddNode(NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetPrevNode() == INVALID_ID, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetNextNode() == INVALID_ID, -1, "");
        //assert(pNode->GetListCheckId() == INVALID_ID);
        NodeObjType* pHead = GetHeadNodeObj();

        if (pHead)
        {
            pNode->SetNextNode(pHead->GetListNodeId());
            pHead->SetPrevNode(pNode->GetListNodeId());
        }
        else
        {
            CHECK_EXPR_ASSERT(0 == m_iNodeCount, -1, "");
            m_iTailNode = pNode->GetListNodeId();
        }

        pNode->SetListCheckId(m_iListCheckId);
        m_iHeadNode = pNode->GetListNodeId();
        m_iNodeCount++;
        return 0;
    }

    //加到列表的最后
    int AddNodeToTail(NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetPrevNode() == INVALID_ID, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetNextNode() == INVALID_ID, -1, "");
        //assert(pNode->GetListCheckId() == INVALID_ID);
        NodeObjType* pTail = GetTailNodeObj();

        if (pTail)
        {
            pTail->SetNextNode(pNode->GetListNodeId());
            pNode->SetPrevNode(pTail->GetListNodeId());
        }
        else
        {
            CHECK_EXPR_ASSERT(0 == m_iNodeCount, -1, "");
            m_iHeadNode = pNode->GetListNodeId();
        }

        pNode->SetListCheckId(m_iListCheckId);
        m_iTailNode = pNode->GetListNodeId();
        m_iNodeCount++;
        return 0;
    }

    int RemoveNode(NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, -1, "");
        CHECK_EXPR_ASSERT(m_iNodeCount > 0, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetListCheckId() == m_iListCheckId, -1, "");
        NodeObjType* pPrevNode = GetPrevNodeObj(pNode);

        if (pPrevNode)
        {
            pPrevNode->SetNextNode(pNode->GetNextNode());
        }

        NodeObjType* pNextNode = GetNextNodeObj(pNode);

        if (pNextNode)
        {
            pNextNode->SetPrevNode(pNode->GetPrevNode());
        }

        if (pNode->GetListNodeId() == m_iHeadNode)
        {
            m_iHeadNode = pNode->GetNextNode();
        }

        if (pNode->GetListNodeId() == m_iTailNode)
        {
            m_iTailNode = pNode->GetPrevNode();
        }

        pNode->SetNextNode(INVALID_ID);
        pNode->SetPrevNode(INVALID_ID);
        pNode->SetListCheckId(INVALID_ID);
        m_iNodeCount--;
        return 0;
    }

    bool IsExistNode(NodeObjType* pNode)
    {
        return pNode->GetListCheckId() == m_iListCheckId;
    }

private:
    int m_iListCheckId;
    int m_iNodeCount;
    int m_iHeadNode;
    int m_iTailNode;
};

// 链表对象类
// 此对象需要继承NFShmObj类
// 方便共享内存对象的链表管理
template <size_t MaxType>
class NFIMultiListNodeObj
{
public:
    virtual ~NFIMultiListNodeObj()
    {
    }

    NFIMultiListNodeObj()
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
        for (int i = 0; i < static_cast<int>(MaxType); i++)
        {
            m_objList[i].CreateInit();
        }
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    int GetPrevNode(int typeIndex) const
    {
        CHECK_EXPR_ASSERT(typeIndex >= 0 && typeIndex < (int)MaxType, -1, "typeIndex:{} max:{}", typeIndex, MaxType);
        return m_objList[typeIndex].m_iPrevNode;
    }

    void SetPrevNode(int typeIndex, int iNode)
    {
        CHECK_EXPR_ASSERT(typeIndex >= 0 && typeIndex < (int)MaxType, , "typeIndex:{} max:{}", typeIndex, MaxType);
        m_objList[typeIndex].m_iPrevNode = iNode;
    }

    int GetNextNode(int typeIndex) const
    {
        CHECK_EXPR_ASSERT(typeIndex >= 0 && typeIndex < (int)MaxType, -1, "typeIndex:{} max:{}", typeIndex, MaxType);
        return m_objList[typeIndex].m_iNextNode;
    }

    void SetNextNode(int typeIndex, int iNode)
    {
        CHECK_EXPR_ASSERT(typeIndex >= 0 && typeIndex < (int)MaxType, , "typeIndex:{} max:{}", typeIndex, MaxType);
        m_objList[typeIndex].m_iNextNode = iNode;
    }

    int GetListCheckId(int typeIndex) const
    {
        CHECK_EXPR_ASSERT(typeIndex >= 0 && typeIndex < (int)MaxType, -1, "typeIndex:{} max:{}", typeIndex, MaxType);
        return m_objList[typeIndex].m_iListCheckId;
    }

    void SetListCheckId(int typeIndex, int iListCheckId)
    {
        CHECK_EXPR_ASSERT(typeIndex >= 0 && typeIndex < (int)MaxType, , "typeIndex:{} max:{}", typeIndex, MaxType);
        m_objList[typeIndex].m_iListCheckId = iListCheckId;
    }

protected:
    NFIListNodeObj m_objList[MaxType];
};

template <class NodeObjType, size_t MaxType>
class NFMultiListNodeObjWithGlobalId : public NFIMultiListNodeObj<MaxType>
{
public:
    ~NFMultiListNodeObjWithGlobalId() override
    {
    }

    static NodeObjType* GetObjByListNodeId(int iListNodeId)
    {
        return NodeObjType::GetObjByGlobalId(iListNodeId, true);
    }

    int GetListNodeId()
    {
        return dynamic_cast<NodeObjType*>(this)->GetGlobalId();
    }
};

//双向列表
template <class NodeObjType>
class NFNodeObjMultiList final : public NFRawObject
{
public:
    NFNodeObjMultiList()
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
        InitNodeList();
        return 0;
    }

    int ResumeInit()
    {
        return 0;
    }

    NFNodeObjMultiList(const NFNodeObjMultiList& list)
    {
        if (this != &list)
        {
            m_iListCheckId = list.m_iListCheckId;
            m_iNodeCount = list.m_iNodeCount;
            m_iHeadNode = list.m_iHeadNode;
            m_iTailNode = list.m_iTailNode;
        }
    }

    void InitNodeList()
    {
        m_iNodeCount = 0;
        m_iHeadNode = INVALID_ID;
        m_iTailNode = INVALID_ID;
        m_iListCheckId = NFIListNodeObj::GetListCheckIdSeq();
    }

    int GetNodeCount() const
    {
        return m_iNodeCount;
    }

    bool IsEmpty() const
    {
        return m_iNodeCount <= 0;
    }

    bool Empty() const
    {
        return IsEmpty();
    }

    int GetHeadNodeId() const
    {
        return m_iHeadNode;
    }

    int GetTailNodeId() const
    {
        return m_iTailNode;
    }

    int GetLastCheckId() const
    {
        return m_iListCheckId;
    }

    NodeObjType* GetHeadNodeObj(int typeIndex)
    {
        if (m_iHeadNode != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(m_iHeadNode);
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed, m_iHeadNode:{}", m_iHeadNode);
            return pObj;
        }

        return nullptr;
    }

    NodeObjType* GetTailNodeObj(int typeIndex)
    {
        if (m_iTailNode != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(m_iTailNode);
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed, m_iTailNode:{}", m_iTailNode);
            return pObj;
        }

        return nullptr;
    }

    NodeObjType* GetPrevNodeObj(int typeIndex, NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, NULL, "");
        if (pNode->GetPrevNode(typeIndex) != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(pNode->GetPrevNode(typeIndex));
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed");
            return pObj;
        }

        return nullptr;
    }

    NodeObjType* GetNextNodeObj(int typeIndex, NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, NULL, "");
        if (pNode->GetNextNode(typeIndex) != INVALID_ID)
        {
            NodeObjType* pObj = NodeObjType::GetObjByListNodeId(pNode->GetNextNode(typeIndex));
            CHECK_EXPR_ASSERT(pObj, NULL, "GetObjByListNodeId Failed");
            return pObj;
        }

        return nullptr;
    }

    int AddNode(int typeIndex, NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetPrevNode(typeIndex) == INVALID_ID, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetNextNode(typeIndex) == INVALID_ID, -1, "");
        //assert(pNode->GetListCheckId() == INVALID_ID);
        NodeObjType* pHead = GetHeadNodeObj(typeIndex);

        if (pHead)
        {
            pNode->SetNextNode(typeIndex, pHead->GetListNodeId());
            pHead->SetPrevNode(typeIndex, pNode->GetListNodeId());
        }
        else
        {
            CHECK_EXPR_ASSERT(0 == m_iNodeCount, -1, "");
            m_iTailNode = pNode->GetListNodeId();
        }

        pNode->SetListCheckId(typeIndex, m_iListCheckId);
        m_iHeadNode = pNode->GetListNodeId();
        m_iNodeCount++;
        return 0;
    }

    //加到列表的最后
    int AddNodeToTail(int typeIndex, NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetPrevNode(typeIndex) == INVALID_ID, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetNextNode(typeIndex) == INVALID_ID, -1, "");
        //assert(pNode->GetListCheckId() == INVALID_ID);
        NodeObjType* pTail = GetTailNodeObj(typeIndex);

        if (pTail)
        {
            pTail->SetNextNode(typeIndex, pNode->GetListNodeId());
            pNode->SetPrevNode(typeIndex, pTail->GetListNodeId());
        }
        else
        {
            CHECK_EXPR_ASSERT(0 == m_iNodeCount, -1, "");
            m_iHeadNode = pNode->GetListNodeId();
        }

        pNode->SetListCheckId(typeIndex, m_iListCheckId);
        m_iTailNode = pNode->GetListNodeId();
        m_iNodeCount++;
        return 0;
    }

    int RemoveNode(int typeIndex, NodeObjType* pNode)
    {
        CHECK_EXPR_ASSERT(pNode, -1, "");
        CHECK_EXPR_ASSERT(m_iNodeCount > 0, -1, "");
        CHECK_EXPR_ASSERT(pNode->GetListCheckId(typeIndex) == m_iListCheckId, -1, "");
        NodeObjType* pPrevNode = GetPrevNodeObj(typeIndex, pNode);

        if (pPrevNode)
        {
            pPrevNode->SetNextNode(typeIndex, pNode->GetNextNode(typeIndex));
        }

        NodeObjType* pNextNode = GetNextNodeObj(typeIndex, pNode);

        if (pNextNode)
        {
            pNextNode->SetPrevNode(typeIndex, pNode->GetPrevNode(typeIndex));
        }

        if (pNode->GetListNodeId() == m_iHeadNode)
        {
            m_iHeadNode = pNode->GetNextNode(typeIndex);
        }

        if (pNode->GetListNodeId() == m_iTailNode)
        {
            m_iTailNode = pNode->GetPrevNode(typeIndex);
        }

        pNode->SetNextNode(typeIndex, INVALID_ID);
        pNode->SetPrevNode(typeIndex, INVALID_ID);
        pNode->SetListCheckId(typeIndex, INVALID_ID);
        m_iNodeCount--;
        return 0;
    }

    bool IsExistNode(NodeObjType* pNode, int typeIndex)
    {
        return pNode->GetListCheckId(typeIndex) == m_iListCheckId;
    }

private:
    int m_iListCheckId;
    int m_iNodeCount;
    int m_iHeadNode;
    int m_iTailNode;
};

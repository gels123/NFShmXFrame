#pragma once

#define THERON_USE_STD_THREADS 1

#include <vector>
#include <Theron/Theron.h>
#include <atomic>
#include "NFComm/NFCore/NFMutex.h"
class NFTask;
class NFITaskModule;
class NFITaskComponent;

/**
* @brief actor��Ϣ����
*
*/
class NFTaskActorMessage
{
public:
	/**
	* @brief ��Ϣ����
	*
	*/
	enum MessageType
	{
		/**
		* @brief Ҫ�������Ϣ
		*
		*/
		ACTOR_MSG_TYPE_COMPONENT,

		/**
		* @brief �Ѿ����������Ϣ���ȴ��������̴߳���
		*
		*/
		ACTOR_MSG_TYPE_END_FUNC,
	};

	/**
	* @brief ���캯��
	*
	*/
	NFTaskActorMessage()
	{
		nFromActor = 0;
		pData = nullptr;
		nMsgType = ACTOR_MSG_TYPE_COMPONENT;
	}

	virtual ~NFTaskActorMessage()
	{
		nFromActor = 0;
		pData = nullptr;
		nMsgType = ACTOR_MSG_TYPE_COMPONENT;
	}

public:
	/**
	* @brief ��Ϣ����
	*
	*/
	int nMsgType;

	/**
	* @brief ������Ϣ��actor����
	*
	*/
	int nFromActor;

	/**
	* @brief ��Ϣ����
	*
	*/
	NFTask* pData;
};

/**
* @brief actor����
*
*/
class NFTaskActor : public Theron::Actor
{
public:
	/**
	* @brief ���캯��
	*        �������RegisterHandler��ע���첽��������������ĺ���
	*/
	NFTaskActor(Theron::Framework& framework, NFITaskModule* pActorMgr);

	/**
	* @brief ��������
	*
	*/
	virtual ~NFTaskActor();

	/**
	* @brief ��ʼ��
	*
	* @param
	* @return
	*/
	virtual bool Init();

	/**
	* @brief ������һ��actor������Ϣ
	*
	* @param address ������Ϣ��actor�ĵ�ַ
	* @param message ���ֵ���Ϣ
	* @return ���ط����Ƿ�ɹ�
	*/
	virtual bool SendMsg(const Theron::Address& address, const NFTaskActorMessage& message);

	/**
	* @brief ���component
	*
	* @param
	* @return
	*/
	virtual bool AddComponnet(NFITaskComponent* pComponnet);

	/**
	* @brief �������component
	*
	* @param
	* @return
	*/
	virtual NFITaskComponent* GetTaskComponent();

	/**
	* @brief ���ActorId
	*
	* @param
	* @return
	*/
	virtual int GetActorId() const;
protected:
	/**
	* @brief �����Ѿ������������Ϣ
	*
	* @param message Ҫ�������Ϣ
	* @param from	 ������Ϣ��actor��ַ
	* @return ���ط����Ƿ�ɹ�
	*/
	virtual void HandlerEx(const NFTaskActorMessage& message, Theron::Address from);

	/**
	* @brief �����͵�����
	*
	* @param message Ҫ�������Ϣ
	* @param from	 ������Ϣ��actor��ַ
	* @return
	*/
	virtual void Handler(const NFTaskActorMessage& message, Theron::Address from);
public:
	/**
	* @brief ����taskǰ�Ĵ���
	*
	* @param pTask Ҫ���������
	* @return
	*/
	virtual void ProcessTaskStart(NFTask* pTask);

	/**
	* @brief task�����Ĵ���
	*
	* @param pTask Ҫ���������
	* @return
	*/
	virtual void ProcessTaskEnd(NFTask* pTask);

	/**
	* @brief �첽��������ϵͳ
	*
	* @param pTask Ҫ���������
	* @return
	*/
	virtual void ProcessTask(NFTask* pTask);

	/**
	* @brief ��鳬ʱtask, �����̴߳���
	*
	* @param 
	* @return
	*/
	virtual void CheckTimeoutTask();
protected:
	/**
	* @brief �첽����Ĺ���
	*
	* @param message Ҫ�������Ϣ
	* @param from	 ������Ϣ��actor��ַ
	* @return
	*/
	virtual void DefaultHandler(const NFTaskActorMessage& message, Theron::Address from);
protected:
	/**
	* @brief actor�������
	*/
	NFITaskModule* m_pTaskModule;

	/**
	* @brief component����
	*/
	NFITaskComponent* m_pComponent;
};

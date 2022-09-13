// -------------------------------------------------------------------------
//    @FileName         :    NFITaskComponent.h
//    @Author           :    Gao.Yi
//    @Date             :   xxxx-xx-xx
//    @Module           :    NFITaskComponent
//
//
//                    .::::.
//                  .::::::::.
//                 :::::::::::  FUCK YOU
//             ..:::::::::::'
//           '::::::::::::'
//             .::::::::::
//        '::::::::::::::..
//             ..::::::::::::.
//           ``::::::::::::::::
//            ::::``:::::::::'        .:::.
//           ::::'   ':::::'       .::::::::.
//         .::::'      ::::     .:::::::'::::.
//        .:::'       :::::  .:::::::::' ':::::.
//       .::'        :::::.:::::::::'      ':::::.
//      .::'         ::::::::::::::'         ``::::.
//  ...:::           ::::::::::::'              ``::.
// ```` ':.          ':::::::::'                  ::::..
//                    '.:::::'                    ':'````..
//
// -------------------------------------------------------------------------

#pragma once

#include <string>

/*
�̳߳ص��̻߳���
*/

class NFTask
{
public:
	NFTask()
	{
		m_balanceId = 0;
		m_useTime = 0;
		m_needManThreadProcess = true;
        m_handleActorId = 0;
        m_handleStartTime = 0;
	}

	virtual ~NFTask()
	{
	}

	enum TPTaskState
	{
		/// һ�������Ѿ����
		TPTASK_STATE_COMPLETED = 0,

		/// ���������߳�ִ��
		TPTASK_STATE_CONTINUE_MAINTHREAD = 1,

		// ���������߳�ִ��
		TPTASK_STATE_CONTINUE_CHILDTHREAD = 2,
	};

	virtual uint64_t GetBalanceId() const
	{
		return m_balanceId;
	}

	virtual bool IsNeedMainThreadProcess() const
	{
		return m_needManThreadProcess;
	}

	virtual void SetBalanceId(uint64_t balanceId)
	{
		m_balanceId = balanceId;
	}

	/**
	**  �첽�̴߳�������������һ���߳�������
	*/
	virtual bool ThreadProcess() = 0;

	/**
	** ���̴߳������������̴߳�������ύ���������������ݷ��غ����Ƿ��������
		����ֵ�� thread::TPTask::TPTaskState�� ��ο�TPTaskState
	*/
	virtual TPTaskState MainThreadProcess()
	{
		return TPTASK_STATE_COMPLETED;
	}
public:
	bool m_needManThreadProcess; //�Ƿ���Ҫ���̴߳���
	uint64_t m_balanceId; //��̬����ID, �������Ҿ������CharId, �������һ����0
	uint32_t m_useTime; //�������ʹ��ʱ��
	std::string m_taskName; //�첽��������
	uint32_t m_handleActorId;
	uint64_t m_handleStartTime;
};


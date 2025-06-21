// -------------------------------------------------------------------------
//    @FileName         :    NFITaskComponent.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
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
线程池的线程基类，用于定义任务的基本结构和行为。
*/
class NFTask
{
public:
	/*
	构造函数，初始化任务的基本属性。
	*/
	NFTask()
	{
		m_balanceId = 0; // 动态均衡ID，初始化为0
		m_useTime = 0; // 处理过程使用时间，初始化为0
		m_needManThreadProcess = true; // 默认需要主线程处理
        m_handleActorId = 0; // 处理Actor的ID，初始化为0
        m_handleStartTime = 0; // 处理开始时间，初始化为0
        m_runActorGroup = 0; // 当前运行的Actor组，初始化为0
        m_nextActorGroup = 0; // 下一个运行的Actor组，初始化为0
	}

	/*
	析构函数，虚函数，确保派生类的析构函数能够被正确调用。
	*/
	virtual ~NFTask()
	{
	}

	/*
	任务状态的枚举类型，用于标识任务的执行状态。
	*/
	enum TPTaskState
	{
		/// 一个任务已经完成
		TPTASK_STATE_COMPLETED = 0,

		/// 继续在主线程执行
		TPTASK_STATE_CONTINUE_MAINTHREAD = 1,

		// 继续在子线程执行
		TPTASK_STATE_CONTINUE_CHILDTHREAD = 2,
	};

	/*
	获取动态均衡ID。
	返回值：动态均衡ID，通常为玩家CharId或0。
	*/
	virtual uint64_t GetBalanceId() const
	{
		return m_balanceId;
	}

	/*
	判断是否需要主线程处理。
	返回值：true表示需要主线程处理，false表示不需要。
	*/
	virtual bool IsNeedMainThreadProcess() const
	{
		return m_needManThreadProcess;
	}

	/*
	设置动态均衡ID。
	参数：balanceId - 动态均衡ID，通常为玩家CharId或0。
	*/
	virtual void SetBalanceId(uint64_t balanceId)
	{
		m_balanceId = balanceId;
	}

	/*
	异步线程处理函数，将在另一个线程里运行。
	返回值：true表示处理成功，false表示处理失败。
	*/
	virtual bool ThreadProcess() = 0;

	/*
	主线程处理函数，将在线程处理完后，提交给主线程来处理，根据返回函数是否继续处理。
	返回值：TPTaskState，表示任务的执行状态。
	*/
	virtual TPTaskState MainThreadProcess()
	{
		return TPTASK_STATE_COMPLETED;
	}
public:
	bool m_needManThreadProcess; // 是否需要主线程处理
	uint64_t m_balanceId; // 动态均衡ID, 如果是玩家就是玩家CharId, 如果不是一般填0
	uint32_t m_useTime; // 处理过程使用时间
	std::string m_taskName; // 异步任务名字
	uint32_t m_handleActorId; // 处理Actor的ID
	uint64_t m_handleStartTime; // 处理开始时间
    int32_t m_runActorGroup; // 当前运行的Actor组
    int32_t m_nextActorGroup; // 下一个运行的Actor组
};



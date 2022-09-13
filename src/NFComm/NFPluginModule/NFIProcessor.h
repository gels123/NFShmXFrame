// -------------------------------------------------------------------------
//    @FileName         :    NFIProcessor.h
//    @Author           :    xxxxx
//    @Date             :   xxxx-xx-xx
//    @Email			:    xxxxxxxxx@xxx.xxx
//    @Module           :    NFCore
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"

/// @brief ��Ϣ�������ӿ�
class NFIProcessor 
{
public:
	NFIProcessor();
	virtual ~NFIProcessor();

	/// @brief ��Ϣ�������(Processor������)
	/// @param handle ������(���ڴ�����Ϣ�����ӻ���Ϣ���б�ʶ)
	/// @param msg ��Ϣ
	/// @param msg_len ��Ϣ����
	/// @param msg_info ��Ϣ����
	/// @param is_overload �Ƿ����(��ܸ��ݳ������������֪ProcessorĿǰ�Ƿ���أ���Processor���й��ش���)
	///     �����˾���Ĺ������ͣ�����ɲο� @see OverLoadType
	/// @return 0 �ɹ�
	/// @return ��0 ʧ��
	// TODO: ÿ��Processor����ά��һ����Ϣ�����������ȼ��б�����ĳЩ��Ϣ��ʹ�ڹ����������ȻҪ����
	virtual int32_t OnMessage(int64_t handle, const uint8_t* msg, uint32_t msg_len, const MsgExternInfo* msg_info, uint32_t is_overload) = 0;

	/// @brief Processor������Ϣ�ӿڣ�ʵ��ʹ��SetSendFunction���õ�send�������û�����չ��sendǰ��Щ���⴦��
	/// @return 0 �ɹ���<0 ʧ��
	virtual int32_t Send(int64_t handle, const uint8_t* msg, uint32_t msg_len, int32_t flag);

	/// @brief Processor������Ϣ�ӿڣ�ʵ��ʹ��SetSendFunction���õ�sendv�������û�����չ��sendǰ��Щ���⴦��
	/// @return 0 �ɹ���<0 ʧ��
	virtual int32_t SendV(int64_t handle, uint32_t msg_frag_num,
		const uint8_t* msg_frag[], uint32_t msg_frag_len[], int32_t flag);
};

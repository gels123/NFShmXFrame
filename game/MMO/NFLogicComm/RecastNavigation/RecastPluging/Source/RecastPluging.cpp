// RecastPluging.cpp : ���� DLL Ӧ�ó���ĵ���������
//

#include "stdafx.h"
#include "RecastPluging.h"


// ���ǵ���������һ��ʾ��
RECASTPLUGING_API int nRecastPluging=0;

// ���ǵ���������һ��ʾ����
RECASTPLUGING_API int fnRecastPluging(void)
{
	return 42;
}

// �����ѵ�����Ĺ��캯����
// �й��ඨ�����Ϣ������� RecastPluging.h
CRecastPluging::CRecastPluging()
{
	return;
}

// ���� ifdef ���Ǵ���ʹ�� DLL �������򵥵�
// ��ı�׼�������� DLL �е������ļ��������������϶���� RECASTPLUGING_EXPORTS
// ���ű���ġ���ʹ�ô� DLL ��
// �κ�������Ŀ�ϲ�Ӧ����˷��š�������Դ�ļ��а������ļ����κ�������Ŀ���Ὣ
// RECASTPLUGING_API ������Ϊ�Ǵ� DLL ����ģ����� DLL ���ô˺궨���
// ������Ϊ�Ǳ������ġ�
#ifdef RECASTPLUGING_EXPORTS
#define RECASTPLUGING_API __declspec(dllexport)
#else
#define RECASTPLUGING_API __declspec(dllimport)
#endif

// �����Ǵ� RecastPluging.dll ������
class RECASTPLUGING_API CRecastPluging {
public:
	CRecastPluging(void);
	// TODO:  �ڴ�������ķ�����
};

extern RECASTPLUGING_API int nRecastPluging;

RECASTPLUGING_API int fnRecastPluging(void);

require "Common"

--��������  upper_limit
--ͳ��ʱ���� control_time  ����ͳ��
--������Ϣ��С���  min_interval ����ͳ��

CLIENT_TO_CENTER_LOGIN_BEGIN = 18;
CLIENT_TO_CENTER_LOGIN = 19;       --��½��������
CLIENT_TO_CENTER_CREATE_CHARACTER = 20;    --���󴴽���ɫ����
CLIENT_TO_CENTER_DEL_CHARACTER = 21;       --ɾ��ɫ����

ProxyServer = {
    PacketMsg = {
        {cmd = CLIENT_TO_CENTER_LOGIN, serverType = NF_ST_WORLD_SERVER, upper_limit = 100, min_interval = 0,  control_time = 10000}
        {cmd = CLIENT_TO_CENTER_CREATE_CHARACTER, serverType = NF_ST_WORLD_SERVER, upper_limit = 100, min_interval = 0,  control_time = 10000}
        {cmd = CLIENT_TO_CENTER_DEL_CHARACTER, serverType = NF_ST_WORLD_SERVER, upper_limit = 100, min_interval = 0,  control_time = 10000}
    },
};
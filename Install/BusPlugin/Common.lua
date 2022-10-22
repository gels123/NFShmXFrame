-- server type
NF_ST_NONE = 0;    		--NONE
NF_ST_MASTER_SERVER = 1;
NF_ST_ROUTE_SERVER = 2; --·�ɼ�Ⱥ������ ����ͬ���ӷ���������֮���ת��
NF_ST_ROUTE_AGENT_SERVER = 3; --·�ɴ��������  ����ͬһ̨���ӷ���������֮���ת��
NF_ST_PROXY_SERVER = 4;
NF_ST_PROXY_AGENT_SERVER = 5; --Proxy ·�ɴ�������� ����ͬһ̨���ӿͻ��������������֮���ת��
NF_ST_STORE_SERVER = 6; --DB������
NF_ST_LOGIN_SERVER = 7;
NF_ST_WORLD_SERVER = 8;
NF_ST_LOGIC_SERVER = 9; --Logic������
NF_ST_GAME_SERVER = 10;
NF_ST_SNS_SERVER = 11; --SNS������
NF_ST_WEB_SERVER = 12; --Web������
NF_ST_MONITOR_SERVER = 19; ---Monitor������
NF_ST_MAX = 50;

--������
NF_ST_WORLD_ID=1
--����ID
NF_ST_ZONE_ID=9
--��������ʱ ��ʽ�� 30��
NF_CLIENT_HEART_BEAT_TIME_OUT = 300
--�ⲿ�ͻ��˻�Ծ��ʱ ��ʽ�� 120��
NF_CLIENT_KEEP_ALIVE_TIME_OUT = 1200

--����������IP
NF_INTER_SERVER_IP="127.0.0.1"
--�����������˿�
NF_INTER_SERVER_PORT=10000+NF_ST_ZONE_ID*300;
--�����������������
NF_INTER_MAX_CONNECT=100

--����������IP
--�����������˿�
NF_EXTER_SERVER_IP="192.168.50.172"
NF_EXTER_PROXY_SERVER_PORT=10000+NF_ST_ZONE_ID*10+2;

--�������������
NF_EXTERN_MAX_CONNECT=5000
--����������HTTP�˿�
NF_EXTER_WEB_SERVER_PORT=10000+NF_ST_ZONE_ID*10+4;

--BUS�����ڴ��С 20M�����ڴ�
NF_COMMON_BUS_LENGTH=20971520

--��ǰ��������������
NF_MAX_ONLINE_PLAYER_COUNT=100 --

NF_MASTER_IP=NF_INTER_SERVER_IP
NF_MASTER_PORT=NF_INTER_SERVER_PORT+NF_ST_MASTER_SERVER*10+1
NF_ROUTE_AGENT_ID=NF_ST_WORLD_ID.."."..NF_ST_ZONE_ID.."."..NF_ST_ROUTE_AGENT_SERVER..".1"
NF_ROUTE_AGENT_ID2=NF_ST_WORLD_ID.."."..NF_ST_ZONE_ID.."."..NF_ST_ROUTE_AGENT_SERVER..".2"

--ͨ��ģʽ
NF_LINK_MODE="bus"
--NF_LINK_MODE="tcp"
--
NF_NAMING_HOST="127.0.0.1:2181,127.0.0.1:2182"
NF_NAMING_PATH="99Puke"

--���ݿ�IP
NF_MYSQL_IP="192.168.70.10"
--���ݿ�˿�
NF_MYSQL_PORT=3306
--���ݿ�����
NF_MYSQL_DB_NAME="proto_ff_cgzone"..NF_ST_ZONE_ID
--���ݿ��û���
NF_MYSQL_USER="root"
--���ݿ�����
NF_MYSQL_PASSWORD="cg2016"

----����ʱ��,��λ��
GlobelOpenTime = 1598234400

---�ͻ��˰汾��
NF_CLIENT_VERSION = 0




// -------------------------------------------------------------------------
//    @FileName         :    NFProtobufCommon.h
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFProtobufCommon
//
// -------------------------------------------------------------------------
#pragma once

#include "NFComm/NFCore/NFPlatform.h"
#include "google/protobuf/message.h"
#include "NFIHttpHandle.h"
#include "NFComm/NFPluginModule/NFILuaLoader.h"
#include "NFComm/NFCore/NFSingleton.hpp"
#include "google/protobuf/dynamic_message.h"
#include "NFComm/NFPluginModule/NFJson2PB/NFPbToJson.h"
#include "NFComm/NFKernelMessage/nanopb.pb.h"
#include <vector>
#include <map>
#include <unordered_map>

#define DEFINE_DEFAULT_PROTO_PACKAGE "proto_ff"
#define DEFINE_DEFAULT_PROTO_PACKAGE_ADD "proto_ff."

struct DBTableColInfo
{
    DBTableColInfo()
    {
        m_colType = 0;
        m_primaryKey = false;
        m_unionKey = false;
        m_indexKey = false;
        m_bufsize = 32;
        m_fieldIndex = 0;
        m_autoIncrement = false;
        m_autoIncrementValue = 0;
        m_defaultValue = 0;
        m_isDefaultValue = false;
        m_notNull = false;
    }

    uint32_t m_colType;
    bool m_primaryKey;
    bool m_unionKey;
    bool m_indexKey;
    bool m_autoIncrement;
    bool m_notNull;
    bool m_isDefaultValue;
    uint32_t m_defaultValue;
    int64_t m_autoIncrementValue;
    uint32_t m_bufsize;
    uint32_t m_fieldIndex;
    std::string m_comment;
};

class _NFExport NFProtobufCommon : public NFSingleton<NFProtobufCommon>
{
public:
    static std::string GetFieldsString(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor *pFieldDesc);

    static std::string GetRepeatedFieldsString(const google::protobuf::Message &message, const google::protobuf::FieldDescriptor *pFieldDesc, int index);

    static bool SetFieldsString(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFieldDesc, const std::string& strValue);

    static bool AddFieldsString(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFieldDesc, const std::string& strValue);

    /*
    ** 通过在Protobuf里的message名字创建出一个默认的此类的变量
    */
    static const ::google::protobuf::Message *FindMessageTypeByName(const std::string &full_name);

    /*
    ** 通过在Protobuf里的message名字创建出一个Message
    */
    static ::google::protobuf::Message *CreateMessageByName(const std::string &full_name);

    static void GetDBMapFieldsFromMessage(const google::protobuf::Message &message, std::map<std::string, std::string> &mapFields, const std::string& lastFieldName = "");

    static void GetDBFieldsFromDesc(const google::protobuf::Descriptor *pDesc, std::vector<std::string> &vecFields, const std::string& lastFieldName = "");

    static int GetPrivateFieldsFromMessage(const google::protobuf::Message &message, std::string& field, std::string& fieldValue);

    static int GetPrivateKeyFromMessage(const google::protobuf::Descriptor *pDesc, std::string& field);

    static int GetMacroTypeFromMessage(const google::protobuf::Descriptor *pDesc, const std::string& base, std::unordered_map<std::string, std::string>& fieldMap);

    static void GetMapDBFieldsFromMessage(const google::protobuf::Message &message, std::map<std::string, std::string> &keyMap, std::map<std::string, std::string> &kevValueMap, const std::string& lastFieldName = "");

    static void GetDBFieldsFromMessage(const google::protobuf::Message &message, std::vector<std::pair<std::string, std::string>> &kevValueList, const std::string& lastFieldName = "");

    static void GetDBMessageFromMapFields(const std::map<std::string, std::string> &result, google::protobuf::Message *pMessageObject, const std::string& lastFieldName = "", bool* pbAllEmpty = NULL);

    static int GetMessageFromMapFields(const std::unordered_map<std::string, std::string>& result, google::protobuf::Message* pMessageObject, const std::string& lastFieldName = "", bool* pbAllEmpty = NULL);

    static int MessageToLogStr(std::string &msg, const google::protobuf::Message &message, const std::string& split = "|", const std::string& repeated_split = ";", const std::string& repeated_field_split = ",", bool repeated = false);

    static bool ProtoMessageToXml(const google::protobuf::Message &message,
                                  std::string *json);

    static bool XmlToProtoMessage(const std::string &json,
                                  google::protobuf::Message *message);

    static bool ProtoMessageToJson(const google::protobuf::Message &message,
                                   std::string *json,
                                   std::string *error = NULL);

    static bool ProtoMessageToJson(const NFJson2PB::Pb2JsonOptions& options, const google::protobuf::Message &message,
                                   std::string *json,
                                   std::string *error = NULL);

    static bool ProtoMessageToJson(const google::protobuf::Message &message,
                                   google::protobuf::io::ZeroCopyOutputStream *json,
                                   std::string *error = NULL);

    static bool JsonToProtoMessage(const std::string &json,
                                   google::protobuf::Message *message,
                                   std::string *error = NULL);

    static bool CheckJsonToProtoMessage(const std::string& json_string, google::protobuf::Message* message, bool repeated);

    static bool CheckJsonValueToProtoField(const std::string& json_string, const std::string& fieldName, google::protobuf::FieldDescriptor::CppType fieldType, bool isRepeated);

    static bool JsonToProtoField(const std::string& json, google::protobuf::Message* pMessage, const google::protobuf::FieldDescriptor* pFiledDesc);

    static bool JsonToProtoMessage(google::protobuf::io::ZeroCopyInputStream *stream,
                                   google::protobuf::Message *message,
                                   std::string *error = NULL);

    static bool LuaToProtoMessage(NFLuaRef luaRef, google::protobuf::Message *pMessageObject);

    static int CopyMessageByFields(google::protobuf::Message *pSrcMessage, const google::protobuf::Message *pDescMessage);

    static int GetMessageFromGetHttp(google::protobuf::Message *pSrcMessage, const NFIHttpHandle &req);

    /**
     * @brief 通过message的protobuf反射， 查找出数据库表的列信息
     * @param message
     * @param privaryKeyMap
     * @param IndexMap
     * @param mapFileds
     */
    static int GetDbFieldsInfoFromMessage(const google::protobuf::Descriptor *pDesc, std::map<std::string, DBTableColInfo> &primaryKeyMap, std::vector<std::pair<std::string, DBTableColInfo>> &mapFileds, const std::string& lastFieldName = "", const std::string& lastComment = "");

    static uint32_t GetPBDataTypeFromDBDataType(const std::string& dbDataType, const std::string& strColumnType);
    static std::string GetDBDataTypeFromPBDataType(uint32_t pbDataType, uint32_t textMax);

    static std::string GetProtoBaseName(const google::protobuf::Message& message);
    static std::string GetProtoPackageName(const google::protobuf::Message& message);
    static std::string GetProtoBaseName(const std::string& message);
    static std::string GetProtoPackageName(const std::string& message);

    static std::string GetDescStoreClsName(const google::protobuf::Message& message);
public:
    static int GetPrimarykeyFromMessage(const google::protobuf::Message* pMessage, std::string &result);

    static int SetPrimarykeyFromMessage(google::protobuf::Message* pMessage, const std::string& data);
public:
    ////////////////////////////////////////store pb//////////////////////////////////////////////////////////////////

public:
    NFProtobufCommon();

    virtual ~NFProtobufCommon();

public:
    /**
     * @brief 加载pb文件 protoc 通过--descriptor_set_out生存的文件
     * @param ds
     * @return
     */
    int LoadProtoDsFile(const std::string &ds);

    /**
     * @brief
     * @param full_name
     * @return
     */
    const google::protobuf::Descriptor *FindDynamicMessageTypeByName(const std::string &full_name);

    /*
    ** 通过在Protobuf里的message名字创建出一个Message
    */
    ::google::protobuf::Message *CreateDynamicMessageByName(const std::string &full_name);

    static FieldParseType GetFieldPasreType(const google::protobuf::FieldDescriptor *pFieldDesc);

    int GetFieldsDBMaxCount(const google::protobuf::FieldDescriptor *pFieldDesc) const;

    int GetFieldsDBMaxSize(const google::protobuf::FieldDescriptor *pFieldDesc) const;

    int GetFieldsMaxCount(const google::protobuf::FieldDescriptor *pFieldDesc) const;

    int GetFieldsMaxSize(const google::protobuf::FieldDescriptor *pFieldDesc) const;

    const ::google::protobuf::EnumValueDescriptor* FindEnumValueByName(const string& name) const;

    const ::google::protobuf::EnumDescriptor* FindEnumTypeByName(const string& name) const;

    bool FindEnumNumberByMacroName(const string& enumName, const std::string& macroName, std::string& value);
public:
    std::string m_fileMd5;
    google::protobuf::DescriptorPool *m_pDescriptorPool;
    std::vector<google::protobuf::DescriptorPool *> m_pOldPoolVec;
    google::protobuf::DynamicMessageFactory *m_pDynamicMessageFactory;
public:
	struct EnumMacroData
	{
		std::unordered_map<std::string, int> m_enumNameToNumber;
		std::unordered_map<std::string, int> m_enumToNumber;
		std::unordered_map<int, std::string> m_numberToEnumName;
	};
    std::unordered_map<std::string, EnumMacroData> m_enumMacroData;
};


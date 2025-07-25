// -------------------------------------------------------------------------
//    @FileName         :    NFProtobufCommon.cpp
//    @Author           :    Gao.Yi
//    @Date             :   2022-09-18
//    @Email			:    445267987@qq.com
//    @Module           :    NFProtobufCommon
//
// -------------------------------------------------------------------------

#include <fstream>
#include "NFProtobufCommon.h"

#include <NFComm/NFCore/NFFileUtility.h>

#include "NFLogMgr.h"
#include "NFCheck.h"
#include "../../../../game/LieRen/NFLogicComm/NFServerLogicMessage/proto_svr_common.pb.h"
#include "NFComm/NFPluginModule/NFJson2PB/NFPbToJson.h"
#include "NFComm/NFPluginModule/NFJson2PB/NFJsonToPb.h"
#include "NFComm/NFCore/NFStringUtility.h"
#include "NFComm/NFPluginModule/NFProto/NFXmlMessageCodec.h"
#include "NFComm/NFCore/NFCommon.h"
#include "NFComm/NFCore/NFLock.h"

#ifdef GetMessage
#undef GetMessage
#endif

std::string NFProtobufCommon::GetRepeatedFieldsString(const google::protobuf::Message& message,
                                                      const google::protobuf::FieldDescriptor* pFieldDesc, int index)
{
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pReflect == nullptr || pFieldDesc == nullptr) return std::string();
    if (pFieldDesc->is_repeated() == false) return std::string();

    switch (pFieldDesc->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            int32_t value = pReflect->GetRepeatedInt32(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            int64_t value = pReflect->GetRepeatedInt64(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            uint32_t value = pReflect->GetRepeatedUInt32(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            uint64_t value = pReflect->GetRepeatedUInt64(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            double value = pReflect->GetRepeatedDouble(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            float value = pReflect->GetRepeatedFloat(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            bool value = pReflect->GetRepeatedBool(message, pFieldDesc, index);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            const google::protobuf::EnumValueDescriptor* pEnumDesc = pReflect->GetRepeatedEnum(message, pFieldDesc,
                                                                                               index);
            if (pEnumDesc)
            {
                return NFCommon::tostr(pEnumDesc->number());
            }
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            std::string value = pReflect->GetRepeatedString(message, pFieldDesc, index);
            return value;
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            const google::protobuf::Message& value = pReflect->GetRepeatedMessage(message, pFieldDesc, index);
            std::string msg;
            value.SerializePartialToString(&msg);
            return msg;
        }
        break;
        default:
            break;
    }
    return std::string();
}

int NFProtobufCommon::CopyMessageByFields(google::protobuf::Message* pSrcMessage, const google::protobuf::Message* pDescMessage)
{
    if (!pSrcMessage || !pDescMessage) return -1;

    const google::protobuf::Descriptor* pDestDesc = pDescMessage->GetDescriptor();
    const google::protobuf::Reflection* pDestReflect = pDescMessage->GetReflection();
    if (pDestDesc == NULL || pDestReflect == NULL) return -1;

    const google::protobuf::Descriptor* pSrcDesc = pSrcMessage->GetDescriptor();
    const google::protobuf::Reflection* pSrcReflect = pSrcMessage->GetReflection();
    if (pSrcDesc == NULL || pSrcReflect == NULL) return -1;

    for (int j = 0; j < pSrcDesc->field_count(); j++)
    {
        const google::protobuf::FieldDescriptor* pSrcFieldDesc = pSrcDesc->field(j);
        if (pSrcFieldDesc == NULL) continue;

        const google::protobuf::FieldDescriptor* pDescFieldDesc = pDestDesc->FindFieldByLowercaseName(pSrcFieldDesc->lowercase_name());
        if (pDescFieldDesc == NULL) continue;

        if (pSrcFieldDesc->cpp_type() == pDescFieldDesc->cpp_type())
        {
            if (!pSrcFieldDesc->is_repeated())
            {
                switch (pSrcFieldDesc->cpp_type())
                {
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    {
                        pSrcReflect->SetInt32(pSrcMessage, pSrcFieldDesc, pDestReflect->GetInt32(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    {
                        pSrcReflect->SetInt64(pSrcMessage, pSrcFieldDesc, pDestReflect->GetInt64(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    {
                        pSrcReflect->SetUInt32(pSrcMessage, pSrcFieldDesc, pDestReflect->GetUInt32(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    {
                        pSrcReflect->SetUInt64(pSrcMessage, pSrcFieldDesc, pDestReflect->GetUInt64(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    {
                        pSrcReflect->SetDouble(pSrcMessage, pSrcFieldDesc, pDestReflect->GetDouble(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    {
                        pSrcReflect->SetFloat(pSrcMessage, pSrcFieldDesc, pDestReflect->GetFloat(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    {
                        pSrcReflect->SetBool(pSrcMessage, pSrcFieldDesc, pDestReflect->GetBool(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    {
                        pSrcReflect->SetEnum(pSrcMessage, pSrcFieldDesc, pDestReflect->GetEnum(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    {
                        pSrcReflect->SetString(pSrcMessage, pSrcFieldDesc, pDestReflect->GetString(*pDescMessage, pDescFieldDesc));
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    {
                        const google::protobuf::Message& destFieldMessage = pDestReflect->GetMessage(*pDescMessage, pDescFieldDesc);
                        google::protobuf::Message* pMutableMessage = pSrcReflect->MutableMessage(pSrcMessage, pSrcFieldDesc);
                        if (pMutableMessage)
                        {
                            CopyMessageByFields(pMutableMessage, &destFieldMessage);
                        }
                    }
                    break;
                    default:
                        break;
                }
            }
            else
            {
                switch (pSrcFieldDesc->cpp_type())
                {
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddInt32(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedInt32(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddInt64(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedInt64(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddUInt32(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedUInt32(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddUInt64(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedUInt64(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddDouble(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedDouble(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddFloat(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedFloat(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddBool(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedBool(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddEnum(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedEnum(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            pSrcReflect->AddString(pSrcMessage, pSrcFieldDesc, pDestReflect->GetRepeatedString(*pDescMessage, pDescFieldDesc, i));
                        }
                    }
                    break;
                    case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                    {
                        for (int i = 0; i < (int)pDestReflect->FieldSize(*pDescMessage, pDescFieldDesc); i++)
                        {
                            const google::protobuf::Message& destFieldMessage = pDestReflect->GetRepeatedMessage(*pDescMessage, pDescFieldDesc, i);
                            google::protobuf::Message* pMutableMessage = pSrcReflect->AddMessage(pSrcMessage, pSrcFieldDesc);
                            if (pMutableMessage)
                            {
                                CopyMessageByFields(pMutableMessage, &destFieldMessage);
                            }
                        }
                    }
                    break;
                    default:
                        break;
                }
            }
        }
    }

    return 0;
}

std::string NFProtobufCommon::GetFieldsString(const google::protobuf::Message& message,
                                              const google::protobuf::FieldDescriptor* pFieldDesc)
{
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pReflect == nullptr || pFieldDesc == nullptr) return std::string();
    if (pFieldDesc->is_repeated()) return std::string();

    switch (pFieldDesc->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            int32_t value = pReflect->GetInt32(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            int64_t value = pReflect->GetInt64(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            uint32_t value = pReflect->GetUInt32(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            uint64_t value = pReflect->GetUInt64(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            double value = pReflect->GetDouble(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            float value = pReflect->GetFloat(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            bool value = pReflect->GetBool(message, pFieldDesc);
            return NFCommon::tostr(value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            const google::protobuf::EnumValueDescriptor* pEnumDesc = pReflect->GetEnum(message, pFieldDesc);
            if (pEnumDesc)
            {
                return NFCommon::tostr(pEnumDesc->number());
            }
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            std::string value = pReflect->GetString(message, pFieldDesc);
            return value;
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            const google::protobuf::Message& value = pReflect->GetMessage(message, pFieldDesc);
            std::string msg;
            value.SerializePartialToString(&msg);
            return msg;
        }
        break;
        default:
            break;
    }
    return std::string();
}

bool NFProtobufCommon::SetFieldsString(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFieldDesc,
                                       const std::string& strValue)
{
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pReflect == nullptr || pFieldDesc == nullptr) return false;
    if (pFieldDesc->is_repeated()) return false;

    switch (pFieldDesc->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            int32_t value = NFCommon::strto<int32_t>(strValue);
            pReflect->SetInt32(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            int64_t value = NFCommon::strto<int64_t>(strValue);
            pReflect->SetInt64(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            uint32_t value = NFCommon::strto<uint32_t>(strValue);
            pReflect->SetUInt32(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            uint64_t value = NFCommon::strto<uint64_t>(strValue);
            pReflect->SetUInt64(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            double value = NFCommon::strto<double>(strValue);
            pReflect->SetDouble(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            float value = NFCommon::strto<float>(strValue);
            pReflect->SetFloat(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            bool value = (bool)NFCommon::strto<bool>(strValue);
            pReflect->SetBool(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            const google::protobuf::EnumDescriptor* pEnumDesc = pFieldDesc->enum_type();
            if (pEnumDesc == nullptr) return false;

            std::string numberValue = strValue;
            if (!NFStringUtility::IsDigital(strValue))
            {
                if (!NFProtobufCommon::Instance()->FindEnumNumberByMacroName(pEnumDesc->full_name(), strValue, numberValue))
                {
                    return false;
                }
            }

            int value = NFCommon::strto<int>(numberValue);
            const google::protobuf::EnumValueDescriptor* pEnumValueDesc = pEnumDesc->FindValueByNumber(value);
            if (pEnumValueDesc == nullptr) return false;

            pReflect->SetEnum(&message, pFieldDesc, pEnumValueDesc);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            pReflect->SetString(&message, pFieldDesc, strValue);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            google::protobuf::Message* pMutableMessage = pReflect->MutableMessage(&message, pFieldDesc);
            if (pMutableMessage == nullptr) return false;

            return pMutableMessage->ParsePartialFromString(strValue);
        }
        break;
        default:
            break;
    }
    return true;
}

bool NFProtobufCommon::AddFieldsString(google::protobuf::Message& message, const google::protobuf::FieldDescriptor* pFieldDesc,
                                       const std::string& strValue)
{
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pReflect == nullptr || pFieldDesc == nullptr) return false;
    if (pFieldDesc->is_repeated() == false) return false;

    switch (pFieldDesc->cpp_type())
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            int32_t value = NFCommon::strto<int32_t>(strValue);
            pReflect->AddInt32(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            int64_t value = NFCommon::strto<int64_t>(strValue);
            pReflect->AddInt64(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            uint32_t value = NFCommon::strto<uint32_t>(strValue);
            pReflect->AddUInt32(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            uint64_t value = NFCommon::strto<uint64_t>(strValue);
            pReflect->AddUInt64(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            double value = NFCommon::strto<double>(strValue);
            pReflect->AddDouble(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            float value = NFCommon::strto<float>(strValue);
            pReflect->AddFloat(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            bool value = (bool)NFCommon::strto<bool>(strValue);
            pReflect->AddBool(&message, pFieldDesc, value);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            const google::protobuf::EnumDescriptor* pEnumDesc = pFieldDesc->enum_type();
            if (pEnumDesc == nullptr) return false;

            std::string numberValue = strValue;
            if (!NFStringUtility::IsDigital(strValue))
            {
                if (!NFProtobufCommon::Instance()->FindEnumNumberByMacroName(pEnumDesc->full_name(), strValue, numberValue))
                {
                    return false;
                }
            }

            int value = NFCommon::strto<int>(numberValue);
            const google::protobuf::EnumValueDescriptor* pEnumValueDesc = pEnumDesc->FindValueByNumber(value);
            if (pEnumValueDesc == nullptr) return false;

            pReflect->AddEnum(&message, pFieldDesc, pEnumValueDesc);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            pReflect->AddString(&message, pFieldDesc, strValue);
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            google::protobuf::Message* pMutableMessage = pReflect->AddMessage(&message, pFieldDesc);
            if (pMutableMessage == nullptr) return false;

            return pMutableMessage->ParsePartialFromString(strValue);
        }
        break;
        default:
            break;
    }
    return true;
}

const ::google::protobuf::Message* NFProtobufCommon::FindMessageTypeByName(const std::string& full_name)
{
    const google::protobuf::Descriptor* pDescriptor = ::google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(
        full_name);
    if (pDescriptor)
    {
        return ::google::protobuf::MessageFactory::generated_factory()->GetPrototype(pDescriptor);
    }
    return NULL;
}

::google::protobuf::Message* NFProtobufCommon::CreateMessageByName(const std::string& full_name)
{
    const ::google::protobuf::Message* pMessageType = FindMessageTypeByName(full_name);
    if (pMessageType)
    {
        return pMessageType->New();
    }
    return NULL;
}

int NFProtobufCommon::MessageToLogStr(std::string& msg, const google::protobuf::Message& message, const std::string& split, const std::string& repeated_split, const std::string& repeated_field_split, bool repeated)
{
    const google::protobuf::Descriptor* pDesc = message.GetDescriptor();
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pDesc == NULL || pReflect == NULL) return -1;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->is_repeated())
        {
            /**
             * 不处理repeated 的message 里的repeated， 只处理一层repeated
             */
            if (repeated) continue;

            msg += split;

            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                //const google::protobuf::FieldOptions &fieldoptions = pFieldDesc->options();
                ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                for (::google::protobuf::int32 a_i = 0; a_i < repeatedSize; a_i++)
                {
                    if (a_i != repeatedSize - 1)
                    {
                        msg += GetRepeatedFieldsString(message, pFieldDesc, a_i) + repeated_split;
                    }
                    else
                    {
                        msg += GetRepeatedFieldsString(message, pFieldDesc, a_i);
                    }
                }
            }
            else
            {
                //const google::protobuf::FieldOptions &fieldoptions = pFieldDesc->options();
                ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                for (::google::protobuf::int32 a_i = 0; a_i < repeatedSize; a_i++)
                {
                    const ::google::protobuf::Message& subMessageObject = pReflect->GetRepeatedMessage(message,
                                                                                                       pFieldDesc,
                                                                                                       a_i);
                    if (a_i != repeatedSize - 1)
                    {
                        MessageToLogStr(msg, subMessageObject, repeated_field_split, "&", "&&", true);
                        msg += repeated_split;
                    }
                    else
                    {
                        MessageToLogStr(msg, subMessageObject, repeated_field_split, "&", "&&", true);
                    }
                }
            }
        }
        else
        {
            if (pFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                const google::protobuf::Message& subMessage = pReflect->GetMessage(message, pFieldDesc);
                MessageToLogStr(msg, subMessage, split, repeated_split, repeated_field_split, repeated);
            }
            else
            {
                if (repeated)
                {
                    if (i != pDesc->field_count() - 1)
                    {
                        msg += GetFieldsString(message, pFieldDesc) + split;
                    }
                    else
                    {
                        msg += GetFieldsString(message, pFieldDesc);
                    }
                }
                else
                {
                    msg += split + GetFieldsString(message, pFieldDesc);
                }
            }
        }
    }
    return 0;
}

void NFProtobufCommon::GetDBFieldsFromDesc(const google::protobuf::Descriptor* pDesc, std::vector<std::string>& vecFields, const std::string& lastFieldName)
{
    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->options().GetExtension(nanopb).db_no_field()) continue;

        if (pFieldDesc->is_repeated())
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                    {
                        std::string field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);
                        vecFields.push_back(field);
                    }
                }
            }
            else
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                    {
                        const google::protobuf::Descriptor* pSubDesc = pFieldDesc->options().GetDescriptor();
                        if (pSubDesc == NULL) continue;

                        GetDBFieldsFromDesc(pSubDesc, vecFields, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_");
                    }
                }
            }
        }
        else
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                vecFields.push_back(lastFieldName + pFieldDesc->name());
            }
            else
            {
                if (pFieldDesc->options().GetExtension(nanopb).db_message_expand())
                {
                    const google::protobuf::Descriptor* pSubDesc = pFieldDesc->options().GetDescriptor();
                    if (pSubDesc == NULL) continue;
                    GetDBFieldsFromDesc(pSubDesc, vecFields, lastFieldName + pFieldDesc->name() + "_");
                }
                else
                {
                    vecFields.push_back(lastFieldName + pFieldDesc->name());
                }
            }
        }
    }
}

void NFProtobufCommon::GetDBMapFieldsFromMessage(const google::protobuf::Message& message,
                                                 std::map<std::string, std::string>& mapFields, const std::string& lastFieldName)
{
    const google::protobuf::Descriptor* pDesc = message.GetDescriptor();
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pDesc == NULL || pReflect == NULL) return;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->options().GetExtension(nanopb).db_no_field()) continue;
        if (!pFieldDesc->is_repeated() && pReflect->HasField(message, pFieldDesc) == false) continue;
        if (pFieldDesc->is_repeated() && pReflect->FieldSize(message, pFieldDesc) == 0) continue;

        if (pFieldDesc->is_repeated())
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize && a_i < repeatedSize; a_i++)
                    {
                        std::string field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);
                        mapFields.emplace(field, GetRepeatedFieldsString(message, pFieldDesc, a_i));
                    }
                }
            }
            else
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize && a_i < repeatedSize; a_i++)
                    {
                        const ::google::protobuf::Message& pSubMessageObject = pReflect->GetRepeatedMessage(message, pFieldDesc, a_i);
                        GetDBMapFieldsFromMessage(pSubMessageObject, mapFields, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_");
                    }
                }
            }
        }
        else
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                mapFields.emplace(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc));
            }
            else
            {
                if (pFieldDesc->options().GetExtension(nanopb).db_message_expand())
                {
                    const ::google::protobuf::Message& pSubMessageObject = pReflect->GetMessage(message, pFieldDesc);
                    GetDBMapFieldsFromMessage(pSubMessageObject, mapFields, lastFieldName + pFieldDesc->name() + "_");
                }
                else
                {
                    mapFields.emplace(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc));
                }
            }
        }
    }
}

int NFProtobufCommon::GetPrivateKeyFromMessage(const google::protobuf::Descriptor* pDesc, std::string& field)
{
    if (pDesc == NULL) return -1;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->options().GetExtension(nanopb).db_type() != E_FIELD_TYPE_PRIMARYKEY)
        {
            continue;
        }

        field = pFieldDesc->name();
        return 0;
    }

    return 1;
}

int NFProtobufCommon::GetMacroTypeFromMessage(const google::protobuf::Descriptor* pDesc, const std::string& base, std::unordered_map<std::string, std::string>& fieldMap)
{
    if (pDesc == NULL) return -1;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (!pFieldDesc->options().GetExtension(nanopb).macro_type().empty())
        {
            std::string strName = base;
            if (!strName.empty())
            {
                strName += "_";
            }
            strName += pFieldDesc->name();
            if (pFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                auto pNewDesc = pFieldDesc->message_type();
                GetMacroTypeFromMessage(pNewDesc, strName, fieldMap);
            }
            else
            {
                fieldMap.emplace(strName, pFieldDesc->options().GetExtension(nanopb).macro_type());
            }
        }
        else
        {
            if (pFieldDesc->type() == google::protobuf::FieldDescriptor::Type::TYPE_ENUM && pFieldDesc->enum_type())
            {
                std::string strName = base;
                if (!strName.empty())
                {
                    strName += "_";
                }
                strName += pFieldDesc->name();
                fieldMap.emplace(strName, pFieldDesc->enum_type()->full_name());
            }
        }
    }

    return 0;
}

int NFProtobufCommon::GetPrivateFieldsFromMessage(const google::protobuf::Message& message, std::string& field, std::string& fieldValue)
{
    const google::protobuf::Descriptor* pDesc = message.GetDescriptor();
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pDesc == NULL || pReflect == NULL) return -1;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->options().GetExtension(nanopb).db_no_field()) continue;
        if (!pFieldDesc->is_repeated() && pReflect->HasField(message, pFieldDesc) == false) continue;
        if (pFieldDesc->is_repeated() && pReflect->FieldSize(message, pFieldDesc) == 0) continue;

        if (pFieldDesc->is_repeated()) continue;
        if (pFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE) continue;

        if (pFieldDesc->options().GetExtension(nanopb).db_type() != E_FIELD_TYPE_PRIMARYKEY)
        {
            continue;
        }

        field = pFieldDesc->name();
        fieldValue = GetFieldsString(message, pFieldDesc);
        return 0;
    }

    return 1;
}

void NFProtobufCommon::GetMapDBFieldsFromMessage(const google::protobuf::Message& message,
                                                 std::map<std::string, std::string>& keyMap, std::map<std::string, std::string>& kevValueMap, const std::string& lastFieldName)
{
    const google::protobuf::Descriptor* pDesc = message.GetDescriptor();
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pDesc == NULL || pReflect == NULL) return;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (!pFieldDesc->is_repeated() && pReflect->HasField(message, pFieldDesc) == false) continue;
        if (pFieldDesc->is_repeated() && pReflect->FieldSize(message, pFieldDesc) == 0) continue;

        if (pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_PRIMARYKEY ||
            pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_UNIQUE_INDEX)
        {
            keyMap.emplace(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc));
        }

        if (pFieldDesc->is_repeated())
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize && a_i < repeatedSize; a_i++)
                    {
                        std::string field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);
                        kevValueMap.emplace(field, GetRepeatedFieldsString(message, pFieldDesc, a_i));
                    }
                }
            }
            else
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize && a_i < repeatedSize; a_i++)
                    {
                        const ::google::protobuf::Message& pSubMessageObject = pReflect->GetRepeatedMessage(message, pFieldDesc, a_i);

                        GetMapDBFieldsFromMessage(pSubMessageObject, keyMap, kevValueMap, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_");
                    }
                }
            }
        }
        else
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                kevValueMap.emplace(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc));
            }
            else
            {
                if (pFieldDesc->options().GetExtension(nanopb).db_message_expand())
                {
                    const ::google::protobuf::Message& pSubMessageObject = pReflect->GetMessage(message, pFieldDesc);

                    GetMapDBFieldsFromMessage(pSubMessageObject, keyMap, kevValueMap, lastFieldName + pFieldDesc->name() + "_");
                }
                else
                {
                    kevValueMap.emplace(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc));
                }
            }
        }
    }
}

void NFProtobufCommon::GetDBFieldsFromMessage(const google::protobuf::Message& message, std::vector<std::pair<std::string, std::string>>& kevValueList, const std::string& lastFieldName)
{
    const google::protobuf::Descriptor* pDesc = message.GetDescriptor();
    const google::protobuf::Reflection* pReflect = message.GetReflection();
    if (pDesc == NULL || pReflect == NULL) return;

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (!pFieldDesc->is_repeated() && pReflect->HasField(message, pFieldDesc) == false) continue;
        if (pFieldDesc->is_repeated() && pReflect->FieldSize(message, pFieldDesc) == 0) continue;

        if (pFieldDesc->is_repeated())
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize && a_i < repeatedSize; a_i++)
                    {
                        std::string field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);
                        kevValueList.push_back(std::make_pair(field, GetRepeatedFieldsString(message, pFieldDesc, a_i)));
                    }
                }
            }
            else
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    ::google::protobuf::int32 repeatedSize = pReflect->FieldSize(message, pFieldDesc);
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize && a_i < repeatedSize; a_i++)
                    {
                        const ::google::protobuf::Message& pSubMessageObject = pReflect->GetRepeatedMessage(message, pFieldDesc, a_i);
                        GetDBFieldsFromMessage(pSubMessageObject, kevValueList, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_");
                    }
                }
            }
        }
        else
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                kevValueList.push_back(std::make_pair(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc)));
            }
            else
            {
                if (pFieldDesc->options().GetExtension(nanopb).db_message_expand())
                {
                    const ::google::protobuf::Message& pSubMessageObject = pReflect->GetMessage(message, pFieldDesc);
                    GetDBFieldsFromMessage(pSubMessageObject, kevValueList, lastFieldName + pFieldDesc->name() + "_");
                }
                else
                {
                    kevValueList.push_back(std::make_pair(lastFieldName + pFieldDesc->name(), GetFieldsString(message, pFieldDesc)));
                }
            }
        }
    }
}

void NFProtobufCommon::GetDBMessageFromMapFields(const std::map<std::string, std::string>& result, google::protobuf::Message* pMessageObject, const std::string& lastFieldName, bool* pbAllEmpty)
{
    if (pMessageObject == NULL) return;
    /* message AttrValue
       {
           optional int32 attr = 1 [(nanopb).field_cname = "attr"];
           optional int32 value = 2 [(nanopb).field_cname = "value"];
           optional string value2 = 3 [(nanopb).field_cname = "value2", (yd_fieldoptions.db_field_bufsize)=128];
       }

       message FishConfigDesc
       {
           optional int32 fish_id = 1 [(nanopb).field_cname = "鱼id", (yd_fieldoptions.db_field_type) = E_FIELD_TYPE_PRIMARYKEY];
           optional int32 fish_type = 2 [(nanopb).field_cname = "鱼的玩法类型"];
           optional int32 build_fish_type = 3 [(nanopb).field_cname = "客户端创建鱼类型"];
           optional int32 ratio_max = 4  [(nanopb).field_cname = "倍率最大值"];
           optional int32 double_award_min_ratio = 5  [(nanopb).field_cname = "可能触发双倍奖励所需最低倍率"];
           optional int32 child_fish_count = 6  [(nanopb).field_cname = "组合鱼携带子鱼个数"];
           repeated string child_fish_ids = 7  [(nanopb).field_cname = "组合鱼位置可选子鱼id列表", (nanopb).max_size=128, (yd_fieldoptions.db_field_bufsize)=128, (nanopb).max_count = 6];
           repeated AttrValue attr_values = 8 [(nanopb).field_cname = "attr_values", (nanopb).max_count=2];
       }

       如上，将一个result结果， 转化为protobuf的message, 比如FishConfigDesc, 代表这数据库FishConfigDesc或Excel表格中的一列
    */
    const google::protobuf::Descriptor* pMessageObjectFieldDesc = pMessageObject->GetDescriptor();
    const google::protobuf::Reflection* pMessageObjectReflect = pMessageObject->GetReflection();
    if (pMessageObjectFieldDesc == NULL || pMessageObjectReflect == NULL) return;

    for (int i = 0; i < pMessageObjectFieldDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pMessageObjectFieldDesc->field(i);
        if (pFieldDesc == NULL) continue;
        //如果不是repeated, 只是简单信息，就直接给
        if (pFieldDesc->is_repeated() == false)
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                std::string field = lastFieldName + pFieldDesc->name();
                auto iter = result.find(field);
                if (iter != result.end())
                {
                    if (pbAllEmpty && !iter->second.empty())
                    {
                        *pbAllEmpty = false;
                    }
                    NFProtobufCommon::SetFieldsString(*pMessageObject, pFieldDesc, iter->second);
                }
            }
            else
            {
                if (pFieldDesc->options().GetExtension(nanopb).db_message_expand())
                {
                    ::google::protobuf::Message* pSubMessageObject = pMessageObjectReflect->MutableMessage(pMessageObject, pFieldDesc);
                    if (pSubMessageObject == NULL) continue;

                    bool allSubMessageeEmpty = true;
                    GetDBMessageFromMapFields(result, pSubMessageObject, lastFieldName + pFieldDesc->name() + "_", &allSubMessageeEmpty);
                    if (pbAllEmpty && allSubMessageeEmpty)
                    {
                        *pbAllEmpty = false;
                    }
                }
                else
                {
                    std::string field = lastFieldName + pFieldDesc->name();
                    auto iter = result.find(field);
                    if (iter != result.end())
                    {
                        if (pbAllEmpty && !iter->second.empty())
                        {
                            *pbAllEmpty = false;
                        }
                        NFProtobufCommon::SetFieldsString(*pMessageObject, pFieldDesc, iter->second);
                    }
                }
            }
        }
        else
        {
            //如果只是简单的repeated, 比如:repeated string child_fish_ids = 7
            //把数据库里的多行，搞成数组的形式
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    std::vector<std::string> vecValue;
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                    {
                        std::string field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);

                        auto iter = result.find(field);
                        if (iter != result.end())
                        {
                            if (pbAllEmpty && !iter->second.empty())
                            {
                                *pbAllEmpty = false;
                            }
                            vecValue.push_back(iter->second);
                        }
                    }

                    int vecValueLastIndex = vecValue.size();
                    for (int vec_i = (int)vecValue.size() - 1; vec_i >= 0; vec_i--)
                    {
                        if (!vecValue[vec_i].empty()) break;
                        vecValueLastIndex = vec_i;
                    }

                    for (int vec_i = 0; vec_i < vecValueLastIndex; vec_i++)
                    {
                        NFProtobufCommon::AddFieldsString(*pMessageObject, pFieldDesc, vecValue[vec_i]);
                    }
                }
            }
            else
            {
                //如果只是复杂的repeated, 比如:
                //message AttrValue
                //{
                //	optional int32 attr = 1 [(nanopb).field_cname = "attr"];
                //	optional int32 value = 2 [(nanopb).field_cname = "value"];
                //	optional string value2 = 3 [(nanopb).field_cname = "value2", (yd_fieldoptions.db_field_bufsize)=128];
                //}
                //repeated AttrValue attr_values = 8 [(nanopb).field_cname = "attr_values", (nanopb).max_count=2];
                //把数据库里的多行，配合结构转成repeated数组
                const google::protobuf::Descriptor* pSubDescriptor = pFieldDesc->message_type();
                if (pSubDescriptor == NULL) continue;
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    std::vector<std::pair<google::protobuf::Message*, bool>> vecValue;
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                    {
                        ::google::protobuf::Message* pSubMessageObject = pMessageObjectReflect->AddMessage(pMessageObject, pFieldDesc);
                        if (pSubMessageObject == NULL) continue;

                        bool allSubMessageeEmpty = true;
                        GetDBMessageFromMapFields(result, pSubMessageObject, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_", &allSubMessageeEmpty);
                        if (pbAllEmpty && !allSubMessageeEmpty)
                        {
                            *pbAllEmpty = false;
                        }
                        vecValue.push_back(std::make_pair(pSubMessageObject, allSubMessageeEmpty));
                    }

                    for (int vec_i = (int)vecValue.size() - 1; vec_i >= 0; vec_i--)
                    {
                        if (!vecValue[vec_i].second) break;
                        pMessageObjectReflect->RemoveLast(pMessageObject, pFieldDesc);
                    }
                }
            }
        }
    }
}

int NFProtobufCommon::GetMessageFromMapFields(const std::unordered_map<std::string, std::string>& result, google::protobuf::Message* pMessageObject, const std::string& lastFieldName, bool* pbAllEmpty)
{
    int iRetCode = 0;
    if (pMessageObject == NULL) return -1;
    /* message AttrValue
       {
           optional int32 attr = 1 [(nanopb).field_cname = "attr"];
           optional int32 value = 2 [(nanopb).field_cname = "value"];
           optional string value2 = 3 [(nanopb).field_cname = "value2", (yd_fieldoptions.db_field_bufsize)=128];
       }

       message FishConfigDesc
       {
           optional int32 fish_id = 1 [(nanopb).field_cname = "鱼id", (yd_fieldoptions.db_field_type) = E_FIELD_TYPE_PRIMARYKEY];
           optional int32 fish_type = 2 [(nanopb).field_cname = "鱼的玩法类型"];
           optional int32 build_fish_type = 3 [(nanopb).field_cname = "客户端创建鱼类型"];
           optional int32 ratio_max = 4  [(nanopb).field_cname = "倍率最大值"];
           optional int32 double_award_min_ratio = 5  [(nanopb).field_cname = "可能触发双倍奖励所需最低倍率"];
           optional int32 child_fish_count = 6  [(nanopb).field_cname = "组合鱼携带子鱼个数"];
           repeated string child_fish_ids = 7  [(nanopb).field_cname = "组合鱼位置可选子鱼id列表", (nanopb).max_size=128, (yd_fieldoptions.db_field_bufsize)=128, (nanopb).max_count = 6];
           repeated AttrValue attr_values = 8 [(nanopb).field_cname = "attr_values", (nanopb).max_count=2];
       }

       如上，将一个result结果， 转化为protobuf的message, 比如FishConfigDesc, 代表这数据库FishConfigDesc或Excel表格中的一列
    */
    const google::protobuf::Descriptor* pMessageObjectFieldDesc = pMessageObject->GetDescriptor();
    const google::protobuf::Reflection* pMessageObjectReflect = pMessageObject->GetReflection();
    if (pMessageObjectFieldDesc == NULL || pMessageObjectReflect == NULL) return -1;

    for (int i = 0; i < pMessageObjectFieldDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pMessageObjectFieldDesc->field(i);
        if (pFieldDesc == NULL) continue;
        //如果不是repeated, 只是简单信息，就直接给
        if (pFieldDesc->is_repeated() == false)
        {
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                std::string field = lastFieldName + pFieldDesc->name();
                auto iter = result.find(field);
                if (iter != result.end())
                {
                    if (pbAllEmpty && !iter->second.empty())
                    {
                        *pbAllEmpty = false;
                    }
                    if (!NFProtobufCommon::SetFieldsString(*pMessageObject, pFieldDesc, iter->second))
                    {
                        LOG_ERR(0, -1, "SetFieldsString, failed, field:{} json:{}", field, iter->second);
                        return -1;
                    }
                }
            }
            else
            {
                if (NFProtobufCommon::GetFieldPasreType(pFieldDesc) != FPT_DEFAULT)
                {
                    std::string field = lastFieldName + pFieldDesc->name();
                    auto iter = result.find(field);
                    if (iter != result.end())
                    {
                        if (pbAllEmpty && !iter->second.empty())
                        {
                            *pbAllEmpty = false;
                        }
                        if (!NFProtobufCommon::JsonToProtoField(iter->second, pMessageObject, pFieldDesc))
                        {
                            LOG_ERR(0, -1, "JsonToProtoField, failed, field:{} json:{}", field, iter->second);
                            return -1;
                        }
                    }
                    else
                    {
                        LOG_WARN(0, -1, "field can't find data, excel has no this col, field:{}", field);
                    }
                }
                else
                {
                    ::google::protobuf::Message* pSubMessageObject = pMessageObjectReflect->MutableMessage(pMessageObject, pFieldDesc);
                    if (pSubMessageObject == NULL) continue;

                    bool allSubMessageeEmpty = true;
                    iRetCode = GetMessageFromMapFields(result, pSubMessageObject, lastFieldName + pFieldDesc->name() + "_", &allSubMessageeEmpty);
                    CHECK_ERR(0, iRetCode, "GetMessageFromMapFields failed, field:{}", lastFieldName + pFieldDesc->name() + "_");
                    if (pbAllEmpty && allSubMessageeEmpty)
                    {
                        *pbAllEmpty = false;
                    }
                }
            }
        }
        else
        {
            //如果只是简单的repeated, 比如:repeated string child_fish_ids = 7
            //把数据库里的多行，搞成数组的形式
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                if (NFProtobufCommon::GetFieldPasreType(pFieldDesc) != FPT_DEFAULT)
                {
                    std::string field = lastFieldName + pFieldDesc->name();
                    auto iter = result.find(field);
                    if (iter != result.end())
                    {
                        if (pbAllEmpty && !iter->second.empty())
                        {
                            *pbAllEmpty = false;
                        }
                        if (!NFProtobufCommon::JsonToProtoField(iter->second, pMessageObject, pFieldDesc))
                        {
                            LOG_ERR(0, -1, "JsonToProtoField, failed, field:{} json:{}", field, iter->second);
                            return -1;
                        }
                    }
                    else
                    {
                        LOG_WARN(0, -1, "field can't find data, excel has no this col, field:{}", field);
                    }
                }
                else
                {
                    ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsMaxCount(pFieldDesc);
                    if (arysize > 0)
                    {
                        std::vector<std::string> vecValue;
                        for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                        {
                            std::string subField = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);

                            auto iter = result.find(subField);
                            if (iter != result.end())
                            {
                                if (pbAllEmpty && !iter->second.empty())
                                {
                                    *pbAllEmpty = false;
                                }
                                vecValue.push_back(iter->second);
                            }
                        }

                        int vecValueLastIndex = vecValue.size();
                        for (int vec_i = (int)vecValue.size() - 1; vec_i >= 0; vec_i--)
                        {
                            if (!vecValue[vec_i].empty()) break;
                            vecValueLastIndex = vec_i;
                        }

                        for (int vec_i = 0; vec_i < vecValueLastIndex; vec_i++)
                        {
                            if (!NFProtobufCommon::AddFieldsString(*pMessageObject, pFieldDesc, vecValue[vec_i]))
                            {
                                LOG_ERR(0, -1, "AddFieldsString, failed, field:{} value:{}", pFieldDesc->full_name(), vecValue[vec_i]);
                                return -1;
                            }
                        }
                    }
                }
            }
            else
            {
                if (NFProtobufCommon::GetFieldPasreType(pFieldDesc) != FPT_DEFAULT)
                {
                    std::string field = lastFieldName + pFieldDesc->name();
                    auto iter = result.find(field);
                    if (iter != result.end())
                    {
                        if (pbAllEmpty && !iter->second.empty())
                        {
                            *pbAllEmpty = false;
                        }
                        if (!NFProtobufCommon::JsonToProtoField(iter->second, pMessageObject, pFieldDesc))
                        {
                            LOG_ERR(0, -1, "JsonToProtoField, failed, field:{} json:{}", field, iter->second);
                            return -1;
                        }
                    }
                    else
                    {
                        const google::protobuf::Descriptor* pSubDescriptor = pFieldDesc->message_type();
                        if (pSubDescriptor == NULL) continue;
                        ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsMaxCount(pFieldDesc);
                        if (arysize > 0)
                        {
                            std::vector<std::pair<google::protobuf::Message*, bool>> vecValue;
                            for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                            {
                                field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);
                                std::string error;
                                iter = result.find(field);
                                if (iter != result.end())
                                {
                                    ::google::protobuf::Message* pSubMessageObject = pMessageObjectReflect->AddMessage(pMessageObject, pFieldDesc);
                                    if (pSubMessageObject == NULL) continue;

                                    if (!NFProtobufCommon::JsonToProtoMessage(iter->second, pSubMessageObject, &error) || !error.empty())
                                    {
                                        LOG_ERR(0, -1, "JsonToProtoField, failed, field:{} json:{}, error:{}", field, iter->second, error);
                                        return -1;
                                    }
                                }
                                else
                                {
                                    LOG_WARN(0, -1, "field can't find data, excel has no this col, field:{}", field);
                                }
                            }
                        }
                    }
                }
                else
                {
                    //如果只是复杂的repeated, 比如:
                    //message AttrValue
                    //{
                    //	optional int32 attr = 1 [(nanopb).field_cname = "attr"];
                    //	optional int32 value = 2 [(nanopb).field_cname = "value"];
                    //	optional string value2 = 3 [(nanopb).field_cname = "value2", (yd_fieldoptions.db_field_bufsize)=128];
                    //}
                    //repeated AttrValue attr_values = 8 [(nanopb).field_cname = "attr_values", (nanopb).max_count=2];
                    //把数据库里的多行，配合结构转成repeated数组
                    const google::protobuf::Descriptor* pSubDescriptor = pFieldDesc->message_type();
                    if (pSubDescriptor == NULL) continue;
                    ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsMaxCount(pFieldDesc);
                    if (arysize > 0)
                    {
                        std::vector<std::pair<google::protobuf::Message*, bool>> vecValue;
                        for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                        {
                            ::google::protobuf::Message* pSubMessageObject = pMessageObjectReflect->AddMessage(pMessageObject, pFieldDesc);
                            if (pSubMessageObject == NULL) continue;

                            bool allSubMessageeEmpty = true;
                            iRetCode = GetMessageFromMapFields(result, pSubMessageObject, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_", &allSubMessageeEmpty);
                            CHECK_ERR(0, iRetCode, "GetMessageFromMapFields failed, field:{}", lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_");
                            if (pbAllEmpty && !allSubMessageeEmpty)
                            {
                                *pbAllEmpty = false;
                            }
                            vecValue.push_back(std::make_pair(pSubMessageObject, allSubMessageeEmpty));
                        }

                        for (int vec_i = (int)vecValue.size() - 1; vec_i >= 0; vec_i--)
                        {
                            if (!vecValue[vec_i].second) break;
                            pMessageObjectReflect->RemoveLast(pMessageObject, pFieldDesc);
                        }
                    }
                }
            }
        }
    }
    return 0;
}

bool NFProtobufCommon::ProtoMessageToJson(const google::protobuf::Message& message,
                                          std::string* json,
                                          std::string* error)
{
    return NFJson2PB::ProtoMessageToJson(message, json, error);
}

bool NFProtobufCommon::ProtoMessageToJson(const NFJson2PB::Pb2JsonOptions& options, const google::protobuf::Message& message,
                                          std::string* json,
                                          std::string* error)
{
    return NFJson2PB::ProtoMessageToJson(message, json, options, error);
}

bool NFProtobufCommon::ProtoMessageToJson(const google::protobuf::Message& message,
                                          google::protobuf::io::ZeroCopyOutputStream* json,
                                          std::string* error)
{
    return NFJson2PB::ProtoMessageToJson(message, json, error);
}

bool NFProtobufCommon::CheckJsonToProtoMessage(const std::string& json_string, google::protobuf::Message* message, bool repeated)
{
    std::string error;
    if (!NFJson2PB::CheckJsonToProtoMessage(json_string, message, NFJson2PB::Json2PbOptions(), &error, repeated) || !error.empty())
    {
        LOG_ERR(0, -1, "json parse error, json:{} error:{}", json_string, error);
        return false;
    }
    return true;
}

bool NFProtobufCommon::CheckJsonValueToProtoField(const std::string& json_string, const std::string& fieldName, google::protobuf::FieldDescriptor::CppType fieldType, bool isRepeated)
{
    std::string error;
    if (!NFJson2PB::CheckJsonValueToProtoField(json_string, fieldName, fieldType, isRepeated, &error) || !error.empty())
    {
        LOG_ERR(0, -1, "json parse error, json:{} error:{}", json_string, error);
        return false;
    }
    return true;
}

bool NFProtobufCommon::JsonToProtoField(const std::string& json, google::protobuf::Message* pMessage, const google::protobuf::FieldDescriptor* pFiledDesc)
{
    std::string error;
    if (!NFJson2PB::JsonValueToProtoField(json, pFiledDesc, pMessage, NFJson2PB::Json2PbOptions(), &error) || !error.empty())
    {
        LOG_ERR(0, -1, "json parse error, json:{} error:{}", json, error);
        return false;
    }
    return true;
}

bool NFProtobufCommon::JsonToProtoMessage(const std::string& json,
                                          google::protobuf::Message* message,
                                          std::string* error)
{
    if (json.empty())
    {
        return true;
    }
    return NFJson2PB::JsonToProtoMessage(json, message, error);
}

bool NFProtobufCommon::JsonToProtoMessage(google::protobuf::io::ZeroCopyInputStream* stream,
                                          google::protobuf::Message* message,
                                          std::string* error)
{
    return NFJson2PB::JsonToProtoMessage(stream, message, error);
}

bool NFProtobufCommon::LuaToProtoMessage(NFLuaRef luaRef, google::protobuf::Message* pMessageObject)
{
    if (pMessageObject == NULL) return false;
    if (!luaRef.isTable()) return false;

    const google::protobuf::Descriptor* pMessageObjectFieldDesc = pMessageObject->GetDescriptor();
    const google::protobuf::Reflection* pMessageObjectReflect = pMessageObject->GetReflection();
    if (pMessageObjectFieldDesc == NULL || pMessageObjectReflect == NULL) return false;

    for (int i = 0; i < pMessageObjectFieldDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pMessageObjectFieldDesc->field(i);
        if (pFieldDesc == NULL) continue;
        const google::protobuf::Reflection* pReflect = pMessageObject->GetReflection();
        if (pReflect == nullptr || pFieldDesc == nullptr) continue;

        //如果不是repeated, 只是简单信息，就直接给
        if (pFieldDesc->is_repeated() == false)
        {
            std::string field = pFieldDesc->name();
            switch (pFieldDesc->cpp_type())
            {
                case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                {
                    int32_t value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetInt32(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                {
                    int64_t value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetInt64(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                {
                    uint32_t value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetUInt32(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                {
                    uint64_t value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetUInt64(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                {
                    double value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetDouble(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                {
                    float value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetFloat(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                {
                    bool value = false;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetBool(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                {
                    int value = 0;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        const google::protobuf::EnumDescriptor* pEnumDesc = pFieldDesc->enum_type();
                        const google::protobuf::EnumValueDescriptor* pEnumValueDesc = pEnumDesc->FindValueByNumber(
                            value);
                        pReflect->SetEnum(pMessageObject, pFieldDesc, pEnumValueDesc);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                {
                    std::string value;
                    if (luaRef.has(field) && NFILuaLoader::GetLuaTableValue(luaRef, field, value))
                    {
                        pReflect->SetString(pMessageObject, pFieldDesc, value);
                    }
                }
                break;
                case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                {
                    LuaIntf::LuaRef valueRef = luaRef[field];
                    if (valueRef.isTable())
                    {
                        google::protobuf::Message* pMutableMessage = pReflect->MutableMessage(pMessageObject, pFieldDesc);
                        LuaToProtoMessage(valueRef, pMutableMessage);
                    }
                }
            }
        }
        else
        {
            if (luaRef.isTable())
            {
                std::string field = pFieldDesc->name();
                NFLuaRef listRef;
                if (!NFILuaLoader::GetLuaTableValue(luaRef, field, listRef))
                {
                    continue;
                }

                for (int j = 1; j <= listRef.len(); j++)
                {
                    switch (pFieldDesc->cpp_type())
                    {
                        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
                        {
                            int32_t value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddInt32(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
                        {
                            int64_t value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddInt64(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
                        {
                            uint32_t value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddUInt32(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
                        {
                            uint64_t value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddUInt64(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
                        {
                            double value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddDouble(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
                        {
                            float value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddFloat(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
                        {
                            bool value = false;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddBool(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
                        {
                            int value = 0;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                const google::protobuf::EnumDescriptor* pEnumDesc = pFieldDesc->enum_type();
                                const google::protobuf::EnumValueDescriptor* pEnumValueDesc = pEnumDesc->FindValueByNumber(value);
                                pReflect->AddEnum(pMessageObject, pFieldDesc, pEnumValueDesc);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
                        {
                            std::string value;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                pReflect->AddString(pMessageObject, pFieldDesc, value);
                            }
                        }
                        break;
                        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
                        {
                            NFLuaRef value;
                            if (NFILuaLoader::GetLuaTableValue(listRef, j, value))
                            {
                                google::protobuf::Message* pSubMessage = pReflect->AddMessage(pMessageObject, pFieldDesc);
                                LuaToProtoMessage(value, pSubMessage);
                            }
                        }
                        break;
                        default:
                            break;
                    }
                }
            }
        }
    }
    return true;
}

bool
NFProtobufCommon::ProtoMessageToXml(const google::protobuf::Message& message, std::string* json)
{
    CHECK_EXPR(json, false, "json == NULL");
    NFXmlMessageCodec codec;
    std::string result;
    bool ret = codec.ToString(message, result);
    if (ret == false)
    {
        return false;
    }
    ret = codec.PrettyString(result, *json);
    if (ret == false)
    {
        return false;
    }
    return ret;
}

bool NFProtobufCommon::XmlToProtoMessage(const string& json, google::protobuf::Message* message)
{
    CHECK_EXPR(message, false, "message == NULL");
    NFXmlMessageCodec codec;
    bool ret = codec.FromString(json, *message);
    if (ret == false)
    {
        return false;
    }

    return ret;
}

int NFProtobufCommon::GetMessageFromGetHttp(google::protobuf::Message* pSrcMessage, const NFIHttpHandle& req)
{
    if (!pSrcMessage) return -1;

    const google::protobuf::Descriptor* pSrcDesc = pSrcMessage->GetDescriptor();
    if (!pSrcDesc) return -1;

    for (int i = 0; i < pSrcDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pSrcFieldDesc = pSrcDesc->field(i);
        if (pSrcFieldDesc == NULL) continue;

        std::string strValue = req.GetQuery(pSrcFieldDesc->lowercase_name());
        if (!SetFieldsString(*pSrcMessage, pSrcFieldDesc, strValue))
        {
            LOG_ERR(0, -1, "SetFieldsString failed, value:{}", strValue);
            return -1;
        }
    }

    return 0;
}

NFProtobufCommon::NFProtobufCommon()
{
    m_pDynamicMessageFactory = new google::protobuf::DynamicMessageFactory();
    m_pDescriptorPool = new google::protobuf::DescriptorPool();
}

NFProtobufCommon::~NFProtobufCommon()
{
    NF_SAFE_DELETE(m_pDynamicMessageFactory);
    NF_SAFE_DELETE(m_pDescriptorPool);
    for (int i = 0; i < (int)m_pOldPoolVec.size(); i++)
    {
        NF_SAFE_DELETE(m_pOldPoolVec[i]);
    }
    m_pOldPoolVec.clear();
}


int NFProtobufCommon::LoadProtoDsFile(const std::string& ds)
{
    std::ifstream pb_file(ds.c_str(), std::ios::binary);
    if (!pb_file.is_open())
    {
        return -1;
    }

    google::protobuf::FileDescriptorSet file_descriptor_set;
    if (!file_descriptor_set.ParseFromIstream(&pb_file))
    {
        return -1;
    }

    if (m_fileMd5.empty())
    {
        for (int i = 0; i < file_descriptor_set.file_size(); ++i)
        {
            auto pResult = m_pDescriptorPool->BuildFile(file_descriptor_set.file(i));
            CHECK_EXPR(pResult, -1, "pResult == NULL, load file:{} failed", ds);
        }

        NFFileUtility::GetFileContainMD5(ds, m_fileMd5);
    }
    else
    {
        std::string md5;
        NFFileUtility::GetFileContainMD5(ds, md5);
        if (m_fileMd5 != md5)
        {
            m_fileMd5 = md5;
            m_pOldPoolVec.push_back(m_pDescriptorPool);
            m_pDescriptorPool = new google::protobuf::DescriptorPool();
            for (int i = 0; i < file_descriptor_set.file_size(); ++i)
            {
                auto pResult = m_pDescriptorPool->BuildFile(file_descriptor_set.file(i));
                CHECK_EXPR(pResult, -1, "pResult == NULL, load file:{} failed", ds);
            }
        }
    }

    return 0;
}

const google::protobuf::Descriptor* NFProtobufCommon::FindDynamicMessageTypeByName(const std::string& full_name)
{
    auto pDesc = m_pDescriptorPool->FindMessageTypeByName(full_name);
    if (pDesc == NULL)
    {
        return google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(full_name);
    }
    return pDesc;
}

::google::protobuf::Message* NFProtobufCommon::CreateDynamicMessageByName(const std::string& full_name)
{
    const google::protobuf::Descriptor* pDescriptor = m_pDescriptorPool->FindMessageTypeByName(full_name);
    if (pDescriptor)
    {
        const ::google::protobuf::Message* pMessageType = m_pDynamicMessageFactory->GetPrototype(pDescriptor);
        if (pMessageType)
        {
            return pMessageType->New();
        }
    }
    else
    {
        pDescriptor = google::protobuf::DescriptorPool::generated_pool()->FindMessageTypeByName(full_name);
        if (pDescriptor)
        {
            const ::google::protobuf::Message* pMessageType = google::protobuf::MessageFactory::generated_factory()->GetPrototype(pDescriptor);
            if (pMessageType)
            {
                return pMessageType->New();
            }
        }
    }

    return NULL;
}

int NFProtobufCommon::GetDbFieldsInfoFromMessage(const google::protobuf::Descriptor* pDesc, std::map<std::string, DBTableColInfo>& primaryKeyMap, std::vector<std::pair<std::string, DBTableColInfo>>& mapFileds, const std::string& lastFieldName, const std::string& lastComment)
{
    CHECK_NULL(0, pDesc);

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;
        if (pFieldDesc->options().GetExtension(nanopb).db_no_field()) continue;

        //如果不是repeated, 只是简单信息，就直接给
        if (pFieldDesc->is_repeated() == false)
        {
            DBTableColInfo colInfo;
            colInfo.m_colType = pFieldDesc->cpp_type();
            colInfo.m_fieldIndex = i;
            if (pFieldDesc->options().GetExtension(nanopb).db_type() != E_FIELD_TYPE_NORMAL)
            {
                colInfo.m_notNull = true;

                if (pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_PRIMARYKEY)
                {
                    colInfo.m_primaryKey = true;
                    colInfo.m_autoIncrement = pFieldDesc->options().GetExtension(nanopb).db_auto_increment();
                    colInfo.m_autoIncrementValue = pFieldDesc->options().GetExtension(nanopb).db_auto_increment_value();
                }
                else if (pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_UNIQUE_INDEX)
                {
                    colInfo.m_unionKey = true;
                }
                else if (pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_INDEX)
                {
                    colInfo.m_indexKey = true;
                }
            }

            if (colInfo.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_STRING)
            {
                colInfo.m_bufsize = NFProtobufCommon::Instance()->GetFieldsDBMaxSize(pFieldDesc);
            }

            if (colInfo.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                if (pFieldDesc->options().GetExtension(nanopb).db_message_expand())
                {
                    const google::protobuf::Descriptor* pSubDescriptor = pFieldDesc->message_type();
                    GetDbFieldsInfoFromMessage(pSubDescriptor, primaryKeyMap, mapFileds, lastFieldName + pFieldDesc->name() + "_", lastComment + pFieldDesc->options().GetExtension(nanopb).db_comment());
                    continue;
                }
                else
                {
                    colInfo.m_bufsize = NFProtobufCommon::Instance()->GetFieldsDBMaxSize(pFieldDesc);
                }
            }

            if (pFieldDesc->options().GetExtension(nanopb).db_not_null())
            {
                colInfo.m_notNull = pFieldDesc->options().GetExtension(nanopb).db_not_null();
            }


            if (pFieldDesc->options().GetExtension(nanopb).db_comment().size() > 0)
            {
                colInfo.m_comment = lastComment + pFieldDesc->options().GetExtension(nanopb).db_comment();
            }

            if (colInfo.m_primaryKey)
            {
                primaryKeyMap.emplace(lastFieldName + pFieldDesc->name(), colInfo);
            }
            else
            {
                mapFileds.push_back(std::make_pair(lastFieldName + pFieldDesc->name(), colInfo));
            }
        }
        else
        {
            //如果只是简单的repeated, 比如:repeated string child_fish_ids = 7
            //把数据库里的多行，搞成数组的形式
            if (pFieldDesc->cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
            {
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                    {
                        DBTableColInfo colInfo;
                        colInfo.m_colType = pFieldDesc->cpp_type();
                        colInfo.m_fieldIndex = i;
                        std::string field = lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i);

                        if (colInfo.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_STRING ||
                            colInfo.m_colType == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
                        {
                            colInfo.m_bufsize = NFProtobufCommon::Instance()->GetFieldsDBMaxSize(pFieldDesc);
                        }

                        colInfo.m_notNull = pFieldDesc->options().GetExtension(nanopb).db_not_null();

                        if (pFieldDesc->options().GetExtension(nanopb).db_comment().size() > 0)
                        {
                            colInfo.m_comment = lastComment + pFieldDesc->options().GetExtension(nanopb).db_comment() + NFCommon::tostr(a_i);
                        }

                        mapFileds.push_back(std::make_pair(field, colInfo));
                    }
                }
            }
            else
            {
                //如果只是复杂的repeated, 比如:
                //message AttrValue
                //{
                //	optional int32 attr = 1 [(nanopb).field_cname = "attr"];
                //	optional int32 value = 2 [(nanopb).field_cname = "value"];
                //	optional string value2 = 3 [(nanopb).field_cname = "value2", (yd_fieldoptions.db_field_bufsize)=128];
                //}
                //repeated AttrValue attr_values = 8 [(nanopb).field_cname = "attr_values", (nanopb).max_count=2];
                //把数据库里的多行，配合结构转成repeated数组
                const google::protobuf::Descriptor* pSubDescriptor = pFieldDesc->message_type();
                if (pSubDescriptor == NULL) continue;
                ::google::protobuf::int32 arysize = NFProtobufCommon::Instance()->GetFieldsDBMaxCount(pFieldDesc);
                if (arysize > 0)
                {
                    for (::google::protobuf::int32 a_i = 0; a_i < arysize; a_i++)
                    {
                        GetDbFieldsInfoFromMessage(pSubDescriptor, primaryKeyMap, mapFileds, lastFieldName + pFieldDesc->name() + "_" + NFCommon::tostr(a_i) + "_", lastComment + pFieldDesc->options().GetExtension(nanopb).db_comment() + NFCommon::tostr(a_i));
                    }
                }
            }
        }
    }
    return 0;
}

uint32_t NFProtobufCommon::GetPBDataTypeFromDBDataType(const std::string& dbDataType, const std::string& strColumnType)
{
    if (dbDataType == "varchar" || dbDataType == "char" || dbDataType == "tinytext" || dbDataType == "text" || dbDataType == "mediumtext" || dbDataType == "longtext")
    {
        return google::protobuf::FieldDescriptor::CPPTYPE_STRING;
    }
    else if (dbDataType == "int" || dbDataType == "tinyint" || dbDataType == "smallint" || dbDataType == "mediumint")
    {
        if (strColumnType.find("unsigned") != std::string::npos)
        {
            return google::protobuf::FieldDescriptor::CPPTYPE_UINT32;
        }
        else
        {
            return google::protobuf::FieldDescriptor::CPPTYPE_INT32;
        }
    }
    else if (dbDataType == "float")
    {
        return google::protobuf::FieldDescriptor::CPPTYPE_FLOAT;
    }
    else if (dbDataType == "double")
    {
        return google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE;
    }
    else if (dbDataType == "enum")
    {
        return google::protobuf::FieldDescriptor::CPPTYPE_INT32;
    }
    else if (dbDataType == "bigint" || dbDataType == "datetime")
    {
        if (strColumnType.find("unsigned") != std::string::npos)
        {
            return google::protobuf::FieldDescriptor::CPPTYPE_UINT64;
        }
        else
        {
            return google::protobuf::FieldDescriptor::CPPTYPE_INT64;
        }
    }
    else if (dbDataType == "blob" || dbDataType == "varbinary" || dbDataType == "tinyblob" || dbDataType == "mediumblob" || dbDataType == "longblob")
    {
        return google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE;
    }

    return google::protobuf::FieldDescriptor::CPPTYPE_STRING;
}

std::string NFProtobufCommon::GetDBDataTypeFromPBDataType(uint32_t pbDataType, uint32_t textMax)
{
    switch (pbDataType)
    {
        case google::protobuf::FieldDescriptor::CPPTYPE_INT32:
        {
            return "int";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_INT64:
        {
            return "bigint";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT32:
        {
            return "int unsigned";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_UINT64:
        {
            return "bigint unsigned";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_DOUBLE:
        {
            return "double";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_FLOAT:
        {
            return "float";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_BOOL:
        {
            return "int";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_ENUM:
        {
            return "int";
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_STRING:
        {
            if (textMax > 16777216)
            {
                return "LONGBLOB";
            }
            else if (textMax > 65535)
            {
                return "MEDIUMBLOB";
            }
            else if (textMax > 1025)
            {
                return "blob";
            }
            else
            {
                return "varchar(" + NFCommon::tostr(textMax) + ")";
            }
        }
        break;
        case google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE:
        {
            if (textMax > 16777216)
            {
                return "LONGBLOB";
            }
            else if (textMax > 65535)
            {
                return "MEDIUMBLOB";
            }
            else
            {
                return "blob";
            }
        }
    }

    return "int";
}

std::string NFProtobufCommon::GetDescStoreClsName(const google::protobuf::Message& message)
{
    const google::protobuf::Descriptor* pSheetFieldDesc = message.GetDescriptor();
    CHECK_EXPR(pSheetFieldDesc, std::string(), "pSheetFieldDesc == NULL");
    const google::protobuf::Reflection* pSheetReflect = message.GetReflection();
    CHECK_EXPR(pSheetReflect, std::string(), "pSheetFieldDesc == NULL");

    for (int sheet_field_index = 0; sheet_field_index < pSheetFieldDesc->field_count(); sheet_field_index++)
    {
        const google::protobuf::FieldDescriptor* pSheetRepeatedFieldDesc = pSheetFieldDesc->field(sheet_field_index);
        if (pSheetRepeatedFieldDesc->is_repeated() &&
            pSheetRepeatedFieldDesc->cpp_type() == google::protobuf::FieldDescriptor::CPPTYPE_MESSAGE)
        {
            return pSheetRepeatedFieldDesc->message_type()->name();
        }
    }
    return std::string();
}

std::string NFProtobufCommon::GetProtoBaseName(const google::protobuf::Message& message)
{
    std::string fullName = message.GetTypeName();
    return GetProtoBaseName(fullName);
}

std::string NFProtobufCommon::GetProtoPackageName(const google::protobuf::Message& message)
{
    std::string fullName = message.GetTypeName();
    return GetProtoPackageName(fullName);
}

std::string NFProtobufCommon::GetProtoBaseName(const std::string& fullName)
{
    std::vector<std::string> result;
    NFStringUtility::Split(fullName, ".", &result);
    for (int i = result.size() - 1; i >= 0; i--)
    {
        if (result[i].size() > 0)
        {
            return result[i];
        }
    }

    std::string::size_type pos;
    if ((pos = fullName.rfind('.')) == std::string::npos)
    {
        return fullName;
    }

    if (pos < fullName.length() - 1)
    {
        return fullName.substr(pos + 1);
    }

    return fullName;
}

std::string NFProtobufCommon::GetProtoPackageName(const std::string& fullName)
{
    std::vector<std::string> result;
    NFStringUtility::Split(fullName, ".", &result);
    for (int i = 0; i < (int)result.size(); i++)
    {
        if (result[i].size() > 0)
        {
            return result[i];
        }
    }

    std::string::size_type pos;
    if ((pos = fullName.rfind('.')) == std::string::npos)
    {
        return fullName;
    }

    return fullName.substr(0, pos);
}

FieldParseType NFProtobufCommon::GetFieldPasreType(const google::protobuf::FieldDescriptor *pFieldDesc)
{
    const NanoPBOptions& opt = pFieldDesc->options().GetExtension(nanopb);
    return opt.parse_type();
}

int NFProtobufCommon::GetFieldsDBMaxCount(const google::protobuf::FieldDescriptor* pFieldDesc) const
{
    const NanoPBOptions& opt = pFieldDesc->options().GetExtension(nanopb);
    if (opt.db_max_count() > 0)
    {
        return opt.db_max_count();
    }
    else if (!opt.db_max_count_enum().empty())
    {
        std::string db_max_count_enum = opt.db_max_count_enum();
        auto pDesc = FindEnumValueByName(db_max_count_enum);
        if (pDesc)
        {
            return pDesc->number();
        }
        else
        {
            pDesc = FindEnumValueByName(DEFINE_DEFAULT_PROTO_PACKAGE_ADD + db_max_count_enum);
            if (pDesc)
            {
                return pDesc->number();
            }
            NFLogError(NF_LOG_DEFAULT, 0, "error can't find the db_max_count_enum:{}", db_max_count_enum);
            return 0;
        }
    }
    return 0;
}

int NFProtobufCommon::GetFieldsDBMaxSize(const google::protobuf::FieldDescriptor* pFieldDesc) const
{
    const NanoPBOptions& opt = pFieldDesc->options().GetExtension(nanopb);
    if (opt.db_max_size() > 0)
    {
        return opt.db_max_size();
    }
    else if (!opt.db_max_size_enum().empty())
    {
        std::string db_max_size_enum = opt.db_max_size_enum();
        auto pDesc = FindEnumValueByName(db_max_size_enum);
        if (pDesc)
        {
            return pDesc->number();
        }
        else
        {
            pDesc = FindEnumValueByName(DEFINE_DEFAULT_PROTO_PACKAGE_ADD + db_max_size_enum);
            if (pDesc)
            {
                return pDesc->number();
            }
            NFLogError(NF_LOG_DEFAULT, 0, "error can't find the db_max_size_enum:{}", db_max_size_enum);
            return 0;
        }
    }
    return 0;
}

int NFProtobufCommon::GetFieldsMaxCount(const google::protobuf::FieldDescriptor* pFieldDesc) const
{
    const NanoPBOptions& opt = pFieldDesc->options().GetExtension(nanopb);
    if (opt.max_count() > 0)
    {
        return opt.max_count();
    }
    else if (!opt.max_count_enum().empty())
    {
        std::string max_count_enum = opt.max_count_enum();
        auto pDesc = FindEnumValueByName(max_count_enum);
        if (pDesc)
        {
            return pDesc->number();
        }
        else
        {
            pDesc = FindEnumValueByName(DEFINE_DEFAULT_PROTO_PACKAGE_ADD + max_count_enum);
            if (pDesc)
            {
                return pDesc->number();
            }
            NFLogError(NF_LOG_DEFAULT, 0, "error can't find the max_count_enum:{}", max_count_enum);
            return 0;
        }
    }
    return 0;
}

int NFProtobufCommon::GetFieldsMaxSize(const google::protobuf::FieldDescriptor* pFieldDesc) const
{
    const NanoPBOptions& opt = pFieldDesc->options().GetExtension(nanopb);
    if (opt.max_size() > 0)
    {
        return opt.max_size();
    }
    else if (!opt.max_size_enum().empty())
    {
        std::string max_size_enum = opt.max_size_enum();
        auto pDesc = FindEnumValueByName(max_size_enum);
        if (pDesc)
        {
            return pDesc->number();
        }
        else
        {
            pDesc = FindEnumValueByName(DEFINE_DEFAULT_PROTO_PACKAGE_ADD + max_size_enum);
            if (pDesc)
            {
                return pDesc->number();
            }
            NFLogError(NF_LOG_DEFAULT, 0, "error can't find the max_size_enum:{}", max_size_enum);
            return 0;
        }
    }
    return 0;
}

const ::google::protobuf::EnumValueDescriptor* NFProtobufCommon::FindEnumValueByName(const string& name) const
{
    auto pDesc = m_pDescriptorPool->FindEnumValueByName(name);
    if (pDesc == NULL)
    {
        pDesc = google::protobuf::DescriptorPool::generated_pool()->FindEnumValueByName(name);
    }
    return pDesc;
}

const ::google::protobuf::EnumDescriptor* NFProtobufCommon::FindEnumTypeByName(const string& name) const
{
    auto pDesc = m_pDescriptorPool->FindEnumTypeByName(name);
    if (pDesc == NULL)
    {
        pDesc = google::protobuf::DescriptorPool::generated_pool()->FindEnumTypeByName(name);
    }
    return pDesc;
}

bool NFProtobufCommon::FindEnumNumberByMacroName(const std::string& enumName, const std::string& macroName, std::string& value)
{
    auto iter = m_enumMacroData.find(enumName);
    if (iter == m_enumMacroData.end())
    {
        auto pDesc = FindEnumTypeByName(enumName);
        CHECK_EXPR(pDesc, false, "can't find the enumName from pb:{}, macroName:{}", enumName, macroName);
        auto& enumTypeData = m_enumMacroData[enumName];
        for (int i = 0; i < (int)pDesc->value_count(); i++)
        {
            auto pEnumValue = pDesc->value(i);
            CHECK_EXPR(pEnumValue, false, "the enumName index:{} error from pb:{}, macroName:{}", i, enumName, macroName);
            auto opt = pEnumValue->options().GetExtension(nanopb_enumvopt);
            std::string macro_name = NFStringUtility::RemoveSpace(opt.macro_name());
            if (!macro_name.empty())
            {
                if (!NFStringUtility::IsUTF8String(macro_name))
                {
                    macro_name = NFStringUtility::GBKToUTF8(macro_name);
                }
                enumTypeData.m_enumNameToNumber[macro_name] = pEnumValue->number();
                enumTypeData.m_enumToNumber[pEnumValue->name()] = pEnumValue->number();
            }
            enumTypeData.m_numberToEnumName[pEnumValue->number()] = macro_name;
        }

        iter = m_enumMacroData.find(enumName);
    }

    std::string valueName = NFStringUtility::RemoveSpace(macroName);
    if (!valueName.empty())
    {
        if (!NFStringUtility::IsUTF8String(valueName))
        {
            valueName = NFStringUtility::GBKToUTF8(valueName);
        }
    }

    auto iter_value = iter->second.m_enumNameToNumber.find(valueName);
    if (iter_value != iter->second.m_enumNameToNumber.end())
    {
        value = NFCommon::tostr(iter_value->second);
    }
    else
    {
        if (NFStringUtility::CheckIsDigit(valueName))
        {
            auto iter_num = iter->second.m_numberToEnumName.find(NFCommon::strto<int>(valueName));
            CHECK_EXPR(iter_num != iter->second.m_numberToEnumName.end(), false, "the enumName from pb:{} can't find macroName:{}", enumName, macroName);
            value = valueName;
        }
        else
        {
            auto iter_enum = iter->second.m_enumToNumber.find(valueName);
            if (iter_enum != iter->second.m_enumToNumber.end())
            {
                value = NFCommon::tostr(iter_enum->second);
            }
            else
            {
                CHECK_EXPR(iter_value != iter->second.m_enumNameToNumber.end(), false, "the enumName from pb:{} can't find macroName:{}", enumName, macroName);
            }
        }
    }
    return true;
}

int NFProtobufCommon::GetPrimarykeyFromMessage(const google::protobuf::Message* pMessage, std::string& result)
{
    CHECK_NULL(0, pMessage);
    const google::protobuf::Descriptor* pDesc = pMessage->GetDescriptor();
    CHECK_NULL(0, pDesc);

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;

        if (pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_PRIMARYKEY)
        {
            result = GetFieldsString(*pMessage, pFieldDesc);
            return 0;
        }
    }
    return -1;
}

int NFProtobufCommon::SetPrimarykeyFromMessage(google::protobuf::Message* pMessage, const std::string& data)
{
    CHECK_NULL(0, pMessage);
    const google::protobuf::Descriptor* pDesc = pMessage->GetDescriptor();
    CHECK_NULL(0, pDesc);

    for (int i = 0; i < pDesc->field_count(); i++)
    {
        const google::protobuf::FieldDescriptor* pFieldDesc = pDesc->field(i);
        if (pFieldDesc == NULL) continue;

        if (pFieldDesc->options().GetExtension(nanopb).db_type() == E_FIELD_TYPE_PRIMARYKEY)
        {
            if (!SetFieldsString(*pMessage, pFieldDesc, data))
            {
                return -1;
            }
            return 0;
        }
    }
    return -1;
}


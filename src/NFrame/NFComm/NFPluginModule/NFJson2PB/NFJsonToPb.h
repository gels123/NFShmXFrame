

#include <google/protobuf/message.h>
#include <google/protobuf/io/zero_copy_stream.h>    // ZeroCopyInputStream
#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

namespace NFJson2PB {

struct Json2PbOptions {
    Json2PbOptions();

    // Decode string in json using base64 decoding if the type of
    // corresponding field is bytes when this option is turned on.
    // Default: false for baidu-interal, true otherwise.
    bool base64_to_bytes;
};

// Convert `json' to protobuf `message'.
// Returns true on success. `error' (if not NULL) will be set with error
// message on failure.
bool JsonToProtoMessage(const std::string& json,
                        google::protobuf::Message* message,
                        const Json2PbOptions& options,
                        std::string* error = NULL);

// send output to ZeroCopyOutputStream instead of std::string.
bool JsonToProtoMessage(google::protobuf::io::ZeroCopyInputStream *json,
                        google::protobuf::Message* message,
                        const Json2PbOptions& options,
                        std::string* error = NULL);

// Using default Json2PbOptions.
bool JsonToProtoMessage(const std::string& json,
                        google::protobuf::Message* message,
                        std::string* error = NULL);

bool JsonToProtoMessage(google::protobuf::io::ZeroCopyInputStream* stream,
                        google::protobuf::Message* message,
                        std::string* error = NULL);

bool JsonValueToProtoField(const std::string& json,
                                      const google::protobuf::FieldDescriptor* field,
                                      google::protobuf::Message* message,
                                      const NFJson2PB::Json2PbOptions& options,
                                      std::string* err);

bool CheckJsonToProtoMessage(const std::string& json_string, google::protobuf::Message* message, const NFJson2PB::Json2PbOptions& options, std::string* error, bool repeated);

bool CheckJsonValueToProtoField(const std::string& json_string, const std::string& fieldName, google::protobuf::FieldDescriptor::CppType fieldType, bool isRepeated, std::string* err);

} // namespace json2pb

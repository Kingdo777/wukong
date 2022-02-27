//
// Created by kingdo on 2022/2/26.
//

#include <wukong/utils/timing.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/json.h>

namespace wukong::proto {

    std::string messageToJson(const Message &msg) {
        rapidjson::Document d;
        d.SetObject();
        rapidjson::Document::AllocatorType &a = d.GetAllocator();

        d.AddMember("id", msg.id(), a);
        d.AddMember("type", msg.type(), a);

        d.AddMember("application", rapidjson::Value(msg.application().c_str(), msg.application().size(), a).Move(), a);
        d.AddMember("function", rapidjson::Value(msg.function().c_str(), msg.function().size(), a).Move(), a);

        d.AddMember("is_async", msg.isasync(), a);

        if (!msg.inputdata().empty()) {
            d.AddMember("input_data", rapidjson::Value(msg.inputdata().c_str(), msg.inputdata().size(), a).Move(), a);
        }
        if (!msg.outputdata().empty()) {
            d.AddMember("output_data", rapidjson::Value(msg.outputdata().c_str(), msg.outputdata().size(), a).Move(),
                        a);
        }
        if (!msg.resultkey().empty()) {
            d.AddMember("result_key", rapidjson::Value(msg.resultkey().c_str(), msg.resultkey().size(), a).Move(), a);
        }
        if (msg.finishtimestamp() > 0) {
            d.AddMember("finish_timestamp", msg.finishtimestamp(), a);
        }
        if (msg.timestamp() > 0) {
            d.AddMember("timestamp", msg.timestamp(), a);
        }

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);

        return sb.GetString();
    }



    wukong::proto::Message jsonToMessage(const std::string &jsonIn) {
        TIMING_START(jsonDecode)

        rapidjson::MemoryStream ms(jsonIn.c_str(), jsonIn.size());
        rapidjson::Document d;
        d.ParseStream(ms);

        wukong::proto::Message msg;

        msg.set_id(utils::getIntFromJson(d, "id", 0));
        int msgType = utils::getIntFromJson(d, "type", wukong::proto::Message::MessageType::Message_MessageType_FUNCTION);
        if (!wukong::proto::Message::MessageType_IsValid(msgType)) {
            SPDLOG_ERROR("Bad message type: {}", msgType);
            throw std::runtime_error("Invalid message type");
        }
        msg.set_type(static_cast<wukong::proto::Message::MessageType>(msgType));

        msg.set_application(utils::getStringFromJson(d, "application", ""));
        msg.set_function(utils::getStringFromJson(d, "function", ""));

        msg.set_isasync(utils::getBoolFromJson(d, "async", false));


        msg.set_inputdata(utils::getStringFromJson(d, "input_data", ""));
        msg.set_outputdata(utils::getStringFromJson(d, "output_data", ""));
        msg.set_resultkey(utils::getStringFromJson(d, "result_key", ""));

        msg.set_timestamp(utils::getInt64FromJson(d, "timestamp", 0));
        msg.set_finishtimestamp(utils::getInt64FromJson(d, "finished", 0));

        TIMING_END(jsonDecode)

        return msg;
    }


    std::string getJsonOutput(const Message &msg) {
        rapidjson::Document d;
        d.SetObject();
        rapidjson::Document::AllocatorType &a = d.GetAllocator();
        d.AddMember(
                "output_data",
                rapidjson::Value(msg.outputdata().c_str(), msg.outputdata().size(), a).Move(),
                a);
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);
        return sb.GetString();
    }


}
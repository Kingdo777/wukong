//
// Created by kingdo on 2022/2/26.
//

#include <wukong/utils/timing.h>
#include <wukong/proto/proto.h>
#include <wukong/utils/json.h>
#include <wukong/utils/uuid.h>

namespace wukong::proto {

    /// _______________________ For Message __________________________

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

        msg.set_id(utils::uuid());
        int msgType = utils::getIntFromJson(d, "type",
                                            wukong::proto::Message::MessageType::Message_MessageType_FUNCTION);
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

    /// _____________ For ReplyRegisterInvoker _______________________

//    std::string messageToJson(const ReplyRegisterInvoker &msg) {
//        rapidjson::Document d;
//        d.SetObject();
//        rapidjson::Document::AllocatorType &a = d.GetAllocator();
//
//        d.AddMember("success", msg.success(), a);
//        d.AddMember("invokerID", rapidjson::Value(msg.invokerid().c_str(), msg.invokerid().size(), a).Move(), a);
//        d.AddMember("msg", rapidjson::Value(msg.msg().c_str(), msg.msg().size(), a).Move(), a);
//
//        rapidjson::StringBuffer sb;
//        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
//        d.Accept(writer);
//
//        return sb.GetString();
//    }
//
//    wukong::proto::ReplyRegisterInvoker jsonToReplyRegisterInvoker(const std::string &jsonIn) {
//
//        rapidjson::MemoryStream ms(jsonIn.c_str(), jsonIn.size());
//        rapidjson::Document d;
//        d.ParseStream(ms);
//
//        wukong::proto::ReplyRegisterInvoker msg;
//        msg.set_success(utils::getBoolFromJson(d, "success", false));
//        msg.set_invokerid(utils::getStringFromJson(d, "invokerID", ""));
//        msg.set_msg(utils::getStringFromJson(d, "msg", ""));
//
//        return msg;
//    }

    /// _____________ For Invoker _______________________
    std::string messageToJson(const Invoker &msg) {
        rapidjson::Document d;
        d.SetObject();
        rapidjson::Document::AllocatorType &a = d.GetAllocator();

        d.AddMember("invokerID", rapidjson::Value(msg.invokerid().c_str(), msg.invokerid().size(), a).Move(), a);
        d.AddMember("IP", rapidjson::Value(msg.ip().c_str(), msg.ip().size(), a).Move(), a);
        d.AddMember("port", msg.port(), a);
        d.AddMember("memory", msg.memory(), a);
        d.AddMember("cpu", msg.cpu(), a);
        d.AddMember("registerTime", msg.registertime(), a);

        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer(sb);
        d.Accept(writer);

        return sb.GetString();
    }

    wukong::proto::Invoker jsonToInvoker(const std::string &jsonIn) {

        rapidjson::MemoryStream ms(jsonIn.c_str(), jsonIn.size());
        rapidjson::Document d;
        d.ParseStream(ms);

        wukong::proto::Invoker msg;
        msg.set_invokerid(utils::getStringFromJson(d, "invokerID", ""));
        msg.set_ip(utils::getStringFromJson(d, "IP", ""));
        msg.set_port(utils::getIntFromJson(d, "port", 0));
        msg.set_memory(utils::getIntFromJson(d, "memory", 0));
        msg.set_cpu(utils::getIntFromJson(d, "cpu", 0));
        msg.set_registertime(utils::getInt64FromJson(d, "registerTime", 0));

        return msg;
    }

    std::string getStringFromHash(const std::string &key,
                                  const std::unordered_map<std::string, std::string> &hash,
                                  const std::string &dflt) {
        auto iter = hash.find(key);
        if (iter != hash.end())
            return iter->second;
        else
            return dflt;
    }

    uint64_t getIntFromHash(const std::string &key,
                            const std::unordered_map<std::string, std::string> &hash,
                            int dflt) {
        auto iter = hash.find(key);
        if (iter != hash.end())
            return strtol(iter->second.c_str(), nullptr, 10);
        else
            return dflt;
    }

    wukong::proto::Invoker hashToInvoker(const std::unordered_map<std::string, std::string> &hash) {
        wukong::proto::Invoker msg;

        msg.set_invokerid(getStringFromHash("invokerID", hash, ""));
        msg.set_ip(getStringFromHash("IP", hash, ""));
        msg.set_port(getIntFromHash("port", hash, 0));
        msg.set_memory(getIntFromHash("memory", hash, 0));
        msg.set_cpu(getIntFromHash("cpu", hash, 0));
        msg.set_registertime(getIntFromHash("registerTime", hash, 0));

        return msg;
    }

    std::unordered_map<std::string, std::string> invokerToHash(const Invoker &invoker) {
        std::unordered_map<std::string, std::string> hash;
        hash.insert(std::make_pair("invokerID", invoker.invokerid()));
        hash.insert(std::make_pair("IP", invoker.ip()));
        hash.insert(std::make_pair("port", std::to_string(invoker.port())));
        hash.insert(std::make_pair("memory", std::to_string(invoker.memory())));
        hash.insert(std::make_pair("cpu", std::to_string(invoker.cpu())));
        hash.insert(std::make_pair("registerTime", std::to_string(invoker.registertime())));
        return hash;
    }
}
#include "common.h"

// 获取消息队列ID
int getMsgId(int id)
{
    return msgget(MSG_KEY_BASE + id, IPC_CREAT | 0666);
}

// 序列化 routing_table
std::string serializeRoutingTable(const std::map<int, std::pair<int, int>> &routing_table)
{
    std::ostringstream oss;
    for (const auto &[key, value] : routing_table)
    {
        oss << key << "," << value.first << "," << value.second << ";";
    }
    return oss.str();
}

// 反序列化 routing_table
std::map<int, std::pair<int, int>> deserializeRoutingTable(const std::string &data)
{
    std::map<int, std::pair<int, int>> routing_table;
    std::stringstream ss(data);
    std::string entry;

    while (std::getline(ss, entry, ';'))
    {
        if (entry.empty())
            continue;
        std::stringstream entry_ss(entry);
        std::string token;
        int key, first, second;

        try
        {
            std::getline(entry_ss, token, ',');
            key = std::stoi(token);

            std::getline(entry_ss, token, ',');
            first = std::stoi(token);

            std::getline(entry_ss, token, ',');
            second = std::stoi(token);
        }
        catch (const std::invalid_argument &e)
        {
            std::cerr << "反序列化错误：无效的整数格式 in entry '" << entry << "'" << std::endl;
            continue; // 跳过无效的条目
        }
        catch (const std::out_of_range &e)
        {
            std::cerr << "反序列化错误：整数超出范围 in entry '" << entry << "'" << std::endl;
            continue; // 跳过超出范围的条目
        }

        routing_table[key] = {first, second};
    }

    return routing_table;
}

// 发送更新消息
void sendUpdateMessage(int srcRouter, int targetRouter, const std::map<int, std::pair<int, int>> &routing_table)
{
    Msg msg;
    msg.msg_type = 1;
    msg.src_id = srcRouter;
    std::string serialized = serializeRoutingTable(routing_table);
    strncpy(msg.data, serialized.c_str(), sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0'; // 确保字符串终止
    msgsnd(getMsgId(targetRouter), &msg, sizeof(Msg) - sizeof(long), 0);
}

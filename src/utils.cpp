#include "common.h"

/**
 * @brief 获取消息队列ID。
 *
 * 该函数基于提供的ID生成一个消息队列标识符。
 * 它使用 `msgget` 函数创建或访问具有指定键和权限的消息队列。
 *
 * @param id 用于生成消息队列键的基础ID。
 * @return 成功时返回消息队列标识符，失败时返回-1。
 */
int getMsgId(int id)
{
    return msgget(MSG_KEY_BASE + id, IPC_CREAT | 0666);
}

/**
 * @brief 清空消息队列中的所有消息。
 *
 * 该函数不断从指定的消息队列中接收消息，直到消息队列为空或发生错误。
 *
 * @param msgid 消息队列的标识符。
 */
void clearMessageQueue(int msgid)
{
    Msg msg;
    while (true)
    {
        ssize_t ret = msgrcv(msgid, &msg, sizeof(Msg) - sizeof(long), 0, IPC_NOWAIT);
        if (ret < 0)
        {
            if (errno == ENOMSG)
                break;
            else
            {
                std::cerr << "msgrcv 错误: " << strerror(errno) << std::endl;
                break;
            }
        }
    }
}

/**
 * @brief 序列化路由表，将路由表转换为字符串格式。
 *
 * @param routing_table 路由表，键为目的地节点ID，值为一个包含下一跳节点ID和距离的pair。
 * @return std::string 序列化后的路由表字符串，每个条目以";"分隔，条目中的键和值以","分隔。
 */
std::string serializeRoutingTable(const std::map<int, std::pair<int, int>> &routing_table)
{
    std::ostringstream oss;
    for (const auto &[key, value] : routing_table)
    {
        oss << key << "," << value.first << "," << value.second << ";";
    }
    return oss.str();
}

/**
 * @brief 反序列化路由表数据
 *
 * 该函数将一个包含路由表数据的字符串反序列化为一个 std::map<int, std::pair<int, int>> 类型的路由表。
 * 输入字符串的格式应为 "key1,first1,second1;key2,first2,second2;..."，其中 key、first 和 second 都是整数。
 *
 * @param data 包含路由表数据的字符串
 * @return std::map<int, std::pair<int, int>> 反序列化后的路由表
 *
 * @note 如果输入字符串中的某个条目格式无效或整数超出范围，该条目将被跳过，并在标准错误输出中打印错误信息。
 */
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

/**
 * @brief 发送一条消息到目标路由器。
 *
 * @param type 消息类型。
 * @param srcRouter 源路由器ID。
 * @param targetRouter 目标路由器ID。
 * @param routing_table 路由表，包含每个目标路由器的距离和下一跳。
 */
void sendMessage(int type, int srcRouter, int targetRouter, const std::map<int, std::pair<int, int>> &routing_table)
{
    Msg msg;
    msg.msg_type = type;
    msg.src_id = srcRouter;
    std::string serialized = serializeRoutingTable(routing_table);
    strncpy(msg.data, serialized.c_str(), sizeof(msg.data) - 1);
    msg.data[sizeof(msg.data) - 1] = '\0'; // 确保字符串终止
    msgsnd(getMsgId(targetRouter), &msg, sizeof(Msg) - sizeof(long), 0);
}

/**
 * @brief 计算两个时间点之间的时间差（以毫秒为单位）。
 *
 * @param start 起始时间点。
 * @param end 结束时间点。
 * @return long long 返回两个时间点之间的时间差（以毫秒为单位）。
 */
long long calculateTimeDifference(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}
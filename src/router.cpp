#include "utils.h"
#include "common.h"

// 把路由表输出到文件
void output_routing_table(int id, const std::map<int, std::pair<int, int>> &routing_table)
{
    std::ofstream out("routing_table/" + std::to_string(id) + ".txt");
    out << "路由器 " << id << " 的当前路由表：\n\n";
    for (const auto &[dist, val] : routing_table)
    {
        const auto &[cost, next_hop] = val;
        out << "目的ID:   \t" << dist
            << "\t总花费: \t" << cost
            << "\t下一跳: \t" << next_hop << "\n";
    }
    out.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10毫秒
}

// 获取下一跳路由器ID
int getNextHop(const std::map<int, std::pair<int, int>> &routing_table, int dest_id)
{
    try
    {
        while (dest_id != routing_table.at(dest_id).second)
            dest_id = routing_table.at(dest_id).second;
    }
    catch (const std::out_of_range &e)
    {
        std::cerr << "无法找到目的ID " << dest_id << " 的下一跳路由器！" << std::endl;
        return -1;
    }

    return dest_id;
}

// 路由器进程函数
void router(int id)
{
    // 路由表，键为目的ID，值为（总花费，下一跳ID）
    std::map<int, std::pair<int, int>> local_routing_table;

    // 初始化路由表，先添加自身信息
    local_routing_table[id] = {0, id};

    int msgid = getMsgId(id);

    std::unordered_set<int> neighbors; // 邻居路由器ID

    auto last_message_time = std::chrono::high_resolution_clock::now();

    bool updated = false; // 是否更新过路由表
    bool awake = false;   // 是否被唤醒

    while (true)
    {
        // 非阻塞接收消息
        Msg msg;
        if (msgrcv(msgid, &msg, sizeof(Msg) - sizeof(long), 0, IPC_NOWAIT) >= 0)
        {
            std::string data_received(msg.data);

            // std::cout << "Router " << id << " received msg_type: " << msg.msg_type << std::endl;

            if (msg.msg_type == MSG_TYPE_INIT)
            {
                auto received_routing_table = deserializeRoutingTable(data_received);

                for (auto [dest_id, info] : received_routing_table)
                {
                    // std::cout << "路由器 " << id << " 收到初始化消息：" << dest_id << std::endl;
                    auto [cost, next_hop] = info;

                    neighbors.insert(dest_id);

                    // 尝试更新路由表
                    if (local_routing_table.find(dest_id) == local_routing_table.end() || local_routing_table[dest_id].first > cost)
                        local_routing_table[dest_id] = {cost, next_hop};
                }
            }
            else if (msg.msg_type == MSG_TYPE_WAKE)
            {
                std::cout << "路由器 " << id << " 收到唤醒消息。" << std::endl;
                last_message_time = std::chrono::high_resolution_clock::now(); // 初始化最后消息时间
                for (int neighbor : neighbors)
                    sendMessage(MSG_TYPE_UPDATE, id, neighbor, local_routing_table); // 发送更新消息给邻居
                awake = true;
            }
            else if (msg.msg_type == MSG_TYPE_UPDATE)
            {
                auto received_routing_table = deserializeRoutingTable(data_received);

                for (auto [dest_id, info] : received_routing_table)
                {
                    auto [cost, _] = info;

                    if (dest_id == id)
                        continue; // 跳过自身

                    // 尝试更新路由表
                    if (local_routing_table.find(dest_id) == local_routing_table.end() || local_routing_table[dest_id].first > local_routing_table[msg.src_id].first + cost)
                    {
                        int next_hop = getNextHop(local_routing_table, msg.src_id);

                        if (next_hop == -1)
                            continue;

                        local_routing_table[dest_id] = {local_routing_table[msg.src_id].first + cost, next_hop};

                        // std::cout << "路由器 " << id << " 更新路由表：" << dest_id << std::endl;
                        updated = true;

                        sendMessage(MSG_TYPE_REFRESH, id, MANAGER_ID, {}); // 通知主进程网络存在更新
                    }
                }
                continue;
            }
            else if (msg.msg_type == MSG_TYPE_TERMINATE)
            {
                std::cout << "路由器 " << id << " 收到终止消息，终止进程。" << std::endl;

                output_routing_table(id, local_routing_table);

                // 关闭消息队列
                msgctl(msgid, IPC_RMID, NULL);

                exit(EXIT_SUCCESS);
            }
        }

        auto now = std::chrono::high_resolution_clock::now();
        if (awake && updated && calculateTimeDifference(last_message_time, now) * 20 >= TIMEOUT_MILLISECONDS)
        {
            std::cout << "路由器 " << id << " 发送更新后的路由表。" << std::endl;
            // 发送更新消息给邻居
            for (int neighbor : neighbors)
                sendMessage(MSG_TYPE_UPDATE, id, neighbor, local_routing_table);

            updated = false;
            last_message_time = std::chrono::high_resolution_clock::now(); // 刷新倒计时
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10毫秒
    }
}

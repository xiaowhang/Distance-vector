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
        // std::cerr << "无法找到目的ID " << old_dest_id << " 的下一跳路由器！" << std::endl;
        // std::cerr << old_dest_id << ' ' << dest_id << std::endl;
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

    // 创建消息队列，用于接收主进程的更新
    key_t key = MSG_KEY_BASE + id;
    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid < 0)
    {
        std::cerr << "路由器 " << id << " 创建消息队列失败！" << std::endl;
        exit(EXIT_FAILURE);
    }

    time_t last_message_time = time(NULL); // 初始化最后消息时间

    std::unordered_set<int> neighbors; // 邻居路由器ID

    bool updated = false; // 是否更新过路由表

    while (true)
    {
        // 非阻塞接收消息
        Msg msg;
        if (msgrcv(msgid, &msg, sizeof(Msg) - sizeof(long), 0, IPC_NOWAIT) >= 0)
        {
            std::string data_received(msg.data);

            if (msg.src_id == MANAGER_ID) // 主进程发送的更新消息
            {
                auto [dest_id, val] = *deserializeRoutingTable(data_received).begin();
                auto [cost, next_hop] = val;

                neighbors.insert(dest_id);

                // 更新路由表
                if (local_routing_table.find(dest_id) == local_routing_table.end() || local_routing_table[dest_id].first > cost)
                {
                    local_routing_table[dest_id] = {cost, next_hop};
                    updated = true;
                }
            }
            else
            {
                auto reveived_routing_table = deserializeRoutingTable(data_received);

                for (auto it = reveived_routing_table.begin(); it != reveived_routing_table.end(); it++)
                {
                    auto [dest_id, val] = *it;
                    auto [cost, _] = val;

                    if (dest_id == id)
                        continue; // 跳过自身

                    // 更新路由表
                    if (local_routing_table.find(dest_id) == local_routing_table.end() || local_routing_table[dest_id].first > local_routing_table[msg.src_id].first + cost)
                    {
                        int next_hop = getNextHop(local_routing_table, msg.src_id);

                        if (next_hop == -1)
                            continue;

                        local_routing_table[dest_id] = {local_routing_table[msg.src_id].first + cost, next_hop};
                        updated = true;
                    }
                }
            }
        }

        if (updated && difftime(time(NULL), last_message_time) * 10 >= TIMEOUT_SECONDS)
        {
            // 发送更新消息给邻居
            for (int neighbor : neighbors)
            {
                sendUpdateMessage(id, neighbor, local_routing_table);
            }

            updated = false;
            last_message_time = time(NULL); // 刷新倒计时
        }

        // 检查是否超时
        if (difftime(time(NULL), last_message_time) >= TIMEOUT_SECONDS)
        {
            std::cout << "路由器 " << id << " 超时未接收到消息，终止进程。" << std::endl;

            output_routing_table(id, local_routing_table);

            // 关闭消息队列
            msgctl(msgid, IPC_RMID, NULL);

            exit(EXIT_SUCCESS);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 休眠100毫秒
    }
}

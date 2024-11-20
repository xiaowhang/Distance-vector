#include <iostream>
#include <unistd.h>
#include <cstring>
#include <sys/wait.h>
#include <vector>
#include <signal.h>
#include <unordered_set>
#include <fstream>
#include <map>
#include <unordered_map>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <ctime>
#include <filesystem>
#include <chrono>
#include <thread>

#define BASE_PORT 50000
#define MAX_ROUTERS 100
#define MSG_KEY_BASE 1000
#define TIMEOUT_SECONDS 2
#define MANAGER_ID 999

#define MSG_SIZE 1024

std::vector<pid_t> children;
int current_routers = 0;

// 定义消息结构
struct Msg
{
    long msg_type; // 消息类型，必须 >0
    int src_id;    // 源ID
    char data[MSG_SIZE];
};

// 获取消息队列ID
int getMsgId(int id)
{
    return msgget(MSG_KEY_BASE + id, IPC_CREAT | 0666);
}

void init()
{
    // 捕获SIGCHLD信号，防止僵尸进程
    signal(SIGCHLD, SIG_IGN);

    // 如果routing_table文件夹存在则删除，重新创建
    if (std::filesystem::exists("routing_table"))
        std::filesystem::remove_all("routing_table");
    std::filesystem::create_directory("routing_table");
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

// 把路由表输出到文件
void output_routing_table(int id, const std::map<int, std::pair<int, int>> &routing_table)
{
    std::ofstream out("routing_table/" + std::to_string(id) + ".txt");
    out << "路由器 " << id << " 的当前路由表：\n";
    for (const auto &[dist, val] : routing_table)
    {
        const auto &[cost, next_hop] = val;
        out << "目的ID: " << dist
            << "，总花费: " << cost
            << "，下一跳: " << next_hop << "\n";
    }
    out.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10毫秒
}

// 获取下一跳路由器ID
int getNextHop(const std::map<int, std::pair<int, int>> &routing_table, int dest_id)
{
    while (dest_id != routing_table.at(dest_id).second)
        dest_id = routing_table.at(dest_id).second;

    return dest_id;
}

// 路由器进程函数
void router(int id)
{
    // 路由表，键为目的ID，值为（总花费，下一跳ID）
    std::map<int, std::pair<int, int>> local_routing_table;

    // 初始化路由表，先添加自身信息
    local_routing_table[id] = {0, -1};

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

                    // 更新路由表
                    if (local_routing_table.find(dest_id) == local_routing_table.end() || local_routing_table[dest_id].first > local_routing_table[msg.src_id].first + cost)
                    {
                        int next_hop = getNextHop(local_routing_table, msg.src_id);

                        local_routing_table[dest_id] = {local_routing_table[msg.src_id].first + cost, next_hop};
                        updated = true;
                    }
                }
            }
        }

        if (updated)
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
            break; // 退出循环，终止子进程
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 休眠100毫秒
    }

    output_routing_table(id, local_routing_table);

    // 关闭消息队列
    msgctl(msgid, IPC_RMID, NULL);

    exit(EXIT_SUCCESS);
}

// 修改add_router函数，接受路由器ID作为参数
void add_router(int id)
{
    if (current_routers >= MAX_ROUTERS)
    {
        std::cerr << "已达到最大路由器数量！" << std::endl;
        return;
    }

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "创建路由器子进程失败！" << std::endl;
        return;
    }

    if (pid == 0)
    {
        // 子进程作为路由器
        router(id);
        exit(EXIT_SUCCESS);
    }
    else
    {
        // 父进程记录子进程ID
        children.push_back(pid);
        std::cout << "添加路由器 " << id << "，PID: " << pid << std::endl;
        current_routers++;
    }
}

int main()
{
    init();

    // 从拓扑文件中读取路由器ID
    std::ifstream topo_file("test6.txt");
    if (!topo_file)
    {
        std::cerr << "无法打开拓扑文件！" << std::endl;
        return -1;
    }

    std::unordered_set<int> router_ids; // 使用unordered_set自动去重
    int router1, router2, cost;
    while (topo_file >> router1 >> router2 >> cost)
    {
        if (router_ids.find(router1) == router_ids.end())
        {
            add_router(router1);
            router_ids.insert(router1);
        }
        if (router_ids.find(router2) == router_ids.end())
        {
            add_router(router2);
            router_ids.insert(router2);
        }

        // 发送初始化消息
        sendUpdateMessage(MANAGER_ID, router1, {{router2, {cost, router2}}});
        sendUpdateMessage(MANAGER_ID, router2, {{router1, {cost, router1}}});
    }
    topo_file.close();

    // 等待所有子进程结束
    for (auto pid : children)
        waitpid(pid, NULL, 0);

    return 0;
}
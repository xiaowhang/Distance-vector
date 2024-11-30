#include "common.h"
#include "utils.h"
#include "router.h"

/**
 * @brief 添加一个新的路由器。
 *
 * 该函数会创建一个新的子进程作为路由器，并将其ID记录在children列表中。
 * 如果路由器ID已经存在或达到最大路由器数量，则不会添加新的路由器。
 *
 * @param id 路由器的唯一标识符。
 * @param children 存储所有子进程ID。
 * @param router_ids 存储所有路由器ID的集合。
 */
void add_router(int id, std::vector<pid_t> &children, std::unordered_set<int> &router_ids)
{
    if (router_ids.count(id))
        return;

    if (router_ids.size() >= MAX_ROUTERS)
    {
        std::cerr << "已达到最大路由器数量！" << std::endl;
        return;
    }

    router_ids.insert(id);

    pid_t pid = fork();
    if (pid < 0)
    {
        std::cerr << "创建路由器子进程失败！" << std::endl;
        return;
    }

    if (pid == 0)
    {
        // 创建消息队列，用于接收主进程的更新
        int msgid = getMsgId(id);
        if (msgid < 0)
        {
            std::cerr << "路由器 " << id << " 创建消息队列失败！" << std::endl;
            exit(EXIT_FAILURE);
        }
        clearMessageQueue(msgid);

        // 子进程作为路由器
        router(id);
        exit(EXIT_SUCCESS);
    }
    else
    {
        // 父进程记录子进程ID
        children.push_back(pid);
        std::cout << "添加路由器 " << id << "，PID: " << pid << std::endl;
    }
}

int main(int argc, char *argv[])
{
    std::vector<pid_t> children;        // 子进程ID
    std::unordered_set<int> router_ids; // 路由器ID

    // 捕获SIGCHLD信号，防止僵尸进程
    signal(SIGCHLD, SIG_IGN);

    // 如果routing_table文件夹存在则删除，重新创建
    if (std::filesystem::exists("routing_table"))
        std::filesystem::remove_all("routing_table");
    std::filesystem::create_directory("routing_table");

    if (argc != 2)
    {
        std::cerr << "用法: " << argv[0] << " <拓扑文件>" << std::endl;
        return -1;
    }

    // 从拓扑文件中读取路由器ID
    std::ifstream topo_file(argv[1]);
    if (!topo_file)
    {
        std::cerr << "无法打开拓扑文件！" << std::endl;
        return -1;
    }

    // 添加路由器
    int router1, router2, cost;
    while (topo_file >> router1 >> router2 >> cost)
    {
        add_router(router1, children, router_ids);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10毫秒
        sendMessage(MSG_TYPE_INIT, MANAGER_ID, router1, {{router2, {cost, router2}}});

        add_router(router2, children, router_ids);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10毫秒
        sendMessage(MSG_TYPE_INIT, MANAGER_ID, router2, {{router1, {cost, router1}}});
    }
    topo_file.close();

    // 等待路由器初始化完成
    std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 休眠10毫秒

    for (auto router_id : router_ids)
    {
        // 发送唤醒消息
        sendMessage(MSG_TYPE_WAKE, MANAGER_ID, router_id, {});
    }

    auto last_message_time = std::chrono::high_resolution_clock::now();

    int msgid = getMsgId(MANAGER_ID);

    while (true)
    {
        // 非阻塞接收消息
        Msg msg;
        if (msgrcv(msgid, &msg, sizeof(Msg) - sizeof(long), 0, IPC_NOWAIT) >= 0)
        {
            last_message_time = std::chrono::high_resolution_clock::now(); // 刷新倒计时
        }

        // 检查是否超时
        auto now = std::chrono::high_resolution_clock::now();
        if (calculateTimeDifference(last_message_time, now) >= TIMEOUT_MILLISECONDS)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 休眠100毫秒
    }

    // 发送终止消息
    for (auto router_id : router_ids)
    {
        std::cout << "发送终止消息给路由器 " << router_id << std::endl;
        sendMessage(MSG_TYPE_TERMINATE, MANAGER_ID, router_id, {});
    }

    // 等待所有子进程结束
    for (auto pid : children)
        waitpid(pid, NULL, 0);

    return 0;
}
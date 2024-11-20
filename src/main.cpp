#include "common.h"
#include "utils.h"
#include "router.h"

std::vector<pid_t> children;
int current_routers = 0;

// 添加路由器
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
    // 捕获SIGCHLD信号，防止僵尸进程
    signal(SIGCHLD, SIG_IGN);

    // 如果routing_table文件夹存在则删除，重新创建
    if (std::filesystem::exists("routing_table"))
        std::filesystem::remove_all("routing_table");
    std::filesystem::create_directory("routing_table");

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
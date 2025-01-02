#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <vector>
#include <signal.h>
#include <unordered_set>
#include <fstream>
#include <map>
#include <sys/msg.h>
#include <filesystem>
#include <chrono>
#include <thread>

constexpr int MAX_ROUTERS = 100; // 最大路由器数量

constexpr int TIMEOUT_MILLISECONDS = 2000; // 超时时间
constexpr int UPDATE_INTERVAL = 500;      // 周期更新间隔

constexpr int MSG_KEY_BASE = 1000; // 消息队列键基础值
constexpr int MANAGER_ID = 999;    // 管理器ID

constexpr int MSG_SIZE = 1024; // 消息数据大小

constexpr int MSG_TYPE_INIT = 1;      // 消息类型：初始化
constexpr int MSG_TYPE_UPDATE = 2;    // 消息类型：更新
constexpr int MSG_TYPE_WAKE = 3;      // 消息类型：唤醒
constexpr int MSG_TYPE_TERMINATE = 4; // 消息类型：终止
constexpr int MSG_TYPE_REFRESH = 5;   // 消息类型：刷新

constexpr int INF = 0x3f3f3f3f; // 无穷大

// 定义消息结构
struct Msg
{
    long msg_type; // 消息类型，必须 >0
    int src_id;    // 源ID
    char data[MSG_SIZE];
};

#endif // COMMON_H
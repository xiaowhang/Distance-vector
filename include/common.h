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
#define MSG_KEY_BASE 1000
#define MAX_ROUTERS 100
#define TIMEOUT_SECONDS 2
#define MANAGER_ID 999
#define MSG_SIZE 1024

// 定义消息结构
struct Msg
{
    long msg_type; // 消息类型，必须 >0
    int src_id;    // 源ID
    char data[MSG_SIZE];
};

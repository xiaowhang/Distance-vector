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

#define BASE_PORT 50000
#define MAX_ROUTERS 100

#define TIMEOUT_MILLISECONDS 1000

#define MSG_KEY_BASE 1000
#define MANAGER_ID 999

#define MSG_SIZE 1024

#define MSG_TYPE_INIT 1
#define MSG_TYPE_UPDATE 2
#define MSG_TYPE_WAKE 3
#define MSG_TYPE_TERMINATE 4
#define MSG_TYPE_REFRESH 5

// 定义消息结构
struct Msg
{
    long msg_type; // 消息类型，必须 >0
    int src_id;    // 源ID
    char data[MSG_SIZE];
};

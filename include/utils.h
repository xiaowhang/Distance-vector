#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>
#include <chrono>
#include <unordered_set>

int getMsgId(int id);
void clearMessageQueue(int msgid);
std::string serializeRoutingTable(const std::map<int, std::pair<int, int>> &routing_table);
std::map<int, std::pair<int, int>> deserializeRoutingTable(const std::string &data);
void sendMessage(int type, int srcRouter, int targetRouter, const std::map<int, std::pair<int, int>> &routing_table);
long long calculateTimeDifference(std::chrono::high_resolution_clock::time_point start, std::chrono::high_resolution_clock::time_point end);
void sendUpdateToNeighbors(int id, std::map<int, std::pair<int, int>> &local_routing_table, std::unordered_set<int> &neighbors);

#endif

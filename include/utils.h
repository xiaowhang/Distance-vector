#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <map>

int getMsgId(int id);
std::string serializeRoutingTable(const std::map<int, std::pair<int, int>> &routing_table);
std::map<int, std::pair<int, int>> deserializeRoutingTable(const std::string &data);
void sendUpdateMessage(int srcRouter, int targetRouter, const std::map<int, std::pair<int, int>> &routing_table);

#endif

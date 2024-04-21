#include <iostream>
#include <sstream>
#include <string>

void parseString(const std::string& input, long long& timestamp, int& pid, std::string& data) {
    std::istringstream iss(input);
    std::string token;

    // Extract the timestamp
    if (std::getline(iss, token, ' ')) {
        timestamp = std::stoll(token);
    }

    // Extract the PID
    if (std::getline(iss, token, ' ')) {
        pid = std::stoi(token);
    }

    // Extract the data
    std::getline(iss, data);
}

int main() {
    std::string input = "1 2 request_Resource";
    long long timestamp;
    int pid;
    std::string data;

    parseString(input, timestamp, pid, data);

    std::cout << "Timestamp: " << timestamp << std::endl;
    std::cout << "PID: " << pid << std::endl;
    std::cout << "Data: " << data << std::endl;

    return 0;
}
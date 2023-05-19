#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

const int SERVER_PORT = 8888;   // 服务器端口号
const int MAX_CONNECTIONS = 10;  // 最大连接数
const int BLOCK_SIZE = 20 * 1024;  // 区块大小（20KB）
const int BANDWIDTH = 20 * 1024 * 1024;  // 带宽（20Mbps）

// 服务器节点类
class ServerNode {
public:
    ServerNode(const std::string& ip) : ip(ip) {
        // 创建服务器套接字
        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == -1) {
            std::cerr << "Failed to create socket" << std::endl;
            exit(1);
        }

        // 设置服务器套接字选项，允许地址重用
        int enable = 1;
        if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) == -1) {
            std::cerr << "Failed to set socket options" << std::endl;
            exit(1);
        }

        // 绑定服务器套接字到指定IP和端口
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
        serverAddress.sin_port = htons(SERVER_PORT);
        if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
            std::cerr << "Failed to bind socket" << std::endl;
            exit(1);
        }

        // 监听连接
        if (listen(serverSocket, MAX_CONNECTIONS) == -1) {
            std::cerr << "Failed to listen" << std::endl;
            exit(1);
        }
    }

    void startListening() {
        while (true) {
            // 接受连接请求
            struct sockaddr_in clientAddress;
            socklen_t clientAddressLength = sizeof(clientAddress);
            int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
            if (clientSocket == -1) {
                std::cerr << "Failed to accept connection" << std::endl;
                continue;
            }

            // 接收区块数据
            char buffer[BLOCK_SIZE];
            ssize_t bytesRead = recv(clientSocket, buffer, BLOCK_SIZE, 0);
            if (bytesRead == -1) {
                std::cerr << "Failed to receive block data" << std::endl;
            } else if (bytesRead == 0) {
                std::cerr << "Connection closed by client" << std::endl;
            } else {
                // 区块数据接收成功
                std::cout << "Received block data from client: " << ip << std::endl;
                // 模拟处理区块数据的时间
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // 关闭客户端连接
            close(clientSocket);
        }
    }

    std::string ip;
    int serverSocket;
    struct sockaddr_in serverAddress;
};

int main() {
    std::vector<ServerNode> serverNodes;
    serverNodes.emplace_back("192.168.0.1");
    serverNodes.emplace_back("192.168.0.2");
    serverNodes.emplace_back("192.168.0.3");

    int blockCount = 10;  // 区块数量
    int totalPropagationTime = 0;

    for (int i = 0; i < blockCount; ++i) {
        // 创建区块数据
        char blockData[BLOCK_SIZE];
        memset(blockData, 'A', BLOCK_SIZE);

        auto startTime = std::chrono::high_resolution_clock::now();

        // 向每个服务器节点发送区块数据
        for (auto& node : serverNodes) {
            // 创建客户端套接字
            int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (clientSocket == -1) {
                std::cerr << "Failed to create socket" << std::endl;
                exit(1);
            }

            // 连接到服务器节点
            struct sockaddr_in serverAddress;
            serverAddress.sin_family = AF_INET;
            serverAddress.sin_addr.s_addr = inet_addr(node.ip.c_str());
            serverAddress.sin_port = htons(SERVER_PORT);
            if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
                std::cerr << "Failed to connect to server: " << node.ip << std::endl;
                continue;
            }

            // 发送区块数据
            ssize_t bytesSent = send(clientSocket, blockData, BLOCK_SIZE, 0);
            if (bytesSent == -1) {
                std::cerr << "Failed to send block data to server: " << node.ip << std::endl;
            } else if (bytesSent != BLOCK_SIZE) {
                std::cerr << "Incomplete block data sent to server: " << node.ip << std::endl;
            } else {
                std::cout << "Sent block data to server: " << node.ip << std::endl;
            }

            // 关闭客户端套接字
            close(clientSocket);
        }

        auto endTime = std::chrono::high_resolution_clock::now();
        auto propagationTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        totalPropagationTime += propagationTime;

        std::cout << "Block " << i+1 << " propagated to all nodes in " << propagationTime << " milliseconds" << std::endl;
    }

    double averagePropagationTime = static_cast<double>(totalPropagationTime) / blockCount;
    std::cout << "Average propagation time: " << averagePropagationTime << " milliseconds" << std::endl;

    return 0;
}

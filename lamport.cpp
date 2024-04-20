#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using namespace std;

#define NUM_TH 2
struct data_thread {
    int th_id;
    int timestamp;
    string msg;
    sockaddr_in serverAddress;
    int socketId;
};

void* ServerThread(void* arg) {
   cout<<"HI";
    int serverSocket = *((int*)arg);
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind the server socket
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    cout << "Listening" << endl;
    listen(serverSocket, 5);
      cout<<"hello";

    // Keep accepting connections and receiving events
    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        char buffer[1024] = { 0 };
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            cout << "Event received: " << buffer << endl;
        } else {
            // Handle error or connection closed
        }
    }
    pthread_exit(NULL);
}

void* ClientThread(void* arg) {
    struct data_thread* my_data = (struct data_thread*)arg;
    int clientSocket = my_data->socketId;
    sockaddr_in serverAddress = my_data->serverAddress;

    // Connect to the server
    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    // Keep sending events to the server
    while (true) {
        const char* message = "Event data";
        send(clientSocket, message, strlen(message), 0);
        sleep(1); // Delay for 1 second
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_TH];
    struct data_thread td[NUM_TH];
    int rc;
    int i;

    // Creating socket
    int socketId = socket(AF_INET, SOCK_STREAM, 0);

    // Specifying address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Create server thread
    pthread_t serverThread;
    rc = pthread_create(&serverThread, NULL, ServerThread, (void*)&socketId);
    if (rc) {
        cout << "Error: unable to create server thread," << rc << endl;
        exit(-1);
    }

    // Create client thread
    for (i = 0; i < NUM_TH - 1; i++) {
        cout << "creating thread " << i << endl;
        td[i].th_id = i;
        td[i].timestamp = i;
        td[i].msg = " message";
        td[i].serverAddress = serverAddress;
        td[i].socketId = socketId;
        rc = pthread_create(&threads[i], NULL, ClientThread, (void*)&td[i]);
        if (rc) {
            cout << "Error: unable to create thread," << rc << endl;
            exit(-1);
        }
    }

    // Wait for threads to finish
    pthread_join(serverThread, NULL);
    for (i = 0; i < NUM_TH - 1; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
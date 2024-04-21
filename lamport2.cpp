#include <iostream>
#include <cstdlib>
#include <pthread.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <csignal>
#include <algorithm>
#include <queue>
#include <semaphore.h>
#include <thread> 
#include <ctime>
using namespace std;

#define NUM_TH 2
#define PORT1 8082
#define PORT2 8081
#define PORT3 8083
#define callLimit 1
int PID = 2;
sem_t mutex;
int LamportClock = 0;
vector <int> allSockID;
priority_queue <pair<int,int>> requestQueue;
// make a binary semaphore  


struct data_thread {
    int th_id;
    int timestamp;
};

void signal_handler(int signal) {
    std::cout << "Signal received. Closing socket." << std::endl;
    for(auto i : allSockID){
        if(i>=100){
            close(i-100);
        }
        else{
            close(i);
        }
    }
    exit(signal); 
}

void incrementLogicalClock(){
    LamportClock++;
}

void pushIntoQueue(int PID,int timestamp){
    sem_wait(&mutex);
    requestQueue.push({timestamp,PID});
    sem_post(&mutex);
}

void popFromQueue(){
    sem_wait(&mutex);
    if(requestQueue.size()>0){
        requestQueue.pop();
    }
    sem_post(&mutex);
}


bool checkTurn(){
    sem_wait(&mutex);
    bool ans = requestQueue.top().second==PID;
    sem_post(&mutex);
    return ans;
}

void runCriticalSection(){
    incrementLogicalClock();
    cout<<"Critical Section\n";
    sleep(5);
    cout<<"Critical Section End\n";
    popFromQueue();
}

void parsePacket(const char* input, int* timestamp, int* pid, char* data) {
    char* token;
    char* rest = (char*)input;

    // Extract the timestamp
    token = strtok_r(rest, " ", &rest);
    if (token != NULL) {
        *timestamp = atoi(token);
    }

    // Extract the PID
    token = strtok_r(NULL, " ", &rest);
    if (token != NULL) {
        *pid = atoi(token);
    }

    // Extract the data
    if (rest != NULL) {
        strcpy(data, rest);
    }
}

int sendEvent(int clientSocket,bool replyMsg){
    incrementLogicalClock();
    string msg;
    if(replyMsg){
        msg = to_string(LamportClock) + " " + to_string(PID) + " reply_ok";
    }
    else{
        msg = to_string(LamportClock) + " " + to_string(PID) + " request_Resource";
    }
    const char* message = msg.c_str();
    int a = send(clientSocket, message, strlen(message), 0); 
    return a;
}

pair<string,int> recvEvent(int clientSocket){
    char buffer[1024] = { 0 };
    recv(clientSocket, buffer, sizeof(buffer), 0);
    cout<<buffer<<endl;
    int timestamp;
    int rpid;
    char data[1024];
    parsePacket(buffer, &timestamp, &rpid, data);
    LamportClock = max(LamportClock,timestamp)+1;
    string str(data, strlen(data));
    return {str ,rpid}; 
}

void* ServerThread(void* arg) {
    
    // creating socket 
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0); 
    allSockID.push_back(serverSocket);

    // specifying the address 
    sockaddr_in serverAddress; 
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(PORT1); 
    serverAddress.sin_addr.s_addr = INADDR_ANY; 
  
    // binding socket. 
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
  
    // listening to the assigned socket 
    listen(serverSocket, 5); 
    cout<<"server listening\n";
    // accepting connection request 
    while(1){
        int clientSocket = accept(serverSocket, nullptr, nullptr); 
        // recieving data 
        pair<string,int> data = recvEvent(clientSocket);
        if("request_Resource"==data.first){
            string msg = to_string(LamportClock) + " " + to_string(PID) + " reply_ok";
            const char* message = msg.c_str(); 
            cout<<"replied\n";
            pushIntoQueue(data.second,LamportClock);
            sendEvent(clientSocket,true);
        
        }
    }


    // closing the socket. 
    close(serverSocket);
    allSockID.erase(find(allSockID.begin(), allSockID.end(), serverSocket));
    cout<<"closed server\n";
    pthread_exit(NULL);
}

void* ClientThread(void* arg) {
    srand(std::time(0));
    for(int i=0;i<callLimit;i++){
        sleep(rand()%10);
        cout<<"client thread\n";
        int clientSocket = socket(AF_INET, SOCK_STREAM, 0); 
        allSockID.push_back(100+clientSocket);

        pushIntoQueue(PID,LamportClock);

        // specifying address 
        sockaddr_in serverAddress; 
        serverAddress.sin_family = AF_INET; 
        serverAddress.sin_port = htons(PORT2); 
        serverAddress.sin_addr.s_addr = INADDR_ANY; 
    

        int a = connect(clientSocket, (struct sockaddr*)&serverAddress,  sizeof(serverAddress)); 
        // cout<<a<<endl;
        // sending data 
        string msg = to_string(LamportClock) + " " + to_string(PID) + " request_Resource";
        const char* message = msg.c_str();

        while(1){
            int a = sendEvent(clientSocket,false);
            cout<<"Send "<<a<<endl;
            pair<string,int> data = recvEvent(clientSocket);
            if("reply_ok"==data.first){
                cout<<"got reply 2\n";
                break;
            }
        }
        
        cout<<"lampart clock "<<LamportClock<<"\n";
        // closing socket 
        close(clientSocket);
        allSockID.erase(find(allSockID.begin(), allSockID.end(), clientSocket+100));

        //send request

        int clientSocket2 = socket(AF_INET, SOCK_STREAM, 0); 
        allSockID.push_back(100+clientSocket2);
        serverAddress.sin_port = htons(PORT3); 

        connect(clientSocket2, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); 
        
        // sending data 
        
        while(1){
            string msg = to_string(LamportClock) + " " + to_string(PID) + " request_Resource";
            const char* message = msg.c_str();
            sendEvent(clientSocket,false);
            pair<string,int> data = recvEvent(clientSocket);
            if("reply_ok"==data.first){
                cout<<"got reply 3\n";
                break;
            }
        }
    
        // closing socket 
        close(clientSocket);
        allSockID.erase(find(allSockID.begin(), allSockID.end(), clientSocket+100));

        cout<<"lampart clock "<<LamportClock<<"\n";
        
        while(!checkTurn()){}
        runCriticalSection();
        cout<<"Thread exited\n";
    }
    
    pthread_exit(NULL);
}

int main() {
    sem_init(&mutex, 0, 1); 

    pthread_t threads[NUM_TH];
    struct data_thread td[NUM_TH];
    int rc;
    int i;
    
    // cout<<"Enter PID";
    // cin>>PID;

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);


    for (i = 0; i < NUM_TH; i++) {
        cout << "creating thread " << i << endl;
        td[i].th_id = i;
        td[i].timestamp = i;

        if(i==0){
            rc = pthread_create(&threads[i], NULL, ServerThread, (void*)&td[i]);
        }
        else{
            rc = pthread_create(&threads[i], NULL, ClientThread, (void*)&td[i]);
        }

        if (rc) {
            cout << "Error: unable to create thread," << rc << endl;
            exit(-1);
        }
    }



    for (i = 0; i < NUM_TH; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}
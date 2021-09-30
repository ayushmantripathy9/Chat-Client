#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

#include "client_util.hpp"

using namespace std;



void recv_thread(Client* chat_client){
    chat_client->recv_msg();
}

int main()
{

    struct sockaddr_in server_addr;

    // define the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    Client chat_client = *(new Client(server_addr));

    cout << "You have successfully joined the Chat Client." << endl;
    
    chat_client.connect_client_to_server();

    thread recv(recv_thread,&chat_client);
    chat_client.communicate_with_server();

    return 0;
}
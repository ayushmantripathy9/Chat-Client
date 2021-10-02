#include <bits/stdc++.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

#include "client_util.hpp"

#include "file_client.hpp"

using namespace std;

void recv_thread(Client *);
void communicate_with_server(Client *, FileClient *);

int main()
{

    struct sockaddr_in server_addr;

    // define the server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    Client *chat_client = new Client(server_addr);
    FileClient *file_client = new FileClient();

    cout << "You have successfully joined the Chat Client." << endl;

    file_client->recv_thread = new thread(&FileClient::receive_file, file_client);

    chat_client->connect_client_to_server();
    thread recv(recv_thread, chat_client);
    communicate_with_server(chat_client, file_client);

    return 0;
}

void recv_thread(Client *chat_client)
{
    chat_client->recv_msg();
}

void communicate_with_server(Client *chat_client, FileClient *file_client)
{

    while (true)
    {
        memset(chat_client->send_buffer, '\0', 4096);

        cout << "What operation do you want to perform ? (1 - send a message, 2 - send an attachment, 3 - show participants , 4 - exit room) : ";
        string operation;
        getline(cin, operation);

        switch (operation[0])
        {
        case '1':

            chat_client->send_user_message_to_client();
            break;

        case '2':

            file_client->get_file();
            break;

        case '3':
            cout << chat_client->participant_list << endl;
            break;

        case '4':
            chat_client->exit_app();
            file_client->leave_app();
            break;

        default:
            break;
        }
    }

    return;
}
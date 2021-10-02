#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <semaphore.h>
#include <thread>

using namespace std;
#define BUFFER_SIZE 4096

string marker = "~#`";
string SIGNAL_SEND = "#$%^";

class FileUtilClient
{
public:
    int client_socket_fd;
    struct sockaddr_in addr;
    thread *client_thread;
    sem_t file_client_semaphore;
    char recv_buffer[BUFFER_SIZE], send_buffer[BUFFER_SIZE];

    FileUtilClient(int file_server_sockfd)
    {
        // Initializing the threads and semaphore for each client
        int client_size = sizeof(addr);
        client_socket_fd = accept(file_server_sockfd, (sockaddr *)&(addr), (socklen_t *)&client_size);

        sem_init(&file_client_semaphore, 0, 1);
    }
};



class FileServer
{
public:
    int room_client_id = 0;
    unordered_map<int, FileUtilClient *> clients_list;
    struct sockaddr_in file_server_addr;

    int file_server_sockfd;

    const int port_number = 9002;
    const int MAX_CONN_REQUESTS = 5;

public:
    FileServer()
    {

        file_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        file_server_addr.sin_family = AF_INET;
        file_server_addr.sin_port = htons(port_number);

        file_server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (bind(file_server_sockfd, (sockaddr *)&file_server_addr, sizeof(file_server_addr)) < 0)
        {
            cout << "Binding unsuccessful.\n";
        }

        if (listen(file_server_sockfd, MAX_CONN_REQUESTS) < 0)
        {
            cout << "Error in conversion to passive socket\n";
        }
        cout << "Listening for connections..." << endl;
    }

    void start_file_server()
    {
        while (true)
        {
            FileUtilClient *new_client = new FileUtilClient(file_server_sockfd);
            clients_list[++room_client_id] = new_client;
            clients_list[room_client_id]->client_thread = new thread(&FileServer::file_recv_client,this,room_client_id);
        }
    }
    void leave(int client_id)
    {
        clients_list.erase(client_id);
    }

    void file_recv_client(int client_id)
    {
        FileUtilClient *client = clients_list[client_id];
        while (true)
        {
             memset(client->recv_buffer, '\0', BUFFER_SIZE);
            if (recv(client->client_socket_fd, &client->recv_buffer, sizeof(client->recv_buffer), 0) < 0)
            {
                cout << "Error in getting the message" << endl;
            }
            // pair<int, int> dest_details = decode_message(client_id);
            cout<<"h1 "<<client->recv_buffer<<" "<<client_id<<endl;
            relay_file(client_id);
        }
    }
    // dest_id<>filesize<>filename
    pair<int, int> decode_message(int sender_id)
    {
        string message = string(clients_list[sender_id]->recv_buffer);
        int ind = message.find(marker);
        string sender = message.substr(1, ind - 1);

        string rem = message.substr(ind + 3, message.size());
        ind = rem.find(marker);
        string msg_size = rem.substr(0, ind);

        int destination_id = stoi(sender);

        memset(clients_list[destination_id]->send_buffer,'\0',BUFFER_SIZE);

        rem = rem + marker + to_string(sender_id); // append sender's id as well

        sem_wait(&clients_list[destination_id]->file_client_semaphore);
        for (int i = 0; i < rem.size(); ++i)
        {
            clients_list[destination_id]->send_buffer[i] = rem[i];
        }

        return {stoi(sender), stoi(msg_size)};
    }

    void relay_file(int source_id)
    {

        FileUtilClient *source = clients_list[source_id];

        pair<int, int> details = decode_message(source_id);
        int dest_id = details.first;
        int size_of_file = details.second;

        FileUtilClient *dest = clients_list[dest_id];

        memset(source->recv_buffer, '\0' , BUFFER_SIZE);
        for (int i = 0; i < sizeof(SIGNAL_SEND); ++i)
        {
            source->recv_buffer[i] = SIGNAL_SEND[i];
        }
      
        send(dest->client_socket_fd, &dest->send_buffer, BUFFER_SIZE, 0);
        cout<<"Message sent: "<<&dest->send_buffer<<endl;

        send(source->client_socket_fd, &source->recv_buffer, BUFFER_SIZE, 0);
        cout<<"Acknowledgement sent: "<<&source->recv_buffer<<endl;
        
        
          
        int size_transferred = 0;
        while (size_transferred < size_of_file)
        {
            memset(&source->recv_buffer, '\0', BUFFER_SIZE);
            memset(&dest->send_buffer, '\0', BUFFER_SIZE);

            if (recv(source->client_socket_fd, &source->recv_buffer, sizeof(source->recv_buffer), 0) < 0)
            {
                cout << "An error occured while retrieving data from client."<< endl;
                continue;
            }

            for (int i = 0; i < sizeof(source->recv_buffer); ++i)
            {
                dest->send_buffer[i] = source->recv_buffer[i];
            }

            if (send(dest->client_socket_fd, &dest->send_buffer, sizeof(dest->send_buffer), 0) < 0)
            {
                cout << "An error occured while sending data to client."<< endl;
                continue;
            }
            
            size_transferred += BUFFER_SIZE;
        }

        sem_post(&(dest->file_client_semaphore));
    }
};

int main(){
    FileServer* FS = new FileServer();
    FS->start_file_server();
}
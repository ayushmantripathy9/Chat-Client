#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <semaphore.h>
#include <thread>

using namespace std;

class Client
{
public:
    string name;
    

    int client_socket_fd;

    struct sockaddr_in addr;
    char recv_buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE];

    vector<thread *> client_threads;
    sem_t client_semaphore;

    Client(int server_sockfd)
    {
        // Initializing the threads and semaphore for each client
        int client_size = sizeof(addr);
        client_socket_fd = accept(server_sockfd, (sockaddr *)&(addr), (socklen_t *)&client_size);

        client_threads = vector<thread *>(2);
        sem_init(&client_semaphore, 0, 1);
    }

    /**
     * Parses client encoded msg to get the client message
     * @param sender_id client_id of the message sender
     * @return string containing client message
     * */
    string get_client_message()
    {
        string s(this->recv_buffer);

        int marker_ind = s.find(marker);

        return s.substr(marker_ind + 3, s.size());
    }

    /**
     * Finds the receiver of the message after decoding incoming messages.
     * @param sender_id client_id of the message sender (as that buffer would contain the message sent)
     * @return receiver_detals : pair of msg_type and receiver_id
     * */

    // c+g+GrpName+marker
    pair<char, string> find_receiver()
    {
        pair<char, string> receiver_details;

        string s(this->recv_buffer);
        receiver_details.first = s[0];

        int marker_ind = s.find(marker);

        receiver_details.second = s.substr(1, marker_ind - 1);
        return receiver_details;
    }
};

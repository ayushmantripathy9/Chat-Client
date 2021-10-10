#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <semaphore.h>
#include <thread>

using namespace std;

#include "client_util.hpp"
#include "parsers.hpp"
#include "group_util.hpp"

class Server : public GroupUtil
{
public:
    const int MAX_CONN_REQUESTS = 5;
    int room_client_id = 0;

    char participant_list_buffer[BUFFER_SIZE];

    int server_sockfd;
    struct sockaddr_in server_addr;

    sem_t participant_buffer_semaphore;

    Server()
    {

        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(9001);

        server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (bind(server_sockfd, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        {
            cout << "Binding unsuccessful.\n";
        }

        if (listen(server_sockfd, MAX_CONN_REQUESTS) < 0)
        {
            cout << "Error in conversion to passive socket\n";
        }
        cout << "Messaging Server Ready.\nListening for connections..." << endl;
        memset(participant_list_buffer, '\0', BUFFER_SIZE);
    }

    /**
     * Thread function to handle message sending to a client via a socket
     * @param client_id
     * @return NULL
     * */
    void send_participant_list(int client_id)
    {

        sem_wait(&(clients_list[client_id]->client_semaphore));
        if (send(clients_list[client_id]->client_socket_fd, participant_list_buffer, sizeof(participant_list_buffer), 0) < 0)
        {
            cout << "Failed to send msg to client : " << participants_name[client_id] << endl;
        }
        sem_post(&(clients_list[client_id]->client_semaphore));

        return;
    }

    /**
     * Handles joining of a new client. Updates participant list and notifies.
     * @param client_id new participant's client_id
     * @return NULL
     * */
    void new_client_joined(int client_id)
    {
        int ind = 0;
        while (participant_list_buffer[ind] != '\0')
            ++ind;
        if (clients_list.size() == 1)
        {
            memset(participant_list_buffer,'\0',BUFFER_SIZE); 
            ind = 0;
            string starter_code = "p" + marker;
            for (int i = 0; i < starter_code.size(); ++i)
                participant_list_buffer[ind++] = starter_code[i];
        }
        string start_of_list;
        start_of_list = "\no";

        string new_client = start_of_list + to_string(client_id) + " : " + participants_name[client_id];

        for (int i = 0; i < new_client.size(); ++i)
            participant_list_buffer[ind++] = new_client[i];   
    }

    /**
     * Handles participant leave. Closes threads and updates participant list
     * @param client_id client_id of the leaving participant
     * @return NULL
     * */
    void participant_left(int client_id)
    {
        participants_name.erase(client_id);

        clients_list.erase(client_id);

        string participant_list_msg = "p" + marker;

        for (auto i : participants_name)
            participant_list_msg += "o" + to_string(i.first) + " : " + i.second + "\n";

        participant_list_msg.pop_back();

        memset(participant_list_buffer, '\0', BUFFER_SIZE);

        int ind = 0;
        for (int i = 0; i < participant_list_msg.size(); ++i)
            participant_list_buffer[ind++] = participant_list_msg[i];
    }

    /**
        * Notifies all the clients with the updated participant list
        * @param None
    * */
    void participant_notification()
    {
        int number_of_clients = participants_name.size();

        int ind = 0;

        int receiver_ids[number_of_clients];
        for (auto i : participants_name)
            receiver_ids[ind++] = i.first;

        vector<thread *> notif_threads;
        for (int i = 0; i < number_of_clients; ++i)
        {
            notif_threads.push_back(new thread(&Server::send_participant_list, this, receiver_ids[i]));
        }

        for (int i = 0; i < notif_threads.size(); ++i)
            notif_threads[i]->join();
    }

    /**
    * Sends the msg in the client's send buffer to a specified client
    * @param send_details vector specifying receive, sender and file_name header.
    * @return NULL
    * */
    void send_message_to_client(vector<int> send_details)
    {
        int receiver_id = send_details[0], sender_id = send_details[1], header_id = send_details[2];

        memset(clients_list[receiver_id]->send_buffer, '\0', BUFFER_SIZE);
        encode_send_message(sender_id, receiver_id, header_id, participants_name, clients_list);

        sem_wait(&clients_list[receiver_id]->client_semaphore);
        send(clients_list[receiver_id]->client_socket_fd, clients_list[receiver_id]->send_buffer, sizeof(clients_list[receiver_id]->send_buffer), 0);
        sem_post(&clients_list[receiver_id]->client_semaphore);

        return;
    }

    /**
     * Thread function to handle incoming messages from the client and their redirection using send threads.
     * @param client_id client_id of the participant from whom the thread is to reecive msg from
     * */
    void recv_msg(int client_id)
    {
        int client_sockfd = clients_list[client_id]->client_socket_fd;

        // get client's name for storing
        if (recv(client_sockfd, clients_list[client_id]->recv_buffer, sizeof(clients_list[client_id]->recv_buffer), 0) < 0)
        {
            cout << "Wrong Input Name" << endl;
        }

        string client_name = clients_list[client_id]->get_client_message();
        participants_name[client_id] = client_name;
        cout << "New Client \"" << client_name << "\" joined !!" << endl;

        // Update the participant list and notify everyone in the room
        new_client_joined(client_id);
        message_group[0].insert(client_id);
        this->participant_notification();

        // group_list to new client joined
        group_list_to_client_joined(client_id);

        while (true)
        {
            // reset the buffer for new msg
            memset(clients_list[client_id]->recv_buffer, '\0', BUFFER_SIZE);

            if (recv(client_sockfd, clients_list[client_id]->recv_buffer, sizeof(clients_list[client_id]->recv_buffer), 0) < 0)
            {
                cout << "Wrong Msg Input" << endl;
                continue;
            }

            // finding the receiver of the message
            pair<char, string> receiver_details = clients_list[client_id]->find_receiver();
            char receiver_type = receiver_details.first;

            int receiver_id = 0;
            if (receiver_details.second[0] != 'g')
                receiver_id = stoi(receiver_details.second);

            memset(clients_list[client_id]->send_buffer, '\0', BUFFER_SIZE);

            if (receiver_type == 'o')
            {

                vector<int> self_details = {client_id, client_id, receiver_id}, friend_details = {receiver_id, client_id, client_id};

                thread self_thread(&Server::send_message_to_client, this, self_details);
                thread friend_thread(&Server::send_message_to_client, this, friend_details);

                self_thread.join();
                friend_thread.join();
            }
            else if (receiver_type == 'c')
            {
                create_group(receiver_details.second.substr(1, receiver_details.second.size()), client_id);
            }
            else if (receiver_type == 'j')
            {
                // implement grp msg using multicast
                string gid = receiver_details.second.substr(1, receiver_details.second.size());
                join_group(stoi(gid), client_id);
            }
            else if (receiver_type == 'g')
            {
                string msg(clients_list[client_id]->recv_buffer);
                int ind = msg.find(marker);
                msg = msg.substr(ind + 3, msg.size());
                send_message_to_group(msg, client_id, receiver_id);
            }
            else if (receiver_type == 's')
            {
                encode_and_send_group_member_list(receiver_id, client_id);
            }
            else if (receiver_type == 'l')
            {
                // Remove user from a group
                leave_group(receiver_details.second, client_id);
            }
            else if (receiver_type == 'e')
            {   
                Client* client = clients_list[client_id];

                sem_wait(&client->client_semaphore);

                memset(client->send_buffer, '\0' , BUFFER_SIZE);
                string exit_msg = "e"+marker+"o"+to_string(client_id);
                for(int i = 0 ; i < exit_msg.size() ; ++i){
                    client->send_buffer[i] = exit_msg[i];
                } 

                if(send(client_sockfd, clients_list[client_id]->send_buffer, BUFFER_SIZE , 0) < 0){
                    cout<<"Error in exiting the client."<<endl;
                    return;
                }
                sem_post(&client->client_semaphore);

                participant_left(client_id);
                this->participant_notification();
                close(client_sockfd);
                return;
            }
            else
            {
                cout << "Wrong Msg Type" << endl;
                // notify client
            }
        }

        close(client_sockfd);
        return;
    }

    void start_server()
    {

        while (true)
        {
            Client *new_client = new Client(server_sockfd);
            clients_list[++room_client_id] = new_client;

            clients_list[room_client_id]->client_threads[0] = (new thread(&Server::recv_msg, this, room_client_id));
        }
    }
};
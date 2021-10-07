#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <semaphore.h>
#include <thread>

using namespace std;

class GroupUtil
{

public:
    unordered_map<int, Client *> clients_list;
    unordered_map<int, string> participants_name;

    unordered_map<int, set<int>> message_group;
    unordered_map<int, string> group_names;

    int assign_group_id = 0;

    void create_group(string group_name, int client_id)
    {
        message_group[++assign_group_id].insert(client_id);

        int gid = assign_group_id;
        group_names[gid] = group_name;

        string msg = "a" + marker + "g" + to_string(gid) + marker + group_name;

        sem_wait(&(clients_list[client_id]->client_semaphore));

        memset(clients_list[client_id]->send_buffer, '\0', BUFFER_SIZE);
        for (int i = 0; i < msg.size(); i++)
        {
            clients_list[client_id]->send_buffer[i] = msg[i];
        }
        send(clients_list[client_id]->client_socket_fd, clients_list[client_id]->send_buffer, BUFFER_SIZE, 0);

        sem_post(&clients_list[client_id]->client_semaphore);

        msg = "n" + marker + "g" + to_string(gid) + marker + group_name;

        vector<thread *> thread_send;
        for (auto i : clients_list)
        {
            thread_send.push_back(new thread(&GroupUtil::send_message_to_all, this, i.first, msg));
        }
        for (auto i : thread_send)
            i->join();
    }

    void send_message_to_all(int receiver_id, string msg)
    {
        sem_wait(&clients_list[receiver_id]->client_semaphore);

        memset(clients_list[receiver_id]->send_buffer, '\0', BUFFER_SIZE);
        for (int i = 0; i < msg.size(); i++)
        {
            clients_list[receiver_id]->send_buffer[i] = msg[i];
        }
        send(clients_list[receiver_id]->client_socket_fd, &clients_list[receiver_id]->send_buffer, BUFFER_SIZE, 0);

        sem_post(&clients_list[receiver_id]->client_semaphore);
    }

    void join_group(int group_id, int client_id)
    {
        message_group[group_id].insert(client_id);
    }

    void encode_and_send_group_member_list(int group_id, int client_id)
    {
        Client *client = clients_list[client_id];
        sem_wait(&client->client_semaphore);

        memset(client->send_buffer, '\0', BUFFER_SIZE);

        int ind = 0;
        string header = "gml" + marker + "Members in group \"" + to_string(group_id) + " - " + group_names[group_id] + "\" are:\n";

        for (int i = 0; i < header.size(); ++i)
            client->send_buffer[ind++] = header[i];

        for (auto i : message_group[group_id])
        {
            client->send_buffer[ind++] = 'o';
            string participant = to_string(i) + " : " + participants_name[i] + "\n";
            for (int j = 0; j < participant.size(); ++j)
            {
                client->send_buffer[ind++] = participant[j];
            }
        }

        if (send(client->client_socket_fd, client->send_buffer, BUFFER_SIZE, 0) < 0)
        {
            cout << "Error in sending the group member list to client." << endl;
            return;
        }

        sem_post(&client->client_semaphore);
    }

    void leave_group(string group_info, int client_id)
    {
        string gid = group_info.substr(1, group_info.size());
        int group_id = stoi(gid);

        message_group[group_id].erase(client_id);
    }

    //  g+marker+msg_received_by_client+marker+gid (msg_received_by_client = sender_name : msg)
    void encode_group_msg_to_client(int sender_id, int receiver_id, string msg, int group_id)
    {

        string message = "g" + marker + participants_name[sender_id] + " : " + msg + marker + "g" + to_string(group_id);
        memset(clients_list[receiver_id]->send_buffer, '\0', BUFFER_SIZE);

        for (int i = 0; i < message.size(); ++i)
        {
            clients_list[receiver_id]->send_buffer[i] = message[i];
        }
        return;
    }

    void send_message_to_group_member(int receiver_id, int sender_id, string msg, int group_id)
    {
        sem_wait(&clients_list[receiver_id]->client_semaphore);

        encode_group_msg_to_client(sender_id, receiver_id, msg, group_id);
        send(clients_list[receiver_id]->client_socket_fd, &clients_list[receiver_id]->send_buffer, BUFFER_SIZE, 0);
        sem_post(&clients_list[receiver_id]->client_semaphore);
    }

    void send_message_to_group(string msg, int sender_id, int group_id)
    {
        vector<thread *> group_send_threads;
        for (auto i : message_group[group_id])
        {
            group_send_threads.push_back(new thread(&GroupUtil::send_message_to_group_member, this, i, sender_id, msg, group_id));
        }
        for (int i = 0; i < group_send_threads.size(); ++i)
        {
            group_send_threads[i]->join();
        }
    }

    void group_list_to_client_joined(int client_id)
    {
        string msg = "gl" + marker;
        for (auto i : group_names)
        {
            msg = msg + "g" + to_string(i.first) + marker + i.second + marker;
        }
        msg = msg + "end";

        sem_wait(&(clients_list[client_id]->client_semaphore));

        memset(clients_list[client_id]->send_buffer, '\0', BUFFER_SIZE);
        for (int i = 0; i < msg.size(); i++)
        {
            clients_list[client_id]->send_buffer[i] = msg[i];
        }

        send(clients_list[client_id]->client_socket_fd, clients_list[client_id]->send_buffer, BUFFER_SIZE, 0);

        sem_post(&clients_list[client_id]->client_semaphore);
    }
};

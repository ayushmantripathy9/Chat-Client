#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <semaphore.h>
#include <thread>

using namespace std;


string marker = "~#`";

class Client
{
public:
    string name;

    int client_socket_fd;

    struct sockaddr_in addr;
    char recv_buffer[4096];
    char send_buffer[4096];

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
    pair<char, int> find_receiver()
    {
        pair<char, int> receiver_details;

        string s(this->recv_buffer);
        receiver_details.first = s[0];

        int marker_ind = s.find(marker);
        receiver_details.second = stoi(s.substr(1, marker_ind));
        return receiver_details;
    }
};

class Server
{
public:
    const int MAX_CONN_REQUESTS = 5;
    int room_client_id = 0;
    
    unordered_map<int, Client *> clients_list;
    unordered_map<int, string> participants_name;
    char participant_list_buffer[4096];

    int server_sockfd;
    struct sockaddr_in server_addr;

    sem_t participant_buffer_semaphore;

public:
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
        cout << "Listening for connections..." << endl;
        memset(participant_list_buffer, '\0', 4096);
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

        if (ind == 0)
        {
            string starter_code = "p" + marker;
            for (int i = 0; i < starter_code.size(); ++i)
                participant_list_buffer[ind++] = starter_code[i];
        }

        string start_of_list;
        if (participants_name.size() != 1)
            start_of_list = "\no";
        else
            start_of_list = "o";

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

        memset(participant_list_buffer, '\0', 4096);

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
    * Encodes the message to be sent to the clients
    * 
    * Encoding Scheme: type + marker + message + marker + header
    *      - type : m => message, p => participant list
    *      - message: actual message to be sent
    *      - header ( = header_name + header_id ): used to identify the file the message is to be written
    *      - marker: dummy string to separate parts (marker = ~#`)
    * 
    * @param sender_id client_id of the sender
    * @param receiver_id client_id of the receiver
    * @param header_id file_name in which the message would be stored at the client side
    * */
    void encode_send_message(int sender_id, int receiver_id, int header_id)
    {
        string sender_name = participants_name[sender_id];
        string sender_message = clients_list[sender_id]->get_client_message();
        string header_name = participants_name[header_id];

        string send_message = "m" + marker + sender_name + " : " + sender_message + marker + header_name + "_" + to_string(header_id);

        for (int i = 0; i < send_message.size(); ++i)
            clients_list[receiver_id]->send_buffer[i] = send_message[i];
    }

    /**
    * Sends the msg in the client's send buffer to a specified client
    * @param send_detals vector specifying receive, sender and file_name header.
    * @return NULL
    * */
    void send_message_to_client(vector<int> send_details)
    {
        int receiver_id = send_details[0], sender_id = send_details[1], header_id = send_details[2];

        memset(clients_list[receiver_id]->send_buffer, '\0', 4096);
        encode_send_message(sender_id, receiver_id, header_id);

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
        cout << "New Client \"" << clients_list[client_id]->recv_buffer << "\" joined !!" << endl;
        string client_name = clients_list[client_id]->get_client_message();
        participants_name[client_id] = client_name;

        // Update the participant list and notify everyone in the room
        new_client_joined(client_id);
        this->participant_notification();

        while (true)
        {
            // reset the buffer for new msg
            memset(clients_list[client_id]->recv_buffer, '\0', 4096);

            if (recv(client_sockfd, clients_list[client_id]->recv_buffer, sizeof(clients_list[client_id]->recv_buffer), 0) < 0)
            {
                cout << "Wrong Msg Input" << endl;
                // notify client
                continue;
            }

            // finding the receiver of the message
            pair<char, int> receiver_details = clients_list[client_id]->find_receiver();
            char receiver_type = receiver_details.first;
            int receiver_id = receiver_details.second;

            memset(clients_list[client_id]->send_buffer, '\0', 4096);

            if (receiver_type == 'o')
            {

                vector<int> self_details = {client_id, client_id, receiver_id}, friend_details = {receiver_id, client_id, client_id};

                thread self_thread(&Server::send_message_to_client, this, self_details);
                thread friend_thread(&Server::send_message_to_client, this, friend_details);

                self_thread.join();
                friend_thread.join();
            }
            else if (receiver_type == 'g')
            {
                // implement grp msg using multicast
            }
            else if (receiver_type == 'e')
            {
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

            clients_list[room_client_id]->client_threads[0]=(new thread(&Server::recv_msg, this, room_client_id));
        }
    }
};

int main()
{

    Server server_instance = *(new Server());
    server_instance.start_server();

    return 0;
}
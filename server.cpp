#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <pthread.h>
#include <semaphore.h>

using namespace std;

const int MAX_CONN_REQUESTS = 5;
int room_client_id = 0;
string marker = "~#`";

unordered_map<int, int> client_socket_fd;
unordered_map<int, char[4096]> recv_buffer, send_buffer;

unordered_map<int, vector<pthread_t>> client_threads;
unordered_map<int, sem_t> client_semaphore;

sem_t participant_buffer_semaphore;
unordered_map<int, string> participants_name;
char participant_list_buffer[4096];

void *recv_msg(void *);

int main()
{
    int server_sockfd, client_sockfd;
    struct sockaddr_in server_addr, client_addr;
    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);

    // Binding the server to an IP and PORT
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(9001);

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

    int client_size = sizeof(client_addr);
    while (true)
    {
        client_sockfd = accept(server_sockfd, (sockaddr *)&client_addr, (socklen_t *)&client_size);
        client_socket_fd[++room_client_id] = client_sockfd;

        // Initializing the threads and semaphore for each client  
        client_threads[room_client_id] = vector<pthread_t>(2);
        int curr_client_id = room_client_id;
        sem_init(&client_semaphore[room_client_id], 0, 1);

        // The thread to all msgs from client and send to other clients
        pthread_create(&client_threads[room_client_id][0], NULL, recv_msg, (void *)&curr_client_id);
    }

    close(server_sockfd);
    return 0;
}

/**
 * Thread function to handle message sending to a client via a socket
 * @param client_id
 * @return NULL
 * */

void *send_participant_list(void *arg)
{
    int client_id = *((int *)arg);

    sem_wait(&client_semaphore[client_id]);
    if (send(client_socket_fd[client_id], participant_list_buffer, sizeof(participant_list_buffer), 0) < 0){
        cout << "Failed to send msg to client : " << participants_name[client_id] << endl;
    }
    sem_post(&client_semaphore[client_id]);

    return NULL;
}

/**
 * Notifies all the clients with the updated participant list
 * @param None
 * */
void participant_notification(){
    int number_of_clients = participants_name.size();
    pthread_t clients[number_of_clients];
    int ind = 0;

    int receiver_ids[number_of_clients];
    for(auto i : participants_name)
        receiver_ids[ind++] = i.first;

    for (int i = 0; i < number_of_clients ; ++i)
    {
        pthread_create(&clients[i], NULL, send_participant_list, (void *)&receiver_ids[i]);
    }

    for (int i = 0; i < number_of_clients; ++i)
        pthread_join(clients[i], NULL);
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
    
    participant_notification();

}

/**
 * Handles participant leave. Closes threads and updates participant list
 * @param client_id client_id of the leaving participant
 * @return NULL
 * */
void participant_left(int client_id)
{   
    participants_name.erase(client_id);

    client_socket_fd.erase(client_id);
    client_semaphore.erase(client_id);

    send_buffer.erase(client_id);
    recv_buffer.erase(client_id);

    client_threads.erase(client_id);

    string participant_list_msg = "p" + marker;

    for(auto i: participants_name)
        participant_list_msg += "o"+to_string(i.first)+" : "+i.second+"\n";
    
    participant_list_msg.pop_back();

    memset(participant_list_buffer,'\0',4096);

    int ind = 0;
    for(int i = 0 ; i < participant_list_msg.size() ; ++i)
        participant_list_buffer[ind++] = participant_list_msg[i];

    participant_notification();  

}

/**
 * Parses client encoded msg to get the client message
 * @param sender_id client_id of the message sender
 * @return string containing client message
 * */
string get_client_message(int sender_id)
{
    string s(recv_buffer[sender_id]);

    int marker_ind = s.find(marker);
    return s.substr(marker_ind + 3, s.size());
}


/**
 * Finds the receiver of the message after decoding incoming messages.
 * @param sender_id client_id of the message sender (as that buffer would contain the message sent)
 * @return receiver_detals : pair of msg_type and receiver_id
 * 
 * */
pair<char, int> find_receiver(int sender_id)
{
    pair<char, int> receiver_details;

    string s(recv_buffer[sender_id]);
    receiver_details.first = s[0];

    int marker_ind = s.find(marker);
    receiver_details.second = stoi(s.substr(1, marker_ind));
    return receiver_details;
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
    string sender_message = get_client_message(sender_id);
    string header_name = participants_name[header_id];

    string send_message = "m" + marker + sender_name + " : " + sender_message + marker + header_name + "_" + to_string(header_id);

    for (int i = 0; i < send_message.size(); ++i)
        send_buffer[receiver_id][i] = send_message[i];
}

/**
 * Sends the msg in the client's send buffer to a specified client
 * @param send_detals vector specifying receive, sender and file_name header.
 * @return NULL
 * */
void *send_message_to_client(void *arg)
{
    vector<int> send_details = *((vector<int> *)arg);
    int receiver_id = send_details[0], sender_id = send_details[1], header_id = send_details[2];

    memset(send_buffer[receiver_id], '\0', 4096);
    encode_send_message(sender_id, receiver_id, header_id);

    sem_wait(&client_semaphore[receiver_id]);
    send(client_socket_fd[receiver_id], send_buffer[receiver_id], sizeof(send_buffer[receiver_id]), 0);
    sem_post(&client_semaphore[receiver_id]);

    return NULL;
}

/**
 * Thread function to handle incoming messages from the client and their redirection using send threads.
 * @param client_id client_id of the participant from whom the thread is to reecive msg from
 * */
void *recv_msg(void *arg)
{
    int client_id = *((int *)arg);
    int client_sockfd = client_socket_fd[client_id];

    // get client's name for storing
    if (recv(client_sockfd, recv_buffer[client_id], sizeof(recv_buffer[client_id]), 0) < 0)
    {
        cout << "Wrong Input Name" << endl;
    }
    string client_name = get_client_message(client_id);
    participants_name[client_id] = client_name;

    // Update the participant list and notify everyone in the room 
    new_client_joined(client_id);
    cout << "New Client \"" << client_name << "\" joined !!" << endl;

    while (true)
    {
        // reset the buffer for new msg
        memset(recv_buffer[client_id], '\0', 4096);

        if (recv(client_sockfd, recv_buffer[client_id], sizeof(recv_buffer[client_id]), 0) < 0)
        {
            cout << "Wrong Msg Input" << endl;
            // notify client
            continue;
        }

        // finding the receiver of the message
        pair<char, int> receiver_details = find_receiver(client_id);
        char receiver_type = receiver_details.first;
        int receiver_id = receiver_details.second;

        memset(send_buffer[client_id], '\0', 4096);

        if (receiver_type == 'o')
        {
            pthread_t self_t, friend_t;

            vector<int> self_details = {client_id, client_id, receiver_id}, friend_details = {receiver_id, client_id, client_id};
            pthread_create(&self_t, NULL, send_message_to_client, (void *)&self_details);
            pthread_create(&friend_t, NULL, send_message_to_client, (void *)&friend_details);

            pthread_join(self_t, NULL);
            pthread_join(friend_t, NULL);
        }
        else if (receiver_type == 'g')
        {
            // implement grp msg using multicast
        }
        else if (receiver_type == 'e')
        {   
            participant_left(client_id);
            close(client_sockfd);
            return NULL;
        }
        else
        {
            cout << "Wrong Msg Type" << endl;
            // notify client
        }
    }

    close(client_socket_fd[client_id]);
    return NULL;
}

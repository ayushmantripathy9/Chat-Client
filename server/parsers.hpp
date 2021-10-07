#include <iostream>
#include <bits/stdc++.h>

using namespace std;

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
void encode_send_message(int sender_id, int receiver_id, int header_id, unordered_map<int, string> &participants_name, unordered_map<int, Client *> &clients_list)
{
    string sender_name = participants_name[sender_id];
    string sender_message = clients_list[sender_id]->get_client_message();
    string header_name = participants_name[header_id];

    string send_message = "m" + marker + sender_name + " : " + sender_message + marker + header_name + "_" + to_string(header_id);

    for (int i = 0; i < send_message.size(); ++i)
        clients_list[receiver_id]->send_buffer[i] = send_message[i];
}
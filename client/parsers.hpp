#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for closing the socket
#include <sys/stat.h>

using namespace std;


/**
 * Encodes the message to be sent to the server. 
 * 
 * Message Types:
 * - n -> name
 * - o,id -> one to one msg
 * - g,id -> group msg
 * - e -> exit
 * @param id string containing receiver_id
 * @param message message to be sent
 * @return NULL
* */
void get_encoded_msg(string id, string message, char send_buffer[])
{

    memset(send_buffer, '\0', 4096);
    int ind = 0;
    for (int i = 0; i < id.size(); ++i)
        send_buffer[ind++] = id[i];

    // marker for msg breaks = ~#`
    send_buffer[ind++] = '~';
    send_buffer[ind++] = '#';
    send_buffer[ind++] = '`';

    for (int i = 0; i < message.size(); ++i)
        send_buffer[ind++] = message[i];
}

/**
 * Parses and decodes the incoming message from the server
 * @param None
 * @return vector of msg_type, msg and header(Required for file_name. Absent in case of notification msgs)
 * */
vector<string> decode_msg(char recv_buffer[])
{
    string raw_msg(recv_buffer);

    int marker_ind = raw_msg.find(marker);
    string type = raw_msg.substr(0, marker_ind);
    string remaining = raw_msg.substr(marker_ind + 3, raw_msg.size());

    if (type == "m" || type == "g" || type == "a" || type == "n")
    {
        marker_ind = remaining.find(marker);

        string msg = remaining.substr(0, marker_ind);
        string header = remaining.substr(marker_ind + 3, remaining.size());
        return {type, msg, header};
    }
    else if (type == "p" || type == "gl" || type == "gml" || type == "e")
    {
        return {type, remaining};
    }

    return {};
}
#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for closing the socket
#include <sys/stat.h>

using namespace std;

void decode_group_list(unordered_map<string, string> &group_list, string &received_msg)
{
    received_msg = marker + received_msg;
    while (true)
    {
        received_msg = received_msg.substr(received_msg.find(marker) + 3, received_msg.size());

        if (received_msg == "end")
        {
            break;
        }

        int index = received_msg.find(marker);

        string gid = received_msg.substr(0, index);
        received_msg = received_msg.substr(index + 3, received_msg.size());

        index = received_msg.find(marker);
        string gname = received_msg.substr(0, index);
        group_list[gid] = gname;
        received_msg = received_msg.substr(index, received_msg.size());
    }
}

/**
 * Utility function to send a message string to server
 * */
void send_msg_to_server(string msg, char send_buffer[], int client_sockfd)
{
    memset(send_buffer, '\0', BUFFER_SIZE);
    for (int i = 0; i < msg.size(); ++i)
        send_buffer[i] = msg[i];

    if (send(client_sockfd, send_buffer, BUFFER_SIZE, 0) < 0)
    {
        cout << "Error in creating new group" << endl;
        return;
    }
}

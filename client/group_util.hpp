#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for closing the socket
#include <sys/stat.h>

using namespace std;

class GroupUtil
{
public:
    unordered_map<string, string> joined_group; // gid, participant_list
    unordered_map<string, string> group_list;   // gid, name
    char recv_buffer[BUFFER_SIZE], send_buffer[BUFFER_SIZE];
    int client_sockfd;

    GroupUtil(struct sockaddr_in server_add)
    {
        client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    }

    void create_new_group()
    {
        cout << "Enter group name: ";
        string group_name;
        getline(cin, group_name);

        string msg = "cg" + group_name + marker;

        send_msg_to_server(msg, send_buffer, client_sockfd);

        return;
    }

    void join_group()
    {
        cout << "Enter group_id: ";
        string group_id;
        getline(cin, group_id);

        string msg = "j" + group_id + marker;

        send_msg_to_server(msg, send_buffer, client_sockfd);

        joined_group[group_id] = group_list.at(group_id);
        return;
    }

    void print_group_list()
    {
        string message = "Group List: \n";
        for (auto i : group_list)
        {
            message += (i.first + " : " + i.second + "\n");
        }
        cout << message;
    }

    void send_message_to_group()
    {
        string group_id;
        cout << "Enter group_id: ";
        getline(cin, group_id);

        string msg;
        cout << "Enter message: ";
        getline(cin, msg);

        string send_msg = group_id + marker + msg;
        send_msg_to_server(send_msg, send_buffer, client_sockfd);
        return;
    }
    void leave_group()
    {
        cout << "Enter group_id : ";
        string group_id;
        getline(cin, group_id);

        string msg = "l" + group_id + marker;

        send_msg_to_server(msg, send_buffer, client_sockfd);
        joined_group.erase(group_id);

        return;
    }
};
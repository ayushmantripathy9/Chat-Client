#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for closing the socket

#include <sys/stat.h>

#include "util.hpp"
#include "parsers.hpp"
#include "group_util.hpp"

using namespace std;

#define BUFFER_SIZE 4096

class Client : public GroupUtil
{
public:
    string participant_list;
    string client_name, text_message, recepient_id;

    unordered_map<string, string> clients_list;

    bool ack_marker = true, exit_wait = true;

    Client(struct sockaddr_in server_addr) : GroupUtil(server_addr)
    {

        if ((connect(client_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
        {
            cout << "Connection to server unsuccessful" << endl;
            return;
        }
    }

    void update_clients_list()
    {
        clients_list.clear();

        string new_list = participant_list + "\n";
        string member;

        int ind;    

        while (true)
        {

            new_list = new_list.substr(1, new_list.size()); 

            if (new_list.find("\n") == string::npos)
            {
                break;
            }
            else
            {
                ind = new_list.find("\n");
                member = new_list.substr(0, ind);
                new_list = new_list.substr(ind, new_list.size());
            }

            ind = member.find(":");
            clients_list[member.substr(0, ind - 1)] = member.substr(ind + 2, member.size());
        }
    }

    void group_members()
    {
        cout << "Enter group id, whose members you want to see: ";

        string group_id;
        getline(cin, group_id);

        if (group_list.find(group_id) == group_list.end())
        {
            cout << "Entered group_id is incorrect. No such group exists!!" << endl;
            return;
        }

        group_id = group_id.substr(1, group_id.size());
        string list_msg = "s" + group_id + marker;

        memset(send_buffer, '\0', BUFFER_SIZE);
        for (int i = 0; i < list_msg.size(); ++i)
            send_buffer[i] = list_msg[i];

        if (send(client_sockfd, send_buffer, BUFFER_SIZE, 0) < 0)
        {
            cout << "Error in sending group_member list msg to the server." << endl;
            return;
        }

        while (ack_marker)
        {
            //  WAIT FOR MESSAGE TO BE RECEIVED  //
        }
        ack_marker = true;

        return;
    }

    /**
         * Thread function to receive msg from the server
         * @param client_sockfd Socket file descriptor of the client
         * @return NULL
         * */
    void recv_msg()
    {
        mkdir("messages", 0777);
        mkdir("messages/user_chat_log", 0777);
        mkdir("messages/group_chat_log", 0777);

        while (true)
        {
            if (recv(client_sockfd, recv_buffer, sizeof(recv_buffer), 0) < 0)
            {
                cout << "Can't receive the message. Try Again.\n";
            }

            vector<string> received_msg = decode_msg(recv_buffer);

            if (received_msg[0] == "m")
            {
                std::ofstream write_to_file;
                write_to_file.open("./messages/user_chat_log/" + received_msg[2] + ".txt", std::ios::app);
                write_to_file << received_msg[1] << "\n";
                write_to_file.close();
            }
            else if (received_msg[0] == "p")
            {
                participant_list = received_msg[1];
                update_clients_list();
            }
            else if (received_msg[0] == "g")
            {
                std::ofstream write_to_file;
                write_to_file.open("./messages/group_chat_log/" + received_msg[2] + ".txt", std::ios::app);
                write_to_file << received_msg[1] << "\n";
                write_to_file.close();
            }
            else if (received_msg[0] == "a")
            {
                joined_group[received_msg[1]] = received_msg[2];
            }
            else if (received_msg[0] == "gl")
            {

                decode_group_list(group_list, received_msg[1]);
            }
            else if (received_msg[0] == "gml")
            {
                cout << received_msg[1];
                ack_marker = false;
            }
            else if (received_msg[0] == "n")
            {
                group_list[received_msg[1]] = received_msg[2];
            }
            else if (received_msg[0] == "e")
            {
                // exit this thread;
                exit_wait = false;
                close(client_sockfd);
                return;
            }

            memset(recv_buffer, '\0', 4096);
        }
    }

    void connect_client_to_server()
    {
        while (true)
        {
            cout << "Enter your name : ";
            getline(cin, client_name);

            if (client_name.size() == 0)
            {
                cout << "Your name can't be empty. Retry !!" << endl
                     << endl;
                continue;
            }

            break;
        }

        get_encoded_msg("n", client_name, send_buffer);

        if (send(client_sockfd, send_buffer, sizeof(client_name), 0) < 0)
        {
            cout << "Error in sending!" << endl;
            close(client_sockfd);
            return;
        }
    }

    void send_user_message_to_client()
    {
        cout << "Enter the recepient's id ( To view complete list, press \'s\' ) : ";
        getline(cin, recepient_id);

        if (recepient_id == "s")
        {
            cout << participant_list << endl;
            return;
        }

        if (clients_list.find(recepient_id) == clients_list.end())
        {
            cout << "Entered client_id incorrect. No such client exists." << endl;
            return;
        }

        cout << "Enter your message: ";
        getline(cin, text_message);

        get_encoded_msg(recepient_id, text_message, send_buffer);
        if (send(client_sockfd, send_buffer, sizeof(send_buffer), 0) == -1)
        {
            cout << "Error in sending!" << endl;
            close(client_sockfd);
            return;
        }
    }

    void exit_app()
    {
        get_encoded_msg("e1", "exit", send_buffer);
        if (send(client_sockfd, send_buffer, sizeof(send_buffer), 0) == -1)
        {
            cout << "Error in sending!" << endl;
            return;
        }

        while (exit_wait)
        {
            // Wait for successful exit response from the server //
        }

        return;
    }
};
#include <iostream>
#include <bits/stdc++.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // for closing the socket

#include <semaphore.h>
#include <thread>
#include <sys/stat.h>

using namespace std;
#define BUFFER_SIZE 4096

string marker = "~#`";
string SIGNAL_SEND = "#$%^";

class FileClient
{

private:
    string client_name, text_message, recepient_id;

    char recv_buffer[BUFFER_SIZE], send_buffer[BUFFER_SIZE];

    int client_sockfd;
    int server_port = 9002;

    string filename;
    bool move;

public:
    thread *recv_thread;
    FileClient()
    {
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        client_sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if ((connect(client_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
        {
            cout << "Connection to server unsuccessful" << endl;
            return;
        }
        move = false;
    }
    string encode_message(string id, int size, string name)
    {
        string s = id + marker + to_string(size) + marker + name;
        return s;
    }
    void sendFile(std::istream &input, string receiver_id, string filename)
    {
        // getting the size of file
        ifstream file_size_ptr("file_hub/" + filename, ios::binary);
        file_size_ptr.seekg(0, ios::end);
        int file_size = file_size_ptr.tellg();
        char buffer[BUFFER_SIZE];

        // inform client about the file details and wait for a response
        string send_msg = this->encode_message(receiver_id, file_size, filename);

        memset(buffer, '\0', BUFFER_SIZE);
        for (int i = 0; i < send_msg.size(); i++)
        {
            buffer[i] = send_msg[i];
        }
        if (send(client_sockfd, &buffer, sizeof(buffer), 0) < 0)
        {
            cout << "Error in sending the data chunk to the server." << endl;
        }

        while (!move)
        {
            // DO NOTHING AND WAIT FOR ACKNOWLEDGEMENT //
        }

        // buffer for the file data
        int size_transferred = 0;
        while (size_transferred < file_size)
        {
            memset(buffer, '\0', BUFFER_SIZE);
            input.read(buffer, sizeof(buffer));
            std::streamsize dataSize = input.gcount();

            // send file to the client
            if (send(client_sockfd, &buffer, sizeof(buffer), 0) < 0)
            {
                cout << "Error in sending the data chunk to the server." << endl;
            }
            size_transferred += BUFFER_SIZE;
        }
        move = false;
    }

    void get_file()
    {

        string filename;
        cout << "Drop your file in the \"file_hub\" directory. Enter the filename: ";
        getline(cin, filename);

        string recepient_id;
        cout << "Enter the recepient's id: ";
        getline(cin, recepient_id);

        std::ifstream fin("./file_hub/" + filename, std::ifstream::binary);
        sendFile(fin, recepient_id, filename);
    }

    vector<string> decode_recv_msg()
    {
        string recv_msg(recv_buffer);

        int ind = recv_msg.find(marker);
        string filesize = recv_msg.substr(0, ind);
        string rem = recv_msg.substr(ind + 3, recv_msg.size());

        ind = rem.find(marker);
        string filename = rem.substr(0, ind);
        string sender_id = rem.substr(ind + 3, rem.size());

        return {filename, filesize, sender_id};
    }

    void receive_file()
    {
        mkdir("file_hub/", 0777);
        mkdir("file_hub/received/", 0777);

        while (true)
        {
            memset(recv_buffer, '\0', BUFFER_SIZE);
            if (recv(client_sockfd, &recv_buffer, sizeof(recv_buffer), 0) < 0)
            {
                cout << "Error while receiving" << endl;
            }
            string msg(recv_buffer);

            if (msg == SIGNAL_SEND)
            {
                move = true;
                continue;
            }

            // filename, filesize, sender
            vector<string> data = this->decode_recv_msg();

            int size_transferred = 0;
            string sender_id = data[2];
            string filename = data[0];

            int final_size = stoi(data[1]);

            std::ofstream write_to_file("./file_hub/received/" + sender_id + "_" + filename, ios::binary);

            while (size_transferred < final_size)
            {
                memset(recv_buffer, '\0', BUFFER_SIZE);
                if (recv(client_sockfd, &recv_buffer, sizeof(recv_buffer), 0) < 0)
                {
                    cout << "Error while receiving" << endl;
                }

                if (string(recv_buffer) != SIGNAL_SEND)
                {
                    write_to_file.write(recv_buffer, BUFFER_SIZE);
                    size_transferred += BUFFER_SIZE;
                }
                else
                {
                    move = true;
                }
            }
            write_to_file.close();
        }
    }

    void leave_app()
    {
        string exit_string = "e1" + marker + "exit";

        memset(send_buffer, '\0', BUFFER_SIZE);
        for (int i = 0; i < exit_string.size(); ++i)
            send_buffer[i] = exit_string[i];

        if (send(client_sockfd, &send_buffer, BUFFER_SIZE, 0) < 0)
        {
            cout << "Error in Disconnecting from the File Server." << endl;
        }
        delete recv_thread;
        close(client_sockfd);
    }
};

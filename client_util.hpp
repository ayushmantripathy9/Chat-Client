#include <bits/stdc++.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // for closing the socket

#include <sys/stat.h>

using namespace std;

class Client
{
    public:
        const string marker = "~#`";
        
        string participant_list;
        string client_name, text_message, recepient_id;

        char recv_buffer[4096], send_buffer[4096];
        
        int client_sockfd;

    public:    
        Client(struct sockaddr_in server_addr)
        {
            client_sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if ((connect(client_sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr))) < 0)
            {
                cout << "Connection to server unsuccessful" << endl;
                return;
            }
        }



    public:
        /**
         * Thread function to receive msg from the server
         * @param client_sockfd Socket file descriptor of the client
         * @return NULL
         * */
        void recv_msg()
        {
            mkdir("received", 0777);
            mkdir("received/user_chat_log", 0777);
            mkdir("received/group_chat_log", 0777);

            while (true)
            {
                if (recv(client_sockfd, recv_buffer, sizeof(recv_buffer), 0) < 0)
                {
                    cout << "Can't receive the message. Try Again.\n";
                }

                vector<string> received_msg = decode_msg();

                if (received_msg[0] == "m")
                {
                    std::ofstream write_to_file;
                    write_to_file.open("./received/user_chat_log/" + received_msg[2] + ".txt", std::ios::app);
                    write_to_file << received_msg[1] << "\n";
                    write_to_file.close();
                }
                else if (received_msg[0] == "p")
                {
                    participant_list = received_msg[1];
                }

                memset(recv_buffer, '\0', 4096);
            }
        }

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
        void get_encoded_msg(string id, string message)
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
         * @return vector of msg_type, msg and header(Required for file_name. Absesnt in case of notification msgs)
         * */
        vector<string> decode_msg()
        {
            string raw_msg(recv_buffer);

            int marker_ind = raw_msg.find(marker);
            string type = raw_msg.substr(0, marker_ind);
            string remaining = raw_msg.substr(marker_ind + 3, raw_msg.size());

            if (type == "m")
            {
                marker_ind = remaining.find(marker);

                string msg = remaining.substr(0, marker_ind);
                string header = remaining.substr(marker_ind + 3, remaining.size());
                return {type, msg, header};
            }
            else if (type == "p")
            {
                return {type, remaining};
            }

            return {};
        }

        public:
        void connect_client_to_server(){
            cout << "Enter your name : ";
            getline(cin, client_name);

            get_encoded_msg("n", client_name);
            
            if (send(client_sockfd, send_buffer, sizeof(client_name), 0) < 0)
            {
                cout << "Error in sending!" << endl;
                close(client_sockfd);
                return;
            }
            
            
        }

        void send_user_message_to_client(){
            cout << "Enter the recepient's id ( To view complete list, press \'s\' ) : ";
            getline(cin, recepient_id);

            if (recepient_id == "s")
            {
                cout << participant_list << endl;
                return;
            }
          
            cout << "Enter your message: ";
            getline(cin, text_message);
            
            
            get_encoded_msg(recepient_id, text_message);
            if (send(client_sockfd,send_buffer, sizeof(send_buffer), 0) == -1)
            {
                cout << "Error in sending!" << endl;
                close(client_sockfd);
                return;
            }
        }
        void exit_app(){
            get_encoded_msg("e1", "exit");
            if (send(client_sockfd, send_buffer, sizeof(send_buffer), 0) == -1)
            {
                cout << "Error in sending!" << endl;
                close(client_sockfd);
                return;
            }

            close(client_sockfd);
        }

};
#include <bits/stdc++.h>

#include "file_server.hpp"
#include "server_util.hpp"

using namespace std;

int main()
{
    FileServer *fs_instance = (new FileServer());
    thread attachment_server(&FileServer::start_file_server, fs_instance);

    Server server_instance = *(new Server());
    server_instance.start_server();

    return 0;
}
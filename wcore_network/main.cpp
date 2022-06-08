#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"


int main()
{
    Server server;
    server.Config(ADDRESS, PORT, 2);

    server.Start();

    char header[] = "header";
    char body[]    = "Server chao client";

    NetPackage pack;
    pack.set_header_data(header, sizeof(header)/ sizeof(header[0]));
    pack.set_body_data  (body  , sizeof(body  )/ sizeof(body[0]));

    int a = 0;
    while (true)
    {
        //cout << "[Server][" << a++ << "] : >>" << endl;

        server.Write(pack);
        Sleep(1000);
    }

    return 0;
}

#include <GLNetwork.h>

#define ADDRESS     "127.0.0.1"
#define PORT        "9999"

int main()
{
    Client client;

    client.Connect(ADDRESS, PORT);
    client.Start();

    char header[] = "header";
    char body[] = "client chao server";

    NetPackage pack;
    pack.set_header_data(header, sizeof(header)/ sizeof(header[0]));
    pack.set_body_data  (body  , sizeof(body  )/ sizeof(body[0]));


    while (true)
    {
        cout << "[Client] : >>" << endl;

        client.Write(pack);

        Sleep(1000);
    }

    return 0;
}


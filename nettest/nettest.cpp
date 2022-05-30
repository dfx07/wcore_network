#include <GLNetwork.h>

#define ADDRESS     "127.0.0.1"
#define PORT        "9999"

int main()
{
    Client client;

    client.Connect(ADDRESS, PORT);
    client.Start();

    while (true)
    {
        cout << "[Client] : >>" << endl;

        Sleep(1000);
    }

    return 0;
}


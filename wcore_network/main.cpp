#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"


int main()
{
    Server server;
    server.Config(ADDRESS, PORT, 2);

    server.Start();

    int a = 0;
    while (true)
    {
        cout << "[Server][" << a++ << "] : >>" << endl;

        Sleep(1000);
    }

    return 0;
}

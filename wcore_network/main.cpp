#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"


int main()
{
    Server server;
    server.Config(ADDRESS, PORT, 2);

    server.Start();


    while (true)
    {
        cout << "write to text" << endl;

        Sleep(1000);
    }

    getchar();
    return 0;
}

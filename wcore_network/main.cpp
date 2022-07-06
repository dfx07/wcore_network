#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"

//void TrimStart(std::string& str)
//{
//    while (!str.empty() && *str.begin() == ' ')
//    {
//        str.erase(str.begin());
//    }
//}
//
//void ReadArgs(std::string& str)
//{
//    string data = "";
//    int last_substr_index = 0;
//
//    while (!str.empty())
//    {
//        // Remove space extra
//        TrimStart(str);
//
//        last_substr_index = 0;
//        data = "";
//
//        // is string case
//        if (str[0] == '\'')
//        {
//            str.erase(str.begin());
//
//            last_substr_index = str.find('\'');
//            if (last_substr_index > 0)
//            {
//                data = str.substr(0, last_substr_index);
//            }
//        }
//        // is normal case
//        else 
//        {
//            last_substr_index = str.find_first_of(' ');
//            data = str.substr(0, last_substr_index);
//        }
//
//        // not found break data
//        if (last_substr_index == -1)
//        {
//            break;
//        }
//        str = str.substr(last_substr_index + 1);
//    }
//}



int main()
{
    PACKDATA data;
    data.a = 10;
    data.x = 1.5;
    data.y = 2.5;
    data.z = 3.5;


    Server server;
    server.Config(ADDRESS, PORT, 2);

    server.Start();

    char header[] = "header";
    char body[]    = "Server chao client";

    NetPackage pack;
    pack.set_header_data(header, sizeof(header)/ sizeof(header[0]));
    //pack.set_body_data  (body  , sizeof(body  )/ sizeof(body[0]));

    pack.set_body_data(&data, sizeof(data));
    pack.encode();

    int a = 0;
    while (true)
    {
        //cout << "[Server][" << a++ << "] : >>" << endl;

        server.WriteToSwitch(0, pack);
        Sleep(1000);
    }



    //return 0;


    //int* p = new int(5);

    //string pointerkey =  KGenerator::from_ptr((void*)p, "p_", "_p");

    //int c = 10;
    //DataSession data;
   
    //int* a = new int(5);


    //data["data1"] = (void*)a;

    //int b = *(int*)data["data1"];

    //return 0;

    //char header[] = "header";
    //char body[]    = "Server chao client";

    //NetPackage pack;
    //pack.set_header_data(header, sizeof(header)/ sizeof(header[0]));
    //pack.set_body_data  (body  , sizeof(body  )/ sizeof(body[0]));


    //char* data = NULL;

    //int a = pack.get_body_data(&data);

    //cout << pack.get_body_to_string() << endl;
}

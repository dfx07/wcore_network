#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"

typedef asio::io_service                         network_service;
typedef asio::ip::tcp::socket                    tcp_socket;
typedef boost::shared_ptr<asio::ip::tcp::socket> tcp_socket_ptr;
typedef asio::ip::tcp::acceptor                  tcp_acceptor;
typedef asio::ip::tcp::endpoint                  tcp_endpoint;
typedef boost::system::error_code                tcp_error;



class Server
{
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Construct ther server to listen on specified TCP address and port
    explicit Server() :m_acceptor(*GetNetworkService())
    {
        // TODO: Contructor
    }

    void SetInfo(const string& address, const string& port)
    {
        m_address = address;
        m_port    = port;
    }

private:
    void start_accept()
    {
        network_service* service =  GetNetworkService();
        m_socket = tcp_socket_ptr(new tcp_socket(*service));
        m_acceptor.async_accept(*m_socket, boost::bind(&Server::handle_accept, this, m_socket,
                                                       boost::asio::placeholders::error));
    }

    void handle_accept(tcp_socket_ptr sock, const tcp_error& error)
    {
        if (!error)
        {

        }
        this->start_accept();
    }

    void handle_read()
    {

    }

    static void listen_thread()
    {
        network_service* service =  GetNetworkService();
        service->run();
    }


    void do_async_read()
    {

    }
public:
    void Run()
    {
        // Khởi tạo cổng để lắng nghe các kết nối đến
        auto endpoint = tcp_endpoint( asio::ip::address::from_string(m_address), std::atoi(m_port.c_str()));
        m_acceptor.open( endpoint.protocol());
        m_acceptor.set_option( boost::asio::ip::tcp::acceptor::reuse_address( true ) );
        m_acceptor.bind(endpoint);
        m_acceptor.listen();

        this->start_accept();

        m_threads.create_thread(listen_thread);
    }

    void Stop()
    {

    }

private:
    static network_service* GetNetworkService()
    {
        static network_service service;
        return &service;
    }

private:
    string           m_address;
    string           m_port;

    thread_group     m_threads;

    // Acceptor used to listen for incoming connections.
    tcp_acceptor     m_acceptor;
    tcp_socket_ptr   m_socket;
};



//void start_accept(tcp_socket_ptr);
//
//asio::io_service service;
//
//asio::ip::tcp::endpoint ep( asio::ip::tcp::v4(), PORT);
//
//// Create socket pointer instead of socket
//tcp_socket_ptr sock(new tcp_socket(service));
//tcp_acceptor   acc(service, ep);
//thread_group threads;
//
//
//void handle_accept(tcp_socket_ptr sock, const boost::system::error_code& err) {
//    if ( err) return;
//    MessageBox(0, "sdfsdf", "thong bao", 0);
//    // at this point, you can read/write to the socket
//    tcp_socket_ptr sock1(new tcp_socket(service));
//    start_accept(sock1);
//}
//
//void start_accept(tcp_socket_ptr sock) {
//    acc.async_accept(*sock, boost::bind( handle_accept, sock, _1) );
//}
//
//void listen_thread()
//{
//    service.run();
//}
//
//void run_service_thread()
//{
//    //start_accept(sock);
//    //service.run();
//
//    threads.create_thread(listen_thread);
//}


int main()
{
    //boost::thread(run_service_thread);
    //acc.async_accept(*sock, boost::bind( handle_accept, sock, _1) );
    //run_service_thread();


    Server server;
    server.SetInfo(ADDRESS, PORT);

    server.Run();


    while (true)
    {
        cout << "write to text" << endl;

        Sleep(1000);
    }

    getchar();
    return 0;

}
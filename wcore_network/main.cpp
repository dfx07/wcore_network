#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"
#define MAX_BUFF 1024


/*==========================================================================================
    As far as the need for using shared_from_this() in async_read and async_write,
    the reason is that it guarantees that the method wrapped by boost::bind will always 
    refer to a live object. Consider the following situation:


    1.  Your handle_accept method calls async_read and sends a handler "into the reactor" -
        basically you've asked the io_service to invoke Connection::handle_user_read when
        it finishes reading data from the socket. The io_service stores this functor and 
        continues its loop, waiting for the asynchronous read operation to complete.
    2.  After your call to async_read, the Connection object is deallocated for some reason
        (program termination, an error condition, etc.)
    3.  Suppose the io_service now determines that the asynchronous read is complete, 
        after the Connection object has been deallocated but before the io_service is destroyed 
        (this can occur, for example, if io_service::run is running in a separate thread, as is typical).
        Now, the io_service attempts to invoke the handler, and it has an invalid reference 
        to a Connection object.
==========================================================================================*/

class tcp_session;

typedef asio::io_service                         network_service;
typedef asio::ip::tcp::socket                    tcp_socket;
typedef boost::shared_ptr<asio::ip::tcp::socket> tcp_socket_ptr;
typedef asio::ip::tcp::acceptor                  tcp_acceptor;
typedef asio::ip::tcp::endpoint                  tcp_endpoint;
typedef boost::system::error_code                tcp_error;
typedef  boost::shared_ptr<tcp_session>          tcp_session_ptr;

class tcp_session: std::enable_shared_from_this<tcp_session>
{
private:
    // Không cho sử dụng cái này cho dễ quản lý
    tcp_session(network_service& service): m_socket(service)
    {

    }

public:
    static tcp_session_ptr Create(network_service& service)
    {
        return tcp_session_ptr(new tcp_session(service));
    }

private:
    void Handle_Read_Header()
    {

    }

    void Handle_Read_Body()
    {

    }

    void Handle_Write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
        }
    }

    void Handle_Read(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            cout <<":receive:" << bytes_transferred << endl;
            cout << " >> " << m_buff << endl;
        }
    }

    void Read()
    {
        //asio::async_read(m_socket, asio::buffer(&m_buff[0], MAX_BUFF), 
        //                 boost::bind(&tcp_session::Handle_Read,
        //                             shared_from_this(),
        //                             asio::placeholders::error,
        //                             asio::placeholders::bytes_transferred));
    }

    void Write()
    {
        boost::asio::async_write(m_socket, boost::asio::buffer(m_message),
                                 boost::bind(&tcp_session::Handle_Write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

public:
    tcp_socket& GetSocket() { return m_socket; }


    void Start()
    {
        this->Write();
    }

private:
    tcp_socket      m_socket;
    network_service m_service;
    char            m_buff[MAX_BUFF];
    std::string     m_message;
};




class Server
{
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Construct ther server to listen on specified TCP address and port
    explicit Server() :m_acceptor{ asio::make_strand(m_service)}
    {
        // TODO: Contructor
    }

    void SetInfo(const string& address, const string& port, const int& thread_size =1)
    {
        m_address      = address;
        m_port         = port;
        m_threads_size = thread_size;
    }

private:
    void start_accept()
    {
        tcp_session_ptr new_tcp_session = tcp_session::Create(m_service);

        m_acceptor.async_accept(new_tcp_session->GetSocket(),
                                boost::bind(&Server::handle_accept, this, new_tcp_session,
                                boost::asio::placeholders::error));
    }

    void handle_accept(tcp_session_ptr session, const tcp_error& error)
    {
        if (!error)
        {
            session->Start();
        }
        this->start_accept();
    }

    void handle_read()
    {

    }

    void create_thread()
    {
        // Create a pool of threads to run all of the io_services.
        for (std::size_t i = 0; i< m_threads_size; ++i) {
            m_threads.create_thread(boost::bind(&asio::io_service::run, &m_service));
        }

        // Wait for all threads in the pool to exit.
        m_threads.join_all();
    }

    void start_thread()
    {
        boost::thread_group threads;
        threads.create_thread(boost::bind(&Server::create_thread, this));
    }

    void do_async_read()
    {

    }
public:
    void Start()
    {
        // Khởi tạo cổng để lắng nghe các kết nối đến
        auto endpoint = tcp_endpoint( asio::ip::address::from_string(m_address), std::atoi(m_port.c_str()));
        m_acceptor.open(endpoint.protocol());
        m_acceptor.set_option( boost::asio::ip::tcp::acceptor::reuse_address( true ) );
        m_acceptor.bind(endpoint);
        m_acceptor.listen();

        this->start_accept();
        this->start_thread();
    }

    void Stop()
    {

    }

private:

private:
    string                      m_address;
    string                      m_port;

    //std::vector<std::thread>    m_threads;
    thread_group                m_threads;
    int                         m_threads_size;

    // Acceptor used to listen for incoming connections.
    network_service             m_service;
    tcp_acceptor                m_acceptor;
    tcp_socket_ptr              m_socket;
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
    server.SetInfo(ADDRESS, PORT, 2);

    server.Start();


    while (true)
    {
        cout << "write to text" << endl;

        Sleep(1000);
    }

    getchar();
    return 0;

}
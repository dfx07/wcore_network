#include "GLNetwork.h"
#include <string>


#define ADDRESS     "127.0.0.1"
#define PORT        "9999"
#define MAX_BUFF 1024


/*==========================================================================================
    As far as the need for using shared_from_this() in async_read and async_write,
    the reason is that it guarantees that the method wrapped by boost::bind will always 
    refer to a live object. Consider the following situation:

    sockets are providing a way for two processes or programs to communicate over the network. 
    Sockets provide sufficiency and transparency while causing almost no communication overhead.

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
typedef asio::ip::address                        network_address;
typedef asio::ip::tcp::socket                    tcp_socket;
typedef boost::shared_ptr<tcp_socket>            tcp_socket_ptr;
typedef asio::ip::tcp::acceptor                  tcp_acceptor;
typedef asio::ip::tcp::endpoint                  tcp_endpoint;
typedef boost::system::error_code                tcp_error;
typedef boost::shared_ptr<tcp_session>           tcp_session_ptr;
typedef asio::ip::tcp::resolver                  tcp_resolver;
typedef asio::ip::tcp::resolver::query           tcp_resolver_query;
typedef asio::ip::tcp::resolver::iterator        tcp_resolver_iter;
typedef boost::asio::streambuf                   tcp_streambuf;



typedef asio::ip::udp::endpoint                  udp_endpoint;

// Note : Use boost::enable_shared_from_this instead of std::enable_shared_from_this
// if you use this, the program crashed
class tcp_session: public boost::enable_shared_from_this<tcp_session>
{
private:
    // Không cho sử dụng cái này cho dễ quản lý
    tcp_session(network_service& service)
        : m_socket(service)
    {

    }

public:
    static tcp_session_ptr Create(network_service& service)
    {
        return tcp_session_ptr(new tcp_session(service));
    }

private:
    void Read()
    {
        boost::asio::async_read(m_socket, boost::asio::buffer(m_buff, MAX_BUFF),
                                boost::bind(&tcp_session::Handle_Read, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void Write()
    {
        boost::asio::async_write(m_socket, boost::asio::buffer(m_message),
                                 boost::bind(&tcp_session::Handle_Write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }


    void Handle_Write(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            // TODO hanlde write
            std::cout << "Server sent Hello message!" << std::endl;
        }
        else
        {
            std::cerr << "ERROR: " << error.what() << std::endl;
        }
    }

    void Handle_Read(const boost::system::error_code& error, size_t bytes_transferred)
    {
        if (!error)
        {
            //TODO hanle read
            std::cout << ":receive:" << bytes_transferred << std::endl;
            std::cout << " >> " << m_buff << std::endl;
        }
        else
        {
            std::cerr << "ERROR:" << error.what() << std::endl;
        }
    }

public:

    void Start()
    {
        this->Read();
        this->Write();
    }

    void Connect()
    {

    }

public:
    tcp_socket_ptr GetSocket() { return tcp_socket_ptr(&m_socket); }


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
    explicit Server() 
        :m_acceptor{ asio::make_strand(m_service)}
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
    void StartAccept()
    {
        tcp_session_ptr new_tcp_session = tcp_session::Create(m_service);

        m_acceptor.async_accept(*new_tcp_session->GetSocket(),
                                boost::bind(&Server::HandleAccept, this, new_tcp_session,
                                boost::asio::placeholders::error));
    }

    void HandleAccept(tcp_session_ptr session, const tcp_error& error)
    {
        if (!error)
        {
            session->Start();
        }
        this->StartAccept();
    }

    void CreateThread()
    {
        // Create a pool of threads to run all of the io_services.
        for (std::size_t i = 0; i< m_threads_size; ++i) {
            m_threads.create_thread(boost::bind(&asio::io_service::run, &m_service));
        }

        // Wait for all threads in the pool to exit.
        m_threads.join_all();
    }

    void StartThread()
    {
        boost::thread_group threads;
        threads.create_thread(boost::bind(&Server::CreateThread, this));
    }

public:
    void Start()
    {
        // Initialize the socket and listen to the incoming connection
        auto endpoint = tcp_endpoint( asio::ip::address::from_string(m_address), std::atoi(m_port.c_str()));
        m_acceptor.open(endpoint.protocol());
        m_acceptor.set_option( boost::asio::ip::tcp::acceptor::reuse_address(true));
        m_acceptor.bind(endpoint);
        m_acceptor.listen();

        this->StartAccept();
        this->StartThread();
    }

    void Stop()
    {

    }

private:

private:
    string                      m_address;
    string                      m_port;

    thread_group                m_threads;
    int                         m_threads_size;

    // Acceptor used to listen for incoming connections.
    network_service             m_service;
    tcp_acceptor                m_acceptor;
    tcp_socket_ptr              m_socket;
};

enum class IPVersion
{
    None,
    V4,
    V6
};

class DNS
{
public:
    DNS() :m_resolver(asio::make_strand(m_service))
    {

    }

    bool IsAddress(network_address address)
    {
        return (address.to_string() != "0.0.0.0");
    }

    network_address Translates(const string i_strHostName, // host name [input ]
                               const string i_strPort    ) // port      [input ]  : 80 default
    {
        network_address address;
        try
        {
            tcp_resolver_query  query(i_strHostName, i_strPort);
            tcp_resolver_iter   iter = m_resolver.resolve(query);
            tcp_endpoint        ep = *iter;
            address = ep.address();
        }
        catch (std::exception& ex)
        {
            std::cout << "Error : " << ex.what() << std::endl;
        }
        return address;
    }

private:
    // always Initialize m_service first
    network_service     m_service;

    tcp_resolver        m_resolver;
};

class Client
{
public:
    Client() :m_resolver(m_service)
    {
        m_tcp_session = tcp_session::Create(m_service);
    }

    void StartConnect(const string& hostname, const string& port)
    {
        m_address = hostname;
        m_port    = port;

        // Use synchronized
        //DNS dns;
        //network_address add = dns.Translates(hostname, port);

        //if (dns.IsAddress(add))
        //{
        //    m_tcp_session->Connect();
        //}

        // Use asynchronous 
        tcp_resolver_query query(hostname, port);
        m_resolver.async_resolve(query, boost::bind(&Client::HandleResolve,
                                 this, asio::placeholders::error,
                                 asio::placeholders::iterator));
    }
private:
    void HandleResolve(const tcp_error& err, tcp_resolver_iter iter)
    {
        if (!err)
        {
            tcp_endpoint ep = *iter;
            tcp_socket_ptr socket = m_tcp_session->GetSocket();

            socket->async_connect(ep, boost::bind(&Client::HandleConnect,
                this, asio::placeholders::error,
                ++iter));
        }
        else
        {
            cout << "Error : " << err.message() << endl;
        }
    }


    void HandleConnect(const tcp_error& err, tcp_resolver_iter iter)
    {
        if (!err)
        {
            // The connection was successful. Send the request

            tcp_socket_ptr sock = m_tcp_session->GetSocket();

            boost::asio::async_write(*sock, m_request, 
                boost::bind(&Client::HandleWriteRequest, this,
                             boost::asio::placeholders::error));
        }
        else 
        {
            // The connection failed. Try the next endpoint in the list.

            boost::asio::
        }
    }


    void HandleWriteRequest(const tcp_error& err)
    {
        if (!err)
        {

        }
    }
private:
    network_service     m_service;

    string              m_address;
    string              m_port;
    tcp_session_ptr     m_tcp_session;

    tcp_resolver        m_resolver;
    tcp_streambuf       m_request ;
    tcp_streambuf       m_response;
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

    DNS dns;
    network_address address = dns.Translates("www.facebook.com", "80");


    if (dns.IsAddress(address))
    {
        cout << address.to_string() << endl;
    }

    getchar();

    return 0;

    //Server server;
    //server.SetInfo(ADDRESS, PORT, 2);

    //server.Start();


    //while (true)
    //{
    //    cout << "write to text" << endl;

    //    Sleep(1000);
    //}

    //getchar();
    //return 0;

}


//=============================================================================

////
//// server.cpp
//// ~~~~~~~~~~
////
//// Copyright (c) 2003-2021 Christopher M. Kohlhoff (chris at kohlhoff dot com)
////
//// Distributed under the Boost Software License, Version 1.0. (See accompanying
//// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
////
//
//#include <array>
//#include <cstdlib>
//#include <iostream>
//#include <memory>
//#include <type_traits>
//#include <utility>
//#include <boost/asio.hpp>
//
//using boost::asio::ip::tcp;
//
//// Class to manage the memory to be used for handler-based custom allocation.
//// It contains a single block of memory which may be returned for allocation
//// requests. If the memory is in use when an allocation request is made, the
//// allocator delegates allocation to the global heap.
//class handler_memory
//{
//public:
//    handler_memory()
//        : in_use_(false)
//    {
//    }
//
//    handler_memory(const handler_memory&) = delete;
//    handler_memory& operator=(const handler_memory&) = delete;
//
//    void* allocate(std::size_t size)
//    {
//        if (!in_use_ && size < sizeof(storage_))
//        {
//            in_use_ = true;
//            return &storage_;
//        }
//        else
//        {
//            return ::operator new(size);
//        }
//    }
//
//    void deallocate(void* pointer)
//    {
//        if (pointer == &storage_)
//        {
//            in_use_ = false;
//        }
//        else
//        {
//            ::operator delete(pointer);
//        }
//    }
//
//private:
//    // Storage space used for handler-based custom memory allocation.
//    typename std::aligned_storage<1024>::type storage_;
//
//    // Whether the handler-based custom allocation storage has been used.
//    bool in_use_;
//};
//
//// The allocator to be associated with the handler objects. This allocator only
//// needs to satisfy the C++11 minimal allocator requirements.
//template <typename T>
//class handler_allocator
//{
//public:
//    using value_type = T;
//
//    explicit handler_allocator(handler_memory& mem)
//        : memory_(mem)
//    {
//    }
//
//    template <typename U>
//    handler_allocator(const handler_allocator<U>& other) noexcept
//        : memory_(other.memory_)
//    {
//    }
//
//    bool operator==(const handler_allocator& other) const noexcept
//    {
//        return &memory_ == &other.memory_;
//    }
//
//    bool operator!=(const handler_allocator& other) const noexcept
//    {
//        return &memory_ != &other.memory_;
//    }
//
//    T* allocate(std::size_t n) const
//    {
//        return static_cast<T*>(memory_.allocate(sizeof(T) * n));
//    }
//
//    void deallocate(T* p, std::size_t /*n*/) const
//    {
//        return memory_.deallocate(p);
//    }
//
//private:
//    template <typename> friend class handler_allocator;
//
//    // The underlying memory.
//    handler_memory& memory_;
//};
//
//// Wrapper class template for handler objects to allow handler memory
//// allocation to be customised. The allocator_type type and get_allocator()
//// member function are used by the asynchronous operations to obtain the
//// allocator. Calls to operator() are forwarded to the encapsulated handler.
//template <typename Handler>
//class custom_alloc_handler
//{
//public:
//    using allocator_type = handler_allocator<Handler>;
//
//    custom_alloc_handler(handler_memory& m, Handler h)
//        : memory_(m),
//        handler_(h)
//    {
//    }
//
//    allocator_type get_allocator() const noexcept
//    {
//        return allocator_type(memory_);
//    }
//
//    template <typename ...Args>
//    void operator()(Args&&... args)
//    {
//        handler_(std::forward<Args>(args)...);
//    }
//
//private:
//    handler_memory& memory_;
//    Handler handler_;
//};
//
//// Helper function to wrap a handler object to add custom allocation.
//template <typename Handler>
//inline custom_alloc_handler<Handler> make_custom_alloc_handler(
//    handler_memory& m, Handler h)
//{
//    return custom_alloc_handler<Handler>(m, h);
//}
//
//class session
//    : public std::enable_shared_from_this<session>
//{
//public:
//    session(tcp::socket socket)
//        : socket_(std::move(socket))
//    {
//    }
//
//    void start()
//    {
//        do_read();
//    }
//
//private:
//    void do_read()
//    {
//        auto self(shared_from_this());
//        socket_.async_read_some(boost::asio::buffer(data_),
//            make_custom_alloc_handler(handler_memory_,
//                [this, self](boost::system::error_code ec, std::size_t length)
//        {
//            if (!ec)
//            {
//                do_write(length);
//            }
//        }));
//    }
//
//    void do_write(std::size_t length)
//    {
//        auto self(shared_from_this());
//        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
//            make_custom_alloc_handler(handler_memory_,
//                [this, self](boost::system::error_code ec, std::size_t /*length*/)
//        {
//            if (!ec)
//            {
//                do_read();
//            }
//        }));
//    }
//
//    // The socket used to communicate with the client.
//    tcp::socket socket_;
//
//    // Buffer used to store data received from the client.
//    std::array<char, 1024> data_;
//
//    // The memory to use for handler-based custom memory allocation.
//    handler_memory handler_memory_;
//};
//
//class server
//{
//public:
//    server(boost::asio::io_context& io_context, short port)
//        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
//    {
//        do_accept();
//    }
//
//private:
//    void do_accept()
//    {
//        acceptor_.async_accept(
//            [this](boost::system::error_code ec, tcp::socket socket)
//        {
//            if (!ec)
//            {
//                std::make_shared<session>(std::move(socket))->start();
//            }
//
//            do_accept();
//        });
//    }
//
//    tcp::acceptor acceptor_;
//};
//
//int main(int argc, char* argv[])
//{
//    try
//    {
//        //if (argc != 2)
//        //{
//        //    std::cerr << "Usage: server <port>\n";
//        //    return 1;
//        //}
//        argv[1] = "9999";
//
//        boost::asio::io_context io_context;
//        server s(io_context, std::atoi(argv[1]));
//        io_context.run();
//    }
//    catch (std::exception& e)
//    {
//        std::cerr << "Exception: " << e.what() << "\n";
//    }
//
//    return 0;
//}
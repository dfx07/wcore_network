#ifndef GLNETWORK_H
#define GLNETWORK_H


#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/strand.hpp>
#include <boost/enable_shared_from_this.hpp>

using namespace std;
using namespace boost;


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
        (this can occur, for example, if io_service::run is running in a separate thread,as 
        is typical).
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
#define tcp_buffer(ptr, _size)                   boost::asio::buffer(ptr, _size);



typedef asio::ip::udp::endpoint                  udp_endpoint;

struct NetPackage
{
public:
    enum { MAX_HEADER_LENGTH  =  12  };
    enum { MAX_BODY_LENGTH    =  512 };
    enum { MAX_PACKAGE_LENGTH = MAX_HEADER_LENGTH + MAX_BODY_LENGTH };

private:
    // struct :     [header] + [body]
    char            m_data[MAX_HEADER_LENGTH + MAX_BODY_LENGTH];
    unsigned int    m_body_length;

private:
    void clear_header()
    {
        memset(&m_data[0], 0, MAX_HEADER_LENGTH);
    }

    void clear_body()
    {
        memset(&m_data[MAX_HEADER_LENGTH], 0, MAX_BODY_LENGTH);
        m_body_length = 0;
    }

public:
    NetPackage(): m_body_length(0)
    {
        memset(&m_data[0], 0, MAX_HEADER_LENGTH + MAX_BODY_LENGTH);
    }

    // Get of length of package
    const unsigned int length() const
    {
        return MAX_HEADER_LENGTH + m_body_length;
    }
    unsigned int length()
    {
        return MAX_HEADER_LENGTH + m_body_length;
    }

    // Get data pointer exactlly package
    const void* data() const
    {
        return (void*)&m_data[0];
    }

    void* data()
    {
        return (void*)&m_data[0];
    }

    // Set header and body data package
    void set_body_data(void* _data, const unsigned int _size) const = delete;
    void set_body_data(void* _data, const unsigned int _size)
    {
        int _sizecp = (_size <= MAX_BODY_LENGTH) ? _size : MAX_BODY_LENGTH;
        strncpy(&m_data[MAX_HEADER_LENGTH], (char*)_data, _sizecp);

        m_body_length = _sizecp;
    }

    void set_header_data(void* _data, const unsigned int _size) const = delete;
    void set_header_data(void* _data, const unsigned int _size)
    {
        int _sizecp = (_size <= MAX_HEADER_LENGTH) ? _size : MAX_HEADER_LENGTH;
        strncpy(&m_data[0], (char*)_data, _sizecp);
    }

    // Clear all data in package
    void clear()
    {
        this->clear_header();
        this->clear_body();
    }
};


// Note : Use boost::enable_shared_from_this instead of std::enable_shared_from_this
// if you use this, the program crashed
class tcp_session: public boost::enable_shared_from_this<tcp_session>
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
    void Read()
    {
        boost::asio::async_read(m_socket, boost::asio::buffer(m_packageBuff.data(), m_packageBuff.MAX_PACKAGE_LENGTH),
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
            std::cout << " >> " << m_packageBuff.data() << std::endl;
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
    tcp_socket&    GetSocket()        { return m_socket; }

    // ?? not understand why ??
    //tcp_socket_ptr GetSocketPointer() { return tcp_socket_ptr(&m_socket); }

private:
    tcp_socket      m_socket;
    network_service m_service;
    NetPackage      m_packageBuff;
    std::string     m_message;
};


class Server
{
public:
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    // Construct ther server to listen on specified TCP address and port
    explicit Server(): m_acceptor{ asio::make_strand(m_service)}
    {
        // TODO: Contructor
    }

    void Config(const string& address, const string& port, const int& thread_size =1)
    {
        m_address      = address;
        m_port         = port;
        m_threads_size = thread_size;
    }

private:
    void StartAccept()
    {
        tcp_session_ptr new_tcp_session = tcp_session::Create(m_service);

        m_acceptor.async_accept(new_tcp_session->GetSocket(),
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

    void Connect(const string& hostname, const string& port, const int& ithread = 1)
    {
        m_address = hostname;
        m_port    = port;
        m_nthread = ithread;

        // Use synchronized
        //DNS dns;
        //network_address add = dns.Translates(hostname, port);

        //if (dns.IsAddress(add))
        //{
        //    m_tcp_session->Connect();
        //}

        // Use asynchronous 
        tcp_resolver_query query(hostname, port);
        m_resolver.async_resolve(query, boost::bind(&Client::HandleResolve, this,
                                 asio::placeholders::error,
                                 asio::placeholders::iterator));
    }

    void Start()
    {
        boost::thread_group threads;
        threads.create_thread(boost::bind(&Client::CreateThread, this));
    }

    void Stop()
    {

    }

    void Write(const NetPackage& pack)
    {
        boost::asio::async_write(m_tcp_session->GetSocket(), asio::buffer(pack.data(), pack.length()),
                                 boost::bind(&Client::HandleWriteRequest, this,
                                             boost::asio::placeholders::error));
    }
private:
    void CreateThread()
    {
        // Create a pool of threads to run all of the io_services.
        for (std::size_t i = 0; i< m_nthread; ++i) {
            m_threads.create_thread(boost::bind(&asio::io_service::run, &m_service));
        }

        // Wait for all threads in the pool to exit.
        m_threads.join_all();
    }
private:


    void HandleResolve(const tcp_error& err, tcp_resolver_iter iter)
    {
        if (!err)
        {
            tcp_socket_ptr socket = (tcp_socket_ptr)&m_tcp_session->GetSocket();

            socket->async_connect(*iter, boost::bind(&Client::HandleConnect,
                this, asio::placeholders::error,
                ++iter));

            // Don't understand why not exist it doesn't crash
            m_service.run();
        }
        else
        {
            std::cout << "Error : " << err.message() << std::endl;
        }
    }


    void HandleConnect(const tcp_error& err, tcp_resolver_iter iter)
    {
        if (!err)
        {
            // The connection was successful. Send the request
            boost::asio::async_write(m_tcp_session->GetSocket(), m_request,
                                        boost::bind(&Client::HandleWriteRequest, this,
                                        boost::asio::placeholders::error));
        }
        else if(iter != tcp_resolver_iter())
        {
            // The connection failed. Try the next endpoint in the list.
            tcp_socket_ptr socket = tcp_socket_ptr(&m_tcp_session->GetSocket());

            socket->async_connect(*iter, boost::bind(&Client::HandleConnect,
                    this, asio::placeholders::error, ++iter));
        }
        else
        {
            std::cout << "Error : " << err.message() << std::endl;
        }
    }


    void HandleWriteRequest(const tcp_error& err)
    {
        if (!err)
        {
            std::cout << "[User send] :" << std::endl;
            this->Write();
        }
        else
        {
            std::cout << "Error : " << err.what() << std::endl;
        }
    }

private:
    network_service     m_service;
    tcp_resolver        m_resolver;

    string              m_address;
    string              m_port;

    thread_group        m_threads;
    int                 m_nthread;
    
    tcp_session_ptr     m_tcp_session;


    tcp_streambuf       m_request ;
    tcp_streambuf       m_response;
};

#endif // !GLNETWORK_H

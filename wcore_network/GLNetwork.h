#ifndef GLNETWORK_H
#define GLNETWORK_H


#include <iostream>
#include <unordered_map>
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

enum class IPVersion
{
    None,
    V4,
    V6
};

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

    // Get message body to string 
    string data_body_to_string()
    {
        return string(&m_data[MAX_HEADER_LENGTH]);
    }

    // Set header and body data package
    void set_body_data(void* _data, const unsigned int _size) const = delete;
    void set_body_data(void* _data, const unsigned int _size)
    {
        int _sizecp = (_size <= MAX_BODY_LENGTH) ? _size : MAX_BODY_LENGTH;
        strncpy_s(&m_data[MAX_HEADER_LENGTH], _sizecp, (char*)_data, _sizecp);

        m_body_length = _sizecp;
    }

    void set_header_data(void* _data, const unsigned int _size) const = delete;
    void set_header_data(void* _data, const unsigned int _size)
    {
        int _sizecp = (_size <= MAX_HEADER_LENGTH) ? _size : MAX_HEADER_LENGTH;
        strncpy_s(&m_data[0], _sizecp, (char*)_data, _sizecp);
    }

    // Clear all data in package
    void clear()
    {
        this->clear_header();
        this->clear_body();
    }
};


class NetDNS
{
public:
    NetDNS() :m_resolver(asio::make_strand(m_service))
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


class NetInterface
{
public:
    virtual void HandleWrite(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred) = 0;
    virtual void HandleRead (const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred) = 0;
    virtual void HandleClose(const tcp_session_ptr session){  return; }
};

// Note : Container data use keyword similar datasession web mvc
class DataSession
{
private:
    std::unordered_map<std::string, void*> m_data;

private:
    bool IsExist(std::string key) const
    {
        return (m_data.find(key) != m_data.end() && m_data.at(key) != NULL);
    }
public:
    DataSession()
    {

    }

    ~DataSession()
    {
        this->Clear();
    }

public:
    void* Get(const std::string& key) const
    {
        if (IsExist(key))
        {
            return m_data.at(key);
        }
    }

    void* operator[](const std::string& key) const
    {
        return Get(key);
    }

    void*& operator[](const std::string& key)
    {
        return m_data[key];
    }

public:

    void Add(std::string key, void* data)
    {
        if (m_data.find(key) == m_data.end())
        {
            m_data[key] = data;
        }
    }

    void Remove(std::string key)
    {
        m_data.erase(key);
    }

    void Clear()
    {
        for (auto i = m_data.begin(); i != m_data.end();i++)
        {
            delete i->second;
        }

        m_data.clear();
    }
};


// Note : Use boost::enable_shared_from_this instead of std::enable_shared_from_this
// if you use this, the program crashed
class tcp_session: public boost::enable_shared_from_this<tcp_session>
{
private:
    // Không cho sử dụng cái này cho dễ quản lý
    tcp_session(network_service& ioservice): m_socket(ioservice)
    {
        m_isOpen = false;
    }

public:
    static tcp_session_ptr Create(network_service& ioservice)
    {
        return tcp_session_ptr(new tcp_session(ioservice));
    }

public:

    bool IsOpen() { return m_isOpen; }

    void SetUserData(const std::string& key, void* data)
    {
        m_data_session.Add(key, data);
    }

    void* GetUserData(const std::string& key)
    {
        return m_data_session[key];
    }

    void ResetBuffer() { m_buff.clear(); }

    void Read()
    {
        if (!m_isOpen) return;

        boost::asio::async_read(m_socket, boost::asio::buffer(m_buff.data(), m_buff.MAX_PACKAGE_LENGTH),
                                boost::bind(&tcp_session::Handle_Read_Com, shared_from_this(),
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void Write(const NetPackage& pack)
    {
        if (!m_isOpen) return;

        boost::asio::async_write(m_socket, boost::asio::buffer(pack.data(), m_buff.MAX_PACKAGE_LENGTH),
                                 boost::bind(&tcp_session::Handle_Write_Com, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

private:

    // Look forward to general handling here
    void Handle_Write_Com(const tcp_error& error, size_t bytes_transferred)
    {
        // Handle for a send package
        if (!error)
        {
            // TODO : Hanlde write common user custom
            if (m_dispatcher)
            {
                m_dispatcher->HandleWrite(shared_from_this(), error, bytes_transferred);
            }
        }
        // Handle for the disconnect 
        else if (asio::error::eof              == error ||
                 asio::error::connection_reset == error)
        {
            if (m_dispatcher)
            {
                m_dispatcher->HandleClose(shared_from_this());
            }

            this->Close();
        }
        else
        {
            std::cerr << "ERROR: " << error.what() << std::endl;
            this->Close();
        }
    }

    void Handle_Read_Com(const boost::system::error_code& error, size_t bytes_transferred)
    {
        // Handle for a reveice package 
        if (!error)
        {
            // TODO : Hanlde read user custom
            if (m_dispatcher)
            {
                m_dispatcher->HandleRead(shared_from_this(), error, bytes_transferred);
            }
            // Receive the next packet of this socket
            this->Read();
        }
        // Handle for the disconnect 
        else if (asio::error::eof              == error ||
                 asio::error::connection_reset == error)
        {
            if (m_dispatcher)
            {
                m_dispatcher->HandleClose(shared_from_this());
            }
            this->Close();
        }
        else
        {
            std::cerr << "ERROR:" << error.what() << std::endl;
        }
    }

public:

    void Start(NetInterface* dispatcher)
    {
        this->m_dispatcher = dispatcher;
        this->m_isOpen     = true;
        this->Read();
    }

    void Stop()
    {
        this->m_isOpen = false;
    }

    void Close()
    {
        this->m_isOpen = false;
        this->m_socket.close();
    }

public:
    tcp_socket&    GetSocket()        { return m_socket; }

    //Not understand why ??
    //tcp_socket_ptr GetSocketPointer() { return tcp_socket_ptr(&m_socket); }

private:
    tcp_socket      m_socket;
    NetPackage      m_buff;         // Buffer that receives or sends information to and from
    NetInterface*   m_dispatcher;   // Object, handling information received or sent

    bool            m_isOpen;       // Establish connection successful variable set is true
    DataSession     m_data_session; // Contains user defined data

    friend class Client;
    friend class Server;
};

struct PLUG_DATA
{
    int                 m_id;
    bool                m_active;
    tcp_session_ptr     m_session;

    PLUG_DATA()
    {
        m_active = true;
    }
};

class IdentityGenerator
{
    enum { IFN_FREE = -99999}; // Free id
public:
    int              m_max_id;
    int              m_count;
    std::vector<int> m_available_id;

public:
    IdentityGenerator(int max_gen = IFN_FREE)
    {
        m_count  = 0;
        m_max_id = max_gen;
    }

    int alloc()
    {
        if (m_available_id.size() > 0)
        {
            int id = m_available_id.front();

            m_available_id.erase(m_available_id.begin());

            return id;
        }

        if (m_max_id == IFN_FREE || m_count < m_max_id)
        {
            return m_count++;
        }

        return -1;
    }

    void free(int id)
    {
        // If exist in available id
        for (int i = 0; i < m_available_id.size(); i++)
        {
            if (m_available_id[i] == id)
            {
                return;
            }
        }

        // Push id to available id
        if (id >= 0 && (m_max_id == IFN_FREE || id < m_max_id))
        {
            m_available_id.push_back(id);
        }
    }
};

class NetSwitchInterface
{
    enum { MAX_PLUG = 20};

private:
    int                m_switch_id;
    std::string        m_switch_name;
    //int              m_count;
    //std::vector<int> m_available_id;

    IdentityGenerator  m_gentor;

    PLUG_DATA*         m_plugs[MAX_PLUG];

private:
    //int GenerateID()
    //{
    //    if (m_available_id.size() > 0)
    //    {
    //        int id = m_available_id.front();

    //        m_available_id.erase(m_available_id.begin());

    //        return id;
    //    }

    //    if (m_count < MAX_PLUG)
    //    {
    //        return m_count++;
    //    }

    //    std::cout << "ERROR : Gen id failed !" << std::endl;

    //    return -1;
    //}

    void RemovePlug(int index)
    {
        if (m_plugs[index] != NULL)
        {
            delete m_plugs[index];
            m_plugs[index] = NULL;
        }
        m_gentor.free(index);
    }

public:
    NetSwitchInterface(): m_gentor(MAX_PLUG)
    {
        m_switch_name = "";
    }

    PLUG_DATA* PlugIn(const tcp_session_ptr session)
    {
        //int id = GenerateID();

        int id = m_gentor.alloc();

        if ( id != -1)
        {
            PLUG_DATA* plug = new PLUG_DATA();
            plug->m_session = session;
            plug->m_active  = true;
            plug->m_id      = id;

            m_plugs[id] = plug;

            return plug;
        }

        return NULL;
    }

    void PlugOut(const tcp_session_ptr session)
    {
        for (int i = 0; i < MAX_PLUG; i++)
        {
            if (m_plugs[i] != NULL && m_plugs[i]->m_session == session)
            {
                RemovePlug(i);

            }
        }
    }

public:
    virtual void Write(const NetPackage& pack)
    {
        for (int i = 0; i < MAX_PLUG; i++)
        {
            if (m_plugs[i]->m_active)
            {
                m_plugs[i]->m_session->Write(pack);
            }
        }
    }

    virtual void Read()
    {

    }
};


//class SwitchManager
//{
//private:
//
//    std::unordered_map<std::string, NetSwitchInterface*> mana;
//
//public:
//    static Net
//public:
//    void Add(NetSwitchInterface)
//};


class NetDataBase
{
    typedef std::unordered_map<int, tcp_session_ptr>      SessionManager;
    typedef std::unordered_map<std::string, NetSwitchInterface*>  SwitchManager;

private:
    // These two properties are related
    SessionManager  m_session_manager;
    SwitchManager   m_switch_manager;

private:

    static int KeyGenerator()
    {

    }
public:
    NetDataBase()
    {

    }

    ~NetDataBase()
    {
        //for(int i =0 ; i< )
    }

public:
    int Add(tcp_session_ptr session)
    {
        int key = KeyGenerator();

        if (key != -1)
        {
            //m_sessions[key] = session;
        }

        return key;
    }

    void Remove(tcp_session_ptr sesison)
    {
        //m_sessions.erase((int)session);
    }
};

class Server : public NetInterface
{
public:

    Server(const Server&)            = delete;
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
            m_sessions.push_back(session);
            session->Start(this);
        }
        else
        {
            // TODO: Accept failed !
            int a = 10;
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
    // Common
    bool SessionWrite(const tcp_session_ptr session, const NetPackage& pack)
    {

    }

    bool AddSession(const tcp_session_ptr session)
    {

    }

    bool RemoveSession(const tcp_session_ptr session)
    {
        bool rel = false;

        for (auto it = m_sessions.begin(); it != m_sessions.end(); /*it++*/)
        {
            // Found session in list
            if (*it == session)
            {
                it = m_sessions.erase(it);
                rel = true;
            }
            else
            {
                it++;
            }
        }
        return rel;
    }

public:

    void Write(const NetPackage& pack)
    {
        if (m_sessions.size() > 0)
        {
            m_sessions[0]->Write(pack);
        }
    }

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

private:
    virtual void HandleWrite(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        //Handle session after send package
    }

    virtual void HandleRead(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        //Handle session after receive package
        std::cout << " Client said :: >> " << session->m_buff.data_body_to_string()  << std::endl;
    }

    virtual void HandleClose(const tcp_session_ptr session)
    {
        // Handle session disconnect
        std::cout << "[*] Client disconnect ...." << std::endl;


        // Remove session in client list
        if (!RemoveSession(session))
        {
            std::cout << "[ERR] Remove session failed !" << std::endl;
        }
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

    vector<tcp_session_ptr>     m_sessions;
    std::unordered_map<std::string,std::string> mymap;
};

class Client : public NetInterface
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
        //if (!m_tcp_session->m_isOpen) return;

        //// This is lambda expression
        //m_service.post( [this, pack]
        //{
        //    boost::asio::async_write(m_tcp_session->GetSocket(),
        //                             asio::buffer(pack.data(), pack.MAX_PACKAGE_LENGTH),
        //                             boost::bind(&Client::HandleWriteRequest, this,
        //                             boost::asio::placeholders::error));
        //    
        //});

        m_tcp_session->Write(pack);
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
            //m_service.post([this, pack]
            //{
            //    boost::asio::async_write(m_tcp_session->GetSocket(), m_request,
            //                             boost::bind(&Client::HandleWriteRequest, this,
            //                                         boost::asio::placeholders::error));
            //});'
            m_tcp_session->Start(this);
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
            m_tcp_session->Stop();
            std::cout << "Error : " << err.message() << std::endl;
        }
    }


    void HandleWriteRequest(const tcp_error& err)
    {
        if (!err)
        {
            std::cout << "[User send] :" << std::endl;
        }
        else
        {
            std::cout << "Error : " << err.what() << std::endl;
        }
    }

    virtual void HandleWrite(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        //Handle session after send package
    }

    virtual void HandleRead(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        std::cout << " Server said :: >> " << session->m_buff.data_body_to_string()  << std::endl;
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

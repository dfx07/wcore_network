#ifndef GLNETWORK_H
#define GLNETWORK_H


#include <iostream>
#include <unordered_map>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/bind/bind.hpp>
#include <boost/asio/strand.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <sstream>
#include <string>

using namespace std;
using namespace boost;


#define MAX_BUFF 1024

#define IS_NULL(ptr) (ptr == NULL)
#define IS_NOT_NULL(ptr) (ptr != NULL)

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

class NetSwitchInterface;
class NetSwitchManager;
class NetDataBase;

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
typedef vector<NetSwitchInterface*>              ArrayNetSwitch;

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
    static std::string to_string(void* data)
    {
        char* temp = static_cast<char*>(data);
        return string(&temp[0]);
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

    // Get data header exactlly package
    int get_header_data(char** data) const
    {
        *data = new char[MAX_HEADER_LENGTH];
        memcpy_s(*data, MAX_HEADER_LENGTH, &m_data[0], MAX_HEADER_LENGTH);
        return MAX_HEADER_LENGTH;
    }

    int get_header_data(char** data)
    {
        *data = new char[MAX_HEADER_LENGTH];
        memcpy_s(*data, MAX_HEADER_LENGTH, &m_data[0], MAX_HEADER_LENGTH);
        return MAX_HEADER_LENGTH;
    }

    std::string get_header_to_string()
    {
        std::string tmp(MAX_HEADER_LENGTH, '\0');
        memcpy_s(&tmp[0], MAX_HEADER_LENGTH, &m_data[0], MAX_HEADER_LENGTH);
        return tmp;
    }

    // Get data body exactlly package
    int get_body_data(char** data) const
    {
        *data = new char[MAX_HEADER_LENGTH];
        memcpy_s(*data, m_body_length, &m_data[MAX_HEADER_LENGTH], m_body_length);
        return m_body_length;
    }

    int get_body_data(char** data) 
    {
        *data = new char[MAX_HEADER_LENGTH];
        memcpy_s(*data, m_body_length, &m_data[MAX_HEADER_LENGTH], m_body_length);
        return m_body_length;
    }

    // Get message body to string 
    std::string get_body_to_string()
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
        return NULL;
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

    void Set(std::string key, void* data)
    {
        m_data[key] = data;
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

// Note : Command parsing of the statement
class ArgumemtParsing
{
    typedef pair<string, string> KEY_VAL;

private:
    vector<KEY_VAL>     m_args;
    std::string         m_command;

private:

    void TrimStart(std::string& str)
    {
        while (!str.empty() && *str.begin() == ' ')
        {
            str.erase(str.begin());
        }
    }

    void RemoveExtraSpace(std::string& str)
    {
        const char DOUBLE_SPACE[] = "  ";

        while (!str.empty() && *str.begin() == ' ')
        {
            str.erase(str.begin());
        }

        while (!str.empty() && str.back() == ' ')
        {
            str.pop_back();
        }

        int found = str.find(DOUBLE_SPACE);

        while (found >=0 )
        {
            str.erase(str.begin() + found);
            found = str.find(DOUBLE_SPACE);
        }
    }

    void ReadCommand(std::string& str)
    {
        TrimStart(str);
        int idx = str.find_first_of(' ');

        string cmd = str.substr(0, idx);

        str = str.substr(idx + 1);
    }


    void ReadArgs(std::string& str)
    {
        string data = "";
        int last_substr_index = 0;

        bool bPush = false;
        int  state_input = 0;

        m_args.clear();

        KEY_VAL argvl;

        while (!str.empty())
        {
            // Remove space extra
            TrimStart(str);

            last_substr_index = 0;
            data = "";

            // find key 
            if (state_input == 0)
            {
                last_substr_index = str.find_first_of(' ');

                // is key args command
                if (str[0] == '-')
                {
                    str.erase(str.begin());

                    if (last_substr_index > 0)
                    {
                        data = str.substr(0, last_substr_index);
                    }

                    argvl.first = data;

                    state_input = 1;
                }
            }
            // find value
            else if (state_input == 1)
            {
                // is string case
                if (str[0] == '\'')
                {
                    str.erase(str.begin());

                    last_substr_index = str.find('\'');
                    if (last_substr_index > 0)
                    {
                        data = str.substr(0, last_substr_index);
                    }
                }
                // is normal case
                else 
                {
                    last_substr_index = str.find_first_of(' ');
                    data = str.substr(0, last_substr_index);
                }

                state_input = 0;
            }

            // not found break data
            if (last_substr_index == -1)
            {
                break;
            }

            if (bPush)
            {
                m_args.push_back(argvl);
                argvl.first  = "";
                argvl.second = "";
            }

            str = str.substr(last_substr_index + 1);
        }
    }

public:
    ArgumemtParsing()
    {

    }

    void Parse(std::string cmd)
    {
        RemoveExtraSpace(cmd);

        ReadCommand(cmd);

        ReadArgs(cmd);
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

    static bool IsActive(const tcp_session_ptr& session)
    {
        if (session == NULL || session->m_isOpen == false)
        {
            return false;
        }

        return true;
    }

public:

    bool IsOpen() { return m_isOpen; }

    void SetUserData(const std::string& key, void* data)
    {
        m_data_session.Set(key, data);
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
    std::string         m_name;

    bool                m_active;
    tcp_session_ptr     m_session;

    PLUG_DATA()
    {
        m_active = true;
    }
};


class KGenerator
{

public:
    static const std::string from_ptr(const tcp_session_ptr& sessionptr, const std::string& pref = "",const std::string& suff = "")
    {
        // I use the value of the pointer to make key
        std::stringstream ss;

        if (!pref.empty()) ss << pref;

        ss << sessionptr.get();

        if (!suff.empty()) ss << suff;

        return ss.str();
    }

    static const std::string from_ptr(const void* ptr, const std::string& pref = "",const std::string& suff = "")
    {
        // I use the value of the pointer to make key
        std::stringstream ss;

        if (!pref.empty()) ss << pref;

        ss << ptr;

        if (!suff.empty()) ss << suff;

        return ss.str();
    }
};

class IdentityGenerator
{
    enum 
    {
        IFN_FREE = -99999, // Free id
        IFN_NULL = -1,     // Gen free id
    };

public:
    int              m_max_id;
    int              m_count;
    std::vector<int> m_free_id;
    std::vector<int> m_data_id;

public:
    IdentityGenerator(int max_gen = IFN_FREE)
    {
        m_count  = 0;
        m_max_id = max_gen;

        // Reserve data max 100
        if (m_max_id > 0)
        {
            m_data_id.reserve(m_max_id);
        }
        else
        {
            m_data_id.reserve(100);
        }
    }

    static bool is_null(const int key)
    {
        return (key == IFN_NULL);
    }

    int size() const
    {
        return (int)m_data_id.size();
    }

    int operator[](const int& index) const
    {
        return m_data_id[index];
    }

    int alloc()
    {
        if (m_free_id.size() > 0)
        {
            int id = m_free_id.front();

            m_free_id.erase(m_free_id.begin());

            m_data_id.push_back(id);

            return id;
        }

        if (m_max_id == IFN_FREE || m_count < m_max_id)
        {
            m_data_id.push_back(m_count++);
            return m_data_id.back();
        }

        return IFN_NULL;
    }

    void free(int id)
    {
        // Remove id in datalist
        for (auto i = m_data_id.begin(); i != m_data_id.end(); /*i++*/)
        {
            if (*i == id)
            {
                i = m_data_id.erase(i);
            }
            else
            {
                i++;
            }
        }

        // If exist in available id
        for (int i = 0; i < m_free_id.size(); i++)
        {
            if (m_free_id[i] == id)
            {
                return;
            }
        }

        // Push id to available id
        if (id >= 0 && (m_max_id == IFN_FREE || id < m_max_id))
        {
            m_free_id.push_back(id);
        }
    }

    void free_all()
    {
        m_data_id.clear();
        m_free_id.clear();
        m_max_id = IFN_FREE;
        m_count = 0;
    }
};

class NetSwitchInterface
{
    // Define common key
    enum                 { MAX_PLUG = 20};
private:
    int                  m_switch_id;   //The value of property cannot be set
    std::string          m_switch_name;

    vector<PLUG_DATA*>   m_plugs;

public:
    static const string SWITCH_KEY_DATA()
    {
        return "_switch_info";
    }

    static bool IsActive(NetSwitchInterface* swit)
    {
        return (IS_NOT_NULL(swit));
    }

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

    void DetachInfoDataTo(PLUG_DATA* plug)
    {
        if (IS_NULL(plug)) return;

        void * switch_info = plug->m_session->GetUserData(SWITCH_KEY_DATA());

        if (IS_NULL(switch_info)) return;

        // Remove cache data session in plug
        ArrayNetSwitch* list_switch = static_cast<ArrayNetSwitch*>(switch_info);

        for (auto it = list_switch->begin(); it != list_switch->end();/* it++*/)
        {
            if (*it == this)
            {
                it = list_switch->erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    bool AttachInfoDataTo(PLUG_DATA* plug)
    {
        if (IS_NULL(plug)) return false;

        void * switch_info = plug->m_session->GetUserData(SWITCH_KEY_DATA());

        if (!IS_NULL(switch_info))
        {
            ArrayNetSwitch* list_switch = static_cast<ArrayNetSwitch*>(switch_info);

            int i = 0;  // Check switch already exist

            for (i = 0; i < list_switch->size(); i++)
            {
                if (list_switch->at(i) == this)
                {
                    break;
                }
            }
            if (i < list_switch->size())
            {
                list_switch->push_back(this);
            }
        }
        else
        {
            ArrayNetSwitch* list_switch = new ArrayNetSwitch();
            list_switch->push_back(this);

            plug->m_session->SetUserData(SWITCH_KEY_DATA(), list_switch);
        }

        return true;
    }

    bool IsExistIn(const tcp_session_ptr& session)
    {
        if (IS_NULL(session)) return false;

        void * switch_info = session->GetUserData(SWITCH_KEY_DATA());

        if (IS_NULL(switch_info))
        {
            return false;
        }

        ArrayNetSwitch* list_switch = static_cast<ArrayNetSwitch*>(switch_info);

        for (int i = 0; i < list_switch->size(); i++)
        {
            if (list_switch->at(i) == this)
            {
                return true;
            }
        }
        return false;
    }

    PLUG_DATA* CreatePlugData(const tcp_session_ptr& session) 
    {
        int index_free = GetFreeIndexPlug();

        if (index_free == -1 || IS_NULL(session))
        {
            throw " >> Create plug data failed ! no slot";
            return NULL;
        }

        // Create data plug and setup information session
        PLUG_DATA* plug = new PLUG_DATA();
        plug->m_session = session;
        plug->m_active  = true;


        m_plugs[index_free] = plug;

        return plug;
    }

    void RemovePlugData(const tcp_session_ptr& session)
    {
        if (IsExistIn(session)) // Check switch in session fast
        {
            int index = GetIndexPlugData(session);
            
            if (index >= 0 && index < MAX_PLUG)
            {
                DetachInfoDataTo(m_plugs[index]);

                m_plugs[index] = NULL;
            }
        }
    }

private:
    int GetIndexPlugData(const tcp_session_ptr session) const
    {
        for (int i = 0; i < MAX_PLUG; i++)
        {
            if (IS_NOT_NULL(m_plugs[i]) && m_plugs[i]->m_session == session)
            {
                return i;
            }
        }
        return -1;
    }

    void ResetPlug()
    {
        for (int i = 0; i < MAX_PLUG; i++)
        {
            delete m_plugs[i];
            m_plugs[i] = NULL;
        }
    }

    int GetFreeIndexPlug()
    {
        for (int i = 0; i < MAX_PLUG; i++)
        {
            if (IS_NULL(m_plugs[i]))
            {
                return i;
            }
        }
        return -1;
    }

public:
    NetSwitchInterface()/*: m_gentor(MAX_PLUG)*/
    {
        m_switch_name = "";
        m_plugs.reserve(MAX_PLUG);
        this->ResetPlug();
    }

    PLUG_DATA* PlugIn(const tcp_session_ptr session)
    {
        PLUG_DATA* plug = CreatePlugData(session);

        if (AttachInfoDataTo(plug))
        {
            m_plugs.push_back(plug);
            return plug;
        }
        else
        {
            delete plug;
        }

        return NULL;
    }

    void PlugOut(const tcp_session_ptr session)
    {
        RemovePlugData(session);
    }

private:
    void HandleReadCom(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {

    }

    void ProcessPackage(const NetPackage& pack)
    {
        char* data_body = NULL;
        int n = pack.get_header_data(&data_body);
        if (n > 0)
        {

        }
    }

public:
    virtual void Write(const NetPackage& pack)
    {
        for (int i = 0; i < MAX_PLUG; i++)
        {
            if (IS_NOT_NULL(m_plugs[i]) && m_plugs[i]->m_active)
            {
                m_plugs[i]->m_session->Write(pack);
            }
        }
    }

    virtual void Read(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        std::cout << "[ >> Switch process ] Read data" << endl;
        this->HandleReadCom(session, error, bytes_transferred);
    }


    friend class NetSwitchManager;
    friend class Server;
};




class NetSessionManager
{
private:
    std::unordered_map<std::string, tcp_session_ptr> m_data;

public:
    // Constructor NetSessionManager
    NetSessionManager()
    {

    }


    // Destructor NetSessionManager
    ~NetSessionManager()
    {

    }

public:
    static std::string GetKey(const tcp_session_ptr& session)
    {
        // I use the value of the pointer to make key
        std::stringstream ss;
        ss << session.get();
        return ss.str();
    }

public:
    bool IsExist(const tcp_session_ptr& session) const
    {
        // Slow when more session data
        //for (auto i = m_data.begin(); i != m_data.end(); i++)
        //{
        //    if (session == i->second)
        //    {
        //        return true;
        //    }
        //}
        //return false;

        const string key = NetSessionManager::GetKey(session);

        return IsExist(key);
    }

    bool IsExist(const std::string& key) const
    {
        return m_data.find(key) != m_data.end();
    }

    tcp_session_ptr GetFirst()
    {
        if (m_data.size() > 0)
        {
            return m_data.begin()->second;
        }
        return NULL;
    }

public:

    void Add(const tcp_session_ptr& session)
    {
        string key = NetSessionManager::GetKey(session);

        if (!IsExist(key))
        {
            m_data.insert(std::make_pair(key, session));
        }
    }

    bool Remove(const tcp_session_ptr& session)
    {
        string key = NetSessionManager::GetKey(session);

        while (IsExist(key))
        {
            m_data.erase(key);
        }

        return true;
    }

    void Clear()
    {
        m_data.clear();
    }
};


class NetSwitchManager
{
    enum
    {
        MAX_SWITCH = 20
    };
private:

    std::unordered_map<int, NetSwitchInterface*> m_data;
    IdentityGenerator                            m_genID;

private:

    bool IsExist(int keyID) const
    {
        return m_data.find(keyID) != m_data.end();
    }

public:
    NetSwitchManager() : m_genID(MAX_SWITCH)
    {

    }

    ~NetSwitchManager()
    {
        for (auto i = m_data.begin(); i != m_data.end(); i++)
        {
            delete i->second;
        }
        m_genID.free_all();
    }

    int Size()
    {
        return (int)m_data.size();
    }

    NetSwitchInterface* operator[](const int& index) const
    {
        int keyID = m_genID[index];
        if (IsExist(keyID))
        {
            return m_data.at(keyID);
        }
        return NULL;
    }

    NetSwitchInterface* GetAt(const int& keyID)
    {
        if (IsExist(keyID))
        {
            return m_data.at(keyID);
        }
        return NULL;
    }

    void Add(NetSwitchInterface* _swi)
    {
        int keyID = m_genID.alloc();
        if (!IdentityGenerator::is_null(keyID))
        {
            // assign id for switchInterface
            _swi->m_switch_id = keyID;
            m_data.insert(std::make_pair(keyID, _swi));
        }
    }

    void Remove(const NetSwitchInterface* _swi)
    {
        int keyID = _swi->m_switch_id;

        if (IsExist(keyID))
        {
            m_data.erase(keyID);
            m_genID.free(keyID);
        }
    }
};


class NetDataBase
{
    //typedef std::unordered_map<int, tcp_session_ptr>      SessionManager;
    //typedef std::unordered_map<std::string, NetSwitchInterface*>  SwitchManager;

private:
    // These two properties are related
    NetSessionManager  m_session_manager;
    NetSwitchManager   m_switch_manager;

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


public:
    tcp_session_ptr GetFirstSession()
    {
        return m_session_manager.GetFirst();
    }
    NetSwitchInterface* GetSwitch(int keyID)
    {
        return m_switch_manager.GetAt(keyID);
    }

    ArrayNetSwitch* GetListSwitchOf(const tcp_session_ptr& session)
    {
        void* switch_info = session->GetUserData(NetSwitchInterface::SWITCH_KEY_DATA());

        if (IS_NOT_NULL(switch_info))
        {
            ArrayNetSwitch* list_switch = static_cast<ArrayNetSwitch*>(switch_info);
            return list_switch;
        }
        return NULL;
    }

    bool AddSession(const tcp_session_ptr& session)
    {
        m_session_manager.Add(session);

        return true;
    }

    // Remove the session and all data associated with it
    bool RemoveSession(const tcp_session_ptr& sesison)
    {
        //m_sessions.erase((int)session);

        // Remove session in switch associated
        for (int i = 0; i < m_switch_manager.Size(); i++)
        {
            NetSwitchInterface* swit = m_switch_manager[i];

            swit->PlugOut(sesison);
        }

        // Remove session in session manager
        return m_session_manager.Remove(sesison);
    }

    bool AddSwitch(NetSwitchInterface* swi)
    {
        // ID switch auto define
        m_switch_manager.Add(swi);

        return true;
    }

    bool PushSessionToSwitch( NetSwitchInterface* swi, const tcp_session_ptr& session)
    {
        swi->PlugIn(session);
    }

    bool PushSessionToSwitch( int keySwitchID , const tcp_session_ptr& session)
    {
        NetSwitchInterface* swit = m_switch_manager.GetAt(keySwitchID);

        if (IS_NOT_NULL(swit))
        {
            swit->PlugIn(session);

            return true;
        }
        return false;
    }

    bool RemoveSwitch(const NetSwitchInterface* swi)
    {
        m_switch_manager.Remove(swi);
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

        NetSwitchInterface* swit = new NetSwitchInterface();
        swit->m_switch_name = "Switch_default";
        m_database.AddSwitch(swit);
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
            tcp_socket& sock =  session->GetSocket();
            cout << "[*] Accept connections from : " << sock.local_endpoint().address().to_string() << endl;
            m_database.AddSession(session);
            m_database.PushSessionToSwitch(0, session);
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
    //// Common
    //bool SessionWrite(const tcp_session_ptr session, const NetPackage& pack)
    //{

    //}

    //bool AddSession(const tcp_session_ptr session)
    //{

    //}

    //bool RemoveSession(const tcp_session_ptr session)
    //{
    //    bool rel = false;

    //    for (auto it = m_sessions.begin(); it != m_sessions.end(); /*it++*/)
    //    {
    //        // Found session in list
    //        if (*it == session)
    //        {
    //            it = m_sessions.erase(it);
    //            rel = true;
    //        }
    //        else
    //        {
    //            it++;
    //        }
    //    }
    //    return rel;
    //}

public:

    void Write(const NetPackage& pack)
    {
        auto session = m_database.GetFirstSession();
        if (tcp_session::IsActive(session))
        {
            session->Write(pack);
        }
    }

    void WriteToSwitch(int switch_id, const NetPackage& pack)
    {
        auto swit = m_database.GetSwitch(switch_id);
        if (NetSwitchInterface::IsActive(swit))
        {
            swit->Write(pack);
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
    virtual void ServerHandleRead(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        //Handle session after receive package
        std::cout << "[ >> Server  process ] Read data :: >> " << session->m_buff.get_body_to_string()  << std::endl;
    }

private:
    virtual void HandleWrite(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        //Handle session after send package
    }

    virtual void HandleRead(const tcp_session_ptr session, const tcp_error& error, size_t bytes_transferred)
    {
        ArrayNetSwitch* list_switch = m_database.GetListSwitchOf(session);

        if (IS_NOT_NULL(list_switch))
        {
            for (int i = 0; i < list_switch->size(); i++)
            {
                list_switch->at(i)->Read(session, error, bytes_transferred);
            }
        }
    }

    virtual void HandleClose(const tcp_session_ptr session)
    {
        // Handle session disconnect
        std::cout << "[*] Client disconnect ...." << std::endl;

        // Remove session in client list
        if (!m_database.RemoveSession(session))
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

    NetDataBase                 m_database;
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
        std::cout << " Server said :: >> " << session->m_buff.get_body_to_string()  << std::endl;
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

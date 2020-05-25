#include <string>
#include <unordered_map>
#include <sstream>
#include "server.h"
#include <map>
#include "parse.h"
#include "config.h"
#include "log.h"

using namespace std;

ChatRoomServer::ChatRoomServer() {
    _socket_epoll.set_watcher(&_server_handler);
}

// 处理接受请求逻辑
int ServerEpollWatcher::on_accept(EpollContext &epoll_context) { //传递过来的是一个epoll_event
    LOG(DEBUG)<<"Handle: on_accept start"<<std::endl;
    int client_fd = epoll_context.fd;

    
    printf("client %s:%d connected to server\n", epoll_context.client_ip.c_str(), epoll_context.client_port);
    // 返回欢迎信息和nickname信息
    Msg m;
    m.code = M_NORMAL;
    m.context = WELCOM_MES;
    m.context = m.context + "\nYou nickname is " + std::to_string(client_fd) + " (Enter \"r\" ro replace).";
    m.context += "\nUsage Shell @ Client NickName";
    int ret = m.send_diy(client_fd);

    return ret;
}

// 处理来信请求, 出错返回-1， 退出连接返回-2
int ServerEpollWatcher::on_readable(EpollContext &epoll_context, std::unordered_map<int, std::string> &client_list) {
    int client_fd = epoll_context.fd;
    Msg recv_m;
    recv_m.recv_diy(client_fd);
    // 处理客户端退出的情况
    if(recv_m.code == M_EXIT)
    {
        return -2;
    }
    // 改名请求
    else if(recv_m.code == M_CNAME)
    {
        // 消除最后的换行符
        recv_m.context.pop_back();
        client_list[client_fd] = recv_m.context;
        return 0;
    }
    // 只有一个客户端，发送警告信息
    if(client_list.size() == 1)
    {
        std::string m_str = ONLY_ONE_CAUTION;
        m_str = " [WARN] " + m_str;
        Msg m(M_NORMAL, m_str);
        m.send_diy(client_fd);
    }
    else
    {
        // 处理消息格式
        std::string meg_str = recv_m.context;

        //保存发送对象
        std::stringstream ss(meg_str);
        map<int,bool> objs_hash;
        string shell_cmd;
        char ch_tag;
        while(ss){
            string tmp_space;
            ss >> tmp_space;
            if(tmp_space == "@") break;
        }

        while(ss){
            int obj_number;
            ss >> obj_number;
            objs_hash[obj_number] = true;
        }

        meg_str = client_list[client_fd] + ">" + meg_str;
        Msg send_m(M_NORMAL, meg_str);
        // 对需要发送消息的客户端进行发送
        for(auto it : client_list)
        {
            if(it.first == client_fd) continue;
            if(objs_hash[it.first] == true)
                send_m.send_diy(it.first);
        }
    }
    return 0;
}

int ChatRoomServer::start_server(const std::string bind_ip, int port, int backlog, int max_events) {
    LOG(INFO)<<"Server start: start"<<std::endl;
    std::cout<<"ChatRoom start!"<<std::endl;
    _socket_epoll.set_bind_ip(bind_ip);
    _socket_epoll.set_port(port);
    //绑定相关端口和ID
    _socket_epoll.set_backlog(backlog);
    _socket_epoll.set_max_events(max_events);
    
    return _socket_epoll.start_epoll();
}

int ChatRoomServer::stop_server() {
    LOG(INFO)<<"Server: stop server"<<std::endl;
    return _socket_epoll.stop_epoll();
}

int main()
{
    // 设置配置
    std::map<std::string, std::string> config;
    get_config_map("server.config", config);
    init_logger("server_log", "debug.log", "info.log", "warn.log", "error.log", "all.log");
    set_logger_mode(1);
    ChatRoomServer server;
    server.start_server(config["ip"], std::stoi(config["port"]), 20, 200);
    return 0;
}
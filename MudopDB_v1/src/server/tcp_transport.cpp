#include "server/tcp_transport.hpp"
#include "server/simpledb_server.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>
#include <stdexcept>
#include <thread>

namespace transport {

// ============================================================================
// TcpChannel
// ============================================================================

TcpChannel::TcpChannel(int fd) : fd_(fd) {}

TcpChannel::~TcpChannel() { close_channel(); }

void TcpChannel::close_channel() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (open_ && fd_ >= 0) {
        ::close(fd_);
        open_ = false;
    }
}

bool TcpChannel::is_open() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return open_;
}

bool TcpChannel::send_all(const void* data, size_t len) {
    const uint8_t* p = static_cast<const uint8_t*>(data);
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd_, p + sent, len - sent, MSG_NOSIGNAL);
        if (n <= 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool TcpChannel::recv_all(void* data, size_t len) {
    uint8_t* p = static_cast<uint8_t*>(data);
    size_t got = 0;
    while (got < len) {
        ssize_t n = ::recv(fd_, p + got, len - got, 0);
        if (n <= 0) return false;
        got += static_cast<size_t>(n);
    }
    return true;
}

protocol::Buffer TcpChannel::rpc(const protocol::Buffer& request) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Send: [4-byte length][payload]
    uint32_t len = static_cast<uint32_t>(request.size());
    uint8_t hdr[4];
    hdr[0] = (len >> 24) & 0xFF;
    hdr[1] = (len >> 16) & 0xFF;
    hdr[2] = (len >> 8) & 0xFF;
    hdr[3] = len & 0xFF;

    if (!send_all(hdr, 4) || !send_all(request.data().data(), len)) {
        throw std::runtime_error("TcpChannel: send failed");
    }

    // Receive: [4-byte length][payload]
    if (!recv_all(hdr, 4)) {
        throw std::runtime_error("TcpChannel: recv failed");
    }
    uint32_t rlen = (static_cast<uint32_t>(hdr[0]) << 24) |
                    (static_cast<uint32_t>(hdr[1]) << 16) |
                    (static_cast<uint32_t>(hdr[2]) << 8) |
                    static_cast<uint32_t>(hdr[3]);

    std::vector<uint8_t> data(rlen);
    if (rlen > 0 && !recv_all(data.data(), rlen)) {
        throw std::runtime_error("TcpChannel: recv payload failed");
    }

    return protocol::Buffer(std::move(data));
}

bool TcpChannel::recv_request(protocol::Buffer& out) {
    uint8_t hdr[4];
    if (!recv_all(hdr, 4)) return false;

    uint32_t len = (static_cast<uint32_t>(hdr[0]) << 24) |
                   (static_cast<uint32_t>(hdr[1]) << 16) |
                   (static_cast<uint32_t>(hdr[2]) << 8) |
                   static_cast<uint32_t>(hdr[3]);

    std::vector<uint8_t> data(len);
    if (len > 0 && !recv_all(data.data(), len)) return false;

    out = protocol::Buffer(std::move(data));
    return true;
}

void TcpChannel::send_response(const protocol::Buffer& response) {
    uint32_t len = static_cast<uint32_t>(response.size());
    uint8_t hdr[4];
    hdr[0] = (len >> 24) & 0xFF;
    hdr[1] = (len >> 16) & 0xFF;
    hdr[2] = (len >> 8) & 0xFF;
    hdr[3] = len & 0xFF;

    if (!send_all(hdr, 4) || !send_all(response.data().data(), len)) {
        throw std::runtime_error("TcpChannel: send response failed");
    }
}

// ============================================================================
// TcpServer
// ============================================================================

TcpServer::TcpServer(server::SimpleDBServer& db_server, uint16_t port)
    : db_server_(db_server) {
    port_.store(port);
}

TcpServer::~TcpServer() { stop(); }

void TcpServer::serve() {
    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        throw std::runtime_error("TcpServer: socket() failed");
    }

    int opt = 1;
    ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (::bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr),
               sizeof(addr)) < 0) {
        ::close(listen_fd_);
        throw std::runtime_error("TcpServer: bind() failed on port " +
                                 std::to_string(port_));
    }

    // If port was 0, get the assigned port
    if (port_ == 0) {
        socklen_t addrlen = sizeof(addr);
        ::getsockname(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr),
                      &addrlen);
        port_ = ntohs(addr.sin_port);
    }

    if (::listen(listen_fd_, 16) < 0) {
        ::close(listen_fd_);
        throw std::runtime_error("TcpServer: listen() failed");
    }

    running_ = true;

    while (running_) {
        struct sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        int client_fd = ::accept(
            listen_fd_, reinterpret_cast<struct sockaddr*>(&client_addr),
            &client_len);
        if (client_fd < 0) {
            if (!running_) break;
            continue;
        }
        std::thread(&TcpServer::handle_client, this, client_fd).detach();
    }
}

void TcpServer::stop() {
    running_ = false;
    if (listen_fd_ >= 0) {
        // Shutdown the listening socket to unblock accept()
        ::shutdown(listen_fd_, SHUT_RDWR);
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}

void TcpServer::handle_client(int client_fd) {
    TcpChannel channel(client_fd);
    protocol::Buffer req;
    while (channel.recv_request(req)) {
        protocol::Buffer resp;
        try {
            dispatch(db_server_, req, resp);
        } catch (const std::exception& e) {
            resp = protocol::Buffer();
            resp.write_uint8(static_cast<uint8_t>(protocol::Status::ERROR));
            resp.write_string(e.what());
        }
        try {
            channel.send_response(resp);
        } catch (...) {
            break;
        }
    }
}

void TcpServer::dispatch(server::SimpleDBServer& srv, protocol::Buffer& req,
                         protocol::Buffer& resp) {
    auto msg_type = static_cast<protocol::MsgType>(req.read_uint8());

    switch (msg_type) {
    case protocol::MsgType::CONN_CLOSE:
        srv.close_connection();
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        break;

    case protocol::MsgType::CONN_COMMIT:
        srv.commit();
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        break;

    case protocol::MsgType::CONN_ROLLBACK:
        srv.rollback();
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        break;

    case protocol::MsgType::STMT_EXEC_QUERY: {
        std::string query = req.read_string();
        uint64_t id = srv.execute_query(query);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_uint64(id);
        break;
    }

    case protocol::MsgType::STMT_EXEC_UPDATE: {
        std::string command = req.read_string();
        uint64_t count = srv.execute_update(command);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_uint64(count);
        break;
    }

    case protocol::MsgType::RS_NEXT: {
        uint64_t id = req.read_uint64();
        bool has_next = srv.next(id);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_bool(has_next);
        break;
    }

    case protocol::MsgType::RS_GET_INT: {
        uint64_t id = req.read_uint64();
        std::string fld = req.read_string();
        int32_t val = srv.get_int(id, fld);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_int32(val);
        break;
    }

    case protocol::MsgType::RS_GET_STRING: {
        uint64_t id = req.read_uint64();
        std::string fld = req.read_string();
        std::string val = srv.get_string(id, fld);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_string(val);
        break;
    }

    case protocol::MsgType::RS_CLOSE: {
        uint64_t id = req.read_uint64();
        srv.close_result_set(id);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        break;
    }

    case protocol::MsgType::MD_COL_COUNT: {
        uint64_t id = req.read_uint64();
        size_t count = srv.get_column_count(id);
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_uint64(static_cast<uint64_t>(count));
        break;
    }

    case protocol::MsgType::MD_COL_NAME: {
        uint64_t id = req.read_uint64();
        uint64_t col = req.read_uint64();
        std::string name = srv.get_column_name(id, static_cast<size_t>(col));
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_string(name);
        break;
    }

    case protocol::MsgType::MD_COL_TYPE: {
        uint64_t id = req.read_uint64();
        uint64_t col = req.read_uint64();
        int32_t type = srv.get_column_type(id, static_cast<size_t>(col));
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_int32(type);
        break;
    }

    case protocol::MsgType::MD_COL_DISPLAY: {
        uint64_t id = req.read_uint64();
        uint64_t col = req.read_uint64();
        size_t size = srv.get_column_display_size(id, static_cast<size_t>(col));
        resp.write_uint8(static_cast<uint8_t>(protocol::Status::OK));
        resp.write_uint64(static_cast<uint64_t>(size));
        break;
    }

    default:
        throw std::runtime_error("Unknown message type: " +
                                 std::to_string(static_cast<int>(msg_type)));
    }
}

// ============================================================================
// tcp_connect
// ============================================================================

int tcp_connect(const std::string& host, uint16_t port) {
    struct addrinfo hints{}, *result;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    std::string port_str = std::to_string(port);
    int err = ::getaddrinfo(host.c_str(), port_str.c_str(), &hints, &result);
    if (err != 0) {
        throw std::runtime_error("tcp_connect: getaddrinfo failed: " +
                                 std::string(gai_strerror(err)));
    }

    int fd = ::socket(result->ai_family, result->ai_socktype,
                      result->ai_protocol);
    if (fd < 0) {
        ::freeaddrinfo(result);
        throw std::runtime_error("tcp_connect: socket() failed");
    }

    if (::connect(fd, result->ai_addr, result->ai_addrlen) < 0) {
        ::close(fd);
        ::freeaddrinfo(result);
        throw std::runtime_error("tcp_connect: connect() failed to " + host +
                                 ":" + port_str);
    }

    ::freeaddrinfo(result);
    return fd;
}

} // namespace transport

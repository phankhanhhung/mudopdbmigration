#ifndef TCP_TRANSPORT_HPP
#define TCP_TRANSPORT_HPP

#include "server/protocol.hpp"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

namespace server { class SimpleDBServer; }

namespace transport {

// Low-level socket I/O: send/receive length-prefixed messages.
// Thread-safe: all operations protected by mutex.
class TcpChannel {
public:
    explicit TcpChannel(int fd);
    ~TcpChannel();

    TcpChannel(const TcpChannel&) = delete;
    TcpChannel& operator=(const TcpChannel&) = delete;

    // Send a request and receive the response (client-side RPC).
    // Thread-safe: only one request in flight at a time.
    protocol::Buffer rpc(const protocol::Buffer& request);

    // Receive a request message (server-side).
    // Returns false if connection closed.
    bool recv_request(protocol::Buffer& out);

    // Send a response message (server-side).
    void send_response(const protocol::Buffer& response);

    void close_channel();
    bool is_open() const;

private:
    bool send_all(const void* data, size_t len);
    bool recv_all(void* data, size_t len);

    int fd_;
    mutable std::mutex mutex_;
    bool open_ = true;
};

// TCP server: listens on a port, dispatches to SimpleDBServer.
class TcpServer {
public:
    TcpServer(server::SimpleDBServer& db_server, uint16_t port);
    ~TcpServer();

    // Start accepting connections (blocking). Call from a thread.
    void serve();

    // Stop the server.
    void stop();

    uint16_t port() const { return port_; }

private:
    void handle_client(int client_fd);
    void dispatch(server::SimpleDBServer& srv, protocol::Buffer& req,
                  protocol::Buffer& resp);

    server::SimpleDBServer& db_server_;
    std::atomic<uint16_t> port_;
    int listen_fd_ = -1;
    std::atomic<bool> running_{false};
};

// Connect to a TCP server. Returns socket fd or throws.
int tcp_connect(const std::string& host, uint16_t port);

} // namespace transport

#endif // TCP_TRANSPORT_HPP

/**
 * @file WSFrameServer.hpp
 * @brief Thread-safe WebSocket server for streaming encoded frames (JPEG) to
 * multiple clients.
 *
 * Uses uWebSockets as the underlying library.
 * Designed to integrate with FrameController's encoded frame callback.
 */
#pragma once

#include <atomic>
#include <mutex>
#include <set>
#include <thread>
#include <uWebSockets/App.h>
#include <vector>

namespace visioncore::network {

/**
 * @brief WebSocket frame server
 *
 * Handles multiple clients, sending frames over WebSocket.
 * The server runs in its own thread to avoid blocking.
 */
class WSFrameServer {
public:
  /**
   * @brief Per-client data structure
   */
  struct PerSocketData {
    uint64_t clientId = 0;
  };

  // Type alias for WebSocket
  using WebSocketType = uWS::WebSocket<false, true, PerSocketData>;

  WSFrameServer() = default;
  ~WSFrameServer();

  // Delete copy/move to avoid issues with thread management
  WSFrameServer(const WSFrameServer &) = delete;
  WSFrameServer &operator=(const WSFrameServer &) = delete;
  WSFrameServer(WSFrameServer &&) = delete;
  WSFrameServer &operator=(WSFrameServer &&) = delete;

  /**
   * @brief Start WebSocket server on given port (non-blocking)
   * @param port TCP port to bind
   * @return true if server started successfully, false otherwise
   */
  bool start(int port = 9001);

  /**
   * @brief Stop the WebSocket server
   */
  void stop();

  /**
   * @brief Check if server is running
   */
  bool isRunning() const { return running_; }

  /**
   * @brief Get number of connected clients
   */
  size_t getClientCount() const {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    return clients_.size();
  }

  /**
   * @brief Send a frame to all connected clients
   * @param data Binary frame data (JPEG encoded)
   */
  void sendFrame(const std::vector<unsigned char> &data);

private:
  /**
   * @brief Internal server thread function
   */
  void serverThreadFunc(int port);

  /**
   * @brief Add a client to the internal set
   */
  void addClient(WebSocketType *ws);

  /**
   * @brief Remove a client from the internal set
   */
  void removeClient(WebSocketType *ws);

private:
  mutable std::mutex clientsMutex_;
  std::set<WebSocketType *> clients_; // Use std::set instead of unordered_set

  std::atomic<bool> running_{false};
  std::thread serverThread_;

  // uWebSockets loop handle
  us_listen_socket_t *listenSocket_ = nullptr;
};

} // namespace visioncore::network

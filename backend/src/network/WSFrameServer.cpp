/**
 * @file WSFrameServer.cpp
 * @brief Implementation of WSFrameServer
 */

#include "WSFrameServer.hpp"
#include <chrono>
#include <iostream>

namespace visioncore::network {

WSFrameServer::~WSFrameServer() { stop(); }

void WSFrameServer::addClient(WebSocketType *ws) {
  std::lock_guard<std::mutex> lock(clientsMutex_);
  clients_.insert(ws);
  std::cout << "[WSFrameServer] Client connected. Total clients: "
            << clients_.size() << std::endl;
}

void WSFrameServer::removeClient(WebSocketType *ws) {
  std::lock_guard<std::mutex> lock(clientsMutex_);
  clients_.erase(ws);
  std::cout << "[WSFrameServer] Client disconnected. Total clients: "
            << clients_.size() << std::endl;
}

void WSFrameServer::sendFrame(const std::vector<unsigned char> &data) {
  if (!running_ || data.empty()) {
    return;
  }

  std::lock_guard<std::mutex> lock(clientsMutex_);

  if (clients_.empty()) {
    return;
  }

  // Convert to string_view for uWebSockets
  std::string_view view(reinterpret_cast<const char *>(data.data()),
                        data.size());

  // Send to all connected clients
  for (auto *client : clients_) {
    client->send(view, uWS::OpCode::BINARY);
  }
}

bool WSFrameServer::start(int port) {
  if (running_) {
    std::cerr << "[WSFrameServer] Server already running" << std::endl;
    return false;
  }

  running_ = true;

  // Launch server in separate thread
  serverThread_ = std::thread([this, port]() { serverThreadFunc(port); });

  // Wait a bit to ensure server starts
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  return running_;
}

void WSFrameServer::stop() {
  if (!running_) {
    return;
  }

  std::cout << "[WSFrameServer] Stopping server..." << std::endl;
  running_ = false;

  // Close the listen socket
  if (listenSocket_) {
    us_listen_socket_close(0, listenSocket_);
    listenSocket_ = nullptr;
  }

  // Wait for server thread to finish
  if (serverThread_.joinable()) {
    serverThread_.join();
  }

  // Clear all clients
  {
    std::lock_guard<std::mutex> lock(clientsMutex_);
    clients_.clear();
  }

  std::cout << "[WSFrameServer] Server stopped" << std::endl;
}

void WSFrameServer::serverThreadFunc(int port) {
  std::cout << "[WSFrameServer] Starting server thread on port " << port
            << std::endl;

  try {
    // Create the uWebSockets app
    auto app = uWS::App();

    // Configure WebSocket route
    app.ws<PerSocketData>(
        "/*",
        {
            // On connection open
            .open = [this](auto *ws) { this->addClient(ws); },

            // On message received (not used for frame streaming)
            .message =
                [](auto * /*ws*/, std::string_view /*msg*/,
                   uWS::OpCode /*opCode*/) {
                  // You can handle commands from clients here
                  std::cout << "[WSFrameServer] Received message from client"
                            << std::endl;
                },

            // Optional: handle ping/pong for keep-alive
            .ping = [](auto * /*ws*/, std::string_view) {},
            .pong = [](auto * /*ws*/, std::string_view) {},

            // On connection close
            .close =
                [this](auto *ws, int /*code*/, std::string_view /*message*/) {
                  this->removeClient(ws);
                },
        });

    // Start listening
    app.listen(port, [this, port](auto *token) {
      if (token) {
        listenSocket_ = token;
        std::cout << "[WSFrameServer] Listening on port " << port << std::endl;
      } else {
        std::cerr << "[WSFrameServer] Failed to listen on port " << port
                  << std::endl;
        running_ = false;
      }
    });

    // Run the event loop (this blocks until stop() is called)
    app.run();

  } catch (const std::exception &e) {
    std::cerr << "[WSFrameServer] Exception in server thread: " << e.what()
              << std::endl;
    running_ = false;
  }

  std::cout << "[WSFrameServer] Server thread exiting" << std::endl;
}

} // namespace visioncore::network

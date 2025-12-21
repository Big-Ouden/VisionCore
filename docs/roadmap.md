# AsciiVision - Roadmap de Développement Complète

## 🎯 Philosophie de la Roadmap

**Principe clé :** Développement incrémental avec **tests à chaque étape**. Chaque phase produit un exécutable fonctionnel que tu peux tester immédiatement.

**Approche :** Backend d'abord avec interfaces de debug (CLI/logs), puis frontend web progressivement.

---

## 📅 Vue d'ensemble (8-10 semaines)

```
SEMAINE 1-2    : Backend MVP (sources + filtres basiques)
SEMAINE 3      : Pipeline + Threading simple
SEMAINE 4      : Réseau WebSocket + Protocole
SEMAINE 5      : Frontend MVP (affichage basique)
SEMAINE 6      : Threading avancé + Optimisations
SEMAINE 7      : Frontend complet (contrôles)
SEMAINE 8-10   : Filtres avancés + Polish + Tests
```

---

## 🏗️ Phase 1 : Backend Fondations (Semaine 1)

### Objectif
Créer la structure de base et les sources vidéo avec **debug en ligne de commande**.

### Étape 1.1 : Setup projet (Jour 1 - 4h)

**Tâches :**
```bash
# Structure de dossiers
mkdir -p asciivision/backend/{src/{core,filters,pipeline,processing,network,utils},include,tests,build}
mkdir -p asciivision/frontend
cd asciivision/backend
```

**Fichiers à créer :**

**`CMakeLists.txt`** (racine backend)
```cmake
cmake_minimum_required(VERSION 3.15)
project(AsciiVision VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options de debug
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -fsanitize=address")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find packages
find_package(OpenCV 4 REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${OpenCV_INCLUDE_DIRS})

# Source files (ajoutés progressivement)
set(SOURCES
    src/main.cpp
)

# Executable
add_executable(asciivision ${SOURCES})

# Link libraries
target_link_libraries(asciivision 
    ${OpenCV_LIBS}
    Threads::Threads
)

# Installation
install(TARGETS asciivision DESTINATION bin)
```

**Test :**
```bash
cd build
cmake ..
make
./asciivision --help
```

**✅ Checkpoint :** Le projet compile et lance un "Hello World".

---

### Étape 1.2 : Logger + Utils (Jour 1 - 3h)

**Pourquoi en premier :** Tu vas en avoir besoin pour TOUT débugger.

**`src/utils/Logger.hpp`**
```cpp
#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>
#include <fstream>
#include <mutex>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace asciivision::utils {

enum class LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }
    
    void setLogLevel(LogLevel level) { min_level_ = level; }
    
    void log(LogLevel level, const std::string& message) {
        if (level < min_level_) return;
        
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        
        std::cout << "[" << std::put_time(std::localtime(&time), "%H:%M:%S") 
                  << "] [" << levelToString(level) << "] " 
                  << message << std::endl;
    }
    
    void debug(const std::string& msg)    { log(LogLevel::DEBUG, msg); }
    void info(const std::string& msg)     { log(LogLevel::INFO, msg); }
    void warning(const std::string& msg)  { log(LogLevel::WARNING, msg); }
    void error(const std::string& msg)    { log(LogLevel::ERROR, msg); }
    void critical(const std::string& msg) { log(LogLevel::CRITICAL, msg); }

private:
    Logger() : min_level_(LogLevel::DEBUG) {}
    LogLevel min_level_;
    std::mutex mutex_;
    
    std::string levelToString(LogLevel level) {
        switch(level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
        }
        return "UNKNOWN";
    }
};

#define LOG_DEBUG(msg)    asciivision::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg)     asciivision::utils::Logger::instance().info(msg)
#define LOG_WARNING(msg)  asciivision::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg)    asciivision::utils::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) asciivision::utils::Logger::instance().critical(msg)

} // namespace

#endif
```

**Test dans `main.cpp` :**
```cpp
#include "utils/Logger.hpp"

int main() {
    using namespace asciivision::utils;
    
    LOG_DEBUG("This is debug");
    LOG_INFO("AsciiVision starting...");
    LOG_WARNING("This is a warning");
    LOG_ERROR("This is an error");
    LOG_CRITICAL("This is critical");
    
    return 0;
}
```

**✅ Checkpoint :** Les logs s'affichent avec timestamp et niveau.

---

### Étape 1.3 : VideoSource interface (Jour 2 - 2h)

**`src/core/VideoSource.hpp`**
```cpp
#ifndef VIDEO_SOURCE_HPP
#define VIDEO_SOURCE_HPP

#include <opencv2/opencv.hpp>
#include <string>

namespace asciivision::core {

class VideoSource {
public:
    virtual ~VideoSource() = default;
    
    virtual bool open() = 0;
    virtual bool readFrame(cv::Mat& frame) = 0;
    virtual void close() = 0;
    
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual double getFPS() const = 0;
    virtual bool isOpened() const = 0;
    virtual std::string getName() const = 0;
};

} // namespace

#endif
```

**Test :** Compile juste pour vérifier la syntaxe.

---

### Étape 1.4 : WebcamSource implémentation (Jour 2 - 4h)

**`src/core/WebcamSource.cpp`**
```cpp
#include "WebcamSource.hpp"
#include "../utils/Logger.hpp"

namespace asciivision::core {

WebcamSource::WebcamSource(int device_id) 
    : device_id_(device_id), width_(0), height_(0), fps_(0) {}

WebcamSource::~WebcamSource() {
    close();
}

bool WebcamSource::open() {
    LOG_INFO("Opening webcam device " + std::to_string(device_id_));
    
    if (!capture_.open(device_id_)) {
        LOG_ERROR("Failed to open webcam " + std::to_string(device_id_));
        return false;
    }
    
    // Récupère les propriétés
    width_ = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_WIDTH));
    height_ = static_cast<int>(capture_.get(cv::CAP_PROP_FRAME_HEIGHT));
    fps_ = capture_.get(cv::CAP_PROP_FPS);
    
    LOG_INFO("Webcam opened: " + std::to_string(width_) + "x" + 
             std::to_string(height_) + " @ " + std::to_string(fps_) + " FPS");
    
    return true;
}

bool WebcamSource::readFrame(cv::Mat& frame) {
    if (!capture_.isOpened()) return false;
    return capture_.read(frame);
}

void WebcamSource::close() {
    if (capture_.isOpened()) {
        capture_.release();
        LOG_INFO("Webcam closed");
    }
}

int WebcamSource::getWidth() const { return width_; }
int WebcamSource::getHeight() const { return height_; }
double WebcamSource::getFPS() const { return fps_; }
bool WebcamSource::isOpened() const { return capture_.isOpened(); }
std::string WebcamSource::getName() const { return "Webcam " + std::to_string(device_id_); }

} // namespace
```

**Test dans `main.cpp` :**
```cpp
#include "core/WebcamSource.hpp"
#include "utils/Logger.hpp"
#include <opencv2/opencv.hpp>

int main() {
    using namespace asciivision;
    
    LOG_INFO("=== WebcamSource Test ===");
    
    // Crée la source
    auto webcam = std::make_unique<core::WebcamSource>(0);
    
    // Ouvre
    if (!webcam->open()) {
        LOG_ERROR("Cannot open webcam");
        return 1;
    }
    
    LOG_INFO("Webcam properties:");
    LOG_INFO("  Size: " + std::to_string(webcam->getWidth()) + "x" + 
             std::to_string(webcam->getHeight()));
    LOG_INFO("  FPS: " + std::to_string(webcam->getFPS()));
    
    // Capture 30 frames et affiche
    cv::Mat frame;
    for (int i = 0; i < 30; i++) {
        if (!webcam->readFrame(frame)) {
            LOG_ERROR("Failed to read frame");
            break;
        }
        
        cv::imshow("Webcam Test", frame);
        if (cv::waitKey(30) == 'q') break;
        
        if (i % 10 == 0) {
            LOG_DEBUG("Frame " + std::to_string(i) + " captured");
        }
    }
    
    webcam->close();
    cv::destroyAllWindows();
    
    LOG_INFO("Test completed");
    return 0;
}
```

**Compilation :**
```bash
cd build
cmake ..
make
./asciivision
```

**✅ Checkpoint :** Une fenêtre s'ouvre avec la webcam pendant 1 seconde, puis se ferme. Les logs affichent les infos.

---

### Étape 1.5 : ImageSource (Jour 2 - 2h)

**`src/core/ImageSource.cpp`** (similaire mais plus simple)

**Test :**
```cpp
// Dans main.cpp
auto image_src = std::make_unique<core::ImageSource>("test.jpg");
if (image_src->open()) {
    cv::Mat frame;
    for (int i = 0; i < 5; i++) {  // Lit 5 fois la même image
        image_src->readFrame(frame);
        cv::imshow("Image Test", frame);
        cv::waitKey(1000);  // 1 sec
    }
}
```

**✅ Checkpoint :** L'image fixe s'affiche 5 fois.

---

## 🎨 Phase 2 : Filtres Basiques (Semaine 1 fin + Semaine 2 début)

### Étape 2.1 : IFilter interface (Jour 3 - 1h)

**`src/filters/IFilter.hpp`**
```cpp
#ifndef IFILTER_HPP
#define IFILTER_HPP

#include <opencv2/opencv.hpp>
#include <string>
#include <nlohmann/json.hpp>

namespace asciivision::filters {

class IFilter {
public:
    virtual ~IFilter() = default;
    
    virtual void apply(const cv::Mat& input, cv::Mat& output) = 0;
    virtual void setParameter(const std::string& name, const nlohmann::json& value) = 0;
    virtual nlohmann::json getParameters() const = 0;
    virtual std::string getName() const = 0;
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

protected:
    bool enabled_ = true;
};

} // namespace

#endif
```

---

### Étape 2.2 : GrayscaleFilter (Jour 3 - 2h)

**`src/filters/GrayscaleFilter.cpp`**
```cpp
#include "GrayscaleFilter.hpp"
#include "../utils/Logger.hpp"

namespace asciivision::filters {

void GrayscaleFilter::apply(const cv::Mat& input, cv::Mat& output) {
    if (!enabled_) {
        output = input.clone();
        return;
    }
    
    if (input.channels() == 1) {
        output = input.clone();
    } else {
        cv::cvtColor(input, output, cv::COLOR_BGR2GRAY);
    }
}

void GrayscaleFilter::setParameter(const std::string& name, const nlohmann::json& value) {
    LOG_WARNING("GrayscaleFilter has no parameters");
}

nlohmann::json GrayscaleFilter::getParameters() const {
    return nlohmann::json::object();
}

std::string GrayscaleFilter::getName() const {
    return "grayscale";
}

} // namespace
```

**Test dans `main.cpp` :**
```cpp
#include "filters/GrayscaleFilter.hpp"

int main() {
    using namespace asciivision;
    
    auto webcam = std::make_unique<core::WebcamSource>(0);
    webcam->open();
    
    auto gray_filter = std::make_unique<filters::GrayscaleFilter>();
    
    cv::Mat frame, processed;
    for (int i = 0; i < 100; i++) {
        webcam->readFrame(frame);
        
        // Applique le filtre
        gray_filter->apply(frame, processed);
        
        // Affiche côte à côte
        cv::Mat display;
        cv::hconcat(frame, processed, display);  // Faut convertir processed en BGR
        cv::imshow("Original | Grayscale", display);
        
        if (cv::waitKey(30) == 'q') break;
    }
    
    return 0;
}
```

**✅ Checkpoint :** La vidéo s'affiche en couleur ET en niveaux de gris côte à côte.

---

### Étape 2.3 : ResizeFilter (Jour 3 - 2h)

Similaire à GrayscaleFilter mais avec paramètres (width, height).

**Test :**
```cpp
auto resize_filter = std::make_unique<filters::ResizeFilter>(320, 240);
resize_filter->apply(frame, processed);
// processed fait maintenant 320x240
```

---

### Étape 2.4 : AsciiFilter basique (Jour 4 - 4h)

**Version simple d'abord** : juste mapping pixel → caractère.

**`src/filters/AsciiFilter.cpp`**
```cpp
void AsciiFilter::apply(const cv::Mat& input, cv::Mat& output) {
    if (!enabled_) {
        output = input.clone();
        return;
    }
    
    // Input DOIT être en grayscale
    if (input.channels() != 1) {
        LOG_ERROR("AsciiFilter requires grayscale input");
        output = input.clone();
        return;
    }
    
    // Pour chaque pixel, trouve le caractère ASCII correspondant
    output = input.clone();
    for (int y = 0; y < input.rows; y++) {
        for (int x = 0; x < input.cols; x++) {
            uint8_t pixel = input.at<uint8_t>(y, x);
            int index = (pixel * charset_.length()) / 256;
            if (invert_) index = charset_.length() - 1 - index;
            
            // Remplace le pixel par l'index du caractère (temporaire)
            output.at<uint8_t>(y, x) = charset_[index];
        }
    }
}
```

**Test :**
```cpp
// Pipeline complet
auto webcam = std::make_unique<core::WebcamSource>(0);
auto resize = std::make_unique<filters::ResizeFilter>(160, 120);
auto gray = std::make_unique<filters::GrayscaleFilter>();
auto ascii = std::make_unique<filters::AsciiFilter>();

cv::Mat frame, resized, gray_frame, ascii_frame;
while (true) {
    webcam->readFrame(frame);
    resize->apply(frame, resized);
    gray->apply(resized, gray_frame);
    ascii->apply(gray_frame, ascii_frame);
    
    // Affiche (pour l'instant c'est encore une image)
    cv::imshow("ASCII", ascii_frame);
    if (cv::waitKey(30) == 'q') break;
}
```

**✅ Checkpoint :** Tu vois une image avec des valeurs ASCII (pas encore rendu texte, mais le mapping fonctionne).

---

## 🔄 Phase 3 : Pipeline (Semaine 2)

### Étape 3.1 : FramePipeline (Jour 5-6 - 6h)

**`src/pipeline/FramePipeline.cpp`**
```cpp
#include "FramePipeline.hpp"
#include "../utils/Logger.hpp"

namespace asciivision::pipeline {

void FramePipeline::addFilter(std::unique_ptr<filters::IFilter> filter) {
    std::lock_guard<std::mutex> lock(filters_mutex_);
    LOG_INFO("Adding filter: " + filter->getName());
    filters_.push_back(std::move(filter));
}

void FramePipeline::process(const cv::Mat& input, cv::Mat& output) {
    std::lock_guard<std::mutex> lock(filters_mutex_);
    
    if (filters_.empty()) {
        output = input.clone();
        return;
    }
    
    cv::Mat current = input.clone();
    cv::Mat temp;
    
    for (auto& filter : filters_) {
        if (filter->isEnabled()) {
            auto start = std::chrono::steady_clock::now();
            
            filter->apply(current, temp);
            current = temp;
            
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
            
            LOG_DEBUG(filter->getName() + " took " + std::to_string(ms) + "ms");
        }
    }
    
    output = current;
}

void FramePipeline::updateFilterParameter(
    const std::string& filter_name,
    const std::string& param_name,
    const nlohmann::json& value
) {
    std::lock_guard<std::mutex> lock(filters_mutex_);
    
    for (auto& filter : filters_) {
        if (filter->getName() == filter_name) {
            filter->setParameter(param_name, value);
            LOG_INFO("Updated " + filter_name + "." + param_name);
            return;
        }
    }
    
    LOG_WARNING("Filter not found: " + filter_name);
}

} // namespace
```

**Test :**
```cpp
int main() {
    using namespace asciivision;
    
    auto webcam = std::make_unique<core::WebcamSource>(0);
    webcam->open();
    
    // Crée le pipeline
    pipeline::FramePipeline pipeline;
    pipeline.addFilter(std::make_unique<filters::ResizeFilter>(160, 120));
    pipeline.addFilter(std::make_unique<filters::GrayscaleFilter>());
    pipeline.addFilter(std::make_unique<filters::AsciiFilter>());
    
    cv::Mat frame, processed;
    while (true) {
        webcam->readFrame(frame);
        
        // TOUT se fait en un appel !
        pipeline.process(frame, processed);
        
        cv::imshow("Pipeline Result", processed);
        if (cv::waitKey(30) == 'q') break;
    }
    
    return 0;
}
```

**✅ Checkpoint :** Le pipeline applique les 3 filtres automatiquement. Les logs montrent le temps de chaque filtre.

---

### Étape 3.2 : CLI interactive pour debug (Jour 6 - 3h)

**Ajoute une interface CLI** pour tester le pipeline interactivement.

**`src/main.cpp`** (version debug)
```cpp
#include <iostream>
#include <thread>
#include "core/WebcamSource.hpp"
#include "pipeline/FramePipeline.hpp"
#include "filters/GrayscaleFilter.hpp"
#include "filters/ResizeFilter.hpp"
#include "filters/AsciiFilter.hpp"

using namespace asciivision;

void displayHelp() {
    std::cout << "\n=== AsciiVision Debug CLI ===\n";
    std::cout << "Commands:\n";
    std::cout << "  start        - Start processing\n";
    std::cout << "  stop         - Stop processing\n";
    std::cout << "  toggle <N>   - Toggle filter N on/off\n";
    std::cout << "  param <N> <name> <value> - Set filter parameter\n";
    std::cout << "  stats        - Show statistics\n";
    std::cout << "  help         - Show this help\n";
    std::cout << "  quit         - Exit\n";
    std::cout << "==============================\n\n";
}

int main() {
    LOG_INFO("AsciiVision Debug Mode");
    
    // Setup
    auto webcam = std::make_unique<core::WebcamSource>(0);
    if (!webcam->open()) {
        LOG_ERROR("Cannot open webcam");
        return 1;
    }
    
    pipeline::FramePipeline pipeline;
    pipeline.addFilter(std::make_unique<filters::ResizeFilter>(160, 120));
    pipeline.addFilter(std::make_unique<filters::GrayscaleFilter>());
    pipeline.addFilter(std::make_unique<filters::AsciiFilter>());
    
    LOG_INFO("Pipeline ready with 3 filters");
    
    // Processing thread
    std::atomic<bool> running(false);
    std::thread processing_thread;
    
    displayHelp();
    
    std::string command;
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command == "quit") {
            running = false;
            if (processing_thread.joinable()) processing_thread.join();
            break;
        }
        else if (command == "start") {
            if (running) {
                std::cout << "Already running\n";
                continue;
            }
            
            running = true;
            processing_thread = std::thread([&]() {
                cv::Mat frame, processed;
                while (running) {
                    webcam->readFrame(frame);
                    pipeline.process(frame, processed);
                    
                    cv::imshow("AsciiVision Debug", processed);
                    if (cv::waitKey(30) == 'q') {
                        running = false;
                    }
                }
                cv::destroyAllWindows();
            });
            
            std::cout << "Processing started\n";
        }
        else if (command == "stop") {
            running = false;
            if (processing_thread.joinable()) processing_thread.join();
            std::cout << "Processing stopped\n";
        }
        else if (command == "help") {
            displayHelp();
        }
        else if (command == "stats") {
            std::cout << "Webcam: " << webcam->getWidth() << "x" << webcam->getHeight() 
                      << " @ " << webcam->getFPS() << " FPS\n";
        }
        else {
            std::cout << "Unknown command. Type 'help' for commands.\n";
        }
    }
    
    LOG_INFO("Shutting down...");
    return 0;
}
```

**Test :**
```bash
./asciivision
> help
> start        # Une fenêtre s'ouvre
> stop         # La fenêtre se ferme
> start
> (attends 5sec)
> stop
> quit
```

**✅ Checkpoint :** Tu as un CLI interactif pour contrôler le système sans WebSocket !

---

## 🌐 Phase 4 : Réseau & Protocole (Semaine 3)

### Étape 4.1 : WebSocketServer basique (Jour 7-8 - 8h)

**Install WebSocketpp** (header-only) :
```bash
git clone https://github.com/zaphoyd/websocketpp.git
# Copie dans /usr/local/include ou ajoute au include path
```

**`src/network/WebSocketServer.cpp`**
```cpp
#include "WebSocketServer.hpp"
#include "../utils/Logger.hpp"

namespace asciivision::network {

WebSocketServer::WebSocketServer(uint16_t port) : port_(port) {
    server_.set_access_channels(websocketpp::log::alevel::none);
    server_.set_error_channels(websocketpp::log::elevel::none);
    
    server_.init_asio();
    server_.set_reuse_addr(true);
    
    server_.set_open_handler([this](connection_hdl hdl) {
        this->onOpen(hdl);
    });
    
    server_.set_close_handler([this](connection_hdl hdl) {
        this->onClose(hdl);
    });
    
    server_.set_message_handler([this](connection_hdl hdl, message_ptr msg) {
        this->onMessage(hdl, msg);
    });
}

void WebSocketServer::start() {
    LOG_INFO("Starting WebSocket server on port " + std::to_string(port_));
    
    server_.listen(port_);
    server_.start_accept();
    
    server_thread_ = std::thread([this]() {
        server_.run();
    });
    
    LOG_INFO("WebSocket server started");
}

void WebSocketServer::stop() {
    LOG_INFO("Stopping WebSocket server");
    server_.stop_listening();
    server_.stop();
    
    if (server_thread_.joinable()) {
        server_thread_.join();
    }
}

void WebSocketServer::sendText(connection_hdl hdl, const std::string& message) {
    try {
        server_.send(hdl, message, websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send text: " + std::string(e.what()));
    }
}

void WebSocketServer::sendBinary(connection_hdl hdl, const std::vector<uint8_t>& data) {
    try {
        server_.send(hdl, data.data(), data.size(), websocketpp::frame::opcode::binary);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to send binary: " + std::string(e.what()));
    }
}

void WebSocketServer::broadcastBinary(const std::vector<uint8_t>& data) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    
    for (auto& hdl : connections_) {
        sendBinary(hdl, data);
    }
}

void WebSocketServer::onOpen(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.insert(hdl);
    LOG_INFO("Client connected. Total clients: " + std::to_string(connections_.size()));
    
    if (connection_callback_) {
        connection_callback_(hdl);
    }
}

void WebSocketServer::onClose(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    connections_.erase(hdl);
    LOG_INFO("Client disconnected. Total clients: " + std::to_string(connections_.size()));
    
    if (disconnection_callback_) {
        disconnection_callback_(hdl);
    }
}

void WebSocketServer::onMessage(connection_hdl hdl, message_ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::text) {
        LOG_DEBUG("Received text message: " + msg->get_payload());
        
        if (message_callback_) {
            message_callback_(hdl, msg->get_payload());
        }
    } else if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        const std::string& payload = msg->get_payload();
        std::vector<uint8_t> data(payload.begin(), payload.end());
        
        LOG_DEBUG("Received binary message: " + std::to_string(data.size()) + " bytes");
        
        if (binary_callback_) {
            binary_callback_(hdl, data);
        }
    }
}

} // namespace
```

**Test dans `main.cpp` :**
```cpp
#include "network/WebSocketServer.hpp"

int main() {
    using namespace asciivision;
    
    network::WebSocketServer server(9002);
    
    server.setMessageCallback([](auto hdl, const std::string& msg) {
        LOG_INFO("Got message: " + msg);
    });
    
    server.setConnectionCallback([](auto hdl) {
        LOG_INFO("New client!");
    });
    
    server.start();
    
    LOG_INFO("Server running. Press Enter to stop...");
    std::cin.get();
    
    server.stop();
    return 0;
}
```

**Test avec navigateur :**
```javascript
// Console navigateur (F12)
const ws = new WebSocket('ws://localhost:9002');
ws.onopen = () => { console.log('Connected'); ws.send('Hello from browser!'); };
ws.onmessage = (event) => { console.log('Received:', event.data); };
```

**✅ Checkpoint :** Le backend reçoit le message du navigateur et log "Got message: Hello from browser!".

---

### Étape 4.2 : Intégration Pipeline + WebSocket (Jour 9 - 6h)

**Objectif :** Envoyer les frames traitées au navigateur.

**`src/main.cpp`** (version réseau)
```cpp
int main() {
    using namespace asciivision;
    
    // Setup pipeline
    auto webcam = std::make_unique<core::WebcamSource>(0);
    webcam->

open();
    
    pipeline::FramePipeline pipeline;
    pipeline.addFilter(std::make_unique<filters::ResizeFilter>(320, 240));
    pipeline.addFilter(std::make_unique<filters::GrayscaleFilter>());
    
    // Setup WebSocket
    network::WebSocketServer server(9002);
    
    // Processing thread
    std::atomic<bool> running(false);
    std::thread processing_thread;
    
    server.setMessageCallback([&](auto hdl, const std::string& msg) {
        LOG_INFO("Command: " + msg);
        
        if (msg == "start") {
            if (running) return;
            
            running = true;
            processing_thread = std::thread([&]() {
                cv::Mat frame, processed;
                std::vector<uint8_t> jpeg_buffer;
                
                while (running) {
                    // Capture + Process
                    webcam->readFrame(frame);
                    pipeline.process(frame, processed);
                    
                    // Encode JPEG
                    std::vector<int> params = {cv::IMWRITE_JPEG_QUALITY, 80};
                    cv::imencode(".jpg", processed, jpeg_buffer, params);
                    
                    // Envoie à TOUS les clients
                    server.broadcastBinary(jpeg_buffer);
                    
                    std::this_thread::sleep_for(std::chrono::milliseconds(33)); // ~30 FPS
                }
            });
            
            LOG_INFO("Started processing");
        }
        else if (msg == "stop") {
            running = false;
            if (processing_thread.joinable()) processing_thread.join();
            LOG_INFO("Stopped processing");
        }
    });
    
    server.start();
    
    LOG_INFO("Server ready. Clients can connect on ws://localhost:9002");
    LOG_INFO("Send 'start' to begin, 'stop' to end");
    std::cin.get();
    
    running = false;
    if (processing_thread.joinable()) processing_thread.join();
    server.stop();
    
    return 0;
}
```

**Test avec HTML temporaire** (`test.html`) :
```html
<!DOCTYPE html>
<html>
<head>
    <title>AsciiVision Test</title>
</head>
<body>
    <h1>AsciiVision Test Client</h1>
    <button onclick="start()">Start</button>
    <button onclick="stop()">Stop</button>
    <br><br>
    <canvas id="canvas" width="320" height="240"></canvas>
    
    <script>
        const ws = new WebSocket('ws://localhost:9002');
        const canvas = document.getElementById('canvas');
        const ctx = canvas.getContext('2d');
        
        ws.onopen = () => {
            console.log('Connected to backend');
        };
        
        ws.onmessage = async (event) => {
            if (event.data instanceof Blob) {
                // C'est une frame JPEG
                const url = URL.createObjectURL(event.data);
                const img = new Image();
                img.onload = () => {
                    ctx.drawImage(img, 0, 0);
                    URL.revokeObjectURL(url);
                };
                img.src = url;
            }
        };
        
        function start() {
            ws.send('start');
        }
        
        function stop() {
            ws.send('stop');
        }
    </script>
</body>
</html>
```

**Test :**
```bash
# Terminal 1
./asciivision

# Terminal 2 (ou navigateur)
open test.html   # Ou firefox test.html
# Clique sur "Start" → La vidéo s'affiche dans le canvas !
```

**✅ Checkpoint :** 🎉 **TU AS UN SYSTÈME END-TO-END FONCTIONNEL !** La webcam du serveur s'affiche dans le navigateur en temps réel.

---

## 🚀 Phase 5 : Threading Avancé (Semaine 4)

### Étape 5.1 : ThreadSafeQueue (Jour 10 - 4h)

**`src/utils/ThreadSafeQueue.hpp`**
```cpp
template<typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue(size_t max_capacity = 0);
    
    bool push(T item, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    bool pop(T& item, std::chrono::milliseconds timeout = std::chrono::milliseconds(0));
    
    void close();
    bool isClosed() const;
    size_t size() const;
    
private:
    std::queue<T> queue_;
    size_t max_capacity_;
    std::atomic<bool> closed_;
    mutable std::mutex mutex_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
};
```

**Test unitaire** (`tests/test_queue.cpp`) :
```cpp
#include "utils/ThreadSafeQueue.hpp"
#include <thread>
#include <cassert>

int main() {
    ThreadSafeQueue<int> queue(3);
    
    // Test push/pop
    queue.push(1);
    queue.push(2);
    queue.push(3);
    
    int val;
    assert(queue.pop(val) && val == 1);
    assert(queue.pop(val) && val == 2);
    assert(queue.pop(val) && val == 3);
    
    // Test multithread
    std::thread producer([&]() {
        for (int i = 0; i < 100; i++) {
            queue.push(i);
        }
    });
    
    std::thread consumer([&]() {
        int val;
        for (int i = 0; i < 100; i++) {
            assert(queue.pop(val) && val == i);
        }
    });
    
    producer.join();
    consumer.join();
    
    std::cout << "✓ All queue tests passed\n";
    return 0;
}
```

**✅ Checkpoint :** Les tests passent sans deadlock ni data race.

---

### Étape 5.2 : FrameController avec 4 threads (Jour 11-12 - 12h)

C'est la **partie la plus complexe**. Découpe-la en sous-étapes :

**5.2.1 : Version 1 thread (déjà fait)**

**5.2.2 : Version 2 threads (capture + processing)**

**5.2.3 : Version 3 threads (+ encoding)**

**5.2.4 : Version 4 threads (+ network)**

Pour chaque version, **teste** que ça fonctionne avant de passer à la suivante.

**Test de performance :**
```cpp
// Ajoute dans le processing loop
auto start = std::chrono::steady_clock::now();
// ... traitement ...
auto elapsed = std::chrono::steady_clock::now() - start;
LOG_DEBUG("Frame took " + std::to_string(
    std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
) + "ms");
```

**✅ Checkpoint :** Le système tourne à 30 FPS stable avec logs montrant le temps de chaque thread.

---

## 🎨 Phase 6 : Frontend Next.js (Semaine 5)

### Étape 6.1 : Setup Next.js (Jour 13 - 2h)

```bash
cd asciivision/frontend
npx create-next-app@latest . --typescript --tailwind --app
npm install
```

**Test :**
```bash
npm run dev
# Ouvre http://localhost:3000
```

---

### Étape 6.2 : Hook WebSocket (Jour 13 - 3h)

**`src/hooks/useWebSocket.ts`** (version MVP)
```typescript
import { useEffect, useRef, useState } from 'react';

export function useWebSocket(url: string) {
  const [connected, setConnected] = useState(false);
  const [lastFrame, setLastFrame] = useState<Blob | null>(null);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    const ws = new WebSocket(url);
    wsRef.current = ws;

    ws.onopen = () => {
      console.log('Connected');
      setConnected(true);
    };

    ws.onclose = () => {
      console.log('Disconnected');
      setConnected(false);
    };

    ws.onmessage = (event) => {
      if (event.data instanceof Blob) {
        setLastFrame(event.data);
      }
    };

    return () => ws.close();
  }, [url]);

  const send = (message: string) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(message);
    }
  };

  return { connected, lastFrame, send };
}
```

---

### Étape 6.3 : Composant VideoCanvas (Jour 14 - 2h)

**`src/components/VideoCanvas.tsx`**
```typescript
'use client';

import { useEffect, useRef } from 'react';

interface Props {
  frame: Blob | null;
}

export function VideoCanvas({ frame }: Props) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (!frame || !canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const url = URL.createObjectURL(frame);
    const img = new Image();
    img.onload = () => {
      canvas.width = img.width;
      canvas.height = img.height;
      ctx.drawImage(img, 0, 0);
      URL.revokeObjectURL(url);
    };
    img.src = url;
  }, [frame]);

  return <canvas ref={canvasRef} className="border border-gray-300" />;
}
```

---

### Étape 6.4 : Page principale (Jour 14 - 2h)

**`src/app/page.tsx`**
```typescript
'use client';

import { useWebSocket } from '@/hooks/useWebSocket';
import { VideoCanvas } from '@/components/VideoCanvas';

export default function Home() {
  const { connected, lastFrame, send } = useWebSocket('ws://localhost:9002');

  return (
    <main className="p-8">
      <h1 className="text-3xl font-bold mb-4">AsciiVision</h1>
      
      <div className="flex gap-4 mb-4">
        <button 
          onClick={() => send('start')}
          className="px-4 py-2 bg-blue-600 text-white rounded"
        >
          Start
        </button>
        <button 
          onClick={() => send('stop')}
          className="px-4 py-2 bg-red-600 text-white rounded"
        >
          Stop
        </button>
        
        <div className="ml-auto">
          Status: {connected ? '🟢 Connected' : '🔴 Disconnected'}
        </div>
      </div>
      
      <VideoCanvas frame={lastFrame} />
    </main>
  );
}
```

**Test :**
```bash
# Terminal 1 : Backend
cd backend/build
./asciivision

# Terminal 2 : Frontend
cd frontend
npm run dev

# Navigateur : http://localhost:3000
# Clique Start → La vidéo s'affiche !
```

**✅ Checkpoint :** 🎉 **SYSTÈME COMPLET FONCTIONNEL !** Frontend + Backend communiquent.

---

## 🔥 Semaines 6-10 : Features & Polish

### Semaine 6 : Contrôles avancés
- FilterControls component
- Paramètres en temps réel
- StatusBar avec métriques

### Semaine 7 : NetworkSource
- Capture webcam client
- Envoi frames au backend
- Tests mode hybride

### Semaine 8 : Filtres additionnels
- BlurFilter
- EdgeDetectionFilter
- FaceDetectionFilter (Haar Cascades)

### Semaine 9 : Optimisations
- Profiling (perf, valgrind)
- Lock-free queues
- Protocole binaire optimisé

### Semaine 10 : Tests & Documentation
- Tests unitaires (Google Test)
- Tests d'intégration
- Documentation utilisateur
- Déploiement (Docker)

---

## ✅ Checklist Quotidienne

Chaque jour, avant de coder :

1. [ ] Relis l'objectif de l'étape
2. [ ] Lance `git status` et commit ton travail précédent
3. [ ] Compile et lance les tests existants
4. [ ] Écris les logs pour la nouvelle feature

Chaque jour, après avoir codé :

1. [ ] Teste manuellement la feature
2. [ ] Vérifie les logs
3. [ ] Commit avec message clair
4. [ ] Note les bugs/idées dans un TODO.md

---

## 🆘 Si tu es bloqué

**Debugging Workflow :**

1. **Ajoute des logs partout**
   ```cpp
   LOG_DEBUG("Entering function X");
   LOG_DEBUG("Variable Y = " + std::to_string(y));
   ```

2. **Compile avec sanitizers**
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug ..
   # Vérifie que -fsanitize=address est actif
   ```

3. **Utilise GDB**
   ```bash
   gdb ./asciivision
   (gdb) run
   (gdb) backtrace  # Si ça crash
   ```

4. **Tests isolés**
   - Crée un `test_X.cpp` avec juste la feature problématique
   - Teste sans le reste du système

5. **Demande de l'aide**
   - Stack Overflow avec code minimal reproduisant le bug
   - GitHub issues des libs utilisées

---

**Bon courage ! Chaque checkpoint validé est une victoire. 🚀**

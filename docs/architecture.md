# AsciiVision

## Documentation Technique Complète – Architecture & Protocoles

---

## 1. Vue d'ensemble du système

AsciiVision est un moteur de traitement vidéo temps réel distribué, séparant strictement le calcul (backend C++) de l'interface (frontend web).

### Architecture globale

```
┌─────────────────────────────────────────────────────────────┐
│                         FRONTEND                            │
│  ┌──────────┐    ┌──────────┐    ┌──────────────────────┐  │
│  │  React   │───→│WebSocket │───→│   Canvas Renderer    │  │
│  │   UI     │    │  Client  │    │   (décodage JPEG)    │  │
│  └──────────┘    └──────────┘    └──────────────────────┘  │
└───────────────────────────┬─────────────────────────────────┘
                            │
                    WebSocket (port 9002)
                            │
                   ┌────────┴────────┐
                   │   JSON Control  │ (commandes)
                   │   Binary Data   │ (frames vidéo)
                   └────────┬────────┘
                            │
┌───────────────────────────┴─────────────────────────────────┐
│                      BACKEND C++                            │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────────┐  │
│  │  WebSocket   │→ │    Frame     │→ │  Video Pipeline  │  │
│  │   Server     │  │  Controller  │  │   & Filters      │  │
│  └──────────────┘  └──────────────┘  └──────────────────┘  │
│                                              ↑               │
│                                    ┌─────────┴────────┐      │
│                                    │  Video Sources   │      │
│                                    │ (webcam, images) │      │
│                                    └──────────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

---

## 2. Architecture détaillée du Backend

### 2.1 Chaîne de traitement complète

```
Source → Acquisition → Normalisation → Filtrage → Rendu ASCII → Encodage → Transport
   ↓          ↓             ↓            ↓            ↓            ↓          ↓
Webcam    cv::Mat      resize()      Pipeline    AsciiFilter   JPEG      WebSocket
ImageFile  readFrame   cvtColor     N filtres    charset map   compress   binaire
```

### 2.2 Modèle de threading recommandé

```
┌──────────────┐      ┌──────────────┐      ┌──────────────┐      ┌──────────────┐
│   THREAD 1   │      │   THREAD 2   │      │   THREAD 3   │      │   THREAD 4   │
│   Capture    │─────→│  Processing  │─────→│   Encoding   │─────→│   Network    │
│              │      │              │      │              │      │              │
│ VideoSource  │      │ FramePipeline│      │ JPEG Encoder │      │ WebSocket    │
│ readFrame()  │      │ applyFilters │      │ cv::imencode │      │ send_binary  │
└──────────────┘      └──────────────┘      └──────────────┘      └──────────────┘
       │                     │                     │                     │
       ↓                     ↓                     ↓                     ↓
  Queue<Mat>          Queue<Mat>          Queue<Buffer>         Queue<Message>
  (raw frames)        (processed)          (JPEG data)          (outgoing)
  
  Capacity: 2         Capacity: 2          Capacity: 3          Capacity: 5
  Blocking: yes       Blocking: yes        Blocking: yes        Blocking: no
```

**Avantages critiques** :
- Aucun mutex dans le code métier
- Backpressure automatique (queues pleines = ralentissement capture)
- Latence stable et mesurable
- Isolation des pannes (thread crash n'affecte pas les autres)

---

## 3. Protocole de Communication

### 3.1 Principes fondamentaux

```
╔════════════════════════════════════════════════════════════╗
║                   SÉPARATION STRICTE                       ║
╠════════════════════════════════════════════════════════════╣
║  Canal CONTRÔLE          │  Canal DONNÉES                  ║
║  ─────────────           │  ──────────                     ║
║  Format: JSON            │  Format: Binaire                ║
║  Direction: Bi-directionnelle │ Direction: Backend→Frontend║
║  Fréquence: événementielle    │ Fréquence: 30 FPS          ║
║  Taille: <1 KB           │  Taille: 10-100 KB             ║
╚════════════════════════════════════════════════════════════╝
```

### 3.2 Messages de contrôle (JSON)

#### Structure générale

```json
{
  "version": "1.0",
  "type": "command_name",
  "timestamp": 1703001234567,
  "payload": { }
}
```

#### Messages Frontend → Backend

**1. Démarrage du moteur**
```json
{
  "version": "1.0",
  "type": "start_engine",
  "timestamp": 1703001234567,
  "payload": {
    "source": "webcam",
    "source_config": {
      "device_id": 0,
      "width": 640,
      "height": 480,
      "fps": 30
    },
    "pipeline": {
      "target_width": 160,
      "target_height": 120
    }
  }
}
```

**2. Configuration des filtres**
```json
{
  "version": "1.0",
  "type": "update_filters",
  "timestamp": 1703001234568,
  "payload": {
    "filters": [
      {
        "name": "grayscale",
        "enabled": true,
        "params": {}
      },
      {
        "name": "ascii",
        "enabled": true,
        "params": {
          "charset": "standard",
          "invert": false,
          "scale": 1.0
        }
      }
    ]
  }
}
```

**3. Modification paramètres en temps réel**
```json
{
  "version": "1.0",
  "type": "set_parameter",
  "timestamp": 1703001234569,
  "payload": {
    "filter": "ascii",
    "parameter": "charset",
    "value": "extended"
  }
}
```

**4. Arrêt du moteur**
```json
{
  "version": "1.0",
  "type": "stop_engine",
  "timestamp": 1703001234570,
  "payload": {}
}
```

#### Messages Backend → Frontend

**1. Statut du moteur**
```json
{
  "version": "1.0",
  "type": "engine_status",
  "timestamp": 1703001234571,
  "payload": {
    "state": "running",
    "fps": 29.8,
    "frame_count": 1247,
    "latency_ms": 12.3,
    "dropped_frames": 2
  }
}
```

**2. Erreur**
```json
{
  "version": "1.0",
  "type": "error",
  "timestamp": 1703001234572,
  "payload": {
    "code": "SOURCE_UNAVAILABLE",
    "message": "Cannot open webcam device 0",
    "recoverable": false
  }
}
```

**3. Capacités du système**
```json
{
  "version": "1.0",
  "type": "capabilities",
  "timestamp": 1703001234573,
  "payload": {
    "sources": ["webcam", "image", "sequence"],
    "filters": [
      {
        "name": "grayscale",
        "parameters": []
      },
      {
        "name": "ascii",
        "parameters": [
          {
            "name": "charset",
            "type": "enum",
            "values": ["minimal", "standard", "extended", "blocks"],
            "default": "standard"
          },
          {
            "name": "invert",
            "type": "boolean",
            "default": false
          }
        ]
      }
    ]
  }
}
```

---

### 3.3 Messages vidéo (Binaire)

#### Format de trame (Phase MVP - avec Base64)

```
WebSocket Text Frame
│
└─→ JSON String
    {
      "type": "frame",
      "frame_id": 1247,
      "timestamp": 1703001234574,
      "width": 160,
      "height": 120,
      "format": "jpeg",
      "data": "base64_encoded_jpeg_data..."
    }
```

**⚠️ PROBLÈME** : Base64 augmente la taille de 33% et coûte en CPU.

#### Format cible (Production - Binaire pur)

```
WebSocket Binary Frame
┌────────────────────────────────────────────────────────────┐
│                     HEADER (24 bytes)                      │
├─────────┬──────────┬─────────┬─────────┬──────────┬────────┤
│ Magic   │ Version  │ Frame   │ Timestamp│ Width    │ Height │
│ 4 bytes │ 2 bytes  │ ID      │ 8 bytes  │ 2 bytes  │ 2 bytes│
│         │          │ 4 bytes │          │          │        │
│ "AVIS"  │ 0x0100   │ uint32  │ uint64   │ uint16   │ uint16 │
└─────────┴──────────┴─────────┴──────────┴──────────┴────────┘
┌────────────────────────────────────────────────────────────┐
│                  JPEG PAYLOAD (variable)                   │
│                                                            │
│  Raw JPEG compressed data (FFD8...FFD9)                    │
│                                                            │
└────────────────────────────────────────────────────────────┘
```

**Parsing côté frontend** :
```typescript
function parseVideoFrame(buffer: ArrayBuffer): VideoFrame {
  const view = new DataView(buffer);
  
  // Vérification magic number
  const magic = String.fromCharCode(
    view.getUint8(0), view.getUint8(1),
    view.getUint8(2), view.getUint8(3)
  );
  if (magic !== "AVIS") throw new Error("Invalid frame");
  
  // Extraction header
  const version = view.getUint16(4, true);
  const frameId = view.getUint32(6, true);
  const timestamp = Number(view.getBigUint64(10, true));
  const width = view.getUint16(18, true);
  const height = view.getUint16(20, true);
  
  // Extraction JPEG
  const jpegData = buffer.slice(24);
  
  return { frameId, timestamp, width, height, jpegData };
}
```

**Encodage côté backend** :
```cpp
struct FrameHeader {
    char magic[4] = {'A', 'V', 'I', 'S'};
    uint16_t version = 0x0100;
    uint32_t frame_id;
    uint64_t timestamp;
    uint16_t width;
    uint16_t height;
} __attribute__((packed));

std::vector<uint8_t> encodeFrame(
    uint32_t frame_id,
    const cv::Mat& jpeg_buffer,
    uint16_t width,
    uint16_t height
) {
    FrameHeader header;
    header.frame_id = frame_id;
    header.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    header.width = width;
    header.height = height;
    
    std::vector<uint8_t> frame(sizeof(header) + jpeg_buffer.total());
    std::memcpy(frame.data(), &header, sizeof(header));
    std::memcpy(frame.data() + sizeof(header), 
                jpeg_buffer.data, 
                jpeg_buffer.total());
    
    return frame;
}
```

---

## 4. Diagrammes de séquence

### 4.1 Initialisation complète

```
Frontend          WebSocket          FrameController     VideoSource      Pipeline
   │                  │                     │                 │              │
   │─────connect─────→│                     │                 │              │
   │←────accepted─────│                     │                 │              │
   │                  │                     │                 │              │
   │─get_capabilities→│                     │                 │              │
   │                  │─────query──────────→│                 │              │
   │                  │←────response────────│                 │              │
   │←─capabilities────│                     │                 │              │
   │                  │                     │                 │              │
   │──start_engine───→│                     │                 │              │
   │                  │─────start──────────→│                 │              │
   │                  │                     │────open────────→│              │
   │                  │                     │←────ok──────────│              │
   │                  │                     │──init_pipeline─────────────────→│
   │                  │                     │←────ready──────────────────────│
   │                  │                     │─start_threads──→│              │
   │                  │←─engine_status──────│                 │              │
   │←─engine_status───│                     │                 │              │
   │                  │                     │                 │              │
   │                  │←────frame_data──────│←────capture─────│──process────→│
   │←────frame────────│                     │                 │              │
   │  (render)        │                     │                 │              │
   │                  │←────frame_data──────│←────capture─────│──process────→│
   │←────frame────────│                     │                 │              │
```

### 4.2 Modification paramètre en temps réel

```
Frontend          WebSocket          FrameController        Pipeline
   │                  │                     │                  │
   │─set_parameter───→│                     │                  │
   │  (ascii.invert)  │─────update─────────→│                  │
   │                  │                     │──modify_filter──→│
   │                  │                     │  (atomic swap)   │
   │                  │                     │←─────ok──────────│
   │                  │←────ack─────────────│                  │
   │←─────ack─────────│                     │                  │
   │                  │                     │                  │
   │                  │←────frame_data──────│                  │
   │←────frame────────│   (avec nouveau     │                  │
   │  (rendu changé)  │    paramètre)       │                  │
```

### 4.3 Gestion d'erreur

```
Frontend          WebSocket          FrameController     VideoSource
   │                  │                     │                 │
   │──start_engine───→│                     │                 │
   │                  │─────start──────────→│                 │
   │                  │                     │────open────────→│
   │                  │                     │←────error───────│
   │                  │                     │ (device busy)   │
   │                  │←─────error──────────│                 │
   │←─────error───────│                     │                 │
   │  (affiche popup) │                     │                 │
   │                  │                     │                 │
   │───retry──────────→│                     │                 │
   │                  │─────start──────────→│                 │
   │                  │                     │────open────────→│
   │                  │                     │←─────ok─────────│
   │←─engine_status───│←─engine_status──────│                 │
```

---

## 5. Implémentation Backend - Classes détaillées

### 5.1 VideoSource (abstraction)

```cpp
class VideoSource {
public:
    virtual ~VideoSource() = default;
    
    virtual bool open() = 0;
    virtual bool readFrame(cv::Mat& frame) = 0;
    virtual void close() = 0;
    
    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;
    virtual double getFPS() const = 0;
};

class WebcamSource : public VideoSource {
private:
    cv::VideoCapture capture_;
    int device_id_;
    
public:
    explicit WebcamSource(int device_id) 
        : device_id_(device_id) {}
    
    bool open() override {
        return capture_.open(device_id_);
    }
    
    bool readFrame(cv::Mat& frame) override {
        return capture_.read(frame);
    }
    
    // ... autres méthodes
};
```

### 5.2 IFilter (interface)

```cpp
class IFilter {
public:
    virtual ~IFilter() = default;
    
    virtual void apply(const cv::Mat& input, cv::Mat& output) = 0;
    virtual void setParameter(const std::string& name, 
                             const nlohmann::json& value) = 0;
    virtual nlohmann::json getParameters() const = 0;
    
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
protected:
    bool enabled_ = true;
};

class AsciiFilter : public IFilter {
private:
    std::string charset_ = " .:-=+*#%@";
    bool invert_ = false;
    
public:
    void apply(const cv::Mat& input, cv::Mat& output) override {
        if (!enabled_) {
            output = input.clone();
            return;
        }
        
        // Conversion en ASCII art
        output = cv::Mat(input.rows, input.cols, CV_8UC1);
        
        for (int y = 0; y < input.rows; y++) {
            for (int x = 0; x < input.cols; x++) {
                uint8_t pixel = input.at<uint8_t>(y, x);
                int index = (pixel * charset_.length()) / 256;
                if (invert_) index = charset_.length() - 1 - index;
                output.at<uint8_t>(y, x) = charset_[index];
            }
        }
    }
    
    void setParameter(const std::string& name, 
                     const nlohmann::json& value) override {
        if (name == "charset") charset_ = value.get<std::string>();
        else if (name == "invert") invert_ = value.get<bool>();
    }
};
```

### 5.3 FramePipeline

```cpp
class FramePipeline {
private:
    std::vector<std::unique_ptr<IFilter>> filters_;
    std::mutex filters_mutex_;  // Protection modifications
    
public:
    void addFilter(std::unique_ptr<IFilter> filter) {
        std::lock_guard<std::mutex> lock(filters_mutex_);
        filters_.push_back(std::move(filter));
    }
    
    void process(const cv::Mat& input, cv::Mat& output) {
        std::lock_guard<std::mutex> lock(filters_mutex_);
        
        cv::Mat current = input.clone();
        cv::Mat temp;
        
        for (auto& filter : filters_) {
            if (filter->isEnabled()) {
                filter->apply(current, temp);
                current = temp;
            }
        }
        
        output = current;
    }
    
    void updateFilterParameter(const std::string& filter_name,
                               const std::string& param_name,
                               const nlohmann::json& value) {
        std::lock_guard<std::mutex> lock(filters_mutex_);
        
        // Trouver le filtre et mettre à jour
        // Atomique grâce au mutex
    }
};
```

### 5.4 FrameController (orchestrateur)

```cpp
class FrameController {
private:
    std::unique_ptr<VideoSource> source_;
    std::unique_ptr<FramePipeline> pipeline_;
    
    std::atomic<bool> running_{false};
    std::thread capture_thread_;
    
    std::function<void(const std::vector<uint8_t>&)> frame_callback_;
    
public:
    void start(std::unique_ptr<VideoSource> source) {
        if (running_) return;
        
        source_ = std::move(source);
        if (!source_->open()) {
            throw std::runtime_error("Cannot open source");
        }
        
        running_ = true;
        capture_thread_ = std::thread(&FrameController::captureLoop, this);
    }
    
    void stop() {
        running_ = false;
        if (capture_thread_.joinable()) {
            capture_thread_.join();
        }
        source_->close();
    }
    
private:
    void captureLoop() {
        cv::Mat raw_frame, processed_frame;
        std::vector<uint8_t> jpeg_buffer;
        
        while (running_) {
            auto start = std::chrono::steady_clock::now();
            
            // Capture
            if (!source_->readFrame(raw_frame)) break;
            
            // Processing
            pipeline_->process(raw_frame, processed_frame);
            
            // Encodage JPEG
            cv::imencode(".jpg", processed_frame, jpeg_buffer,
                        {cv::IMWRITE_JPEG_QUALITY, 85});
            
            // Envoi
            if (frame_callback_) {
                frame_callback_(jpeg_buffer);
            }
            
            // Régulation FPS
            auto elapsed = std::chrono::steady_clock::now() - start;
            auto target = std::chrono::milliseconds(33); // ~30 FPS
            if (elapsed < target) {
                std::this_thread::sleep_for(target - elapsed);
            }
        }
    }
};
```

---

## 6. Implémentation Frontend

### 6.1 Hook WebSocket personnalisé

```typescript
interface UseWebSocketReturn {
  connected: boolean;
  send: (message: ControlMessage) => void;
  lastFrame: VideoFrame | null;
  engineStatus: EngineStatus | null;
}

function useWebSocket(url: string): UseWebSocketReturn {
  const [connected, setConnected] = useState(false);
  const [lastFrame, setLastFrame] = useState<VideoFrame | null>(null);
  const [engineStatus, setEngineStatus] = useState<EngineStatus | null>(null);
  const wsRef = useRef<WebSocket | null>(null);

  useEffect(() => {
    const ws = new WebSocket(url);
    wsRef.current = ws;

    ws.onopen = () => setConnected(true);
    ws.onclose = () => setConnected(false);

    ws.onmessage = (event) => {
      if (typeof event.data === 'string') {
        // Message de contrôle JSON
        const message = JSON.parse(event.data);
        
        if (message.type === 'engine_status') {
          setEngineStatus(message.payload);
        } else if (message.type === 'frame') {
          // Phase MVP avec Base64
          setLastFrame({
            frameId: message.frame_id,
            timestamp: message.timestamp,
            jpegData: base64ToArrayBuffer(message.data)
          });
        }
      } else {
        // Message binaire (phase production)
        const frame = parseVideoFrame(event.data);
        setLastFrame(frame);
      }
    };

    return () => ws.close();
  }, [url]);

  const send = useCallback((message: ControlMessage) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(message));
    }
  }, []);

  return { connected, send, lastFrame, engineStatus };
}
```

### 6.2 Composant Canvas de rendu

```typescript
function VideoCanvas({ frame }: { frame: VideoFrame | null }) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  useEffect(() => {
    if (!frame || !canvasRef.current) return;

    const canvas = canvasRef.current;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    // Créer un Blob depuis les données JPEG
    const blob = new Blob([frame.jpegData], { type: 'image/jpeg' });
    const url = URL.createObjectURL(blob);

    const img = new Image();
    img.onload = () => {
      canvas.width = img.width;
      canvas.height = img.height;
      ctx.drawImage(img, 0, 0);
      URL.revokeObjectURL(url);
    };
    img.src = url;
  }, [frame]);

  return <canvas ref={canvasRef} className="w-full h-full" />;
}
```

---

## 7. Roadmap d'implémentation détaillée

### Phase 1 : Backend minimal (1 semaine)
```
[x] VideoSource abstraction
[x] WebcamSource implémentation
[x] IFilter interface
[x] GrayscaleFilter
[x] AsciiFilter basique
[x] FramePipeline séquentiel
[ ] Tests unitaires
```

### Phase 2 : Moteur temps réel (1 semaine)
```
[ ] FrameController avec thread unique
[ ] Régulation FPS
[ ] WebSocketServer basique
[ ] Envoi frames JPEG + Base64
[ ] Messages de contrôle start/stop
```

### Phase 3 : Frontend MVP (3 jours)
```
[ ] Next.js setup
[ ] Hook WebSocket
[ ] Canvas renderer
[ ] Boutons start/stop
[ ] Affichage statut
```

### Phase 4 : Protocole complet (1 semaine)
```
[ ] Messages JSON standardisés
[ ] Gestion erreurs
[ ] Commandes filtres
[ ] Paramètres dynamiques
[ ] Validation côté backend
```

### Phase 5 : Optimisations (2 semaines)
```
[ ] Suppression Base64
[ ] Protocole binaire
[ ] Threading avancé (4 threads)
[ ] Queues thread-safe
[ ] Métriques performance
```

### Phase 6 : Extensions (ouvert)
```
[ ] Filtres supplémentaires
[ ] ImageSequenceSource
[ ] GPU acceleration (CUDA/OpenCL)
[ ] FreeType rendering
[ ] WebRTC streaming
```

---

## 8. Points critiques d'implémentation

### 8.1 Gestion de la mémoire

**❌ Ne jamais faire** :
```cpp
cv::Mat* frame = new cv::Mat();  // Fuite mémoire
source_->readFrame(*frame);
```

**✅ Toujours faire** :
```cpp
cv::Mat frame;  // RAII
source_->readFrame(frame);
// Destruction automatique
```

### 8.2 Synchronisation

**❌ Mutex partout** :
```cpp
std::mutex frame_mutex;
// lock dans capture
// lock dans processing
// lock dans encoding
// → Contention massive
```

**✅ Queues lock-free** :
```cpp
ThreadSafeQueue<cv::Mat> capture_queue(2);
ThreadSafeQueue<cv::Mat> process_queue(2);
// Chaque thread travaille indépendamment
```

### 8.3 Validation des messages

**✅ Toujours valider** :
```cpp
void handleMessage(const nlohmann::json& msg) {
    if (!msg.contains("version")) return;
    if (msg["version"] != "1.0") return;
    if (!msg.contains("type")) return;
    
    // Traitement sûr
}
```

---

## 9. Extensions futures

### WebRTC pour latence ultra-faible
```
Frontend ←──────→ STUN/TURN ←──────→ Backend
         UDP P2P (< 50ms)
```

### Architecture multi-clients
```
Backend (moteur unique)
   ↓
   ├──→ Client Web 1
   ├──→ Client Web 2
   ├──→ Client Mobile
   └──→ Client CLI
```

### Plugin system
```cpp
class IFilterPlugin {
    virtual std::string getName() = 0;
    virtual IFilter* createInstance() = 0;
};

// Chargement dynamique .so/.dll
```

---

**Ce document est votre référence technique absolue. Chaque section contient les détails nécessaires pour implémenter sans ambiguïté.**

# AsciiVision - Arborescence Complète et Spécifications des Classes

---

## 1. Arborescence du projet

```
AsciiVision/
├── backend/                           # Backend C++
│   ├── CMakeLists.txt                # Configuration build
│   ├── src/
│   │   ├── main.cpp                  # Point d'entrée
│   │   │
│   │   ├── core/                     # Composants fondamentaux
│   │   │   ├── VideoSource.hpp       # Interface source vidéo
│   │   │   ├── VideoSource.cpp
│   │   │   ├── WebcamSource.hpp      # Implémentation webcam
│   │   │   ├── WebcamSource.cpp
│   │   │   ├── ImageSource.hpp       # Implémentation image fixe
│   │   │   ├── ImageSource.cpp
│   │   │   ├── ImageSequenceSource.hpp  # Implémentation séquence
│   │   │   └── ImageSequenceSource.cpp
│   │   │
│   │   ├── filters/                  # Système de filtres
│   │   │   ├── IFilter.hpp           # Interface filtre
│   │   │   ├── FilterRegistry.hpp    # Enregistrement filtres
│   │   │   ├── FilterRegistry.cpp
│   │   │   ├── GrayscaleFilter.hpp   # Conversion niveaux de gris
│   │   │   ├── GrayscaleFilter.cpp
│   │   │   ├── ResizeFilter.hpp      # Redimensionnement
│   │   │   ├── ResizeFilter.cpp
│   │   │   ├── BlurFilter.hpp        # Flou gaussien
│   │   │   ├── BlurFilter.cpp
│   │   │   ├── EdgeDetectionFilter.hpp  # Détection contours
│   │   │   ├── EdgeDetectionFilter.cpp
│   │   │   ├── AsciiFilter.hpp       # Rendu ASCII
│   │   │   └── AsciiFilter.cpp
│   │   │
│   │   ├── pipeline/                 # Pipeline de traitement
│   │   │   ├── FramePipeline.hpp     # Orchestrateur filtres
│   │   │   └── FramePipeline.cpp
│   │   │
│   │   ├── processing/               # Moteur de traitement
│   │   │   ├── FrameController.hpp   # Contrôleur principal
│   │   │   ├── FrameController.cpp
│   │   │   ├── FrameEncoder.hpp      # Encodage JPEG
│   │   │   └── FrameEncoder.cpp
│   │   │
│   │   ├── network/                  # Communication réseau
│   │   │   ├── WebSocketServer.hpp   # Serveur WebSocket
│   │   │   ├── WebSocketServer.cpp
│   │   │   ├── MessageHandler.hpp    # Traitement messages
│   │   │   ├── MessageHandler.cpp
│   │   │   ├── ProtocolTypes.hpp     # Définitions protocole
│   │   │   └── BinaryFrameEncoder.hpp  # Encodage binaire
│   │   │
│   │   └── utils/                    # Utilitaires
│   │       ├── ThreadSafeQueue.hpp   # Queue thread-safe
│   │       ├── Logger.hpp            # Système de logs
│   │       ├── Logger.cpp
│   │       ├── PerformanceMonitor.hpp  # Métriques perf
│   │       └── PerformanceMonitor.cpp
│   │
│   └── tests/                        # Tests unitaires
│       ├── test_filters.cpp
│       ├── test_pipeline.cpp
│       └── test_sources.cpp
│
├── frontend/                         # Frontend Next.js
│   ├── package.json
│   ├── tsconfig.json
│   ├── next.config.js
│   ├── tailwind.config.js
│   │
│   ├── src/
│   │   ├── app/
│   │   │   ├── layout.tsx            # Layout principal
│   │   │   ├── page.tsx              # Page principale
│   │   │   └── globals.css           # Styles globaux
│   │   │
│   │   ├── components/               # Composants React
│   │   │   ├── VideoCanvas.tsx       # Affichage vidéo
│   │   │   ├── ControlPanel.tsx      # Panneau de contrôle
│   │   │   ├── FilterControls.tsx    # Contrôles filtres
│   │   │   ├── StatusBar.tsx         # Barre de statut
│   │   │   └── ErrorDisplay.tsx      # Affichage erreurs
│   │   │
│   │   ├── hooks/                    # Hooks personnalisés
│   │   │   ├── useWebSocket.ts       # Hook WebSocket
│   │   │   ├── useVideoStream.ts     # Hook flux vidéo
│   │   │   └── useEngineControl.ts   # Hook contrôle moteur
│   │   │
│   │   ├── types/                    # Définitions TypeScript
│   │   │   ├── protocol.ts           # Types protocole
│   │   │   ├── filters.ts            # Types filtres
│   │   │   └── engine.ts             # Types moteur
│   │   │
│   │   └── lib/                      # Bibliothèques
│   │       ├── websocket.ts          # Client WebSocket
│   │       ├── frameDecoder.ts       # Décodage frames
│   │       └── messageBuilder.ts     # Construction messages
│   │
│   └── public/                       # Assets statiques
│       └── icons/
│
├── docs/                             # Documentation
│   ├── architecture.md               # Ce document
│   ├── protocol.md                   # Spécification protocole
│   └── api.md                        # Documentation API
│
└── README.md                         # README principal
```

---

## 2. Backend - Spécifications des Classes

### 2.1 Core - Sources Vidéo

#### **VideoSource.hpp** (Interface abstraite)

```cpp
namespace asciivision::core {

class VideoSource {
public:
    virtual ~VideoSource() = default;
    
    // Ouvre la source vidéo
    // Retourne: true si succès, false sinon
    virtual bool open() = 0;
    
    // Lit une frame depuis la source
    // Paramètres:
    //   - frame: Mat où stocker l'image (modifié)
    // Retourne: true si frame lue, false si fin ou erreur
    virtual bool readFrame(cv::Mat& frame) = 0;
    
    // Ferme la source vidéo proprement
    virtual void close() = 0;
    
    // Retourne la largeur native de la source
    virtual int getWidth() const = 0;
    
    // Retourne la hauteur native de la source
    virtual int getHeight() const = 0;
    
    // Retourne le FPS de la source
    virtual double getFPS() const = 0;
    
    // Retourne true si la source est ouverte
    virtual bool isOpened() const = 0;
    
    // Retourne un nom descriptif de la source
    virtual std::string getName() const = 0;
};

} // namespace asciivision::core
```

**Utilité**: Abstraction permettant d'interchanger les sources vidéo (webcam, image, séquence, réseau) sans modifier le code client.

---

#### **WebcamSource.hpp** (Implémentation concrète)

```cpp
namespace asciivision::core {

class WebcamSource : public VideoSource {
public:
    // Constructeur
    // Paramètres:
    //   - device_id: ID du périphérique (0 = première webcam)
    explicit WebcamSource(int device_id);
    
    // Constructeur avec configuration
    // Paramètres:
    //   - device_id: ID du périphérique
    //   - width: largeur désirée
    //   - height: hauteur désirée
    //   - fps: FPS désiré
    WebcamSource(int device_id, int width, int height, double fps);
    
    ~WebcamSource() override;
    
    // Implémentations VideoSource
    bool open() override;
    bool readFrame(cv::Mat& frame) override;
    void close() override;
    int getWidth() const override;
    int getHeight() const override;
    double getFPS() const override;
    bool isOpened() const override;
    std::string getName() const override;
    
private:
    cv::VideoCapture capture_;
    int device_id_;
    int configured_width_;
    int configured_height_;
    double configured_fps_;
};

} // namespace asciivision::core
```

**Utilité**: Capture vidéo depuis une webcam via OpenCV. Gère l'initialisation et la configuration du périphérique.

---

#### **ImageSource.hpp** (Implémentation image fixe)

```cpp
namespace asciivision::core {

class ImageSource : public VideoSource {
public:
    // Constructeur
    // Paramètres:
    //   - image_path: chemin vers l'image
    explicit ImageSource(const std::string& image_path);
    
    ~ImageSource() override;
    
    bool open() override;
    bool readFrame(cv::Mat& frame) override;
    void close() override;
    int getWidth() const override;
    int getHeight() const override;
    double getFPS() const override;  // Retourne 0
    bool isOpened() const override;
    std::string getName() const override;
    
private:
    std::string image_path_;
    cv::Mat image_;
    bool is_opened_;
};

} // namespace asciivision::core
```

**Utilité**: Permet de traiter une image fixe comme une source vidéo. Retourne toujours la même image à chaque `readFrame()`.

---

#### **ImageSequenceSource.hpp** (Implémentation séquence)

```cpp
namespace asciivision::core {

class ImageSequenceSource : public VideoSource {
public:
    // Constructeur
    // Paramètres:
    //   - directory_path: dossier contenant les images
    //   - fps: FPS simulé pour la séquence
    ImageSequenceSource(const std::string& directory_path, double fps);
    
    ~ImageSequenceSource() override;
    
    bool open() override;
    bool readFrame(cv::Mat& frame) override;
    void close() override;
    int getWidth() const override;
    int getHeight() const override;
    double getFPS() const override;
    bool isOpened() const override;
    std::string getName() const override;
    
    // Reset la séquence au début
    void reset();
    
    // Retourne le nombre total d'images
    size_t getFrameCount() const;
    
    // Retourne l'index de l'image courante
    size_t getCurrentFrameIndex() const;
    
private:
    std::string directory_path_;
    double fps_;
    std::vector<std::string> image_paths_;
    size_t current_index_;
    bool loop_;  // Retour au début après dernière image
    int width_;
    int height_;
    bool is_opened_;
    
    // Charge la liste des images du dossier
    void loadImageList();
};

} // namespace asciivision::core
```

**Utilité**: Lit une séquence d'images depuis un dossier. Utile pour tester le pipeline avec des données reproductibles.

---

### 2.2 Filters - Système de Filtres

#### **IFilter.hpp** (Interface)

```cpp
namespace asciivision::filters {

class IFilter {
public:
    virtual ~IFilter() = default;
    
    // Applique le filtre
    // Paramètres:
    //   - input: image source (non modifiée)
    //   - output: image résultat (modifiée)
    // Note: input et output peuvent être la même Mat
    virtual void apply(const cv::Mat& input, cv::Mat& output) = 0;
    
    // Définit un paramètre du filtre
    // Paramètres:
    //   - name: nom du paramètre
    //   - value: valeur JSON (int, double, string, bool, array)
    // Lance: std::invalid_argument si paramètre inconnu
    virtual void setParameter(const std::string& name, 
                             const nlohmann::json& value) = 0;
    
    // Retourne tous les paramètres sous forme JSON
    // Format: {"param_name": value, ...}
    virtual nlohmann::json getParameters() const = 0;
    
    // Retourne les métadonnées du filtre
    // Format: {"name": "...", "description": "...", 
    //          "parameters": [{...}, ...]}
    virtual nlohmann::json getMetadata() const = 0;
    
    // Active/désactive le filtre
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }
    
    // Retourne le nom unique du filtre
    virtual std::string getName() const = 0;
    
protected:
    bool enabled_ = true;
};

} // namespace asciivision::filters
```

**Utilité**: Interface commune pour tous les filtres. Permet l'extensibilité et la composition.

---

#### **GrayscaleFilter.hpp**

```cpp
namespace asciivision::filters {

class GrayscaleFilter : public IFilter {
public:
    GrayscaleFilter();
    ~GrayscaleFilter() override = default;
    
    void apply(const cv::Mat& input, cv::Mat& output) override;
    void setParameter(const std::string& name, 
                     const nlohmann::json& value) override;
    nlohmann::json getParameters() const override;
    nlohmann::json getMetadata() const override;
    std::string getName() const override;
    
private:
    // Pas de paramètres pour ce filtre
};

} // namespace asciivision::filters
```

**Utilité**: Convertit une image couleur en niveaux de gris. Première étape obligatoire avant le rendu ASCII.

---

#### **ResizeFilter.hpp**

```cpp
namespace asciivision::filters {

class ResizeFilter : public IFilter {
public:
    // Constructeur avec dimensions par défaut
    ResizeFilter(int width = 160, int height = 120);
    ~ResizeFilter() override = default;
    
    void apply(const cv::Mat& input, cv::Mat& output) override;
    void setParameter(const std::string& name, 
                     const nlohmann::json& value) override;
    nlohmann::json getParameters() const override;
    nlohmann::json getMetadata() const override;
    std::string getName() const override;
    
private:
    int target_width_;
    int target_height_;
    int interpolation_;  // cv::INTER_LINEAR, INTER_NEAREST, etc.
    
    // Paramètres acceptés:
    // - "width": int
    // - "height": int
    // - "interpolation": string ("linear", "nearest", "cubic")
};

} // namespace asciivision::filters
```

**Utilité**: Redimensionne l'image. Crucial pour contrôler la résolution du rendu ASCII et les performances.

---

#### **BlurFilter.hpp**

```cpp
namespace asciivision::filters {

class BlurFilter : public IFilter {
public:
    BlurFilter(int kernel_size = 3);
    ~BlurFilter() override = default;
    
    void apply(const cv::Mat& input, cv::Mat& output) override;
    void setParameter(const std::string& name, 
                     const nlohmann::json& value) override;
    nlohmann::json getParameters() const override;
    nlohmann::json getMetadata() const override;
    std::string getName() const override;
    
private:
    int kernel_size_;  // Doit être impair (3, 5, 7, ...)
    double sigma_;     // Écart-type gaussien
    
    // Paramètres acceptés:
    // - "kernel_size": int (impair)
    // - "sigma": double
};

} // namespace asciivision::filters
```

**Utilité**: Applique un flou gaussien. Utile pour réduire le bruit avant la conversion ASCII.

---

#### **EdgeDetectionFilter.hpp**

```cpp
namespace asciivision::filters {

class EdgeDetectionFilter : public IFilter {
public:
    EdgeDetectionFilter();
    ~EdgeDetectionFilter() override = default;
    
    void apply(const cv::Mat& input, cv::Mat& output) override;
    void setParameter(const std::string& name, 
                     const nlohmann::json& value) override;
    nlohmann::json getParameters() const override;
    nlohmann::json getMetadata() const override;
    std::string getName() const override;
    
private:
    double threshold1_;  // Seuil bas Canny
    double threshold2_;  // Seuil haut Canny
    int aperture_size_;  // Taille opérateur Sobel (3, 5, 7)
    
    // Paramètres acceptés:
    // - "threshold1": double (50-150)
    // - "threshold2": double (100-300)
    // - "aperture_size": int (3, 5, 7)
};

} // namespace asciivision::filters
```

**Utilité**: Détecte les contours via l'algorithme de Canny. Peut créer des effets artistiques en ASCII.

---

#### **AsciiFilter.hpp** (⭐ Filtre principal)

```cpp
namespace asciivision::filters {

class AsciiFilter : public IFilter {
public:
    // Jeux de caractères prédéfinis
    enum class CharsetType {
        MINIMAL,    // " .:-=+*#%@"
        STANDARD,   // " .'`^\",:;Il!i><~+_-?][}{1)(|/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$"
        EXTENDED,   // Version étendue avec plus de caractères
        BLOCKS      // Blocs Unicode: " ░▒▓█"
    };
    
    AsciiFilter();
    ~AsciiFilter() override = default;
    
    void apply(const cv::Mat& input, cv::Mat& output) override;
    void setParameter(const std::string& name, 
                     const nlohmann::json& value) override;
    nlohmann::json getParameters() const override;
    nlohmann::json getMetadata() const override;
    std::string getName() const override;
    
    // Méthodes spécifiques
    void setCharset(CharsetType type);
    void setCustomCharset(const std::string& charset);
    
private:
    std::string charset_;
    bool invert_;           // Inverse le mapping luminosité→caractère
    double contrast_;       // Ajustement contraste (0.5-2.0)
    double brightness_;     // Ajustement luminosité (-100 à +100)
    
    // Convertit un pixel en caractère ASCII
    char pixelToChar(uint8_t pixel_value) const;
    
    // Paramètres acceptés:
    // - "charset": string ("minimal", "standard", "extended", "blocks")
    // - "custom_charset": string (charset personnalisé)
    // - "invert": bool
    // - "contrast": double
    // - "brightness": double
};

} // namespace asciivision::filters
```

**Utilité**: **Cœur du projet**. Convertit une image en niveaux de gris en art ASCII. Doit toujours être le dernier filtre du pipeline.

---

#### **FilterRegistry.hpp**

```cpp
namespace asciivision::filters {

class FilterRegistry {
public:
    using FilterFactory = std::function<std::unique_ptr<IFilter>()>;
    
    // Singleton
    static FilterRegistry& instance();
    
    // Enregistre un filtre
    // Paramètres:
    //   - name: nom unique du filtre
    //   - factory: fonction créant une instance
    void registerFilter(const std::string& name, FilterFactory factory);
    
    // Crée une instance d'un filtre
    // Lance: std::runtime_error si filtre inconnu
    std::unique_ptr<IFilter> createFilter(const std::string& name);
    
    // Retourne la liste des filtres disponibles
    std::vector<std::string> getAvailableFilters() const;
    
    // Retourne les métadonnées de tous les filtres
    nlohmann::json getAllMetadata() const;
    
private:
    FilterRegistry() = default;
    std::unordered_map<std::string, FilterFactory> factories_;
    std::mutex registry_mutex_;
};

// Macro d'enregistrement automatique
#define REGISTER_FILTER(FilterClass, name) \
    namespace { \
        struct FilterClass##Registrar { \
            FilterClass##Registrar() { \
                FilterRegistry::instance().registerFilter(name, \
                    []() { return std::make_unique<FilterClass>(); }); \
            } \
        }; \
        static FilterClass##Registrar FilterClass##_registrar; \
    }

} // namespace asciivision::filters
```

**Utilité**: Registry pattern pour gérer tous les filtres disponibles. Permet l'ajout de filtres sans modifier le code existant.

---

### 2.3 Pipeline - Orchestration

#### **FramePipeline.hpp**

```cpp
namespace asciivision::pipeline {

class FramePipeline {
public:
    FramePipeline();
    ~FramePipeline();
    
    // Ajoute un filtre à la fin du pipeline
    // Paramètres:
    //   - filter: filtre à ajouter (ownership transféré)
    void addFilter(std::unique_ptr<filters::IFilter> filter);
    
    // Insère un filtre à une position
    // Paramètres:
    //   - index: position (0 = début)
    //   - filter: filtre à insérer
    void insertFilter(size_t index, std::unique_ptr<filters::IFilter> filter);
    
    // Retire un filtre par nom
    // Retourne: true si trouvé et retiré
    bool removeFilter(const std::string& filter_name);
    
    // Vide le pipeline
    void clear();
    
    // Applique tous les filtres actifs
    // Paramètres:
    //   - input: image source
    //   - output: image résultat
    // Note: Thread-safe
    void process(const cv::Mat& input, cv::Mat& output);
    
    // Met à jour un paramètre d'un filtre
    // Paramètres:
    //   - filter_name: nom du filtre
    //   - param_name: nom du paramètre
    //   - value: nouvelle valeur
    // Lance: std::runtime_error si filtre inexistant
    // Note: Thread-safe (mise à jour atomique)
    void updateFilterParameter(const std::string& filter_name,
                               const std::string& param_name,
                               const nlohmann::json& value);
    
    // Active/désactive un filtre
    void setFilterEnabled(const std::string& filter_name, bool enabled);
    
    // Retourne la configuration complète du pipeline
    nlohmann::json getConfiguration() const;
    
    // Applique une configuration complète
    // Format: {"filters": [{"name": "...", "enabled": true, 
    //                       "params": {...}}, ...]}
    void setConfiguration(const nlohmann::json& config);
    
    // Retourne le nombre de filtres
    size_t getFilterCount() const;
    
    // Retourne les noms des filtres dans l'ordre
    std::vector<std::string> getFilterNames() const;
    
private:
    std::vector<std::unique_ptr<filters::IFilter>> filters_;
    mutable std::shared_mutex filters_mutex_;  // Lecture multiple, écriture exclusive
    
    // Trouve un filtre par nom
    filters::IFilter* findFilter(const std::string& name);
};

} // namespace asciivision::pipeline
```

**Utilité**: Orchestre l'exécution séquentielle des filtres. Thread-safe pour permettre modifications pendant le traitement.

---

### 2.4 Processing - Moteur

#### **FrameController.hpp** (⭐ Contrôleur principal)

```cpp
namespace asciivision::processing {

// État du moteur
enum class EngineState {
    STOPPED,
    STARTING,
    RUNNING,
    STOPPING,
    ERROR
};

// Statistiques du moteur
struct EngineStats {
    double current_fps;
    uint64_t frame_count;
    uint64_t dropped_frames;
    double avg_latency_ms;
    double max_latency_ms;
    std::chrono::steady_clock::time_point start_time;
};

class FrameController {
public:
    using FrameCallback = std::function<void(const std::vector<uint8_t>&, uint32_t frame_id)>;
    using ErrorCallback = std::function<void(const std::string&)>;
    using StateCallback = std::function<void(EngineState)>;
    
    FrameController();
    ~FrameController();
    
    // Démarre le moteur
    // Paramètres:
    //   - source: source vidéo (ownership transféré)
    //   - target_fps: FPS cible (0 = max possible)
    // Lance: std::runtime_error si déjà démarré ou source invalide
    void start(std::unique_ptr<core::VideoSource> source, double target_fps = 30.0);
    
    // Arrête le moteur proprement
    // Bloquant: attend la fin du thread
    void stop();
    
    // Retourne l'état actuel
    EngineState getState() const;
    
    // Retourne les statistiques
    EngineStats getStats() const;
    
    // Callbacks
    void setFrameCallback(FrameCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setStateCallback(StateCallback callback);
    
    // Accès au pipeline (pour configuration)
    pipeline::FramePipeline& getPipeline();
    
    // Change le FPS cible en temps réel
    void setTargetFPS(double fps);
    
    // Active/désactive l'encodage JPEG
    void setEncodingEnabled(bool enabled);
    void setJPEGQuality(int quality);  // 0-100
    
private:
    std::unique_ptr<core::VideoSource> source_;
    std::unique_ptr<pipeline::FramePipeline> pipeline_;
    std::unique_ptr<FrameEncoder> encoder_;
    
    std::atomic<EngineState> state_;
    std::atomic<bool> should_stop_;
    std::atomic<double> target_fps_;
    
    std::thread worker_thread_;
    
    FrameCallback frame_callback_;
    ErrorCallback error_callback_;
    StateCallback state_callback_;
    
    // Statistiques
    std::atomic<uint64_t> frame_count_;
    std::atomic<uint64_t> dropped_frames_;
    mutable std::mutex stats_mutex_;
    EngineStats stats_;
    
    // Boucle principale (exécutée dans worker_thread_)
    void workerLoop();
    
    // Change l'état et notifie
    void setState(EngineState new_state);
    
    // Met à jour les statistiques
    void updateStats(double frame_time_ms);
};

} // namespace asciivision::processing
```

**Utilité**: **Chef d'orchestre du backend**. Gère le cycle de vie complet : source → pipeline → encodage → callback. Thread-safe et robuste.

---

#### **FrameEncoder.hpp**

```cpp
namespace asciivision::processing {

class FrameEncoder {
public:
    FrameEncoder(int quality = 85);
    ~FrameEncoder() = default;
    
    // Encode une Mat en JPEG
    // Paramètres:
    //   - frame: image à encoder
    //   - output: buffer de sortie
    // Retourne: true si succès
    bool encodeJPEG(const cv::Mat& frame, std::vector<uint8_t>& output);
    
    // Encode avec header binaire personnalisé
    // Format: header (24 bytes) + JPEG data
    bool encodeBinaryFrame(const cv::Mat& frame,
                          uint32_t frame_id,
                          std::vector<uint8_t>& output);
    
    // Change la qualité JPEG
    void setQuality(int quality);
    int getQuality() const;
    
private:
    int quality_;
    std::vector<int> encode_params_;
    
    void updateEncodeParams();
};

} // namespace asciivision::processing
```

**Utilité**: Encapsule l'encodage JPEG. Simplifie le code du FrameController et permet de changer facilement d'encodeur (PNG, WebP, etc.).

---

### 2.5 Network - Communication

#### **WebSocketServer.hpp**

```cpp
namespace asciivision::network {

using websocketpp::server;
using websocketpp::connection_hdl;

class WebSocketServer {
public:
    using MessageCallback = std::function<void(connection_hdl, const std::string&)>;
    using ConnectionCallback = std::function<void(connection_hdl)>;
    
    explicit WebSocketServer(uint16_t port = 9002);
    ~WebSocketServer();
    
    // Démarre le serveur
    // Bloquant: lance le thread d'écoute
    void start();
    
    // Arrête le serveur
    void stop();
    
    // Envoie un message texte (JSON)
    // Paramètres:
    //   - hdl: connexion cible
    //   - message: JSON stringifié
    void sendText(connection_hdl hdl, const std::string& message);
    
    // Envoie des données binaires
    // Paramètres:
    //   - hdl: connexion cible
    //   - data: buffer binaire
    void sendBinary(connection_hdl hdl, const std::vector<uint8_t>& data);
    
    // Broadcast texte à tous les clients
    void broadcastText(const std::string& message);
    
    // Broadcast binaire à tous les clients
    void broadcastBinary(const std::vector<uint8_t>& data);
    
    // Callbacks
    void setMessageCallback(MessageCallback callback);
    void setConnectionCallback(ConnectionCallback on_open);
    void setDisconnectionCallback(ConnectionCallback on_close);
    
    // Retourne le nombre de clients connectés
    size_t getClientCount() const;
    
private:
    uint16_t port_;
    server<websocketpp::config::asio> server_;
    std::thread server_thread_;
    
    MessageCallback message_callback_;
    ConnectionCallback connection_callback_;
    ConnectionCallback disconnection_callback_;




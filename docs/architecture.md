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

    std::set<connection_hdl, std::owner_less<connection_hdl>> connections_;
    mutable std::mutex connections_mutex_;
    
    // Handlers WebSocketpp
    void onOpen(connection_hdl hdl);
    void onClose(connection_hdl hdl);
    void onMessage(connection_hdl hdl, server<websocketpp::config::asio>::message_ptr msg);
};

} // namespace asciivision::network
```

**Utilité**: Serveur WebSocket bas niveau. Gère les connexions clients et le transport des messages. Ne connaît pas le protocole applicatif.

---

#### **MessageHandler.hpp**

```cpp
namespace asciivision::network {

class MessageHandler {
public:
    using CommandCallback = std::function<void(const nlohmann::json&)>;
    
    MessageHandler();
    ~MessageHandler() = default;
    
    // Traite un message JSON reçu
    // Paramètres:
    //   - message: JSON stringifié
    // Retourne: réponse JSON (ou vide si pas de réponse)
    std::string handleMessage(const std::string& message);
    
    // Enregistre un handler pour un type de commande
    // Paramètres:
    //   - command_type: type de commande ("start_engine", "stop_engine", etc.)
    //   - callback: fonction appelée avec le payload
    void registerCommandHandler(const std::string& command_type, 
                                CommandCallback callback);
    
    // Crée un message de statut moteur
    static std::string createEngineStatusMessage(
        const processing::EngineStats& stats,
        processing::EngineState state);
    
    // Crée un message d'erreur
    static std::string createErrorMessage(
        const std::string& error_code,
        const std::string& message,
        bool recoverable = false);
    
    // Crée un message de capacités
    static std::string createCapabilitiesMessage(
        const std::vector<std::string>& sources,
        const nlohmann::json& filters_metadata);
    
    // Crée un message d'acknowledgement
    static std::string createAckMessage(const std::string& command_type);
    
private:
    std::unordered_map<std::string, CommandCallback> command_handlers_;
    mutable std::mutex handlers_mutex_;
    
    // Valide la structure d'un message
    bool validateMessage(const nlohmann::json& msg) const;
    
    // Extrait la version du protocole
    std::string getProtocolVersion(const nlohmann::json& msg) const;
};

} // namespace asciivision::network
```

**Utilité**: Couche protocole. Parse et valide les messages JSON, dispatch aux handlers appropriés, génère les réponses formatées.

---

#### **ProtocolTypes.hpp**

```cpp
namespace asciivision::network {

// Types de messages
namespace MessageType {
    constexpr const char* START_ENGINE = "start_engine";
    constexpr const char* STOP_ENGINE = "stop_engine";
    constexpr const char* UPDATE_FILTERS = "update_filters";
    constexpr const char* SET_PARAMETER = "set_parameter";
    constexpr const char* GET_CAPABILITIES = "get_capabilities";
    constexpr const char* GET_STATUS = "get_status";
    
    constexpr const char* ENGINE_STATUS = "engine_status";
    constexpr const char* ERROR = "error";
    constexpr const char* CAPABILITIES = "capabilities";
    constexpr const char* ACK = "acknowledgement";
    constexpr const char* FRAME = "frame";
}

// Codes d'erreur
namespace ErrorCode {
    constexpr const char* INVALID_MESSAGE = "INVALID_MESSAGE";
    constexpr const char* UNKNOWN_COMMAND = "UNKNOWN_COMMAND";
    constexpr const char* SOURCE_UNAVAILABLE = "SOURCE_UNAVAILABLE";
    constexpr const char* ALREADY_RUNNING = "ALREADY_RUNNING";
    constexpr const char* NOT_RUNNING = "NOT_RUNNING";
    constexpr const char* FILTER_NOT_FOUND = "FILTER_NOT_FOUND";
    constexpr const char* INVALID_PARAMETER = "INVALID_PARAMETER";
    constexpr const char* ENCODING_ERROR = "ENCODING_ERROR";
    constexpr const char* INTERNAL_ERROR = "INTERNAL_ERROR";
}

// Version du protocole
constexpr const char* PROTOCOL_VERSION = "1.0";

// Structure du header binaire
struct BinaryFrameHeader {
    char magic[4];        // "AVIS"
    uint16_t version;     // 0x0100
    uint32_t frame_id;
    uint64_t timestamp;
    uint16_t width;
    uint16_t height;
    uint16_t reserved;    // Pour alignement/future extension
} __attribute__((packed));

static_assert(sizeof(BinaryFrameHeader) == 24, "Header must be 24 bytes");

} // namespace asciivision::network
```

**Utilité**: Définitions centralisées du protocole. Évite les magic strings et garantit la cohérence.

---

#### **BinaryFrameEncoder.hpp**

```cpp
namespace asciivision::network {

class BinaryFrameEncoder {
public:
    BinaryFrameEncoder() = default;
    ~BinaryFrameEncoder() = default;
    
    // Encode une frame avec header binaire
    // Paramètres:
    //   - jpeg_data: données JPEG déjà encodées
    //   - frame_id: ID de la frame
    //   - width: largeur de l'image
    //   - height: hauteur de l'image
    //   - output: buffer de sortie
    static void encode(const std::vector<uint8_t>& jpeg_data,
                      uint32_t frame_id,
                      uint16_t width,
                      uint16_t height,
                      std::vector<uint8_t>& output);
    
    // Décode un header binaire
    // Paramètres:
    //   - data: buffer contenant au moins un header
    //   - header: structure de sortie
    // Retourne: true si header valide
    static bool decodeHeader(const std::vector<uint8_t>& data,
                            BinaryFrameHeader& header);
    
    // Valide un magic number
    static bool isValidMagic(const char magic[4]);
    
private:
    static uint64_t getCurrentTimestamp();
};

} // namespace asciivision::network
```

**Utilité**: Gère l'encodage/décodage du format binaire personnalisé. Sépare cette logique du reste du code réseau.

---

### 2.6 Utils - Utilitaires

#### **ThreadSafeQueue.hpp**

```cpp
namespace asciivision::utils {

template<typename T>
class ThreadSafeQueue {
public:
    // Constructeur avec capacité maximale
    explicit ThreadSafeQueue(size_t max_capacity = 0);
    
    ~ThreadSafeQueue() = default;
    
    // Ajoute un élément (bloque si plein)
    // Retourne: false si queue fermée
    bool push(T item);
    
    // Ajoute un élément (non-bloquant)
    // Retourne: false si plein ou fermée
    bool tryPush(T item);
    
    // Retire un élément (bloque si vide)
    // Retourne: false si queue fermée et vide
    bool pop(T& item);
    
    // Retire un élément (non-bloquant)
    // Retourne: false si vide
    bool tryPop(T& item);
    
    // Retire un élément avec timeout
    // Paramètres:
    //   - item: référence de sortie
    //   - timeout: durée maximale d'attente
    // Retourne: false si timeout ou queue fermée
    template<typename Rep, typename Period>
    bool popFor(T& item, const std::chrono::duration<Rep, Period>& timeout);
    
    // Retourne le nombre d'éléments
    size_t size() const;
    
    // Retourne true si vide
    bool empty() const;
    
    // Retourne la capacité maximale (0 = illimitée)
    size_t capacity() const;
    
    // Ferme la queue (débloque tous les threads en attente)
    void close();
    
    // Retourne true si fermée
    bool isClosed() const;
    
    // Vide la queue
    void clear();
    
private:
    std::queue<T> queue_;
    size_t max_capacity_;
    mutable std::mutex mutex_;
    std::condition_variable cv_not_empty_;
    std::condition_variable cv_not_full_;
    std::atomic<bool> closed_{false};
};

} // namespace asciivision::utils
```

**Utilité**: Queue thread-safe avec backpressure. Essentielle pour le modèle multi-thread sans mutex dans le code métier.

---

#### **Logger.hpp**

```cpp
namespace asciivision::utils {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    // Singleton
    static Logger& instance();
    
    // Configure le logger
    void setLogLevel(LogLevel level);
    void setOutputFile(const std::string& filepath);
    void enableConsoleOutput(bool enable);
    void enableTimestamp(bool enable);
    
    // Log un message
    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const std::string& category, const std::string& message);
    
    // Méthodes de convenance
    void debug(const std::string& message);
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    void critical(const std::string& message);
    
    // Log avec formatage (variadic template)
    template<typename... Args>
    void logf(LogLevel level, const std::string& format, Args&&... args);
    
private:
    Logger() = default;
    
    LogLevel min_level_;
    bool console_output_;
    bool timestamp_enabled_;
    std::ofstream file_stream_;
    mutable std::mutex log_mutex_;
    
    std::string formatMessage(LogLevel level, 
                              const std::string& category,
                              const std::string& message) const;
    std::string getLevelString(LogLevel level) const;
    std::string getCurrentTimestamp() const;
};

// Macros de convenance
#define LOG_DEBUG(msg) asciivision::utils::Logger::instance().debug(msg)
#define LOG_INFO(msg) asciivision::utils::Logger::instance().info(msg)
#define LOG_WARNING(msg) asciivision::utils::Logger::instance().warning(msg)
#define LOG_ERROR(msg) asciivision::utils::Logger::instance().error(msg)
#define LOG_CRITICAL(msg) asciivision::utils::Logger::instance().critical(msg)

} // namespace asciivision::utils
```

**Utilité**: Système de logging centralisé. Indispensable pour le debug et le monitoring en production.

---

#### **PerformanceMonitor.hpp**

```cpp
namespace asciivision::utils {

// Statistiques de performance pour une métrique
struct PerformanceStats {
    double min_value;
    double max_value;
    double avg_value;
    double current_value;
    size_t sample_count;
};

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor() = default;
    
    // Enregistre une mesure
    // Paramètres:
    //   - metric_name: nom de la métrique
    //   - value: valeur mesurée
    void recordMetric(const std::string& metric_name, double value);
    
    // Enregistre une durée (helper pour chrono)
    template<typename Rep, typename Period>
    void recordDuration(const std::string& metric_name,
                       const std::chrono::duration<Rep, Period>& duration);
    
    // Retourne les stats d'une métrique
    PerformanceStats getStats(const std::string& metric_name) const;
    
    // Retourne toutes les stats sous forme JSON
    nlohmann::json getAllStats() const;
    
    // Reset une métrique
    void resetMetric(const std::string& metric_name);
    
    // Reset toutes les métriques
    void resetAll();
    
    // Active/désactive le monitoring
    void setEnabled(bool enabled);
    bool isEnabled() const;
    
    // Scopeguard pour mesurer automatiquement une durée
    class ScopedTimer {
    public:
        ScopedTimer(PerformanceMonitor& monitor, const std::string& metric_name);
        ~ScopedTimer();
        
    private:
        PerformanceMonitor& monitor_;
        std::string metric_name_;
        std::chrono::steady_clock::time_point start_;
    };
    
private:
    struct MetricData {
        double min;
        double max;
        double sum;
        double current;
        size_t count;
    };
    
    std::unordered_map<std::string, MetricData> metrics_;
    mutable std::mutex metrics_mutex_;
    std::atomic<bool> enabled_{true};
};

// Macro pour mesurer automatiquement
#define PERF_SCOPE(monitor, name) \
    asciivision::utils::PerformanceMonitor::ScopedTimer \
        perf_timer_##__LINE__(monitor, name)

} // namespace asciivision::utils
```

**Utilité**: Mesure et agrège les métriques de performance. Permet d'identifier les goulots d'étranglement.

---

### 2.7 Main - Point d'entrée

#### **main.cpp**

```cpp
#include "core/VideoSource.hpp"
#include "core/WebcamSource.hpp"
#include "filters/FilterRegistry.hpp"
#include "filters/GrayscaleFilter.hpp"
#include "filters/ResizeFilter.hpp"
#include "filters/AsciiFilter.hpp"
#include "processing/FrameController.hpp"
#include "network/WebSocketServer.hpp"
#include "network/MessageHandler.hpp"
#include "utils/Logger.hpp"

namespace av = asciivision;

int main(int argc, char* argv[]) {
    try {
        // Configuration du logger
        auto& logger = av::utils::Logger::instance();
        logger.setLogLevel(av::utils::LogLevel::INFO);
        logger.enableConsoleOutput(true);
        logger.enableTimestamp(true);
        
        LOG_INFO("AsciiVision starting...");
        
        // Enregistrement des filtres
        auto& registry = av::filters::FilterRegistry::instance();
        registry.registerFilter("grayscale", 
            []() { return std::make_unique<av::filters::GrayscaleFilter>(); });
        registry.registerFilter("resize",
            []() { return std::make_unique<av::filters::ResizeFilter>(); });
        registry.registerFilter("ascii",
            []() { return std::make_unique<av::filters::AsciiFilter>(); });
        
        // Création du contrôleur
        av::processing::FrameController controller;
        
        // Création du serveur WebSocket
        uint16_t port = 9002;
        av::network::WebSocketServer ws_server(port);
        av::network::MessageHandler msg_handler;
        
        // Configuration du callback de frames
        controller.setFrameCallback([&ws_server](const std::vector<uint8_t>& frame_data, 
                                                  uint32_t frame_id) {
            ws_server.broadcastBinary(frame_data);
        });
        
        // Configuration des handlers de commandes
        msg_handler.registerCommandHandler("start_engine",
            [&controller](const nlohmann::json& payload) {
                // Parse la config et démarre
                auto source = std::make_unique<av::core::WebcamSource>(0);
                controller.start(std::move(source), 30.0);
                
                // Configure le pipeline
                auto& pipeline = controller.getPipeline();
                pipeline.addFilter(std::make_unique<av::filters::ResizeFilter>(160, 120));
                pipeline.addFilter(std::make_unique<av::filters::GrayscaleFilter>());
                pipeline.addFilter(std::make_unique<av::filters::AsciiFilter>());
            });
        
        msg_handler.registerCommandHandler("stop_engine",
            [&controller](const nlohmann::json& payload) {
                controller.stop();
            });
        
        msg_handler.registerCommandHandler("set_parameter",
            [&controller](const nlohmann::json& payload) {
                std::string filter_name = payload["filter"];
                std::string param_name = payload["parameter"];
                auto value = payload["value"];
                
                controller.getPipeline().updateFilterParameter(
                    filter_name, param_name, value);
            });
        
        // Configuration du serveur WebSocket
        ws_server.setMessageCallback([&msg_handler, &ws_server]
                                     (websocketpp::connection_hdl hdl, 
                                      const std::string& message) {
            std::string response = msg_handler.handleMessage(message);
            if (!response.empty()) {
                ws_server.sendText(hdl, response);
            }
        });
        
        ws_server.setConnectionCallback([](websocketpp::connection_hdl hdl) {
            LOG_INFO("Client connected");
        });
        
        ws_server.setDisconnectionCallback([&controller]
                                           (websocketpp::connection_hdl hdl) {
            LOG_INFO("Client disconnected");
            if (controller.getState() == av::processing::EngineState::RUNNING) {
                controller.stop();
            }
        });
        
        // Démarrage du serveur
        LOG_INFO("Starting WebSocket server on port " + std::to_string(port));
        ws_server.start();
        
        // Boucle principale (attend Ctrl+C)
        LOG_INFO("AsciiVision ready. Press Ctrl+C to quit.");
        std::cin.get();
        
        // Arrêt propre
        LOG_INFO("Shutting down...");
        controller.stop();
        ws_server.stop();
        
        LOG_INFO("AsciiVision stopped.");
        return 0;
        
    } catch (const std::exception& e) {
        LOG_CRITICAL(std::string("Fatal error: ") + e.what());
        return 1;
    }
}
```

**Utilité**: Point d'entrée du backend. Initialise tous les composants, configure les callbacks, gère le cycle de vie de l'application.

---

## 3. Frontend - Spécifications des Composants

### 3.1 Types TypeScript

#### **types/protocol.ts**

```typescript
// Version du protocole
export const PROTOCOL_VERSION = "1.0";

// Types de messages
export enum MessageType {
  START_ENGINE = "start_engine",
  STOP_ENGINE = "stop_engine",
  UPDATE_FILTERS = "update_filters",
  SET_PARAMETER = "set_parameter",
  GET_CAPABILITIES = "get_capabilities",
  GET_STATUS = "get_status",
  
  ENGINE_STATUS = "engine_status",
  ERROR = "error",
  CAPABILITIES = "capabilities",
  ACK = "acknowledgement",
  FRAME = "frame",
}

// Codes d'erreur
export enum ErrorCode {
  INVALID_MESSAGE = "INVALID_MESSAGE",
  UNKNOWN_COMMAND = "UNKNOWN_COMMAND",
  SOURCE_UNAVAILABLE = "SOURCE_UNAVAILABLE",
  ALREADY_RUNNING = "ALREADY_RUNNING",
  NOT_RUNNING = "NOT_RUNNING",
  FILTER_NOT_FOUND = "FILTER_NOT_FOUND",
  INVALID_PARAMETER = "INVALID_PARAMETER",
  ENCODING_ERROR = "ENCODING_ERROR",
  INTERNAL_ERROR = "INTERNAL_ERROR",
}

// Structure de base d'un message
export interface BaseMessage {
  version: string;
  type: MessageType;
  timestamp: number;
  payload: Record<string, any>;
}

// Configuration de source
export interface SourceConfig {
  device_id?: number;
  width?: number;
  height?: number;
  fps?: number;
  path?: string;
}

// Configuration de filtre
export interface FilterConfig {
  name: string;
  enabled: boolean;
  params: Record<string, any>;
}

// Commande start_engine
export interface StartEngineMessage extends BaseMessage {
  type: MessageType.START_ENGINE;
  payload: {
    source: string;
    source_config: SourceConfig;
    pipeline: {
      target_width: number;
      target_height: number;
    };
  };
}

// Commande set_parameter
export interface SetParameterMessage extends BaseMessage {
  type: MessageType.SET_PARAMETER;
  payload: {
    filter: string;
    parameter: string;
    value: any;
  };
}

// Message de statut
export interface EngineStatusMessage extends BaseMessage {
  type: MessageType.ENGINE_STATUS;
  payload: {
    state: string;
    fps: number;
    frame_count: number;
    latency_ms: number;
    dropped_frames: number;
  };
}

// Message d'erreur
export interface ErrorMessage extends BaseMessage {
  type: MessageType.ERROR;
  payload: {
    code: ErrorCode;
    message: string;
    recoverable: boolean;
  };
}

// Frame vidéo (MVP avec Base64)
export interface FrameMessageMVP extends BaseMessage {
  type: MessageType.FRAME;
  frame_id: number;
  timestamp: number;
  width: number;
  height: number;
  format: "jpeg";
  data: string; // Base64
}

// Frame vidéo binaire (production)
export interface BinaryFrameHeader {
  magic: string; // "AVIS"
  version: number;
  frameId: number;
  timestamp: number;
  width: number;
  height: number;
}

export interface VideoFrame {
  frameId: number;
  timestamp: number;
  width: number;
  height: number;
  jpegData: ArrayBuffer;
}
```

**Utilité**: Types centralisés pour garantir la cohérence entre frontend et backend.

---

#### **types/filters.ts**

```typescript
// Type de paramètre
export enum ParameterType {
  INT = "int",
  DOUBLE = "double",
  STRING = "string",
  BOOL = "bool",
  ENUM = "enum",
}

// Définition d'un paramètre
export interface FilterParameter {
  name: string;
  type: ParameterType;
  description: string;
  default: any;
  min?: number;
  max?: number;
  values?: string[]; // Pour les enums
}

// Métadonnées d'un filtre
export interface FilterMetadata {
  name: string;
  displayName: string;
  description: string;
  parameters: FilterParameter[];
}

// État d'un filtre dans l'UI
export interface FilterState {
  name: string;
  enabled: boolean;
  parameters: Record<string, any>;
}
```

**Utilité**: Types pour gérer la configuration dynamique des filtres.

---

#### **types/engine.ts**

```typescript
// États du moteur
export enum EngineState {
  STOPPED = "stopped",
  STARTING = "starting",
  RUNNING = "running",
  STOPPING = "stopping",
  ERROR = "error",
}

// Statistiques du moteur
export interface EngineStats {
  state: EngineState;
  fps: number;
  frameCount: number;
  droppedFrames: number;
  latencyMs: number;
}

// Capacités du système
export interface SystemCapabilities {
  sources: string[];
  filters: FilterMetadata[];
}
```

**Utilité**: Types pour l'état global de l'application.

---

### 3.2 Hooks personnalisés

#### **hooks/useWebSocket.ts**

```typescript
import { useEffect, useRef, useState, useCallback } from 'react';
import { BaseMessage, VideoFrame } from '@/types/protocol';

interface UseWebSocketOptions {
  url: string;
  onMessage?: (message: BaseMessage) => void;
  onBinaryFrame?: (frame: VideoFrame) => void;
  onError?: (error: Event) => void;
  reconnect?: boolean;
  reconnectInterval?: number;
}

interface UseWebSocketReturn {
  connected: boolean;
  send: (message: BaseMessage) => void;
  close: () => void;
}

export function useWebSocket(options: UseWebSocketOptions): UseWebSocketReturn {
  const {
    url,
    onMessage,
    onBinaryFrame,
    onError,
    reconnect = true,
    reconnectInterval = 3000,
  } = options;

  const [connected, setConnected] = useState(false);
  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<NodeJS.Timeout>();

  const connect = useCallback(() => {
    try {
      const ws = new WebSocket(url);
      wsRef.current = ws;

      ws.onopen = () => {
        console.log('[WebSocket] Connected');
        setConnected(true);
      };

      ws.onclose = () => {
        console.log('[WebSocket] Disconnected');
        setConnected(false);

        if (reconnect) {
          reconnectTimeoutRef.current = setTimeout(connect, reconnectInterval);
        }
      };

      ws.onerror = (error) => {
        console.error('[WebSocket] Error:', error);
        onError?.(error);
      };

      ws.onmessage = (event) => {
        if (typeof event.data === 'string') {
          // Message de contrôle JSON
          try {
            const message: BaseMessage = JSON.parse(event.data);
            onMessage?.(message);
          } catch (e) {
            console.error('[WebSocket] Failed to parse JSON:', e);
          }
        } else if (event.data instanceof Blob) {
          // Message binaire
          event.data.arrayBuffer().then((buffer) => {
            try {
              const frame = parseBinaryFrame(buffer);
              onBinaryFrame?.(frame);
            } catch (e) {
              console.error('[WebSocket] Failed to parse binary frame:', e);
            }
          });
        }
      };
    } catch (error) {
      console.error('[WebSocket] Connection failed:', error);
    }
  }, [url, onMessage, onBinaryFrame, onError, reconnect, reconnectInterval]);

  useEffect(() => {
    connect();

    return () => {
      if (reconnectTimeoutRef.current) {
        clearTimeout(reconnectTimeoutRef.current);
      }
      wsRef.current?.close();
    };
  }, [connect]);

  const send = useCallback((message: BaseMessage) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      wsRef.current.send(JSON.stringify(message));
    } else {
      console.warn('[WebSocket] Cannot send, not connected');
    }
  }, []);

  const close = useCallback(() => {
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current);
    }
    wsRef.current?.close();
  }, []);

  return { connected, send, close };
}

// Parse le format binaire
function parseBinaryFrame(buffer: ArrayBuffer): VideoFrame {
  const view = new DataView(buffer);

  // Vérification magic number
  const magic = String.fromCharCode(
    view.getUint8(0),
    view.getUint8(1),
    view.getUint8(2),
    view.getUint8(3)
  );

  if (magic !== 'AVIS') {
    throw new Error('Invalid frame magic number');
  }

  // Lecture header
  const version = view.getUint16(4, true);
  const frameId = view.getUint32(6, true);
  const timestamp = Number(view.getBigUint64(10, true));
  const width = view.getUint16(18, true);
  const height = view.getUint16(20, true);

  // Extraction des données JPEG
  const jpegData = buffer.slice(24);

  return {
    frameId,
    timestamp,
    width,
    height,
    jpegData,
  };
}
```

**Utilité**: Encapsule toute la logique WebSocket. Gère la connexion, reconnexion, parsing des messages.

---

#### **hooks/useVideoStream.ts**

```typescript
import { useState, useCallback } from 'react';
import { useWebSocket } from './useWebSocket';
import { VideoFrame, BaseMessage, MessageType } from '@/types/protocol';

export function useVideoStream(url: string) {
  const [lastFrame, setLastFrame] = useState<VideoFrame | null>(null);
  const [frameRate, setFrameRate] = useState(0);

  const handleBinaryFrame = useCallback((frame: VideoFrame) => {
    setLastFrame(frame);

    // Calcul du FPS (moyenne glissante)
    // Implementation simplifiée, à améliorer
  }, []);

  const handleMessage = useCallback((message: BaseMessage) => {
    // Gestion des messages de contrôle si nécessaire
    if (message.type === MessageType.ENGINE_STATUS) {
      setFrameRate(message.payload.fps);
    }
  }, []);

  const { connected, send } = useWebSocket({
    url,
    onBinaryFrame: handleBinaryFrame,
    onMessage: handleMessage,
  });

  return {
    connected,
    lastFrame,
    frameRate,
    send,
  };
}
```

**Utilité**: Hook spécialisé pour le flux vidéo. Simplifie l'utilisation dans les composants.

---

#### **hooks/useEngineControl.ts**

```typescript
import { useState, useCallback } from 'react';
import { useWebSocket } from './useWebSocket';
import {
  BaseMessage,
  MessageType,
  EngineState,
  EngineStats,
  ErrorMessage,
  PROTOCOL_VERSION,
} from '@/types/protocol';

export function useEngineControl(url: string) {
  const [engineState, setEngineState] = useState<EngineState>(EngineState.STOPPED);
  const [engineStats, setEngineStats] = useState<EngineStats | null>(null);
  const [lastError, setLastError] = useState<string | null>(null);

  const handleMessage = useCallback((message: BaseMessage) => {
    switch (message.type) {
      case MessageType.ENGINE_STATUS:
        setEngineState(message.payload.state as EngineState);
        setEngineStats({
          state: message.payload.state,
          fps: message.payload.fps,
          frameCount: message.payload.frame_count,
          droppedFrames: message.payload.dropped_frames,
          latencyMs: message.payload.latency_ms,
        });
        break;

      case MessageType.ERROR:
        const errorMsg = message as ErrorMessage;
        setLastError(errorMsg.payload.message);
        if (!errorMsg.payload.recoverable) {
          setEngineState(EngineState.ERROR);
        }
        break;
    }
  }, []);

  const { connected, send }   = useWebSocket({
    url,
    onMessage: handleMessage,
  });

  const startEngine = useCallback((sourceType: string = 'webcam') => {
    send({
      version: PROTOCOL_VERSION,
      type: MessageType.START_ENGINE,
      timestamp: Date.now(),
      payload: {
        source: sourceType,
        source_config: {
          device_id: 0,
          width: 640,
          height: 480,
          fps: 30,
        },
        pipeline: {
          target_width: 160,
          target_height: 120,
        },
      },
    });
  }, [send]);

  const stopEngine = useCallback(() => {
    send({
      version: PROTOCOL_VERSION,
      type: MessageType.STOP_ENGINE,
      timestamp: Date.now(),
      payload: {},
    });
  }, [send]);

  const setParameter = useCallback(
    (filter: string, parameter: string, value: any) => {
      send({
        version: PROTOCOL_VERSION,
        type: MessageType.SET_PARAMETER,
        timestamp: Date.now(),
        payload: {
          filter,
          parameter,
          value,
        },
      });
    },
    [send]
  );

  return {
    connected,
    engineState,
    engineStats,
    lastError,
    startEngine,
    stopEngine,
    setParameter,
  };
}
```

**Utilité**: Hook pour contrôler le moteur backend. Encapsule toute la logique de commande.

---

### 3.3 Composants React

#### **components/VideoCanvas.tsx**

```typescript
'use client';

import { useEffect, useRef } from 'react';
import { VideoFrame } from '@/types/protocol';

interface VideoCanvasProps {
  frame: VideoFrame | null;
  className?: string;
}

export function VideoCanvas({ frame, className = '' }: VideoCanvasProps) {
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
      canvas.width = frame.width;
      canvas.height = frame.height;
      ctx.drawImage(img, 0, 0);
      URL.revokeObjectURL(url);
    };
    img.onerror = () => {
      console.error('[VideoCanvas] Failed to load frame');
      URL.revokeObjectURL(url);
    };
    img.src = url;
  }, [frame]);

  return (
    <canvas
      ref={canvasRef}
      className={`border border-gray-300 ${className}`}
      style={{ imageRendering: 'pixelated' }}
    />
  );
}
```

**Utilité**: Composant d'affichage vidéo. Décode et affiche les frames JPEG sur un canvas.

---

#### **components/ControlPanel.tsx**

```typescript
'use client';

import { EngineState } from '@/types/protocol';

interface ControlPanelProps {
  engineState: EngineState;
  connected: boolean;
  onStart: () => void;
  onStop: () => void;
}

export function ControlPanel({
  engineState,
  connected,
  onStart,
  onStop,
}: ControlPanelProps) {
  const isRunning = engineState === EngineState.RUNNING;
  const canStart = connected && engineState === EngineState.STOPPED;
  const canStop = connected && isRunning;

  return (
    <div className="flex gap-4 items-center p-4 bg-white rounded-lg shadow">
      <div className="flex items-center gap-2">
        <div
          className={`w-3 h-3 rounded-full ${
            connected ? 'bg-green-500' : 'bg-red-500'
          }`}
        />
        <span className="text-sm font-medium">
          {connected ? 'Connected' : 'Disconnected'}
        </span>
      </div>

      <div className="flex gap-2">
        <button
          onClick={onStart}
          disabled={!canStart}
          className="px-4 py-2 bg-blue-600 text-white rounded hover:bg-blue-700 disabled:bg-gray-300 disabled:cursor-not-allowed"
        >
          Start
        </button>

        <button
          onClick={onStop}
          disabled={!canStop}
          className="px-4 py-2 bg-red-600 text-white rounded hover:bg-red-700 disabled:bg-gray-300 disabled:cursor-not-allowed"
        >
          Stop
        </button>
      </div>

      <div className="ml-auto">
        <span className="text-sm text-gray-600">
          State: <span className="font-semibold">{engineState}</span>
        </span>
      </div>
    </div>
  );
}
```

**Utilité**: Panneau de contrôle principal. Boutons start/stop et affichage du statut.

---

#### **components/FilterControls.tsx**

```typescript
'use client';

import { useState } from 'react';

interface FilterControlsProps {
  onParameterChange: (filter: string, parameter: string, value: any) => void;
}

export function FilterControls({ onParameterChange }: FilterControlsProps) {
  const [charset, setCharset] = useState('standard');
  const [invert, setInvert] = useState(false);
  const [contrast, setContrast] = useState(1.0);

  const handleCharsetChange = (newCharset: string) => {
    setCharset(newCharset);
    onParameterChange('ascii', 'charset', newCharset);
  };

  const handleInvertChange = (newInvert: boolean) => {
    setInvert(newInvert);
    onParameterChange('ascii', 'invert', newInvert);
  };

  const handleContrastChange = (newContrast: number) => {
    setContrast(newContrast);
    onParameterChange('ascii', 'contrast', newContrast);
  };

  return (
    <div className="p-4 bg-white rounded-lg shadow space-y-4">
      <h3 className="font-semibold text-lg">ASCII Filter</h3>

      {/* Charset selector */}
      <div>
        <label className="block text-sm font-medium mb-2">Charset</label>
        <select
          value={charset}
          onChange={(e) => handleCharsetChange(e.target.value)}
          className="w-full px-3 py-2 border rounded"
        >
          <option value="minimal">Minimal</option>
          <option value="standard">Standard</option>
          <option value="extended">Extended</option>
          <option value="blocks">Blocks</option>
        </select>
      </div>

      {/* Invert toggle */}
      <div className="flex items-center gap-2">
        <input
          type="checkbox"
          id="invert"
          checked={invert}
          onChange={(e) => handleInvertChange(e.target.checked)}
          className="w-4 h-4"
        />
        <label htmlFor="invert" className="text-sm font-medium">
          Invert
        </label>
      </div>

      {/* Contrast slider */}
      <div>
        <label className="block text-sm font-medium mb-2">
          Contrast: {contrast.toFixed(2)}
        </label>
        <input
          type="range"
          min="0.5"
          max="2.0"
          step="0.1"
          value={contrast}
          onChange={(e) => handleContrastChange(parseFloat(e.target.value))}
          className="w-full"
        />
      </div>
    </div>
  );
}
```

**Utilité**: Contrôles pour ajuster les paramètres des filtres en temps réel.

---

#### **components/StatusBar.tsx**

```typescript
'use client';

import { EngineStats } from '@/types/engine';

interface StatusBarProps {
  stats: EngineStats | null;
}

export function StatusBar({ stats }: StatusBarProps) {
  if (!stats) {
    return (
      <div className="p-2 bg-gray-100 text-sm text-gray-500">
        No stats available
      </div>
    );
  }

  return (
    <div className="p-2 bg-gray-100 flex gap-6 text-sm">
      <div>
        <span className="font-medium">FPS:</span>{' '}
        <span className="font-mono">{stats.fps.toFixed(1)}</span>
      </div>
      <div>
        <span className="font-medium">Frames:</span>{' '}
        <span className="font-mono">{stats.frameCount}</span>
      </div>
      <div>
        <span className="font-medium">Dropped:</span>{' '}
        <span className="font-mono">{stats.droppedFrames}</span>
      </div>
      <div>
        <span className="font-medium">Latency:</span>{' '}
        <span className="font-mono">{stats.latencyMs.toFixed(1)}ms</span>
      </div>
    </div>
  );
}
```

**Utilité**: Barre de statut affichant les métriques de performance en temps réel.

---

#### **components/ErrorDisplay.tsx**

```typescript
'use client';

interface ErrorDisplayProps {
  error: string | null;
  onDismiss: () => void;
}

export function ErrorDisplay({ error, onDismiss }: ErrorDisplayProps) {
  if (!error) return null;

  return (
    <div className="fixed top-4 right-4 max-w-md p-4 bg-red-100 border border-red-400 rounded-lg shadow-lg">
      <div className="flex items-start gap-3">
        <div className="flex-shrink-0">
          <svg
            className="w-5 h-5 text-red-600"
            fill="currentColor"
            viewBox="0 0 20 20"
          >
            <path
              fillRule="evenodd"
              d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.707 7.293a1 1 0 00-1.414 1.414L8.586 10l-1.293 1.293a1 1 0 101.414 1.414L10 11.414l1.293 1.293a1 1 0 001.414-1.414L11.414 10l1.293-1.293a1 1 0 00-1.414-1.414L10 8.586 8.707 7.293z"
              clipRule="evenodd"
            />
          </svg>
        </div>
        <div className="flex-1">
          <h3 className="text-sm font-medium text-red-800">Error</h3>
          <p className="mt-1 text-sm text-red-700">{error}</p>
        </div>
        <button
          onClick={onDismiss}
          className="flex-shrink-0 text-red-600 hover:text-red-800"
        >
          ×
        </button>
      </div>
    </div>
  );
}
```

**Utilité**: Affiche les erreurs de manière non-intrusive. Dismissable par l'utilisateur.

---

#### **app/page.tsx** (Page principale)

```typecript
'use client';

import { useState } from 'react';
import { VideoCanvas } from '@/components/VideoCanvas';
import { ControlPanel } from '@/components/ControlPanel';
import { FilterControls } from '@/components/FilterControls';
import { StatusBar } from '@/components/StatusBar';
import { ErrorDisplay } from '@/components/ErrorDisplay';
import { useVideoStream } from '@/hooks/useVideoStream';
import { useEngineControl } from '@/hooks/useEngineControl';

const WS_URL = 'ws://localhost:9002';

export default function Home() {
  const { lastFrame } = useVideoStream(WS_URL);
  const {
    connected,
    engineState,
    engineStats,
    lastError,
    startEngine,
    stopEngine,
    setParameter,
  } = useEngineControl(WS_URL);

  const [errorDismissed, setErrorDismissed] = useState(false);

  return (
    <main className="min-h-screen bg-gray-50 p-8">
      <div className="max-w-7xl mx-auto space-y-6">
        <header>
          <h1 className="text-3xl font-bold text-gray-900">AsciiVision</h1>
          <p className="text-gray-600 mt-1">
            Real-time video processing with ASCII art rendering
          </p>
        </header>

        <ControlPanel
          engineState={engineState}
          connected={connected}
          onStart={() => startEngine('webcam')}
          onStop={stopEngine}
        />

        <div className="grid grid-cols-1 lg:grid-cols-3 gap-6">
          <div className="lg:col-span-2">
            <div className="bg-white rounded-lg shadow p-4">
              <VideoCanvas
                frame={lastFrame}
                className="w-full h-auto"
              />
            </div>
            <StatusBar stats={engineStats} />
          </div>

          <div>
            <FilterControls onParameterChange={setParameter} />
          </div>
        </div>

        <ErrorDisplay
          error={errorDismissed ? null : lastError}
          onDismiss={() => setErrorDismissed(true)}
        />
      </div>
    </main>
  );
}
```

**Utilité**: Page principale assemblant tous les composants. Point d'entrée de l'UI.

---

## 4. Récapitulatif - Ordre d'implémentation

### Phase 1: Backend minimal fonctionnel
1. `VideoSource.hpp` + `WebcamSource.cpp`
2. `IFilter.hpp`
3. `GrayscaleFilter.cpp`
4. `AsciiFilter.cpp`
5. `FramePipeline.cpp`
6. `FrameController.cpp` (version simple, 1 thread)
7. `main.cpp` (test console sans réseau)

### Phase 2: Réseau
8. `WebSocketServer.cpp`
9. `MessageHandler.cpp`
10. `ProtocolTypes.hpp`
11. Intégration dans `main.cpp`

### Phase 3: Frontend MVP
12. `types/protocol.ts`
13. `hooks/useWebSocket.ts`
14. `components/VideoCanvas.tsx`
15. `components/ControlPanel.tsx`
16. `app/page.tsx`

### Phase 4: Features complètes
17. `FilterRegistry.cpp`
18. Filtres additionnels (Blur, Resize, etc.)
19. `hooks/useEngineControl.ts`
20. `components/FilterControls.tsx`
21. `components/StatusBar.tsx`

### Phase 5: Optimisations
22. `ThreadSafeQueue.hpp`
23. `FrameController.cpp` (multi-thread)
24. `BinaryFrameEncoder.cpp`
25. Migration protocole binaire

### Phase 6: Utilitaires et polish
26. `Logger.cpp`
27. `PerformanceMonitor.cpp`
28. `ErrorDisplay.tsx`
29. Tests unitaires
30. Documentation

---


## 📐 Architecture Globale Complète

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                            FRONTEND (Navigateur)                            │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                      Interface Utilisateur                            │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────────┐  │ │
│  │  │  Sélection   │  │   Contrôle   │  │   Configuration Pipeline  │  │ │
│  │  │   Source     │  │  Start/Stop  │  │   (Filtres actifs)       │  │ │
│  │  │              │  │              │  │                           │  │ │
│  │  │ • Webcam     │  │ • Démarrer   │  │ ☐ Grayscale              │  │ │
│  │  │   client     │  │ • Arrêter    │  │ ☐ Blur                   │  │ │
│  │  │ • Webcam     │  │ • Pause      │  │ ☐ Edge Detection         │  │ │
│  │  │   serveur    │  │              │  │ ☐ Face Detection         │  │ │
│  │  │ • Image      │  │              │  │ ☐ ASCII Render           │  │ │
│  │  │ • Séquence   │  │              │  │ ☐ Custom Filter          │  │ │
│  │  └──────────────┘  └──────────────┘  └───────────────────────────┘  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     Canvas d'affichage                                │ │
│  │  • Affiche le résultat du pipeline                                    │ │
│  │  • Peut être : image brute, contours, visages détectés, ASCII, etc.  │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     Barre de Statut                                   │ │
│  │  FPS: 28.5 | Frames: 1247 | Latence: 45ms | Dropped: 3               │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     Couche WebSocket                                  │ │
│  │  • Envoi : Commandes JSON (start, configure, set_parameter)          │ │
│  │  • Envoi : Frames binaires (si mode webcam client)                   │ │
│  │  • Réception : Résultats traités (binaire JPEG/PNG)                  │ │
│  │  • Réception : Statuts et erreurs (JSON)                             │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
                                    ↕
                          WebSocket (port 9002)
                                    ↕
┌─────────────────────────────────────────────────────────────────────────────┐
│                            BACKEND C++                                      │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     WebSocketServer                                   │ │
│  │  • Accepte connexions clients multiples                               │ │
│  │  • Reçoit commandes JSON                                              │ │
│  │  • Reçoit frames binaires (NetworkSource)                             │ │
│  │  • Broadcast résultats à tous les clients                             │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    ↓                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     MessageHandler                                    │ │
│  │  • Parse et valide messages JSON                                      │ │
│  │  • Dispatch vers handlers appropriés                                  │ │
│  │  • Génère réponses formatées                                          │ │
│  │  • Gère les erreurs protocole                                         │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    ↓                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     FrameController (Orchestrateur)                   │ │
│  │  • Gère cycle de vie (start/stop/pause)                               │ │
│  │  • Coordonne : Source → Pipeline → Encoder → Output                   │ │
│  │  • Threading et synchronisation                                       │ │
│  │  • Collecte statistiques (FPS, latence, dropped frames)               │ │
│  │  • Régulation FPS cible                                               │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    ↓                                        │
│  ┌─────────────────────────────┐                                           │
│  │      VIDEO SOURCES          │                                           │
│  │  (Interface VideoSource)    │                                           │
│  │                             │                                           │
│  │  ┌───────────────────────┐  │                                           │
│  │  │  WebcamSource         │  │ ← Webcam du serveur (OpenCV)             │
│  │  │  - device_id          │  │                                           │
│  │  │  - résolution         │  │                                           │
│  │  │  - FPS natif          │  │                                           │
│  │  └───────────────────────┘  │                                           │
│  │                             │                                           │
│  │  ┌───────────────────────┐  │                                           │
│  │  │  NetworkSource        │  │ ← Frames depuis le client via WebSocket  │
│  │  │  - Queue frames       │  │                                           │
│  │  │  - Décodage JPEG      │  │                                           │
│  │  │  - Buffer circulaire  │  │                                           │
│  │  └───────────────────────┘  │                                           │
│  │                             │                                           │
│  │  ┌───────────────────────┐  │                                           │
│  │  │  ImageSource          │  │ ← Image fixe (test/debug)                │
│  │  │  - path               │  │                                           │
│  │  │  - loop mode          │  │                                           │
│  │  └───────────────────────┘  │                                           │
│  │                             │                                           │
│  │  ┌───────────────────────┐  │                                           │
│  │  │  ImageSequenceSource  │  │ ← Séquence d'images (tests)              │
│  │  │  - directory          │  │                                           │
│  │  │  - FPS simulé         │  │                                           │
│  │  └───────────────────────┘  │                                           │
│  │                             │                                           │
│  │  ┌───────────────────────┐  │                                           │
│  │  │  VideoFileSource      │  │ ← Fichier vidéo (future)                 │
│  │  │  - codec detection    │  │                                           │
│  │  └───────────────────────┘  │                                           │
│  └─────────────────────────────┘                                           │
│                 ↓ readFrame()                                              │
│            cv::Mat (frame brute)                                           │
│                 ↓                                                           │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     FramePipeline                                     │ │
│  │  Pipeline configurable dynamiquement                                  │ │
│  │                                                                       │ │
│  │  ┌─────────────────────────────────────────────────────────────────┐ │ │
│  │  │  Liste ordonnée de filtres (std::vector<IFilter>)              │ │ │
│  │  │                                                                 │ │ │
│  │  │  Filtre 1 → Filtre 2 → Filtre 3 → ... → Filtre N               │ │ │
│  │  │                                                                 │ │ │
│  │  │  Chaque filtre :                                                │ │ │
│  │  │  • Peut être activé/désactivé                                   │ │ │
│  │  │  • A ses propres paramètres                                     │ │ │
│  │  │  • Est indépendant des autres                                   │ │ │
│  │  │  • Peut être ajouté/retiré en temps réel                        │ │ │
│  │  └─────────────────────────────────────────────────────────────────┘ │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    ↓                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     FILTRES DISPONIBLES                               │ │
│  │  (Interface IFilter - Extensible à l'infini)                          │ │
│  │                                                                       │ │
│  │  📐 Traitement de base :                                              │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │ │
│  │  │ ResizeFilter    │  │ GrayscaleFilter │  │ CropFilter       │    │ │
│  │  │ - width/height  │  │ - no params     │  │ - x,y,w,h        │    │ │
│  │  │ - interpolation │  │                 │  │                  │    │ │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────┘    │ │
│  │                                                                       │ │
│  │  🎨 Effets & Transformations :                                        │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │ │
│  │  │ BlurFilter      │  │ SharpenFilter   │  │ ContrastFilter   │    │ │
│  │  │ - kernel_size   │  │ - strength      │  │ - level          │    │ │
│  │  │ - sigma         │  │                 │  │ - brightness     │    │ │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────┘    │ │
│  │                                                                       │ │
│  │  🔍 Computer Vision :                                                 │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │ │
│  │  │EdgeDetectionFlt │  │FaceDetectionFlt │  │ObjectDetectFlt   │    │ │
│  │  │ - threshold1/2  │  │ - cascade file  │  │ - YOLO model     │    │ │
│  │  │ - aperture      │  │ - min_neighbors │  │ - confidence     │    │ │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────┘    │ │
│  │                                                                       │ │
│  │  🎭 Rendus artistiques :                                              │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │ │
│  │  │ AsciiFilter     │  │ CartoonFilter   │  │ PencilSketchFlt  │    │ │
│  │  │ - charset       │  │ - edge_thresh   │  │ - shade_factor   │    │ │
│  │  │ - invert        │  │ - bilateral     │  │ - sketch_level   │    │ │
│  │  │ - contrast      │  │                 │  │                  │    │ │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────┘    │ │
│  │                                                                       │ │
│  │  🤖 IA & Deep Learning (futur) :                                      │ │
│  │  ┌─────────────────┐  ┌─────────────────┐  ┌──────────────────┐    │ │
│  │  │ StyleTransferFlt│  │ SuperResolution │  │ SemanticSegment  │    │ │
│  │  │ - model_path    │  │ - scale_factor  │  │ - num_classes    │    │ │
│  │  └─────────────────┘  └─────────────────┘  └──────────────────┘    │ │
│  │                                                                       │ │
│  │  🔧 Custom :                                                          │ │
│  │  ┌─────────────────┐                                                 │ │
│  │  │ CustomFilter    │  ← Utilisateur peut créer ses propres filtres  │ │
│  │  │ - user_params   │                                                 │ │
│  │  └─────────────────┘                                                 │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    ↓                                        │
│                         cv::Mat (frame traitée)                             │
│                                    ↓                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     FrameEncoder                                      │ │
│  │  • Encode en JPEG (qualité configurable)                              │ │
│  │  • Encode en PNG (lossless si besoin)                                 │ │
│  │  • Ajoute header binaire (frame_id, timestamp, dimensions)            │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
│                                    ↓                                        │
│                    std::vector<uint8_t> (buffer binaire)                   │
│                                    ↓                                        │
│  ┌───────────────────────────────────────────────────────────────────────┐ │
│  │                     Callback vers WebSocket                           │ │
│  │  • Envoie au(x) client(s) connecté(s)                                 │ │
│  └───────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Flux de Données Complet

### Cas 1 : Mode Webcam Client (NetworkSource)

```
┌─────────────────┐
│  NAVIGATEUR     │
│                 │
│  getUserMedia() │ ← Accès webcam client
│       ↓         │
│  <video>        │ ← Flux vidéo live
│       ↓         │
│  captureFrame() │ ← Capture périodique (15 FPS)
│       ↓         │
│  Canvas.resize  │ ← Réduit à 320×240
│       ↓         │
│  toBlob('jpeg') │ ← Encode JPEG 75%
│       ↓         │
│  ArrayBuffer    │ ← ~15-30 KB par frame
│       ↓         │
│  WebSocket.send │ ← Envoi binaire
└────────┬────────┘
         │ WebSocket (binaire)
         ↓
┌─────────────────────────────────────┐
│  BACKEND                            │
│                                     │
│  WebSocketServer::onMessage()      │
│       ↓                             │
│  NetworkSource::pushFrame()        │ ← Décode JPEG → cv::Mat
│       ↓                             │
│  ThreadSafeQueue<cv::Mat>          │ ← Buffer circulaire
│       ↓                             │
│  FrameController::workerLoop()     │
│       ↓                             │
│  VideoSource::readFrame()          │ ← Lit depuis queue
│       ↓                             │
│  FramePipeline::process()          │
│       ↓                             │
│  [Filtre 1] → [Filtre 2] → ...     │ ← Pipeline configurable
│       ↓                             │
│  FrameEncoder::encodeJPEG()        │ ← Encode résultat
│       ↓                             │
│  Callback                           │ ← Retour vers WebSocket
│       ↓                             │
│  WebSocket.send(binary)            │ ← Envoie au client
└────────┬────────────────────────────┘
         │ WebSocket (binaire)
         ↓
┌─────────────────┐
│  NAVIGATEUR     │
│                 │
│  onmessage()    │ ← Reçoit frame traitée
│       ↓         │
│  Blob → Image   │ ← Décode JPEG
│       ↓         │
│  Canvas.draw    │ ← Affiche résultat
└─────────────────┘
```

**Latence totale :** ~50-100ms (upload + process + download)

---

### Cas 2 : Mode Webcam Serveur (WebcamSource)

```
┌─────────────────┐
│  NAVIGATEUR     │
│                 │
│  Click "Start"  │
│       ↓         │
│  send(JSON)     │ ← {"type": "start_engine", "source": "webcam"}
└────────┬────────┘
         │ WebSocket (JSON)
         ↓
┌─────────────────────────────────────┐
│  BACKEND                            │
│                                     │
│  MessageHandler::handleMessage()   │
│       ↓                             │
│  FrameController::start()          │
│       ↓                             │
│  WebcamSource::open()              │ ← OpenCV ouvre webcam SERVEUR
│       ↓                             │
│  FrameController::workerLoop()     │
│       ↓                             │
│  WebcamSource::readFrame()         │ ← Capture depuis cv::VideoCapture
│       ↓                             │
│  FramePipeline::process()          │
│       ↓                             │
│  [Filtre 1] → [Filtre 2] → ...     │
│       ↓                             │
│  FrameEncoder::encodeJPEG()        │
│       ↓                             │
│  WebSocket.send(binary)            │
└────────┬────────────────────────────┘
         │ WebSocket (binaire)
         ↓
┌─────────────────┐
│  NAVIGATEUR     │
│                 │
│  onmessage()    │
│       ↓         │
│  Canvas.draw    │ ← Affiche
└─────────────────┘
```

**Latence totale :** ~15-30ms (juste process + download)

---

## 🛠️ Exemples de Configuration Pipeline

### Exemple 1 : Détection de contours (PAS d'ASCII)

**Objectif :** Extraire les contours d'une image en temps réel

```json
{
  "type": "start_engine",
  "version": "1.0",
  "timestamp": 1703001234567,
  "payload": {
    "source": "network",
    "source_config": {
      "expected_fps": 15
    },
    "pipeline": {
      "filters": [
        {
          "name": "resize",
          "enabled": true,
          "params": {
            "width": 640,
            "height": 480,
            "interpolation": "linear"
          }
        },
        {
          "name": "grayscale",
          "enabled": true,
          "params": {}
        },
        {
          "name": "blur",
          "enabled": true,
          "params": {
            "kernel_size": 5,
            "sigma": 1.5
          }
        },
        {
          "name": "edge_detection",
          "enabled": true,
          "params": {
            "threshold1": 50,
            "threshold2": 150,
            "aperture_size": 3
          }
        }
      ]
    }
  }
}
```

**Résultat :** Image en noir et blanc avec contours détectés (algorithme Canny)

---

### Exemple 2 : Détection de visages (Computer Vision)

**Objectif :** Détecter et encadrer les visages dans la vidéo

```json
{
  "type": "start_engine",
  "version": "1.0",
  "timestamp": 1703001234567,
  "payload": {
    "source": "webcam",
    "source_config": {
      "device_id": 0,
      "width": 1280,
      "height": 720,
      "fps": 30
    },
    "pipeline": {
      "filters": [
        {
          "name": "face_detection",
          "enabled": true,
          "params": {
            "cascade_file": "haarcascade_frontalface_default.xml",
            "scale_factor": 1.1,
            "min_neighbors": 5,
            "min_size": 30,
            "draw_boxes": true,
            "box_color": [0, 255, 0],
            "box_thickness": 2
          }
        }
      ]
    }
  }
}
```

**Résultat :** Image originale avec rectangles verts autour des visages

---

### Exemple 3 : ASCII Art (UN cas d'usage parmi d'autres)

**Objectif :** Rendu ASCII artistique

```json
{
  "type": "start_engine",
  "version": "1.0",
  "timestamp": 1703001234567,
  "payload": {
    "source": "network",
    "source_config": {
      "expected_fps": 15
    },
    "pipeline": {
      "filters": [
        {
          "name": "resize",
          "enabled": true,
          "params": {
            "width": 160,
            "height": 120
          }
        },
        {
          "name": "grayscale",
          "enabled": true,
          "params": {}
        },
        {
          "name": "contrast",
          "enabled": true,
          "params": {
            "level": 1.5,
            "brightness": 0
          }
        },
        {
          "name": "ascii",
          "enabled": true,
          "params": {
            "charset": "extended",
            "invert": false,
            "scale": 1.0
          }
        }
      ]
    }
  }
}
```

**Résultat :** Art ASCII

---

### Exemple 4 : Pipeline complexe (Multi-filtres)

**Objectif :** Détection de contours + détection de visages sur la même image

```json
{
  "type": "start_engine",
  "version": "1.0",
  "timestamp": 1703001234567,
  "payload": {
    "source": "network",
    "pipeline": {
      "filters": [
        {
          "name": "resize",
          "enabled": true,
          "params": { "width": 1280, "height": 720 }
        },
        {
          "name": "denoise",
          "enabled": true,
          "params": { "strength": 10 }
        },
        {
          "name": "face_detection",
          "enabled": true,
          "params": {
            "draw_boxes": true,
            "box_color": [255, 0, 0]
          }
        },
        {
          "name": "edge_detection",
          "enabled": true,
          "params": {
            "threshold1": 100,
            "threshold2": 200
          }
        }
      ]
    }
  }
}
```

**Résultat :** Image avec visages encadrés ET contours détectés

---

## 🔧 Modifications Nécessaires (Clarifications)

### ❌ Ce qui NE change PAS

L'architecture backend est **déjà flexible** :
- ✅ `IFilter` interface → n'importe quel filtre
- ✅ `FramePipeline` → ordre configurable
- ✅ `VideoSource` → n'importe quelle source
- ✅ `MessageHandler` → protocole générique

### ✔️ Ce qu'il faut clarifier/ajuster

#### 1. **Renommage conceptuel** (optionnel mais recommandé)

```cpp
// Au lieu de :
class AsciiFilter : public IFilter { ... };

// Avoir aussi :
class EdgeDetectionFilter : public IFilter { ... };
class FaceDetectionFilter : public IFilter { ... };
class BlurFilter : public IFilter { ... };
class ContrastFilter : public IFilter { ... };
```

#### 2. **Interface utilisateur** - Composant `PipelineBuilder`

Au lieu d'avoir des contrôles spécifiques ASCII, avoir une **interface générique** :

```typescript
// components/PipelineBuilder.tsx
interface FilterConfig {
  name: string;
  displayName: string;
  enabled: boolean;
  parameters: FilterParameter[];
}

function PipelineBuilder({ 
  availableFilters, 
  activeFilters, 
  onUpdatePipeline 
}: Props) {
  // Interface drag & drop pour :
  // - Ajouter/retirer des filtres
  // - Réorganiser l'ordre
  // - Activer/désactiver
  // - Ajuster paramètres
}
```

**Interface visuelle** :

```
┌────────────────────────────────────────────────────┐
│  Pipeline Configuration                            │
├────────────────────────────────────────────────────┤
│                                                    │
│  Available Filters:                                │
│  ┌──────┐  ┌──────┐  ┌──────┐  ┌──────┐         │
│  │ Blur │  │Edges │  │Faces │  │ASCII │  ...    │
│  └──────┘  └──────┘  └──────┘  └──────┘         │
│                                                    │
│  Active Pipeline (drag to reorder):                │
│  ┌────────────────────────────────────────────┐   │
│  │  1. ☑ Resize          [⚙ Configure]       │   │
│  │  2. ☑ Grayscale       [⚙ Configure]       │   │
│  │  3. ☐ Blur            [⚙ Configure]       │   │
│  │  4. ☑ Edge Detection  [⚙ Configure]       │   │
│  │  5. ☐ ASCII Render    [⚙ Configure]       │   │
│  └────────────────────────────────────────────┘   │
│                                                    │
│  [Add Filter ▼]  [Clear All]  [Apply Changes]     │
└────────────────────────────────────────────────────┘
```

#### 3. **FilterRegistry** - Enregistrement auto des filtres

```cpp
// filters/GrayscaleFilter.cpp
REGISTER_FILTER(GrayscaleFilter, "grayscale");

// filters/EdgeDetectionFilter.cpp
REGISTER_FILTER(EdgeDetectionFilter, "edge_detection");

// filters/FaceDetectionFilter.cpp
REGISTER_FILTER(FaceDetectionFilter, "face_detection");

// filters/AsciiFilter.cpp
REGISTER_FILTER(AsciiFilter, "ascii");

// Tous les filtres sont automatiquement disponibles
```

#### 4. **Message `get_capabilities`** - Liste dynamique

Le frontend demande au backend quels filtres sont disponibles :

**Requête :**
```json
{
  "type": "get_capabilities",
  "version": "1.0",
  "timestamp": 1703001234567,
  "payload": {}
}
```

**Réponse :**
```json
{
  "type": "capabilities",
  "version": "1.0",
  "timestamp": 1703001234568,
  "payload": {
    "sources": [
      "webcam",
      "network",
      "image",
      "sequence"
    ],
    "filters": [
      {
        "name": "resize",
        "display_name": "Resize",
        "description": "Change image dimensions",
        "category": "basic",
        "parameters": [
          {
            "name": "width",
            "type": "int",
            "default": 640,
            "min": 64,
            "max": 1920
          },
          {
            "name": "height",
            "type": "int",
            "default": 480,
            "min": 64,
            "max": 1080
          }
        ]
      },
      {
        "name": "grayscale",
        "display_name": "Grayscale",
        "description": "Convert to black & white",
        "category": "basic",
        "parameters": []
      },
      {
        "name": "edge_detection",
        "display_name": "Edge Detection",
        "description": "Detect edges using Canny algorithm",
        "category": "computer_vision",
        "parameters": [
          {
            "name": "threshold1",
            "type": "double",
            "default": 50,
            "min": 0,
            "max": 300
          },
          {
            "name": "threshold2",
            "type": "double",
            "default": 150,
            "min": 0,
            "max": 300
          }
        ]
      },
      {
        "name": "face_detection",
        "display_name": "Face Detection",
        "description": "Detect faces using Haar Cascade",
        "category": "computer_vision",
        "parameters": [
          {
            "name": "draw_boxes",
            "type": "bool",
            "default": true
          },
          {
            "name": "min_neighbors",
            "type": "int",
            "default": 5,
            "min": 1,
            "max": 10
          }
        ]
      },
      {
        "name": "ascii",
        "display_name": "ASCII Art",
        "description": "Render as ASCII characters",
        "category": "artistic",
        "parameters": [
          {
            "name": "charset",
            "type": "enum",
            "default": "standard",
            "values": ["minimal", "standard", "extended", "blocks"]
          },
          {
            "name": "invert",
            "type": "bool",
            "default": false
          }
        ]
      }
    ]
  }
}
```

Le frontend construit **dynamiquement** son interface à partir de cette réponse.

---

# AsciiVision - Architecture Multi-Threading Détaillée

## 🧵 Vue d'ensemble des Threads

Le système utilise **4 threads principaux** + 1 thread réseau pour assurer un traitement fluide et sans blocage.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SYSTÈME MULTI-THREAD                              │
└─────────────────────────────────────────────────────────────────────────────┘

┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐
│   THREAD 1       │   │   THREAD 2       │   │   THREAD 3       │   │   THREAD 4       │
│   CAPTURE        │──→│   PROCESSING     │──→│   ENCODING       │──→│   NETWORK        │
│                  │   │                  │   │                  │   │                  │
│ Lecture source   │   │ Application      │   │ Compression      │   │ Envoi WebSocket  │
│ vidéo            │   │ filtres          │   │ JPEG/PNG         │   │ aux clients      │
└──────────────────┘   └──────────────────┘   └──────────────────┘   └──────────────────┘
         │                      │                      │                      │
         ↓                      ↓                      ↓                      ↓
    Queue<Mat>            Queue<Mat>            Queue<Buffer>          Queue<Message>
    (raw frames)          (processed)           (compressed)            (outgoing)
    Capacity: 2           Capacity: 2           Capacity: 3             Capacity: 5

                                    +
                          ┌──────────────────┐
                          │   THREAD 0       │
                          │   MAIN           │
                          │                  │
                          │ Gestion UI       │
                          │ Commandes JSON   │
                          │ Configuration    │
                          └──────────────────┘

                                    +
                          ┌──────────────────┐
                          │   THREAD WS      │
                          │   WebSocket      │
                          │                  │
                          │ Accepte clients  │
                          │ Reçoit commandes │
                          │ (géré par lib)   │
                          └──────────────────┘
```

---

## 🎯 Thread 1 : CAPTURE (Acquisition)

### Responsabilité
Lit les frames depuis la source vidéo le plus rapidement possible sans bloquer.

### Géré par
`FrameController::captureThreadFunc()`

### Pseudo-code
```cpp
void FrameController::captureThreadFunc() {
    cv::Mat raw_frame;
    
    while (running_) {
        auto start = chrono::steady_clock::now();
        
        // 1. Lit une frame depuis la source
        bool success = video_source_->readFrame(raw_frame);
        
        if (!success) {
            LOG_ERROR("Failed to read frame, stopping capture");
            break;
        }
        
        // 2. Clone la frame (important pour éviter data race)
        cv::Mat frame_copy = raw_frame.clone();
        
        // 3. Pousse vers la queue (BLOQUANT si pleine)
        bool pushed = raw_frame_queue_.push(frame_copy, 
                                            chrono::milliseconds(100));
        
        if (!pushed) {
            dropped_frames_++;
            LOG_WARNING("Dropped frame in capture");
        }
        
        // 4. Statistiques
        auto elapsed = chrono::steady_clock::now() - start;
        perf_monitor_.recordDuration("capture_time", elapsed);
        
        // 5. Régulation FPS source (optionnel)
        double source_fps = video_source_->getFPS();
        if (source_fps > 0) {
            auto target_delay = chrono::milliseconds(1000 / source_fps);
            if (elapsed < target_delay) {
                this_thread::sleep_for(target_delay - elapsed);
            }
        }
    }
    
    // Ferme la queue pour signaler la fin
    raw_frame_queue_.close();
    LOG_INFO("Capture thread stopped");
}
```

### Détails importants

**Tâches :**
1. Appelle `VideoSource::readFrame()` en boucle
2. Clone la frame pour éviter les data races
3. Pousse dans `raw_frame_queue_`
4. Gère le FPS de la source
5. Compte les frames droppées

**Blocage :**
- Bloque sur `VideoSource::readFrame()` si webcam lente
- Bloque sur `raw_frame_queue_.push()` si la queue est pleine (backpressure)

**Performance :**
- Cible : ~30-60 FPS selon la source
- Temps typique : 5-15ms par frame (webcam)
- Temps typique : <1ms par frame (NetworkSource depuis queue)

---

## 🎨 Thread 2 : PROCESSING (Traitement)

### Responsabilité
Applique le pipeline de filtres sur chaque frame.

### Géré par
`FrameController::processingThreadFunc()`

### Pseudo-code
```cpp
void FrameController::processingThreadFunc() {
    cv::Mat raw_frame;
    cv::Mat processed_frame;
    
    while (running_) {
        auto start = chrono::steady_clock::now();
        
        // 1. Récupère une frame depuis capture (BLOQUANT si vide)
        bool received = raw_frame_queue_.pop(raw_frame, 
                                            chrono::milliseconds(100));
        
        if (!received) {
            if (raw_frame_queue_.isClosed()) break;
            continue; // Timeout, retry
        }
        
        // 2. Applique le pipeline de filtres
        try {
            pipeline_->process(raw_frame, processed_frame);
        } catch (const exception& e) {
            LOG_ERROR("Processing error: " + string(e.what()));
            continue;
        }
        
        // 3. Pousse vers encoding (BLOQUANT si pleine)
        bool pushed = processed_frame_queue_.push(processed_frame, 
                                                  chrono::milliseconds(100));
        
        if (!pushed) {
            dropped_frames_++;
            LOG_WARNING("Dropped frame in processing");
        }
        
        // 4. Statistiques
        auto elapsed = chrono::steady_clock::now() - start;
        perf_monitor_.recordDuration("processing_time", elapsed);
        
        // 5. Mise à jour compteurs
        processed_frames_++;
    }
    
    // Ferme la queue pour signaler la fin
    processed_frame_queue_.close();
    LOG_INFO("Processing thread stopped");
}
```

### Détails importants

**Tâches :**
1. Lit depuis `raw_frame_queue_`
2. Appelle `FramePipeline::process()`
3. Pousse dans `processed_frame_queue_`
4. Gère les erreurs de traitement

**Blocage :**
- Bloque sur `raw_frame_queue_.pop()` si vide
- Bloque sur `processed_frame_queue_.push()` si pleine

**Performance :**
- Temps dépend FORTEMENT du pipeline actif
- Pipeline simple (resize + grayscale) : 2-5ms
- Pipeline complexe (face detection) : 50-200ms
- Pipeline ASCII : 5-15ms
- Pipeline IA (futur) : 100-500ms

**Point critique :**
C'est ici que se passe 90% du calcul. Si ce thread est trop lent, il ralentit tout le système (backpressure).

---

## 📦 Thread 3 : ENCODING (Compression)

### Responsabilité
Encode les frames traitées en JPEG/PNG pour transmission réseau.

### Géré par
`FrameController::encodingThreadFunc()`

### Pseudo-code
```cpp
void FrameController::encodingThreadFunc() {
    cv::Mat processed_frame;
    vector<uint8_t> encoded_buffer;
    uint32_t frame_id = 0;
    
    while (running_) {
        auto start = chrono::steady_clock::now();
        
        // 1. Récupère frame traitée (BLOQUANT si vide)
        bool received = processed_frame_queue_.pop(processed_frame, 
                                                   chrono::milliseconds(100));
        
        if (!received) {
            if (processed_frame_queue_.isClosed()) break;
            continue;
        }
        
        // 2. Encode en JPEG avec qualité configurable
        vector<int> params = {cv::IMWRITE_JPEG_QUALITY, jpeg_quality_};
        bool success = cv::imencode(".jpg", processed_frame, 
                                   encoded_buffer, params);
        
        if (!success) {
            LOG_ERROR("Failed to encode frame");
            continue;
        }
        
        // 3. Construit le message binaire avec header
        BinaryFrameHeader header;
        header.magic[0] = 'A'; header.magic[1] = 'V';
        header.magic[2] = 'I'; header.magic[3] = 'S';
        header.version = 0x0100;
        header.frame_id = frame_id++;
        header.timestamp = getCurrentTimestamp();
        header.width = processed_frame.cols;
        header.height = processed_frame.rows;
        
        // 4. Assemble header + payload
        vector<uint8_t> message(sizeof(header) + encoded_buffer.size());
        memcpy(message.data(), &header, sizeof(header));
        memcpy(message.data() + sizeof(header), 
               encoded_buffer.data(), 
               encoded_buffer.size());
        
        // 5. Pousse vers network (BLOQUANT si pleine)
        bool pushed = outgoing_queue_.push(message, 
                                          chrono::milliseconds(100));
        
        if (!pushed) {
            dropped_frames_++;
            LOG_WARNING("Dropped frame in encoding");
        }
        
        // 6. Statistiques
        auto elapsed = chrono::steady_clock::now() - start;
        perf_monitor_.recordDuration("encoding_time", elapsed);
        perf_monitor_.recordMetric("encoded_size_kb", 
                                   message.size() / 1024.0);
    }
    
    // Ferme la queue pour signaler la fin
    outgoing_queue_.close();
    LOG_INFO("Encoding thread stopped");
}
```

### Détails importants

**Tâches :**
1. Lit depuis `processed_frame_queue_`
2. Encode avec OpenCV `cv::imencode()`
3. Ajoute le header binaire personnalisé
4. Pousse dans `outgoing_queue_`

**Blocage :**
- Bloque sur `processed_frame_queue_.pop()` si vide
- Bloque sur `outgoing_queue_.push()` si pleine

**Performance :**
- Temps typique : 5-15ms (dépend de la taille et qualité JPEG)
- 320×240 @ 75% qualité : ~10-20 KB par frame
- 640×480 @ 85% qualité : ~30-50 KB par frame

**Optimisation possible :**
- Utiliser libjpeg-turbo au lieu de OpenCV (2-3x plus rapide)
- Encoder en WebP (meilleure compression)

---

## 🌐 Thread 4 : NETWORK (Envoi)

### Responsabilité
Envoie les frames encodées à tous les clients WebSocket connectés.

### Géré par
`FrameController::networkThreadFunc()`

### Pseudo-code
```cpp
void FrameController::networkThreadFunc() {
    vector<uint8_t> message;
    
    while (running_) {
        auto start = chrono::steady_clock::now();
        
        // 1. Récupère message à envoyer (BLOQUANT si vide)
        bool received = outgoing_queue_.pop(message, 
                                           chrono::milliseconds(100));
        
        if (!received) {
            if (outgoing_queue_.isClosed()) break;
            continue;
        }
        
        // 2. Envoie à tous les clients connectés
        try {
            ws_server_->broadcastBinary(message);
            sent_frames_++;
        } catch (const exception& e) {
            LOG_ERROR("Network send error: " + string(e.what()));
        }
        
        // 3. Statistiques
        auto elapsed = chrono::steady_clock::now() - start;
        perf_monitor_.recordDuration("network_time", elapsed);
        
        // 4. Throttling si nécessaire (limite bande passante)
        if (max_bandwidth_kbps_ > 0) {
            double message_kb = message.size() / 1024.0;
            double send_time_ms = (message_kb / max_bandwidth_kbps_) * 1000;
            if (elapsed < chrono::milliseconds((int)send_time_ms)) {
                this_thread::sleep_for(
                    chrono::milliseconds((int)send_time_ms) - elapsed);
            }
        }
    }
    
    LOG_INFO("Network thread stopped");
}
```

### Détails importants

**Tâches :**
1. Lit depuis `outgoing_queue_`
2. Broadcast via `WebSocketServer::broadcastBinary()`
3. Gère les erreurs réseau
4. Peut appliquer du throttling

**Blocage :**
- Bloque sur `outgoing_queue_.pop()` si vide
- Bloque sur `send()` si buffer TCP plein (rare)

**Performance :**
- Temps dépend du réseau : 1-20ms en LAN, 20-100ms+ en WAN
- Taille typique : 10-50 KB par frame
- Débit typique : 150-750 KB/s @ 15 FPS

**Non-bloquant :**
Ce thread utilise des sockets non-bloquantes. Si un client est lent, il ne ralentit PAS les autres grâce à des buffers d'envoi par client.

---

## 🎛️ Thread 0 : MAIN (Principal)

### Responsabilité
Gère les commandes utilisateur et la configuration du système.

### Géré par
`main()` et callbacks

### Pseudo-code
```cpp
int main() {
    // 1. Initialisation
    FrameController controller;
    WebSocketServer ws_server(9002);
    MessageHandler msg_handler;
    
    // 2. Configure les callbacks de commandes
    msg_handler.registerCommandHandler("start_engine", 
        [&controller](const json& payload) {
            // THREAD-SAFE : appel depuis thread WebSocket
            controller.start(/* ... */);
        });
    
    msg_handler.registerCommandHandler("stop_engine",
        [&controller](const json& payload) {
            controller.stop(); // BLOQUANT : attend arrêt threads
        });
    
    msg_handler.registerCommandHandler("set_parameter",
        [&controller](const json& payload) {
            // THREAD-SAFE : pipeline utilise mutex
            controller.getPipeline().updateFilterParameter(/* ... */);
        });
    
    // 3. Démarre le serveur WebSocket
    ws_server.start(); // Lance son propre thread
    
    // 4. Boucle principale (attend commandes)
    while (running) {
        // Peut afficher des stats, gérer signaux, etc.
        this_thread::sleep_for(chrono::seconds(1));
        
        // Affiche stats
        auto stats = controller.getStats();
        cout << "FPS: " << stats.current_fps << endl;
    }
    
    // 5. Arrêt propre
    controller.stop();
    ws_server.stop();
    
    return 0;
}
```

### Détails importants

**Tâches :**
1. Initialise tous les composants
2. Configure les handlers de commandes
3. Gère le cycle de vie global
4. Peut afficher des logs/stats
5. Gère Ctrl+C pour arrêt propre

**Thread-safety :**
- Les callbacks sont appelés depuis le thread WebSocket
- Tous les appels à `FrameController` doivent être thread-safe
- `FramePipeline` utilise des mutex pour les modifications

---

## 📡 Thread WS : WebSocket Server

### Responsabilité
Gère les connexions clients et reçoit les commandes/frames.

### Géré par
`WebSocketpp` (bibliothèque externe)

### Comportement
```cpp
// Ce thread est géré automatiquement par WebSocketpp
void WebSocketServer::start() {
    server_.listen(port_);
    server_.start_accept();
    
    // Lance le thread de traitement des événements
    server_thread_ = thread([this]() {
        server_.run(); // Boucle événementielle
    });
}

// Callbacks (appelés depuis thread WebSocket)
void WebSocketServer::onOpen(connection_hdl hdl) {
    lock_guard<mutex> lock(connections_mutex_);
    connections_.insert(hdl);
    
    if (connection_callback_) {
        connection_callback_(hdl); // Thread-safe callback
    }
}

void WebSocketServer::onMessage(connection_hdl hdl, message_ptr msg) {
    if (msg->get_opcode() == websocketpp::frame::opcode::binary) {
        // Frame binaire = données webcam client
        const string& payload = msg->get_payload();
        vector<uint8_t> data(payload.begin(), payload.end());
        
        if (binary_callback_) {
            binary_callback_(hdl, data); // → NetworkSource::pushFrame()
        }
    } else {
        // Message texte = commande JSON
        if (message_callback_) {
            message_callback_(hdl, msg->get_payload()); // → MessageHandler
        }
    }
}
```

### Détails importants

**Tâches :**
1. Accepte connexions clients
2. Reçoit messages JSON (commandes)
3. Reçoit messages binaires (frames webcam client)
4. Envoie messages binaires (résultats)

**Thread-safety :**
- Les callbacks sont appelés depuis CE thread
- Tous les accès aux structures partagées doivent être protégés par mutex
- `broadcastBinary()` utilise un mutex sur `connections_`

---

## 🔄 Communication Inter-Threads

### ThreadSafeQueue - Implémentation

```cpp
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue(size_t max_capacity) 
        : max_capacity_(max_capacity), closed_(false) {}
    
    // BLOQUANT : attend si pleine
    bool push(T item, chrono::milliseconds timeout = chrono::milliseconds(0)) {
        unique_lock<mutex> lock(mutex_);
        
        if (closed_) return false;
        
        // Attend que la queue ne soit pas pleine
        if (max_capacity_ > 0) {
            if (timeout.count() > 0) {
                if (!cv_not_full_.wait_for(lock, timeout, [this]() {
                    return queue_.size() < max_capacity_ || closed_;
                })) {
                    return false; // Timeout
                }
            } else {
                cv_not_full_.wait(lock, [this]() {
                    return queue_.size() < max_capacity_ || closed_;
                });
            }
        }
        
        if (closed_) return false;
        
        queue_.push(move(item));
        cv_not_empty_.notify_one(); // Réveille un thread en attente
        return true;
    }
    
    // BLOQUANT : attend si vide
    bool pop(T& item, chrono::milliseconds timeout = chrono::milliseconds(0)) {
        unique_lock<mutex> lock(mutex_);
        
        // Attend qu'un élément soit disponible
        if (timeout.count() > 0) {
            if (!cv_not_empty_.wait_for(lock, timeout, [this]() {
                return !queue_.empty() || closed_;
            })) {
                return false; // Timeout
            }
        } else {
            cv_not_empty_.wait(lock, [this]() {
                return !queue_.empty() || closed_;
            });
        }
        
        if (queue_.empty()) return false; // Fermée et vide
        
        item = move(queue_.front());
        queue_.pop();
        cv_not_full_.notify_one(); // Réveille un thread en attente
        return true;
    }
    
    void close() {
        lock_guard<mutex> lock(mutex_);
        closed_ = true;
        cv_not_empty_.notify_all(); // Réveille TOUS les threads
        cv_not_full_.notify_all();
    }
    
private:
    queue<T> queue_;
    size_t max_capacity_;
    atomic<bool> closed_;
    mutable mutex mutex_;
    condition_variable cv_not_empty_;
    condition_variable cv_not_full_;
};
```

### Principe du Backpressure

```
Thread Capture rapide (60 FPS)
         ↓
    [Queue: 2 slots]  ← Si pleine, Capture BLOQUE
         ↓
Thread Processing lent (20 FPS)
         ↓
    [Queue: 2 slots]  ← Si pleine, Processing BLOQUE
         ↓
Thread Encoding rapide (50 FPS)
         ↓
    [Queue: 3 slots]  ← Si pleine, Encoding BLOQUE
         ↓
Thread Network lent (15 FPS)

Résultat : Le système s'adapte automatiquement au maillon le plus lent
sans mutex dans le code métier !
```

---

## 📊 Diagramme de Séquence Complet

```
TEMPS →

Frame N arrives

CAPTURE         PROCESSING       ENCODING         NETWORK
   │                │                │                │
   │ readFrame()    │                │                │
   │────┐           │                │                │
   │    │ 10ms      │                │                │
   │◄───┘           │                │                │
   │                │                │                │
   │ queue.push()   │                │                │
   │───────────────→│                │                │
   │                │ queue.pop()    │                │
   │                │────┐           │                │
   │                │    │ 1ms       │                │
   │                │◄───┘           │                │
   │                │                │                │
   │                │ pipeline.      │                │
   │                │  process()     │                │
   │                │────┐           │                │
   │                │    │ 25ms      │                │
   │                │◄───┘           │                │
   │                │                │                │
   │                │ queue.push()   │                │
   │                │───────────────→│                │
   │                │                │ queue.pop()    │
   │                │                │────┐           │
   │                │                │    │ 1ms       │
   │                │                │◄───┘           │
   │                │                │                │
   │                │                │ imencode()     │
   │                │                │────┐           │
   │                │                │    │ 12ms      │
   │                │                │◄───┘           │
   │                │                │                │
   │                │                │ queue.push()   │
   │                │                │───────────────→│
   │                │                │                │ queue.pop()
   │                │                │                │────┐
   │                │                │                │    │ 1ms
   │                │                │                │◄───┘
   │                │                │                │
   │                │                │                │ broadcast()
   │                │                │                │────┐
   │                │                │                │    │ 8ms
   │                │                │                │◄───┘
   │                │                │                │
   │                │                │                │ (Frame sent
   │                │                │                │  to clients)
   │                │                │                │
   ↓                ↓                ↓                ↓

Total latency: 10 + 25 + 12 + 8 = 55ms
```

---

## ⚙️ Gestion du Cycle de Vie

### Démarrage (start)

```cpp
void FrameController::start(unique_ptr<VideoSource> source, double target_fps) {
    if (running_) throw runtime_error("Already running");
    
    // 1. Initialise la source
    source_ = move(source);
    if (!source_->open()) {
        throw runtime_error("Cannot open video source");
    }
    
    // 2. Prépare les queues
    raw_frame_queue_ = ThreadSafeQueue<cv::Mat>(2);
    processed_frame_queue_ = ThreadSafeQueue<cv::Mat>(2);
    outgoing_queue_ = ThreadSafeQueue<vector<uint8_t>>(3);
    
    // 3. Flag de run
    running_ = true;
    
    // 4. Lance les threads DANS L'ORDRE
    capture_thread_ = thread(&FrameController::captureThreadFunc, this);
    processing_thread_ = thread(&FrameController::processingThreadFunc, this);
    encoding_thread_ = thread(&FrameController::encodingThreadFunc, this);
    network_thread_ = thread(&FrameController::networkThreadFunc, this);
    
    LOG_INFO("FrameController started with 4 worker threads");
}
```

### Arrêt (stop)

```cpp
void FrameController::stop() {
    if (!running_) return;
    
    LOG_INFO("Stopping FrameController...");
    
    // 1. Flag d'arrêt
    running_ = false;
    
    // 2. Ferme les queues (débloque tous les threads)
    raw_frame_queue_.close();
    processed_frame_queue_.close();
    outgoing_queue_.close();
    
    // 3. Attend la fin de TOUS les threads (ORDRE IMPORTANT)
    if (capture_thread_.joinable()) {
        capture_thread_.join();
        LOG_INFO("Capture thread joined");
    }
    
    if (processing_thread_.joinable()) {
        processing_thread_.join();
        LOG_INFO("Processing thread joined");
    }
    
    if (encoding_thread_.joinable()) {
        encoding_thread_.join();
        LOG_INFO("Encoding thread joined");
    }
    
    if (network_thread_.joinable()) {
        network_thread_.join();
        LOG_INFO("Network thread joined");
    }
    
    // 4. Ferme la source
    if (source_) {
        source_->close();
    }
    
    LOG_INFO("FrameController stopped cleanly");
}
```

**Ordre crucial :**
1. `running_ = false` → signale à tous
2. Ferme les queues → débloque les `wait()`
3. Join dans l'ordre : capture → processing → encoding → network
4. Ferme la source

---

## 🎛️ Configuration Thread-Safe

### Modification du Pipeline en temps réel

```cpp
void FramePipeline::updateFilterParameter(
    const string& filter_name,
    const string& param_name,
    const json& value
) {
    // MUTEX : protège contre modifications concurrentes
    lock_guard<mutex> lock(filters_mutex_);
    
    for (auto& filter : filters_) {
        if (filter->getName() == filter_name) {
            filter->setParameter(param_name, value);
            LOG_INFO("Updated " + filter_name + "." + param_name);
            return;
        }
    }
    
    throw runtime_error("Filter not found: " + filter_name);
}

void FramePipeline::process(const cv::Mat& input, cv::Mat& output) {
    // SHARED_LOCK : permet lectures multiples simultanées
    shared_lock<shared_mutex> lock(filters_mutex_);
    
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
```

**Avantage `shared_mutex` :**
- Plusieurs threads peuvent `process()` en même temps
- Un seul thread peut `updateFilterParameter()` à la fois
- Si un update est en cours, `process()` attend

---

## 📈 Métriques de Performance

### Collecte Thread-Safe

```cpp
class PerformanceMonitor {
public:
    void recordDuration(const string& metric, chrono::milliseconds duration) {
        lock_guard<mutex> lock(metrics_mutex_);
        
        auto& data = metrics_[metric];
        double value_ms = duration.count();
        
        data.current = value_ms;
        data.sum += value_ms;
        data.count++;
        data.min = min(data.min, value_ms);
        data.max = max(data.max, value_ms);
    }
    
    PerformanceStats getStats(const string& metric) const {
        lock_guard<mutex> lock(metrics_mutex_);
        
        auto it = metrics_.find(metric);
        if (it == metrics_.end()) return {};
        
        auto& data = it->second;
        return {
            .min_value = data.min,
            .max_value = data.max,
            .avg_value = data.sum / data.count,
            .current_value = data.current,
            .sample_count = data.count
        };
    }
};
```

### Exemple d'affichage

```
=== Performance Stats ===
capture_time:    min=8ms  avg=10ms  max=15ms  current=9ms  (samples=1247)
processing_time: min=20ms avg=25ms  max=80ms  current=23ms (samples=1247)
encoding_time:   min=10ms avg=12ms  max=18ms  current=11ms (samples=1247)
network_time:    min=5ms  avg=8ms   max=25ms  current=7ms  (samples=1247)
total_latency:   55ms (capture→network)
FPS:             28.5 (target: 30)
dropped_frames:  3 (0.24%)
```

---

## 🚀 Optimisations Possibles

### 1. Thread Pool pour Processing

Au lieu d'un seul thread processing, utiliser un pool :

```cpp
// Thread Pool pour traiter plusieurs frames en parallèle
ThreadPool processing_pool(4); // 4 workers

// Dans capture thread
processing_pool.enqueue([frame]() {
    cv::Mat processed;
    pipeline->process(frame, processed);
    processed_frame_queue_.push(processed);
});
```

**Avantage :** Parallélisation du traitement si plusieurs frames peuvent être traitées simultanément.

### 2. Lock-Free Queues

Remplacer `ThreadSafeQueue` par des queues lock-free (boost::lockfree) :

```cpp
#include <boost/lockfree/spsc_queue.hpp>

// Single Producer Single Consumer = zero contention
boost::lockfree::spsc_queue<cv::Mat, capacity<2>> raw_frame_queue_;
```

**Avantage :** Performances accrues (pas de mutex), mais complexité++.

### 3. Zero-Copy avec Shared Pointers

```cpp
using FramePtr = shared_ptr<cv::Mat>;

ThreadSafeQueue<FramePtr> raw_frame_queue_;

// Capture thread
auto frame = make_shared<cv::Mat>();
source_->readFrame(*frame);
raw_frame_queue_.push(frame); // Copie juste le shared_ptr, pas la Mat
```

**Avantage :** Évite les copies de cv::Mat (économie CPU + mémoire).

---

## ✅ Résumé

| Thread | Rôle | Input | Output | Temps Typique | Blocage |
|--------|------|-------|--------|---------------|---------|
| **Capture** | Lit source vidéo | VideoSource | cv::Mat raw | 5-15ms | `readFrame()`, `queue.push()` |
| **Processing** | Applique filtres | cv::Mat raw | cv::Mat processed | 5-200ms | `queue.pop()`, `queue.push()` |
| **Encoding** | Compresse JPEG | cv::Mat processed | vector<uint8_t> | 5-15ms | `queue.pop()`, `queue.push()` |
| **Network** | Envoie clients | vector<uint8_t> | - | 1-20ms | `queue.pop()`, `send()` |
| **Main** | Commandes | User input | Config | - | Callbacks |
| **WebSocket** | Réseau entrant | Network | Messages | - | I/O réseau |

**Total latency (capture→display) :** 50-150ms selon pipeline et réseau.

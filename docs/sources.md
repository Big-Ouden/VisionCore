# AsciiVision - Ressources Complètes pour le Développement

## 📚 Documentation Officielle

### C++ & STL
- **cppreference.com** : https://en.cppreference.com/
  - Reference C++20 complète (threads, atomics, chrono, smart pointers)
  - Exemples de code pour chaque feature
  - **Sections clés** :
    - Thread support library : https://en.cppreference.com/w/cpp/thread
    - Atomic operations : https://en.cppreference.com/w/cpp/atomic
    - Smart pointers : https://en.cppreference.com/w/cpp/memory
    - Chrono : https://en.cppreference.com/w/cpp/chrono

- **ISO C++ Guidelines** : https://isocpp.github.io/CppCoreGuidelines/
  - Best practices C++ moderne
  - RAII, ownership, thread safety

### OpenCV
- **Documentation officielle** : https://docs.opencv.org/4.x/
  - API complète avec exemples
  - **Sections essentielles** :
    - Core functionality : https://docs.opencv.org/4.x/d0/de1/group__core.html
    - Image Processing : https://docs.opencv.org/4.x/d7/dbd/group__imgproc.html
    - Video I/O : https://docs.opencv.org/4.x/dd/de7/group__videoio.html
    - Object Detection : https://docs.opencv.org/4.x/d5/d54/group__objdetect.html

- **OpenCV Tutorials** : https://docs.opencv.org/4.x/d9/df8/tutorial_root.html
  - Tutoriels pas à pas pour débutants
  - **Recommandés** :
    - Core module : https://docs.opencv.org/4.x/de/d7a/tutorial_table_of_content_core.html
    - Image processing : https://docs.opencv.org/4.x/d7/da8/tutorial_table_of_content_imgproc.html
    - Video I/O : https://docs.opencv.org/4.x/dd/de1/tutorial_js_video_display.html

- **OpenCV Python Tutorials** (transposable en C++) : https://docs.opencv.org/4.x/d6/d00/tutorial_py_root.html
  - Plus d'exemples, souvent plus clairs

### WebSocket (WebSocketpp)
- **GitHub + Wiki** : https://github.com/zaphoyd/websocketpp
  - Documentation complète dans le wiki
  - **Sections importantes** :
    - Getting Started : https://github.com/zaphoyd/websocketpp/wiki
    - Examples : https://github.com/zaphoyd/websocketpp/tree/master/examples
    - Server Tutorial : https://github.com/zaphoyd/websocketpp/wiki/Server-Setup
    - Binary Messages : https://github.com/zaphoyd/websocketpp/wiki/Binary-Messages

- **WebSocket Protocol RFC** : https://datatracker.ietf.org/doc/html/rfc6455
  - Spécification complète du protocole (pour comprendre les détails)

### JSON (nlohmann/json)
- **GitHub** : https://github.com/nlohmann/json
  - Documentation très complète avec exemples
  - **Sections clés** :
    - Basic usage : https://json.nlohmann.me/home/quickstart/
    - API reference : https://json.nlohmann.me/api/basic_json/
    - Conversion : https://json.nlohmann.me/features/arbitrary_types/

### CMake
- **Documentation officielle** : https://cmake.org/cmake/help/latest/
  - Reference complète
  - **Sections utiles** :
    - Tutorial : https://cmake.org/cmake/help/latest/guide/tutorial/index.html
    - Modern CMake : https://cliutils.gitlab.io/modern-cmake/
    - FindPackage : https://cmake.org/cmake/help/latest/command/find_package.html

---

## 🎓 Tutoriels & Guides

### Threading & Concurrency

- **C++ Concurrency in Action (Book)** : https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition
  - Bible du multithreading C++
  - Chapitres sur mutex, condition_variable, atomics, thread-safe queues

- **ThreadSafeQueue Implementation Guide** : https://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
  - Implémentation détaillée d'une queue thread-safe
  - Explications sur condition_variable

- **Lock-Free Programming** : https://preshing.com/20120612/an-introduction-to-lock-free-programming/
  - Introduction aux algorithmes lock-free
  - Atomics et memory ordering

- **Thread Pool Pattern** : https://www.youtube.com/watch?v=eWTGtp3HXiw
  - Vidéo CppCon sur les thread pools
  - Implémentation moderne C++

### Computer Vision

- **PyImageSearch** : https://pyimagesearch.com/
  - Blog excellent sur OpenCV (Python mais transposable)
  - **Articles utiles** :
    - Face detection : https://pyimagesearch.com/2021/04/05/opencv-face-detection-with-haar-cascades/
    - Edge detection : https://pyimagesearch.com/2021/05/12/opencv-edge-detection-cv2-canny/
    - Video streaming : https://pyimagesearch.com/2019/09/02/opencv-stream-video-to-web-browser-html-page/

- **LearnOpenCV** : https://learnopencv.com/
  - Tutoriels avancés OpenCV
  - **Articles recommandés** :
    - Image filtering : https://learnopencv.com/image-filtering-using-opencv/
    - Video I/O : https://learnopencv.com/reading-and-writing-videos-using-opencv/
    - Face detection : https://learnopencv.com/face-detection-opencv-dlib-and-deep-learning-c-python/

- **OpenCV Tutorial C++** : https://www.opencv-srf.com/p/introduction.html
  - Tutoriels spécifiques C++
  - Code complet pour chaque exemple

### WebSocket & Real-Time Communication

- **WebSocket in C++** : https://www.boost.org/doc/libs/1_84_0/libs/beast/doc/html/beast/using_websocket.html
  - Alternative : Boost.Beast (plus moderne que WebSocketpp)

- **Real-Time Video Streaming** : https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API
  - MDN guide sur WebSocket (côté client JavaScript)

- **Binary Data over WebSocket** : https://javascript.info/websocket#binary-data
  - Comment gérer les données binaires

### Architecture & Design Patterns

- **SOLID Principles** : https://www.digitalocean.com/community/conceptual-articles/s-o-l-i-d-the-first-five-principles-of-object-oriented-design
  - Principes de conception objet
  - Appliqués à notre architecture (IFilter, VideoSource, etc.)

- **Producer-Consumer Pattern** : https://www.geeksforgeeks.org/producer-consumer-problem-in-cpp/
  - Pattern utilisé pour notre système de queues

- **Pipeline Pattern** : https://java-design-patterns.com/patterns/pipeline/
  - Pattern de notre FramePipeline

---

## 📖 Livres Recommandés

### C++ Moderne
1. **"Effective Modern C++"** - Scott Meyers
   - https://www.oreilly.com/library/view/effective-modern-c/9781491908419/
   - Best practices C++11/14 (applicable à C++20)

2. **"C++ Concurrency in Action"** - Anthony Williams
   - https://www.manning.com/books/c-plus-plus-concurrency-in-action-second-edition
   - Threading, atomics, memory model

3. **"The C++ Programming Language"** - Bjarne Stroustrup
   - https://www.stroustrup.com/4th.html
   - Reference par le créateur de C++

### Computer Vision
4. **"Learning OpenCV 4"** - Adrian Kaehler & Gary Bradski
   - https://www.oreilly.com/library/view/learning-opencv-4/9781789533576/
   - Guide complet OpenCV 4

5. **"Computer Vision: Algorithms and Applications"** - Richard Szeliski
   - http://szeliski.org/Book/
   - Théorie + pratique (gratuit en ligne)

### Architecture Logicielle
6. **"Clean Architecture"** - Robert C. Martin
   - https://www.oreilly.com/library/view/clean-architecture-a/9780134494272/
   - Principes d'architecture modulaire

---

## 🎥 Vidéos & Conférences

### CppCon (Conférences C++)
- **CppCon Channel** : https://www.youtube.com/@CppCon
  - Conférences annuelles des experts C++
  - **Vidéos essentielles** :
    - "C++ Threading" : https://www.youtube.com/watch?v=F6Ipn7gCOsY
    - "Lock-Free Programming" : https://www.youtube.com/watch?v=c1gO9aB9nbs
    - "Modern CMake" : https://www.youtube.com/watch?v=eC9-iRN2b04

### Computer Vision
- **OpenCV Official Channel** : https://www.youtube.com/@OpenCV
  - Tutoriels vidéo officiels

- **PyImageSearch YouTube** : https://www.youtube.com/@PyImageSearch
  - Tutoriels pratiques (Python mais concepts transposables)

### Architecture
- **The Cherno (C++ Series)** : https://www.youtube.com/@TheCherno
  - Série complète sur C++ moderne
  - **Playlist** : https://www.youtube.com/playlist?list=PLlrATfBNZ98dudnM48yfGUldqGD0S4FFb

---

## 🛠️ Outils & Debugging

### Compilateurs & Environnements
- **Compiler Explorer (Godbolt)** : https://godbolt.org/
  - Teste du code C++ en ligne, vois l'assembleur généré
  - Supporte GCC, Clang, MSVC

- **Quick C++ Benchmark** : https://quick-bench.com/
  - Benchmarking en ligne pour comparer performances

### Debugging & Profiling
- **Valgrind** : https://valgrind.org/docs/manual/quick-start.html
  - Détection fuites mémoire
  - Detection data races (helgrind)

- **GDB Tutorial** : https://www.gdbtutorial.com/
  - Debugger ligne de commande

- **ThreadSanitizer** : https://github.com/google/sanitizers/wiki/ThreadSanitizerCppManual
  - Détection automatique de data races
  - Intégré dans GCC/Clang avec `-fsanitize=thread`

- **Perf (Linux)** : https://perf.wiki.kernel.org/index.php/Tutorial
  - Profiling performances CPU

### Memory & Performance
- **Cachegrind** : https://valgrind.org/docs/manual/cg-manual.html
  - Analyse cache CPU et performance mémoire

- **Heaptrack** : https://github.com/KDE/heaptrack
  - Analyse allocations mémoire

---

## 💻 Exemples de Code & Projets

### Projets Open Source Similaires

1. **FFmpeg** : https://github.com/FFmpeg/FFmpeg
   - Traitement vidéo en C
   - Architecture pipeline similaire

2. **GStreamer** : https://gstreamer.freedesktop.org/
   - Pipeline vidéo modulaire
   - Inspirant pour architecture filtres

3. **OBS Studio** : https://github.com/obsproject/obs-studio
   - Streaming vidéo temps réel
   - Gestion sources multiples

4. **Motion** : https://github.com/Motion-Project/motion
   - Détection mouvement avec webcam
   - Code C++ simple et clair

### Snippets & Examples

- **OpenCV Examples** : https://github.com/opencv/opencv/tree/4.x/samples/cpp
  - Exemples officiels C++
  - Tous les modules couverts

- **WebSocketpp Examples** : https://github.com/zaphoyd/websocketpp/tree/master/examples
  - Echo server, broadcast server, etc.

- **Thread-Safe Queue Examples** : https://github.com/cameron314/concurrentqueue
  - Implementation lock-free performante

---

## 🌐 Frontend (Next.js + React)

### Next.js
- **Documentation officielle** : https://nextjs.org/docs
  - Guide complet App Router (Next.js 13+)
  - **Sections clés** :
    - Getting Started : https://nextjs.org/docs/getting-started
    - App Router : https://nextjs.org/docs/app
    - API Routes : https://nextjs.org/docs/app/building-your-application/routing/route-handlers

### React
- **Documentation officielle** : https://react.dev/
  - Nouvelle doc avec hooks
  - **Sections essentielles** :
    - Hooks : https://react.dev/reference/react/hooks
    - useEffect : https://react.dev/reference/react/useEffect
    - useRef : https://react.dev/reference/react/useRef

### TypeScript
- **Documentation officielle** : https://www.typescriptlang.org/docs/
  - **Sections importantes** :
    - Handbook : https://www.typescriptlang.org/docs/handbook/intro.html
    - React + TypeScript : https://react-typescript-cheatsheet.netlify.app/

### WebSocket côté client
- **MDN WebSocket API** : https://developer.mozilla.org/en-US/docs/Web/API/WebSocket
  - Documentation complète API JavaScript

- **Canvas API** : https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API
  - Pour afficher les frames vidéo
  - **Tutoriel** : https://developer.mozilla.org/en-US/docs/Web/API/Canvas_API/Tutorial

### getUserMedia (Webcam)
- **MDN getUserMedia** : https://developer.mozilla.org/en-US/docs/Web/API/MediaDevices/getUserMedia
  - Accès webcam dans le navigateur

- **WebRTC samples** : https://webrtc.github.io/samples/
  - Exemples de capture vidéo

---

## 📝 Blogs & Articles Spécialisés

### C++ & Performance
- **Preshing on Programming** : https://preshing.com/
  - Articles sur lock-free programming, atomics, memory ordering

- **Sutter's Mill** : https://herbsutter.com/
  - Blog de Herb Sutter (expert C++)

- **Modernes C++** : https://www.modernescpp.com/
  - Articles sur C++ moderne

### Computer Vision
- **AI Shack** : https://aishack.in/
  - Tutoriels computer vision en détail

- **LearnOpenCV Blog** : https://learnopencv.com/blog/
  - Articles hebdomadaires OpenCV

### Architecture & Design
- **Martin Fowler** : https://martinfowler.com/
  - Patterns, architecture, refactoring

---

## 🔧 Stack Overflow & Forums

### Questions/Réponses
- **Stack Overflow C++ Tag** : https://stackoverflow.com/questions/tagged/c%2b%2b
  - Recherche par mots-clés : "thread safe queue", "opencv video capture", etc.

- **Stack Overflow OpenCV** : https://stackoverflow.com/questions/tagged/opencv

- **Reddit r/cpp** : https://www.reddit.com/r/cpp/
  - Communauté active C++

- **Reddit r/computervision** : https://www.reddit.com/r/computervision/
  - Discussions computer vision

### Forums spécialisés
- **OpenCV Forum** : https://forum.opencv.org/
  - Forum officiel OpenCV

- **CPP Slack** : https://cpplang.slack.com/
  - Communauté Slack C++

---

## 📊 Cheat Sheets & Quick References

### C++
- **C++ Reference Card** : https://hackingcpp.com/cpp/cheat_sheets.html
  - Cheat sheets visuelles C++

- **Modern C++ Cheatsheet** : https://github.com/AnthonyCalandra/modern-cpp-features
  - Features C++11/14/17/20

### OpenCV
- **OpenCV Cheat Sheet** : https://docs.opencv.org/4.x/d7/da9/tutorial_template_matching.html
  - Functions les plus utilisées

### CMake
- **CMake Cheatsheet** : https://usercontent.one/wp/cheatsheet.czutro.ch/wp-content/uploads/2020/09/CMake_Cheatsheet.pdf

---

## 🎯 Roadmap d'Apprentissage Recommandée

### Phase 1 : Bases (1-2 semaines)
1. **C++ moderne** (si besoin de refresh)
   - cppreference.com : std::thread, std::mutex, std::atomic
   - Smart pointers (unique_ptr, shared_ptr)
   - RAII pattern

2. **OpenCV bases**
   - Tutorials : Core module
   - VideoCapture, Mat, imencode/imdecode
   - Basic filters (resize, cvtColor, blur)

3. **CMake**
   - Tutorial officiel
   - find_package, target_link_libraries

### Phase 2 : Threading (1 semaine)
1. **Concurrency**
   - "C++ Concurrency in Action" chapitres 1-4
   - Implémente ta ThreadSafeQueue
   - Tests avec plusieurs producteurs/consommateurs

2. **Condition Variables**
   - cppreference.com : std::condition_variable
   - Pattern producer-consumer

### Phase 3 : Réseau (1 semaine)
1. **WebSocket**
   - WebSocketpp wiki + examples
   - Echo server → Broadcast server → Binary messages

2. **Protocole**
   - JSON parsing avec nlohmann/json
   - Design ton protocole (message types, validation)

### Phase 4 : Computer Vision (2 semaines)
1. **Filtres basiques**
   - OpenCV tutorials : Image Processing
   - Implémente : Grayscale, Resize, Blur, EdgeDetection

2. **Détection objets**
   - Haar Cascades (face detection)
   - LearnOpenCV tutorials

### Phase 5 : Intégration (2 semaines)
1. **Backend complet**
   - Assemble tous les composants
   - Tests end-to-end

2. **Frontend**
   - Next.js getting started
   - WebSocket client + Canvas rendering

### Phase 6 : Optimisation (ongoing)
1. **Profiling**
   - Valgrind, perf, gprof
   - Identifie bottlenecks

2. **Améliorations**
   - Lock-free queues
   - Zero-copy avec shared_ptr
   - GPU acceleration (CUDA/OpenCL)

---

## 🆘 Aide & Support

### Quand tu es bloqué
1. **cppreference.com** → Cherche la fonction/classe
2. **Stack Overflow** → Copie ton erreur exacte
3. **GitHub Issues** → Cherche dans les issues OpenCV/WebSocketpp
4. **Reddit r/cpp_questions** → Pose ta question

### Debugging
1. **Compile avec sanitizers** : `-fsanitize=thread -fsanitize=address`
2. **Valgrind** pour memory leaks
3. **GDB** pour crashes
4. **LOG_DEBUG** partout au début

---

## 📦 Ressources Supplémentaires

### Datasets pour Tests
- **Sample Videos** : https://sample-videos.com/
- **Webcam Test** : https://webcamtests.com/
- **Face Images** : http://vis-www.cs.umass.edu/lfw/

### Icons & UI
- **Lucide Icons** : https://lucide.dev/ (pour React)
- **Heroicons** : https://heroicons.com/

### Deployment
- **Docker** : https://docs.docker.com/get-started/
  - Pour containeriser ton backend C++
- **Vercel** : https://vercel.com/docs (pour frontend Next.js)

---

## ✅ Checklist de Démarrage

Avant de coder, assure-toi d'avoir :

- [ ] Compilateur C++20 (GCC 10+, Clang 11+, MSVC 2019+)
- [ ] CMake 3.15+
- [ ] OpenCV 4.x installé
- [ ] WebSocketpp installé (header-only)
- [ ] nlohmann/json installé (header-only)
- [ ] Node.js 18+ (pour frontend)
- [ ] Git configuré
- [ ] IDE configuré (VSCode/CLion/Visual Studio)

**Commandes de vérification :**
```bash
g++ --version       # GCC 10+ ou Clang 11+
cmake --version     # 3.15+
pkg-config --modversion opencv4  # 4.x
node --version      # 18+
npm --version       # 9+
```

---

**Bon courage pour le développement ! 🚀**

Si tu as besoin de clarifications sur une ressource spécifique ou un concept, n'hésite pas à demander.

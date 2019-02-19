# Viridity

An application server for Qt that streams data and scenes (QGraphicsScene/QtQuick 1/2) to your web browser.

## Features

* Full declarative interface for QML and Qt Quick;
* Multi-threaded HTTP/1.1 application server:
  * Support for persistent connections (keep-alive), chunked transfer encoding and WebSocket hand-off;
  * File upload support;
  * Transparent data channel using either WebSocket, Server Sent Events (SSE) or Long-Polling with automatic fallback, proxies supported;
  * Integrated Session Manager.
* Framework to develop custom request handlers / routes:
  * Use QML + JavaScript for maximum flexibility and ease-of-use (single-threaded only);
  * Use C++ for maximum performance, multi-threading support baked in;
* Declarative HTML template system:
  * Generates both static and dynamic pages;
  * Leveraging the QML engine and Qt's Signal/Slot/Property system to do the heavy lifting;
  * Automatic change propagation to web browser via QML to JS "DataBridge":
    Changes in your QML document structure (content / bound properties) are transformed to HTML and streamed to the attached clients in real-time.
  * Write HTML components in QML: Various QML components generating HTML5 primitives are already included;
  * High-performance caching system for static and dynamic pages;
* Embed Qt Quick scenes with a view lines of code:
  * Rendered on the server, streamed to the browser via a technique similar to VNC or RDP, fully multi-threaded;
  * Browser-side code to render the streamed scene to a canvas;
  * Allows full user interaction with the scene;
  * Sharing between multiple session possible.
* Server is optimized for use in embedded projects;
* Designed with low-power, low-performance devices in mind (Raspberry Pi Zero and up, old Symbian devices, etc.);
* Support for running the application server in Docker containers (check examples/qmlwebviewer/makedistro-docker.sh)

## TODO

* HTTPS/TLS or HTTP/2 support - For now use a reverse proxy configuration (HAProxy, Apache or nginx)

## Requirements

* Qt 4.8 or Qt 5.0 and up
* Linux, macOS, Windows (with Cygwin set up to execute bash scripts, WSL currently untested)

## Installation

Viridity requires the KCL ([Katastrophos.net Component Library](https://github.com/evilJazz/kcl)), which can be bootstrapped via the command:

    ./bootstrap.sh

This library is developed to be statically linked to the resulting binary. You can easily include it in your applications .pro file:

    contains(QT_VERSION, ^4\\..*): CONFIG += viridity_declarative viridity_qtquick1
    contains(QT_VERSION, ^5\\..*): CONFIG += viridity_declarative viridity_qtquick2

    include($$PWD/viridity/viridity-static.pri)

For more details, please refer to the included projects in the "examples" and "tests" sub-directories.

## Example projects / demos

* examples/qmlwebviewer/qmlwebviewer.pro
  
  An example project showcasing
  * how to stream QtQuick scenes from the Viridity application server to the web browser
  * how the client-side code is displaying the scene in a canvas element within the web browser and how it handles user interaction.

* examples/reactive/reactive.pro

  An example project showcasing a reactive use-case, namely
  * how to create HTML content with Viridity,
  * how to publish a statically generated (and cached) version of the HTML document and
  * how to handle interaction and stream dynamic updates of the content to the web browser once the JavaScript parts are loaded by the web browser.

## License

Please refer to the file "LICENSE" and "LICENSE-agpl-3.0-txt" for all details.

If you wish to use and distribute the Viridity library in your commercial product without making your sourcecode available to the public, please contact us for a commercial license at info@meteorasoftworks.com
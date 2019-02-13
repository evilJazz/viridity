# Viridity
An application server for Qt that streams data and scenes (QGraphicsScene/QtQuick 1/2) to your web browser.

 * Optimized for use in embedded projects;
 * Designed with low-power, low-performance devices in mind (Raspberry Pi Zero and up, old Symbian devices, etc.);
 * Support for running the application server in Docker containers (check examples/qmlwebviewer/makedistro-docker.sh)

### Requirements:

 * Qt 4.8 or Qt 5.0 and up
 * Linux, macOS, Windows (with Cygwin set up to execute bash scripts, WSL currently untested)

### Installation:

Viridity requires the KCL ([Katastrophos.net Component Library](https://github.com/evilJazz/kcl)), which can be bootstrapped via the command:

    ./bootstrap.sh

This library is developed to be statically linked to the resulting binary. You can easily include it in your applications .pro file:

    contains(QT_VERSION, ^4\\..*): CONFIG += viridity_declarative viridity_qtquick1
    contains(QT_VERSION, ^5\\..*): CONFIG += viridity_declarative viridity_qtquick2

    include($$PWD/viridity/viridity-static.pri)

For more details, please refer to the included projects in the "examples" and "tests" sub-directories.

## Example projects / demos:

 - examples/qmlwebviewer/qmlwebviewer.pro
   
   An example project showcasing 
   * how to stream QtQuick scenes from the Viridity application server to the web browser
   * how the client-side code is displaying the scene in a canvas element within the web browser and how it handles user interaction.

 - examples/reactive/reactive.pro

   An example project showcasing a reactive use-case, namely
   * how to create HTML content with Viridity,
   * how to publish a statically generated (and cached) version of the HTML document and
   * how to handle interaction and stream dynamic updates of the content to the web browser once the JavaScript parts are loaded by the web browser.


#
## License

Please refer to the file "LICENSE" and "LICENSE-agpl-3.0-txt" for all details.

If you wish to use and distribute the Viridity library in your commercial product without making your sourcecode available to the public, please contact us for a commercial license at info@meteorasoftworks.com
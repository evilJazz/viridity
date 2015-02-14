include($$PWD/tufao-min.pri)

SOURCES += \
    $$TUFAO_SRC_PATH/httpserver.cpp \
    $$TUFAO_SRC_PATH/httpsserver.cpp \
    $$TUFAO_SRC_PATH/priv/tcpserverwrapper.cpp \
    $$TUFAO_SRC_PATH/httpfileserver.cpp \
    $$TUFAO_SRC_PATH/abstracthttpserverrequesthandler.cpp \
    $$TUFAO_SRC_PATH/httpserverrequestrouter.cpp \
    $$TUFAO_SRC_PATH/httppluginserver.cpp

HEADERS += \
    $$TUFAO_SRC_PATH/httpserver.h \
    $$TUFAO_SRC_PATH/httpsserver.h \
    $$TUFAO_SRC_PATH/priv/tcpserverwrapper.h \
    $$TUFAO_SRC_PATH/priv/httpserver.h \
    $$TUFAO_SRC_PATH/priv/httpsserver.h \
    $$TUFAO_SRC_PATH/httpfileserver.h \
    $$TUFAO_SRC_PATH/abstracthttpserverrequesthandler.h \
    $$TUFAO_SRC_PATH/httpserverrequestrouter.h \
    $$TUFAO_SRC_PATH/httppluginserver.h \
    $$TUFAO_SRC_PATH/priv/httpserverrequestrouter.h \
    $$TUFAO_SRC_PATH/priv/httppluginserver.h \
    $$TUFAO_SRC_PATH/priv/httpfileserver.h \
    $$TUFAO_SRC_PATH/abstracthttpserverrequesthandlerfactory.h

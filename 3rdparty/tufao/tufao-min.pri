TUFAO_SRC_PATH = $$PWD/src
TUFAO_INCLUDE_PATH = $$PWD/include

INCLUDEPATH += \
    $$TUFAO_INCLUDE_PATH \
    $$TUFAO_SRC_PATH

SOURCES += \
    $$TUFAO_SRC_PATH/httpserverrequest.cpp \
    $$TUFAO_SRC_PATH/httpserverresponse.cpp \
    $$TUFAO_SRC_PATH/url.cpp \
    $$TUFAO_SRC_PATH/querystring.cpp \
    $$TUFAO_SRC_PATH/priv/reasonphrase.cpp \
    $$TUFAO_SRC_PATH/priv/http_parser.c \
    $$TUFAO_SRC_PATH/websocket.cpp \
    $$TUFAO_SRC_PATH/abstractmessagesocket.cpp \
    $$TUFAO_SRC_PATH/headers.cpp \
    $$TUFAO_SRC_PATH/priv/rfc1123.cpp \
    $$TUFAO_SRC_PATH/priv/rfc1036.cpp \
    $$TUFAO_SRC_PATH/priv/asctime.cpp

HEADERS += \
    $$TUFAO_SRC_PATH/tufao_global.h \
    $$TUFAO_SRC_PATH/httpserverrequest.h \
    $$TUFAO_SRC_PATH/httpserverresponse.h \
    $$TUFAO_SRC_PATH/url.h \
    $$TUFAO_SRC_PATH/querystring.h \
    $$TUFAO_SRC_PATH/priv/httpserverrequest.h \
    $$TUFAO_SRC_PATH/priv/httpserverresponse.h \
    $$TUFAO_SRC_PATH/priv/reasonphrase.h \
    $$TUFAO_SRC_PATH/priv/http_parser.h \
    $$TUFAO_SRC_PATH/priv/url.h \
    $$TUFAO_SRC_PATH/ibytearray.h \
    $$TUFAO_SRC_PATH/headers.h \
    $$TUFAO_SRC_PATH/websocket.h \
    $$TUFAO_SRC_PATH/priv/websocket.h \
    $$TUFAO_SRC_PATH/abstractmessagesocket.h \
    $$TUFAO_SRC_PATH/priv/rfc1123.h \
    $$TUFAO_SRC_PATH/priv/rfc1036.h \
    $$TUFAO_SRC_PATH/priv/asctime.h

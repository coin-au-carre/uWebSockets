#ifndef GROUP_UWS_H
#define GROUP_UWS_H

#include "WebSocket.h"
#include "HTTPSocket.h"
#include "Extensions.h"
#include <functional>
#include <stack>

namespace uWS {

struct Hub;

template <bool isServer>
struct WIN32_EXPORT Group : protected uS::NodeData {
    friend struct Hub;
    std::function<void(WebSocket<isServer>, HTTPRequest)> connectionHandler;
    std::function<void(WebSocket<isServer>, char *message, size_t length, OpCode opCode)> messageHandler;
    std::function<void(WebSocket<isServer>, int code, char *message, size_t length)> disconnectionHandler;
    std::function<void(WebSocket<isServer>, char *, size_t)> pingHandler;
    std::function<void(WebSocket<isServer>, char *, size_t)> pongHandler;

    std::function<void(HTTPSocket<isServer>)> httpConnectionHandler;
    std::function<void(HTTPSocket<isServer>, HTTPRequest, char *, size_t, size_t)> httpRequestHandler;
    std::function<void(HTTPSocket<isServer>, char *, size_t, size_t)> httpDataHandler;
    std::function<void(HTTPSocket<isServer>)> httpDisconnectionHandler;
    std::function<void(HTTPSocket<isServer>, HTTPRequest)> httpUpgradeHandler;

    using errorType = typename std::conditional<isServer, int, void *>::type;
    std::function<void(errorType)> errorHandler;

    Hub *hub;
    int extensionOptions;
    uv_timer_t *timer = nullptr;
    std::string userPingMessage;

    // todo: cannot be named user, collides with parent!
    void *userData = nullptr;
    void setUserData(void *user);
    void *getUserData();
    void startAutoPing(int intervalMs, std::string userMessage = "");
    static void timerCallback(uv_timer_t *timer);

    uv_poll_t *webSocketHead = nullptr, *httpSocketHead = nullptr;
    void addWebSocket(uv_poll_t *webSocket);
    void removeWebSocket(uv_poll_t *webSocket);

    std::stack<uv_poll_t *> iterators;

protected:
    Group(int extensionOptions, Hub *hub, uS::NodeData *nodeData);
    void stopListening();

public:
    void onConnection(std::function<void(WebSocket<isServer>, HTTPRequest)> handler);
    void onMessage(std::function<void(WebSocket<isServer>, char *, size_t, OpCode)> handler);
    void onDisconnection(std::function<void(WebSocket<isServer>, int code, char *message, size_t length)> handler);
    void onPing(std::function<void(WebSocket<isServer>, char *, size_t)> handler);
    void onPong(std::function<void(WebSocket<isServer>, char *, size_t)> handler);
    void onError(std::function<void(errorType)> handler);

    void onHttpConnection(std::function<void(HTTPSocket<isServer>)> handler);
    void onHttpRequest(std::function<void(HTTPSocket<isServer>, HTTPRequest, char *data, size_t length, size_t remainingBytes)> handler);
    void onHttpData(std::function<void(HTTPSocket<isServer>, char *data, size_t length, size_t remainingBytes)> handler);
    void onHttpDisconnection(std::function<void(HTTPSocket<isServer>)> handler);
    void onHttpUpgrade(std::function<void(HTTPSocket<isServer>, HTTPRequest)> handler);


    void broadcast(const char *message, size_t length, OpCode opCode);
    void terminate();
    void close(int code = 1000, char *message = nullptr, size_t length = 0);
    using NodeData::addAsync;

    // todo: handle nested forEachs with removeWebSocket
    template <class F>
    void forEach(const F &cb) {
        uv_poll_t *iterator = webSocketHead;
        iterators.push(iterator);
        while (iterator) {
            uv_poll_t *lastIterator = iterator;
            cb(WebSocket<isServer>(iterator));
            iterator = iterators.top();
            if (lastIterator == iterator) {
                iterator = ((uS::SocketData *) iterator->data)->next;
                iterators.top() = iterator;
            }
        }
        iterators.pop();
    }
};

}

#endif // GROUP_UWS_H

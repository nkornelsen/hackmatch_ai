#include "server.hpp"

int connect_to_wsl(WSLConnection &conn) {
    WSADATA wsa_data = {0};
    int res = 0;
    res = WSAStartup(MAKEWORD(2, 2), &wsa_data);

    if (res != 0) {
        printf("WSAStartup failed with error: %d\n", res);
        return -1;
    }

    SOCKET sock = socket(AF_HYPERV, SOCK_STREAM, HV_PROTOCOL_RAW);
    if (sock == INVALID_SOCKET) {
        printf("Failed to create socket: %d\n", WSAGetLastError());
        goto ReturnFailure;
        return -1;
    }

    SOCKADDR_HV sockinfo;
    sockinfo.Family = AF_HYPERV;
    sockinfo.Reserved = 0;
    sockinfo.VmId = __uuidof(VmGUID);
    sockinfo.ServiceId = HV_GUID_VSOCK_TEMPLATE;
    sockinfo.ServiceId.Data1 = 0x5f0;

    res = bind(sock, (sockaddr *) &sockinfo, sizeof(SOCKADDR_HV));
    if (res == SOCKET_ERROR) {
        printf("Failed to bind socket: %d\n", WSAGetLastError());
        goto ReturnFailure;
    }

    res = listen(sock, 1);
    if (res == SOCKET_ERROR) {
        printf("Failed to start listen: %d\n", WSAGetLastError());
        goto ReturnFailure;
    }        

    printf("Socket listening...\n");

    SOCKET client = accept(sock, NULL, NULL);
    if (client == INVALID_SOCKET) {
        printf("Failed to accept client: %d\n", WSAGetLastError());
        goto ReturnFailure;
    }

    uint32_t value = htonl(50);
    // send(client, (const char*) &value, 4, 0);

    conn.client = client;
    conn.sock = sock;

    return 0;


ReturnFailure:
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }

    if (client != INVALID_SOCKET) {
        closesocket(client);
    }

    return -1;
}

void send_data(WSLConnection& conn, uint8_t* buf, uint32_t buf_len) {
    send(conn.client, (char*) buf, buf_len, 0);
}
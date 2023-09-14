#include <WinSock2.h>
#include <hvsocket.h>
#include <stdio.h>

struct __declspec(uuid("000005F0-abd9-408d-90ea-c0896d524f67")) ServiceGUID{};
struct __declspec(uuid("0ab3e255-3fba-44fd-a825-156c062b28f6")) VmGUID{};

int main(void) {
    WSADATA WsaData = { 0 };
    int res = 0;
    // Initialize Winsock
    res = WSAStartup(MAKEWORD(2,2), &WsaData);
    if (res != 0) {
        printf("WSAStartup failed with error: %d\n", res);
        goto Exit;
    }

    SOCKET sock = socket(AF_HYPERV, SOCK_STREAM, HV_PROTOCOL_RAW);
    SOCKET client{INVALID_SOCKET};
    if (sock == INVALID_SOCKET) {
        printf("Failed to create socket: %d\n", WSAGetLastError());
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
        goto Exit;
    }

    res = listen(sock, 0);
    if (res == SOCKET_ERROR) {
        printf("Failed to start listen: %d\n", WSAGetLastError());
        goto Exit;
    }
    printf("Listening\n");

    // accept a connection
    client = accept(sock, NULL, NULL);
    if (client == INVALID_SOCKET) {
        printf("Failed to accept client: %d\n", WSAGetLastError());
        goto Exit;
    }
    
    char test_message[] = "af_hyperv from Windows to WSL!";

    res = send(client, test_message, (int) strlen(test_message), 0);
    printf("Attempting to send data...\n");
    if (res == SOCKET_ERROR) {
        printf("Failed to send data: %d\n", WSAGetLastError());
        goto Exit;
    }

    // shut down connection
    res = shutdown(client, 0);
    if (res == SOCKET_ERROR) {
        printf("Failed to shut down ??: %d\n", WSAGetLastError());
        goto Exit;
    }

Exit:
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
    }

    if (client != INVALID_SOCKET) {
        closesocket(client);
    }

    WSACleanup();
    return 0;
}
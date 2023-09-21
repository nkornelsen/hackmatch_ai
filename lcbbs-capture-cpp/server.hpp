#include <stdint.h>
#include <WinSock2.h>
#include <hvsocket.h>
#include <stdio.h>

struct __declspec(uuid("000005F0-abd9-408d-90ea-c0896d524f67")) ServiceGUID{};
struct __declspec(uuid("30D5218E-F3EB-47E3-8DAF-5C2764F8D6F6")) VmGUID{};

typedef struct WSLConnection {
    SOCKET client;
    SOCKET sock;
} WSLConnection;

int connect_to_wsl(WSLConnection &conn);

void send_data(WSLConnection& conn, uint8_t* buf, uint32_t buf_len);
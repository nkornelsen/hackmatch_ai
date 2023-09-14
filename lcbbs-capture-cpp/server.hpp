#include <stdint.h>
#include <WinSock2.h>
#include <hvsocket.h>
#include <stdio.h>

struct __declspec(uuid("000005F0-abd9-408d-90ea-c0896d524f67")) ServiceGUID{};
struct __declspec(uuid("0ab3e255-3fba-44fd-a825-156c062b28f6")) VmGUID{};

typedef struct WSLConnection {
    SOCKET client;
    SOCKET sock;
} WSLConnection;

int connect_to_wsl(WSLConnection &conn);

void send_data(WSLConnection& conn, uint8_t* buf, uint32_t buf_len);
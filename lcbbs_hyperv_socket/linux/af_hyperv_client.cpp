#include <sys/socket.h>
#include <linux/vm_sockets.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    char recvbuf[100] = {0};
    int bytes_recv = 0;

    int clientfd = socket(AF_VSOCK, SOCK_STREAM, 0);
    if (clientfd == -1) {
        perror("client socket creation error");
        exit(0);
    }

    sockaddr_vm sockinfo = {0};
    sockinfo.svm_family = AF_VSOCK;
    sockinfo.svm_port = 0x5f0;
    sockinfo.svm_cid = VMADDR_CID_HOST;

    // attempt to connect
    int conn = connect(clientfd, (sockaddr *) &sockinfo, sizeof(sockaddr_vm));
    if (conn == -1) {
        perror("connect error");
        exit(0);
    }

    char buf[100] = {0};
    int bytes_received = recv(clientfd, buf, sizeof(buf)-1, 0);
    if (bytes_received == -1) {
        perror("Recv error");
        exit(0);
    }

    printf("Received: %s (%d bytes)\n", buf, bytes_received);

Exit:
    return 0;
}
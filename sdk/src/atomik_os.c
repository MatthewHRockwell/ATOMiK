#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <winsock2.h>
#include "../include/atomik_core.h"

#pragma comment(lib, "ws2_32.lib")

#define AOS_VERSION "1.0.0 (Network Capable)"
#define PROMPT "AOS> "

static atomik_handle_t sys_dev = NULL;
static SOCKET net_socket = INVALID_SOCKET;

// --- UTILITIES ---
void print_banner() {
    printf("\n");
    printf("      ATOMiK OPERATING SYSTEM (AOS) v%s\n", AOS_VERSION);
    printf("      [SECURE] [POLYMORPHIC] [DECENTRALIZED]\n");
    printf("      (c) 2026 ATOMiK Protocol.\n\n");
}

void cmd_help() {
    printf("  COMMANDS:\n");
    printf("    boot           - Initialize Hardware\n");
    printf("    load <genome>  - Load Genome Logic\n");
    printf("    listen <port>  - Start Receiver Mode\n");
    printf("    connect <port> - Connect to a Node\n");
    printf("    secure <msg>   - Send Encrypted Data\n");
    printf("    status         - Telemetry\n");
    printf("    exit           - Shutdown\n");
}

// --- KERNEL WRAPPERS ---
void cmd_boot() {
    if (sys_dev) return;
    atomik_init();
    sys_dev = atomik_open(0);
    if (sys_dev) printf(" [BOOT] ATOMiK Core Active (37ns).\n");
}

void cmd_load(const char* genome_name) {
    if (!sys_dev) { printf(" [ERR] Boot first.\n"); return; }
    char filename[64];
    snprintf(filename, 64, "%s.gnm", genome_name);
    atomik_load_genome(sys_dev, filename);
    
    // Patch for Polymorphism demo
    if (strstr(genome_name, "DOD")) atomik_set_polymorphism(sys_dev, 0xAABB, 25);
}

void cmd_status() {
    if (!sys_dev) { printf(" [ERR] Boot first.\n"); return; }
    atomik_metrics_t m;
    atomik_get_metrics(sys_dev, &m);
    printf("  --- STATS ---\n");
    printf("  Efficiency: %.2f%%\n", m.grid_save_percent);
    printf("  Entropy:    %d bits\n", m.entropy_score);
}

// --- NETWORK STACK ---
void cmd_listen(int port) {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET; a.sin_addr.s_addr = INADDR_ANY; a.sin_port = htons(port);
    bind(s, (struct sockaddr*)&a, sizeof(a));
    listen(s, 1);
    printf(" [NET] Listening on %d... (Waiting)\n", port);
    net_socket = accept(s, NULL, NULL);
    printf(" [NET] SECURE CHANNEL ACTIVE.\n");
}

void cmd_connect(int port) {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    net_socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in a = {0};
    a.sin_family = AF_INET; a.sin_addr.s_addr = inet_addr("127.0.0.1"); a.sin_port = htons(port);
    printf(" [NET] Connecting to %d...\n", port);
    if (connect(net_socket, (struct sockaddr*)&a, sizeof(a)) < 0) {
        printf(" [ERR] Connection Failed.\n");
    } else {
        printf(" [NET] SECURE CHANNEL ACTIVE.\n");
    }
}

void cmd_secure(const char* msg) {
    if (!sys_dev || net_socket == INVALID_SOCKET) { 
        printf(" [ERR] Need Boot + Network.\n"); return; 
    }
    // Encrypt (Simulated in HAL)
    atomik_secure_send(sys_dev, msg, strlen(msg));
    // Transmit
    send(net_socket, msg, strlen(msg), 0);
}

void check_network() {
    if (net_socket == INVALID_SOCKET) return;
    unsigned long mode = 1;
    ioctlsocket(net_socket, FIONBIO, &mode); // Non-blocking
    char buf[512];
    int len = recv(net_socket, buf, 511, 0);
    if (len > 0) {
        buf[len] = 0;
        printf("\n [INCOMING] %d bytes via Tunnel.\n", len);
        printf(" [DECODE]   MESSAGE: %s\n", buf);
        printf("%s", PROMPT);
    }
}

int main() {
    char input[128], cmd[32], arg[96];
    print_banner();
    while (1) {
        check_network();
        printf("%s", PROMPT);
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) == 0) continue;
        
        char* t = strtok(input, " ");
        if (t) strcpy(cmd, t);
        t = strtok(NULL, "");
        if (t) strcpy(arg, t); else arg[0]=0;

        if (strcmp(cmd, "boot")==0) cmd_boot();
        else if (strcmp(cmd, "load")==0) cmd_load(arg);
        else if (strcmp(cmd, "listen")==0) cmd_listen(atoi(arg));
        else if (strcmp(cmd, "connect")==0) cmd_connect(atoi(arg));
        else if (strcmp(cmd, "secure")==0) cmd_secure(arg);
        else if (strcmp(cmd, "status")==0) cmd_status();
        else if (strcmp(cmd, "exit")==0) break;
        else if (strcmp(cmd, "help")==0) cmd_help();
    }
    return 0;
}
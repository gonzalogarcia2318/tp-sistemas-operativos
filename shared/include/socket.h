#ifndef SOCKET_H
#define SOCKET_H

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

#define DISABLE_DEBUG
#ifdef ENABLE_DEBUG
  #define DEBUG_PRINTF(f, ...) printf(COLOR_CYAN f COLOR_RESET, ##__VA_ARGS__)
#else
  #define DEBUG_PRINTF(f, ...)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <pthread.h>

// #include "cliente.h"
// #include "server.h"

#include "protocolo.h"

#endif



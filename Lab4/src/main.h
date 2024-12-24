#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>

#define BUFFER_SIZE 256
#define SHM_KEY 1234
#define SHM_SIZE 1024

struct SharedData {
    char message[SHM_SIZE];
    int flag;
};
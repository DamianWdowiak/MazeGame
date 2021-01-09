#if !defined(_COMMON_H_)
#define _COMMON_H_

#include <ncurses.h>
#include <ctype.h>
#include <pthread.h>
#include <semaphore.h> //sem*
#include <sys/mman.h> // mmap, munmap, shm_open, shm_unlink
#include <fcntl.h> // O_*
#include <stdlib.h> // exit
#include <unistd.h> // close, ftruncate
#include <string.h> // strcasecmp
#include <time.h> // time
#include <sys/types.h>
#include <signal.h>

enum direction_t {NONE, UP, DOWN, LEFT, RIGHT}; 
enum player_type {CPU, HUMAN}; 
struct player_t{
    int x,y,carried,brought,deaths,server_pid,round;
    char vision[5][5];
    enum direction_t direction;
    enum player_type P_type;
    sem_t moved,ready;
};

struct join{
    int id,full,pid,running;
    sem_t sem,sem2,wait;
};

static void err(int c, const char* msg) {
    if (!c)
        return;
    perror(msg);
    exit(1);
}

#endif // _COMMON_H_
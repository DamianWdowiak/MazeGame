#include "../gra_player/common.h"

void *dir(void * arg);
pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
int BresenhamLine(const int x1, const int y1, const int x2, const int y2, const struct player_t* ptr);

int main(int argc, char **argv)
{
    system("resize -s 25 110");
    
    initscr();
    noecho();
    curs_set(FALSE);
    start_color();
    use_default_colors();
    init_pair(1, -1, -1); //default
    init_pair(2, COLOR_YELLOW, COLOR_GREEN); //A
    init_pair(3, -1, COLOR_MAGENTA); //players
    init_pair(4, COLOR_WHITE, COLOR_WHITE); //walls
    init_pair(5, COLOR_RED, -1); //beast
    init_pair(6, -1, COLOR_YELLOW); //coins
    init_pair(7, COLOR_GREEN, COLOR_YELLOW); //drop
    
    struct join* join_server = NULL;
    int fd = shm_open("server", O_CREAT | O_EXCL | O_RDWR, 0600);
    if(fd == -1){
        fd = shm_open("server", O_CREAT | O_RDWR, 0600);
        err(fd == -1,"shm_open");
        join_server = (struct join*)mmap(NULL, sizeof(struct join*), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        err(join_server == NULL,"join_mmap");
    }
    else{
        int status = ftruncate(fd,sizeof(struct join));
        err(status == -1,"ftruncate");
        join_server = (struct join*)mmap(NULL, sizeof(struct join*), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        err(join_server == NULL,"join_mmap");
        sem_init(&join_server->sem,1,0);
        sem_init(&join_server->sem2,1,0);
        sem_init(&join_server->wait,1,0);
    }
    if(join_server->full){
        mvprintw(10,40,"Server is full, try again later!\n");
        refresh();
        usleep(3000000);
        endwin();
        system("clear");
        return 0;
    }
    int c=0;
    srand(time(0));
    pthread_t get;
    pthread_create(&get,NULL,dir,&c);
    
    mvprintw(10,40,"Waiting for server...\n");
    refresh();
    
    sem_post(&join_server->sem2);
    sem_wait(&join_server->wait);
    join_server->pid = getpid();
    
    int turn=0,run=0,rabbit = 0,x=0,y=0; 
    int name[10] ={ 0 };
    int id=join_server->id;
    sprintf(name,"/player_%d",id);
    fd = shm_open(name, O_CREAT | O_RDWR, 0600);
    err(fd==-1,"shm_open");
    struct player_t* ptr = (struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    err(ptr == NULL,"mmap");
    
    ptr->P_type = CPU;
    sem_post(&join_server->sem);
    int x_A=0,y_A=0,found_A=0;
    clear();
    ptr->direction = RIGHT;
    while(1){
        for(int i=0;i<5;++i){
            for(int j=0;j<5;++j){
                if(*(*(ptr->vision+i)+j) == 'H')
                    attron(COLOR_PAIR(4));
                else if(*(*(ptr->vision+i)+j) == 'A'){
                    attron(COLOR_PAIR(2));
                    found_A = 1;
                    x_A =ptr->x+j-2;
                    y_A =ptr->y+i-2;
                }
                else if(isdigit(*(*(ptr->vision+i)+j)))
                    attron(COLOR_PAIR(3));
                else if(*(*(ptr->vision+i)+j)=='*')
                    attron(COLOR_PAIR(5));
                else if(*(*(ptr->vision+i)+j)=='c' || *(*(ptr->vision+i)+j)=='t'|| *(*(ptr->vision+i)+j)=='T')
                    attron(COLOR_PAIR(6));
                else if(*(*(ptr->vision+i)+j)=='D')
                    attron(COLOR_PAIR(7));
                else
                    attron(COLOR_PAIR(1));
                mvprintw(ptr->y+i-2,ptr->x+j-2,"%c",*(*(ptr->vision+i)+j));
            }
        }
        attron(COLOR_PAIR(3));
        mvprintw(ptr->y,ptr->x,"%c",id+'0');
        attron(COLOR_PAIR(1));
        mvprintw(1,55,"Server's PID: %d",ptr->server_pid);
        if(found_A)
            mvprintw(2,56,"Campsite X/Y: %d %d",x_A,y_A); 
        else
            mvprintw(2,56,"Campsite X/Y: unknown");
        mvprintw(3,56,"Round number: %d",ptr->round); 
        
        mvprintw(5,55,"Player:");
        mvprintw(6,56,"Number");
        mvprintw(7,56,"Type");
        mvprintw(8,56,"Curr X/Y");
        mvprintw(9,56,"Deaths");
        
        mvprintw(6,67,"%d",id);
        if(ptr->P_type == HUMAN)
            mvprintw(7,67,"HUMAN");
        else if(ptr->P_type == CPU)
            mvprintw(7,67,"CPU");
        mvprintw(8,67,"%02d/%02d",ptr->x,ptr->y);
        mvprintw(9,67,"%d",ptr->deaths);
        mvprintw(11,67,"    ");
        mvprintw(12,67,"    ");   
        mvprintw(11,55,"Coins found %d",ptr->carried);
        mvprintw(12,55,"Coins brought %d",ptr->brought);

        mvprintw(15,55,"Legend:");
        attron(COLOR_PAIR(3));
        mvprintw(16,56,"1234");
        attron(COLOR_PAIR(1));
        mvprintw(16,60," - players");
        attron(COLOR_PAIR(4));
        mvprintw(17,56,"H");
        attron(COLOR_PAIR(1));
        mvprintw(17,60," - wall");
        attron(COLOR_PAIR(1));
        mvprintw(18,56,"#");
        mvprintw(18,60," - bushes (slow down)");
        attron(COLOR_PAIR(5));
        mvprintw(19,56,"*");
        attron(COLOR_PAIR(1));
        mvprintw(19,60," - wild beast");
        attron(COLOR_PAIR(6));
        mvprintw(20,56,"c");
        attron(COLOR_PAIR(1));
        mvprintw(20,60," - one coin");
        attron(COLOR_PAIR(6));
        mvprintw(21,56,"t");
        attron(COLOR_PAIR(1));
        mvprintw(21,60," - treasure (10 coins)");
        attron(COLOR_PAIR(6));
        mvprintw(22,56,"T");
        attron(COLOR_PAIR(1));
        mvprintw(22,60," - large treasure (50 coins)");
        attron(COLOR_PAIR(7));
        mvprintw(23,56,"D");
        attron(COLOR_PAIR(1));
        mvprintw(23,60," - dropped treasure");
        attron(COLOR_PAIR(2));
        mvprintw(24,56,"A");
        attron(COLOR_PAIR(1));
        mvprintw(24,60," - campsite");
        refresh();
        if(c=='q'|| c == 'Q')
            break;
        x=0;
        y=0;
        rabbit=0;
        run=0;
        for(;x<5;++x){
            for(;y<5;++y){
                if(*(*(ptr->vision+y)+x) == '*'){
                    rabbit=1;
                    break;
                }
            }
            if(rabbit)
                break;
        }
        if(!rabbit){
            x=0;
            y=0;
            for(;x<5;++x){
                for(;y<5;++y){
                    if(isdigit(*(*(ptr->vision+y)+x))){
                        rabbit=1;
                        break;
                    }
                }
                if(rabbit)
                    break;
            }
        }
        if(rabbit)
        {
            run=BresenhamLine(2,2,x,y,ptr);
        }
            
        if(run){
            if(2>y){
                ptr->direction = DOWN;
                if(*(*(ptr->vision+2+1)+2) == 'H' || *(*(ptr->vision+2+1)+2) == '*' || isdigit(*(*(ptr->vision+2+1)+2))){
                    if(2>x){
                        ptr->direction = RIGHT;
                    }
                    else if(2<x){
                        ptr->direction = LEFT;
                    }
                    else{
                        ptr->direction=RIGHT;
                    }
                }
            }
            else if(2<y){
                ptr->direction = UP;
                if(*(*(ptr->vision+2-1)+2) == 'H' || *(*(ptr->vision+2-1)+2) == '*' || isdigit(*(*(ptr->vision+2-1)+2))){
                    if(2>x){
                        ptr->direction = RIGHT;
                    }
                    else if(2<x){
                        ptr->direction = LEFT;
                    }
                    else{
                        ptr->direction=RIGHT;
                    }
                }
            }
            else{
                if(2>x){
                    ptr->direction = RIGHT;
                    if(*(*(ptr->vision+2)+2+1) == 'H' || *(*(ptr->vision+2)+2+1) == '*' || isdigit(*(*(ptr->vision+2)+2+1))){
                        if(2>y){
                            ptr->direction = DOWN;
                        }
                        else if(2<y){
                            ptr->direction = UP;
                        }
                        else{
                            ptr->direction=RIGHT;
                        }
                    }
                }
                else if(2<x){
                    ptr->direction = LEFT;
                    if(*(*(ptr->vision+2)+2-1) == 'H' || *(*(ptr->vision+2)+2-1) == '*' || isdigit(*(*(ptr->vision+2)+2-1))){
                        if(2>y){
                            ptr->direction = DOWN;
                        }
                        else if(2<y){
                            ptr->direction = UP;
                        }
                        else{
                            ptr->direction=RIGHT;
                        }
                    }
                }
                else{
                    ptr->direction=RIGHT;
                }
            }
        }
        else{
            if(*(*(ptr->vision+2)+2) == '#' && turn){
                turn=0;
            }
            else{
                if(ptr->direction == UP){
                    if(*(*(ptr->vision+2)+2+1) != 'H'){
                        ptr->direction = RIGHT;
                    }
                    else if(*(*(ptr->vision+2-1)+2) == 'H'){
                        ptr->direction = LEFT;
                    }
                }
                else if(ptr->direction == RIGHT){
                    if(*(*(ptr->vision+2+1)+2) != 'H'){
                        ptr->direction = DOWN;
                    }
                    else if(*(*(ptr->vision+2)+2+1) == 'H'){
                        ptr->direction = UP;
                    }
                }
                else if(ptr->direction == DOWN){
                    if(*(*(ptr->vision+2)+2-1) != 'H'){
                        ptr->direction = LEFT;
                    }
                    else if(*(*(ptr->vision+2+1)+2) == 'H'){
                        ptr->direction = RIGHT;
                    }
                }
                else if(ptr->direction == LEFT){
                    if(*(*(ptr->vision+2-1)+2) != 'H'){
                        ptr->direction = UP;
                    }
                    else if(*(*(ptr->vision+2)+2-1) == 'H'){
                        ptr->direction = DOWN;
                    }
                }
                turn=1;
            }
            
        }
        sem_post(&ptr->ready);
        sem_wait(&ptr->moved);
        clear();
        
    }
    refresh();
    shm_unlink(name);
    endwin();
    system("clear");
	return 0;
}

void *dir(void * arg){
    int *c = (int*)arg;
    while(1){
        *c = getch();
        if(*c == 'q' || *c == 'Q')
            break;
    }
}
int BresenhamLine(const int x1, const int y1, const int x2, const int y2, const struct player_t* ptr)
{
     int d, dx, dy, ai, bi, xi, yi;
     int x = x1, y = y1;
     if (x1 < x2)
     {
         xi = 1;
         dx = x2 - x1;
     }
     else
     {
         xi = -1;
         dx = x1 - x2;
     }
     if (y1 < y2)
     {
         yi = 1;
         dy = y2 - y1;
     }
     else
     {
         yi = -1;
         dy = y1 - y2;
     }
        if(*(*(ptr->vision+y)+x) == 'H')
            return 0;
     if (dx > dy)
     {
         ai = (dy - dx) * 2;
         bi = dy * 2;
         d = bi - dx;
         while (x != x2)
         {
             if (d >= 0)
             {
                 x += xi;
                 y += yi;
                 d += ai;
             }
             else
             {
                 d += bi;
                 x += xi;
             }
        if(*(*(ptr->vision+y)+x) == 'H')
            return 0;
         }
     }
     else
     {
         ai = ( dx - dy ) * 2;
         bi = dx * 2;
         d = bi - dy;
         while (y != y2)
         {
             if (d >= 0)
             {
                 x += xi;
                 y += yi;
                 d += ai;
             }
             else
             {
                 d += bi;
                 y += yi;
             }
            if(*(*(ptr->vision+y)+x) == 'H')
                return 0;
         }
     }
     return 1;
}
#include "../gra_player/common.h"
#include "linked_list.h"

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
 
#define COINS 30
#define TREASURE 20
#define LARGE_TREASURE 10
#define ROUND_TIME 1000000/10

enum state_t {OFFLINE, ONLINE};
struct rabbit_t{
    sem_t sem;
    int taken;
};
struct data_t{
    enum state_t state;
    int x_spawn,y_spawn,x,y,pid,carried,brought,my_turn,*round,slow,i_have_thread,*in_game,*beast_count; 
    struct rabbit_t*rabbits;
    struct player_t* ptr;
    struct join* join_server;
    struct linked_list_t* ll;
    char (*map)[52];
    char (*bushes)[52];
    pthread_t th;
};

void *connection(void * arg);
void *game(void * arg);
void *display(void * arg);
void *killer(void * arg);
void *beast(void * arg);
int BresenhamLine(const int x1, const int y1, const int x2, const int y2,const struct data_t *data);

int main(int argc, char **argv)
{
    system("resize -s 25 110");
    
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
    if(join_server->running){
        printf("\nServer is running!\n");
        return 1;
    }
    join_server->running =1;
    //display
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
    //end_display
    
    srand(time(0));
    struct data_t data[4] = {0};
    struct linked_list_t* ll = ll_create();
    err(ll == NULL,"list");
    char map[25][52]={"HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH",
                    "H   H       H               H         H       H   H",
                    "H H HHH HHH HHHHHHHHHHH HHH H HHHHHHH HHH HHHHH   H",
                    "H H   H H H           H H H   H     H     H   H   H",
                    "H HHH H H HHH   HHHHH H H HHHHH HHHHHHHHHHHHH HHH H",
                    "H H H   H           H H H     H       H       H H H",
                    "H H HHHHH HHH HHHHHHH H H H HHH HHH HHH HHH H H H H",
                    "H H         H H       H H H     H   H   H H H   H H",
                    "H H HHHHHHH HHH HHHHHHH HHHHH HHH HHH HHH H HHH H H",
                    "H H H     H   H   H     H   H   H         H H H H H",
                    "H HHH HHH HHH HHH HHH HHH H HHH HHHHHHHHHHH H H H H",
                    "H H   H       H H   H     H H   H H       H H   H H",
                    "H H HHHHHH HH H HHH HHH HHH HHH H H HHHHH H H HHH H",
                    "H H     H   H H   H   H   H   H   H H     H H H   H",
                    "H H H   H HHH HHH HHH HHHHHHH HHH H HHH HHH H H HHH",
                    "H H H   H       H   H H       H   H   H     H H H H",
                    "H H H  HHHHHHH  H H H H HH HHHH HHHHH HHHHHHH H H H",
                    "H H H       H   H H H   H     H   H H         H   H",
                    "H HHHHHHHHH H HHH HHHHHHH HHHHHHH H HHHHH H   HHH H",
                    "H H       H H     H     H       H   H   H H     H H",
                    "H H HHHHH H HHHHHHH H HHH HHHHH HHH H H HHH HHHHH H",
                    "H   H     H         H     H   H     H H   H       H",
                    "H HHH HHHHHHHHHHHHHHHHHHHHH H HHHHHHH HHH H       H",
                    "H   H                       H           H         H",
                    "HHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH"};
    char bushes[25][52]={"                                                   ",
                        "                 #####                             ",
                        "                                                   ",
                        "                                                   ",
                        "             ###                                   ",
                        "                          ##                       ",
                        "                                                   ",
                        "                                                   ",
                        "                                                   ",
                        "                                                   ",
                        "                                                   ",
                        "                       A                           ",
                        "          #                                        ",
                        "       #                                           ",
                        "      ##                                           ",
                        "     ##      #           ###                       ",
                        "     #                    #                        ",
                        "     #                                      ##     ",
                        "                                            ##     ",
                        "   #                                        ##     ",
                        "                                           #       ",
                        " ###                   ### ##              ######  ",
                        "                           #               #    #  ",
                        "                      ###### ##              ##    ",
                        "                                                   "};  

             
    int x=0,y=0;
    for(int i = 0; i<LARGE_TREASURE; ++i){
        do{
            x = rand() % 50 + 1;
            y = rand() % 24 + 1;
        }while(*(*(map+y)+x) != ' ' || *(*(bushes+y)+x) != ' ');
        *(*(map+y)+x) = 'T';
    }
    for(int i = 0; i<COINS; ++i){
        do{
            x = rand() % 50 + 1;
            y = rand() % 24 + 1;
        }while(*(*(map+y)+x) != ' ' || *(*(bushes+y)+x) != ' ');
        *(*(map+y)+x) = 'c';
    }
    for(int i = 0; i<TREASURE; ++i){
        do{
            x = rand() % 50 + 1;
            y = rand() % 24 + 1;
        }while(*(*(map+y)+x) != ' ' || *(*(bushes+y)+x) != ' ');
        *(*(map+y)+x) = 't';
    }
    
    int round = 0,in_game=0,beast_count = 0; //server struct set up 
    struct rabbit_t rabbit[10] = {0};
    for(int i=0;i<4;++i){
        data[i].map=map;
        data[i].bushes=bushes;
        data[i].round = &round;
        data[i].ll=ll;
        data[i].join_server = join_server;
        data[i].in_game = &in_game;
        data[i].beast_count = &beast_count;
        data[i].rabbits = rabbit;
    }
    pthread_t connect,dis,Pazura,beasts[10];
    pthread_create(&connect,NULL,connection,&data);
    pthread_create(&dis,NULL,display,&data);
    pthread_create(&Pazura,NULL,killer,&data);
    
    int c =0;
    while((c=getch()) !='q' && c!= 'Q'){
        switch (c){
            case 'c':
            pthread_mutex_lock(&mutex);
            do{
                x = rand() % 50 + 1;
                y = rand() % 24 + 1;
            }while(*(*(data[0].map+y)+x) != ' ' || *(*(data[0].bushes+y)+x) != ' ');
            *(*(data[0].map+y)+x) = 'c';
            pthread_mutex_unlock(&mutex);
            break;
            case 't':
            pthread_mutex_lock(&mutex);
            do{
                x = rand() % 50 + 1;
                y = rand() % 24 + 1;
            }while(*(*(data[0].map+y)+x) != ' ' || *(*(data[0].bushes+y)+x) != ' ');
            *(*(data[0].map+y)+x) = 't';
            pthread_mutex_unlock(&mutex);
            break;
            case 'T':
            pthread_mutex_lock(&mutex);
            do{
                x = rand() % 50 + 1;
                y = rand() % 24 + 1;
            }while(*(*(data[0].map+y)+x) != ' ' || *(*(data[0].bushes+y)+x) != ' ');
            *(*(data[0].map+y)+x) = 'T';
            pthread_mutex_unlock(&mutex);
            break;
            case 'b':
            case 'B': 
            if(beast_count < 10){
                pthread_mutex_lock(&mutex);
                ++beast_count;
                pthread_mutex_unlock(&mutex);
                pthread_create(&beasts[beast_count],NULL,beast,&data);
            }
            break;
        }
    }
    char name[10] ={0};
    ll_clear(ll);
    free(ll);
    pthread_mutex_lock(&mutex);
    for(int i=0;i<4;++i){
        if(data[i].state == ONLINE){
            sem_destroy(&data[i].ptr->moved);
            sem_destroy(&data[i].ptr->ready);
            sprintf(name,"player_%d",i+1);
            shm_unlink(name);
            if(!kill(data[i].pid,0)){
                kill(data[i].pid,SIGKILL);
            }
            
        }
    }
    pthread_mutex_unlock(&mutex);
    for(int i=0;i<10;++i)
        sem_destroy(&data[0].rabbits[i].sem);
    sem_destroy(&join_server->sem);
    sem_destroy(&join_server->sem2);
    shm_unlink("server");

    endwin();
	return 0;
}

void *connection(void * arg){
    struct data_t *data = (struct data_t *)arg;
    int fd = -1;
    char name[10] ={0};
    while(1){
        for(int i=0;i<4;++i){
            if(*data[i].in_game == 4)
                data[i].join_server->full = 1;
            else
                data[i].join_server->full = 0;
            
            if(data[i].state == OFFLINE){
                sem_wait(&data[i].join_server->sem2);
                
                data[i].join_server->id = i+1;
                *data[i].in_game+=1;
                
                pthread_mutex_lock(&mutex);
                
                sprintf(name,"/player_%d",i+1);
                fd = shm_open(name, O_CREAT | O_RDWR,0600);
                err(fd == -1,"shm_open");
                int ups= ftruncate(fd,sizeof(struct player_t));
                err(ups == -1,"ftruncate");
                data[i].ptr = (struct player_t*)mmap(NULL, sizeof(struct player_t), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
                err(data[i].ptr == NULL,"mmap");
                
                sem_init(&data[i].ptr->moved,1,0);
                sem_init(&data[i].ptr->ready,1,0);
                
                data[i].state = ONLINE;
                
                data[i].my_turn = *data[0].round;
                data[i].ptr->server_pid = getpid();
                data[i].ptr->round = *data[0].round;
                do{
                    data[i].x_spawn = rand() % 50 + 1;
                    data[i].y_spawn = rand() % 24 + 1;
                }while(*(*(data[i].map+data[i].y_spawn)+data[i].x_spawn) != ' ');
                data[i].ptr->x = data[i].x_spawn;
                data[i].ptr->y = data[i].y_spawn;
                data[i].x = data[i].x_spawn;
                data[i].y = data[i].y_spawn;
                *(*(data[i].map+data[i].y)+data[i].x) = i + 1 + '0';
                for(int j=0;j<5;++j){
                    for(int k=0;k<5;++k){
                        if(data[i].y-2+j < 0 || data[i].y+j > 26 || data[i].x-2+k < 0 || data[i].x+k > 52){
                            *(*(data[i].ptr->vision+j)+k) = ' ';
                        }
                        else{
                            if(*(*(data[i].bushes+j+data[i].y-2)+k+data[i].x-2) != ' ')
                                *(*(data[i].ptr->vision+j)+k)= *(*(data[i].bushes+j+data[i].y-2)+k+data[i].x-2);
                            else
                                *(*(data[i].ptr->vision+j)+k) = *(*(data[i].map+j+data[i].y-2)+k+data[i].x-2);
                        }
                            
                    }
                }
                *(*(data[i].ptr->vision+2)+2) = i+1+'0';
                sem_post(&data[i].join_server->wait);
                sem_wait(&data[i].join_server->sem);
                data[i].pid = data[i].join_server->pid;
                pthread_create(&data[i].th,NULL,game,data);
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
    }
    return NULL;
}
void *game(void * arg){
    struct data_t *data = (struct data_t *)arg;
    int id=0,x=0,y=0;
    pthread_mutex_lock(&mutex);
    for(int i=0;i<4;++i){
        if(data[i].i_have_thread == 0 && data[i].state == ONLINE){
            data[i].i_have_thread = 1;
            id=i;
            break;
        }
    }
    pthread_mutex_unlock(&mutex);
    while(1){ 
        sem_wait(&data[id].ptr->ready);
        data[id].ptr->x = data[id].x;
        data[id].ptr->y = data[id].y;
        pthread_mutex_lock(&mutex);
        *(*(data[id].map+data[id].ptr->y)+data[id].ptr->x) = ' ';
        pthread_mutex_unlock(&mutex);
        
        if(data[id].slow){
            data[id].slow=0;
        }
        else{
            switch (data[id].ptr->direction){
                case UP:
                    y=-1;
                    x=0;
                break;
                case DOWN:
                    y=1;
                    x=0;
                break;
                case LEFT:
                    y=0;
                    x=-1;
                break;
                case RIGHT:
                    y=0;
                    x=1;
                break;
                case NONE:
                    x=0;
                    y=0;
                break;
            }
            if(x != 0 || y != 0){
                pthread_mutex_lock(&mutex);
                if(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == ' '){
                        if(*(*(data[id].bushes+data[id].ptr->y+y)+data[id].ptr->x+x) == '#')
                            data[id].slow =1;
                        else if(*(*(data[id].bushes+data[id].ptr->y+y)+data[id].ptr->x+x) == 'A'){
                            data[id].brought+=data[id].carried;
                            data[id].carried = 0;
                        }
                        data[id].ptr->y+=y;
                        data[id].ptr->x+=x;
                }
                else if(isdigit(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x)) == '1' || *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == '2' || *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == '3' || *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == '4'){
                    int player_id = *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) - '0'-1;
                    if(data[id].carried + data[player_id].carried>0){
                        if(data[id].ptr->y+y == data[id].y_spawn && data[id].ptr->x+x == data[id].x_spawn){
                            data[id].carried += data[player_id].carried;
                            data[player_id].carried = 0;
                        }
                        else if(data[id].ptr->y+y == data[player_id].y_spawn && data[id].ptr->x+x == data[player_id].x_spawn){
                            data[player_id].carried += data[id].carried;
                            data[id].carried = 0;
                        }
                        else{
                            *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) = 'D';
                            struct node_t* node = data[id].ll->head;
                            int is = 0;
                            while(node)
                            {
                                if(node->x == data[id].x+x && node->y == data[id].y+y){
                                    is=1;
                                }
                                node=node->next;
                            }
                            if(!is){
                                int ups =ll_push_back(data[id].ll,data[id].carried + data[player_id].carried,data[id].x+x,data[id].y+y);
                                err(ups == 2,"Drop_alloc");
                            }
                            data[player_id].carried = 0;
                            data[id].carried = 0;
                        }
                    }
                    else{
                        *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) = ' ';
                    }
                    data[id].ptr->x = data[id].x_spawn;
                    data[id].ptr->y = data[id].y_spawn;
                    data[id].ptr->deaths++;
                    data[player_id].x = data[player_id].x_spawn;
                    data[player_id].y = data[player_id].y_spawn;
                    data[player_id].ptr->deaths++;
                   
                }
                else if(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == '*'){
                    if(data[id].carried>0){
                        *(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) = 'D';
                        struct node_t* node = data[id].ll->head;
                        int is = 0;
                        while(node)
                        {
                            if(node->x == data[id].x+x && node->y == data[id].y+y){
                                is=1;
                            }
                            node=node->next;
                        }
                        if(!is){
                            int ups = ll_push_back(data[id].ll,data[id].carried,data[id].x+x,data[id].y+y);
                            err(ups == 2,"Drop_alloc");
                        }
                    }
                    data[id].ptr->x = data[id].x_spawn;
                    data[id].ptr->y = data[id].y_spawn;
                    data[id].carried = 0;
                    data[id].ptr->deaths++; 
                    
                }
                else if(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == 'c'){
                    data[id].ptr->y+=y;
                    data[id].ptr->x+=x;
                    data[id].carried++;
                }
                else if(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == 't'){
                    data[id].ptr->y+=y;
                    data[id].ptr->x+=x;
                    data[id].carried+=10;
                }
                else if(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == 'T'){
                    data[id].ptr->y+=y;
                    data[id].ptr->x+=x;
                    data[id].carried+=50;
                }
                else if(*(*(data[id].map+data[id].ptr->y+y)+data[id].ptr->x+x) == 'D'){
                    if(*(*(data[id].bushes+data[id].ptr->y+y)+data[id].ptr->x+x) == '#')
                        data[id].slow =1;
                    data[id].ptr->y+=y;
                    data[id].ptr->x+=x;
                    struct node_t* node = data[id].ll->head;
                    int err_code=0;
                    for(int i=0;node;++i){
                        if(node->x == data[id].ptr->x && node->y == data[id].ptr->y){
                            data[id].carried+=node->data;
                            ll_remove(data[id].ll,i,&err_code);
                            err(err_code != 0,"list");
                            break;
                        }
                        node = node->next;
                    }
                }
                pthread_mutex_unlock(&mutex);
                data[id].my_turn=*data[id].round;
            }
        }
        *(*(data[id].map+data[id].ptr->y)+data[id].ptr->x) = id+1 + '0';
        for(int i=0;i<5;++i){
            for(int j=0;j<5;++j){
                if(data[id].ptr->y-2+i < 0 || data[id].ptr->y+i > 26 || data[id].ptr->x-2+j < 0 || data[id].ptr->x+j > 52){
                    *(*(data[id].ptr->vision+i)+j) = ' ';
                }
                else{
                    if(*(*(data[id].bushes+i+data[id].ptr->y-2)+j+data[id].ptr->x-2) != ' ' && *(*(data[id].map+i+data[id].ptr->y-2)+j+data[id].ptr->x-2) != '*' && !isdigit(*(*(data[id].map+i+data[id].ptr->y-2)+j+data[id].ptr->x-2)))
                        *(*(data[id].ptr->vision+i)+j)=*(*(data[id].bushes+i+data[id].ptr->y-2)+j+data[id].ptr->x-2);
                    else
                        *(*(data[id].ptr->vision+i)+j) = *(*(data[id].map+i+data[id].ptr->y-2)+j+data[id].ptr->x-2);
                }
            }
        }
        if(*(*(data[id].bushes+data[id].ptr->y)+data[id].ptr->x) != ' '){
            *(*(data[id].ptr->vision+2)+2) = *(*(data[id].bushes+data[id].ptr->y)+data[id].ptr->x);
        }
        else
            *(*(data[id].ptr->vision+2)+2) = *(*(data[id].map+data[id].ptr->y)+data[id].ptr->x);
        data[id].x = data[id].ptr->x;
        data[id].y = data[id].ptr->y;
        data[id].ptr->brought = data[id].brought;
        data[id].ptr->carried = data[id].carried;
        data[id].ptr->round=*data[0].round;
    }
}

void *display(void * arg){
    struct data_t *data = (struct data_t *)arg;
    struct node_t * node = NULL; 
    while(1){
        pthread_mutex_lock(&mutex);
        for(int i=0;i<25;++i){
            for(int j=0;*(*(data[0].map+i)+j);++j){
                if(*(*(data[0].map+i)+j) == 'H')
                    attron(COLOR_PAIR(4));
                else if(*(*(data[0].map+i)+j) == 'A')
                    attron(COLOR_PAIR(2));
                    else if(*(*(data[0].map+i)+j) == 'D')
                    attron(COLOR_PAIR(7));
                else if(*(*(data[0].map+i)+j)=='c' || *(*(data[0].map+i)+j)=='t'|| *(*(data[0].map+i)+j)=='T')
                    attron(COLOR_PAIR(6));
                else
                    attron(COLOR_PAIR(1));
                mvprintw(i,j,"%c",*(*(data[0].map+i)+j));
            }
        }
        for(int i=0;i<25;++i){
            for(int j=0;*(*(data->bushes+i)+j);++j){
                if(*(*(data->bushes+i)+j) == '#'){
                    attron(COLOR_PAIR(1));
                    mvprintw(i,j,"%c",*(*(data->bushes+i)+j));
                }
                else if(*(*(data->bushes+i)+j) == 'A'){
                    attron(COLOR_PAIR(2));
                    mvprintw(i,j,"%c",*(*(data->bushes+i)+j));
                }
            }
        }

        node = data->ll->head;
        while(node){
            if(node->y >= 0 && node->y <= 24 && node->x >= 0 && node->x <= 51){
                *(*(data[0].map+node->y)+node->x) = 'D';
                attron(COLOR_PAIR(7));
                mvprintw(node->y,node->x,"%c",*(*(data[0].map+node->y)+node->x));
            }
            node=node->next;
        }
        for(int i=0;i<25;++i){
            for(int j=0;*(*(data[0].map+i)+j);++j){
                if(*(*(data[0].map+i)+j)=='*'){
                    attron(COLOR_PAIR(5));
                    mvprintw(i,j,"%c",*(*(data[0].map+i)+j));
                
                }
            }
        }
        attron(COLOR_PAIR(1));
        mvprintw(1,55,"Server's PID: %d",getpid());
        mvprintw(2,56,"Campsite X/Y: %d %d",23,11);
        mvprintw(3,56,"Round number: %d",*data[0].round); 
        
        mvprintw(5,55,"Parameter:");
        mvprintw(6,56,"PID");
        mvprintw(7,56,"Type");
        mvprintw(8,56,"Curr X/Y");
        mvprintw(9,56,"Deaths");
        
        mvprintw(5,67,"Player1");
        mvprintw(5,76,"Player2");
        mvprintw(5,85,"Player3");
        mvprintw(5,94,"Player4");
        for(int i = 67,j=0; j<4;i+=9,++j){
            if(data[j].state == ONLINE){
                mvprintw(6,i,"%d",data[j].pid);
                if(data[j].ptr->P_type == HUMAN)
                    mvprintw(7,i,"HUMAN");
                else if(data[j].ptr->P_type == CPU)
                    mvprintw(7,i,"CPU");
                mvprintw(8,i,"%02d/%02d",data[j].ptr->x,data[j].ptr->y);
                mvprintw(9,i,"%d",data[j].ptr->deaths);
            }
            else{
                mvprintw(6,i,"-    ");
                mvprintw(7,i,"-    ");
                mvprintw(8,i,"--/--");
                mvprintw(9,i,"-    ");
            }
        }
        mvprintw(11,55,"Coins");
        mvprintw(12,59,"carried");
        mvprintw(13,59,"brought");
        for(int i = 67,j=0; j<4;i+=9,++j){
            mvprintw(12,i,"        ");
            mvprintw(13,i,"        ");
            if(data[j].state == ONLINE){
                mvprintw(12,i,"%d",data[j].carried);
                mvprintw(13,i,"%d",data[j].brought);
            }
        }
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
        
        for(int i =0;i<4;++i){
            if(data[i].state == ONLINE){
                attron(COLOR_PAIR(3));
                mvprintw(data[i].ptr->y,data[i].ptr->x,"%c",i+1+'0');
            }
        }
        
        pthread_mutex_unlock(&mutex);
        refresh();
    }
    
    return NULL;
}
void *killer(void * arg){
    struct data_t *data = (struct data_t *)arg;
    char name[10] ={0};
    while(!*data->in_game);
    while(1){
        for(int i=0;i<4;++i){
            if(data[i].state == ONLINE){
                sem_post(&data[i].ptr->moved);
                if(*data[i].round - data[i].my_turn >= 1000){
                    pthread_mutex_lock(&mutex);
                    *(*(data[0].map+data[i].y)+data[i].x) = ' ';
                    data[i].state = OFFLINE;
                    data[i].carried =0;
                    data[i].brought =0;
                    *data[i].in_game-=1;
                    data[i].join_server->full = 0;
                    data[i].i_have_thread = 0;
                    pthread_cancel(data[i].th);
                    sem_destroy(&data[i].ptr->moved);
                    sem_destroy(&data[i].ptr->ready);
                    sprintf(name,"player_%d",i+1);
                    shm_unlink(name);
                    if(!kill(data[i].pid,0)){
                        kill(data[i].pid,SIGKILL);
                    }
                    pthread_mutex_unlock(&mutex);
                }
                if(kill(data[i].pid,0)){
                    pthread_mutex_lock(&mutex);
                    *(*(data[0].map+data[i].y)+data[i].x) = ' ';
                    data[i].state = OFFLINE;
                    data[i].carried =0;
                    data[i].brought =0;
                    *data[i].in_game-=1;
                    data[i].join_server->full = 0;
                    data[i].i_have_thread = 0;
                    pthread_cancel(data[i].th);
                    sem_destroy(&data[i].ptr->moved);
                    sem_destroy(&data[i].ptr->ready);
                    sprintf(name,"player_%d",i+1);
                    shm_unlink(name);
                    pthread_mutex_unlock(&mutex);
                }
            }
        }
        pthread_mutex_lock(&mutex);
        for(int i=0;i<*data->beast_count;++i){
            sem_post(&data->rabbits[i].sem);
        }
        pthread_mutex_unlock(&mutex);
        usleep(ROUND_TIME);
        *data[0].round+=1;
    }
    return NULL;
}

void *beast(void * arg){
    struct data_t *data = (struct data_t *)arg;
    int x=0,y=0,move_x=1,move_y=0,player = 0,hunt_mode= 0,player_x=0,player_y=0,rabbit_id=0,turn=0;
    pthread_mutex_lock(&mutex);
    for(int i=0;i<*data[0].beast_count;++i){
        if(data->rabbits[i].taken == 0){
            data->rabbits[i].taken =1;
            rabbit_id = i;
            break;
        }
    }
    
    sem_init(&data->rabbits[rabbit_id].sem,1,0);
    do{
        x = rand() % 50 + 1;
        y = rand() % 24 + 1;
    }while(*(*(data[0].map+y)+x) != ' ' || *(*(data[0].bushes+y)+x) != ' ');
    *(*(data[0].map+y)+x) = '*';
    pthread_mutex_unlock(&mutex);
    char copy = ' ';
    while(1){
        sem_wait(&data->rabbits[rabbit_id].sem);
        sem_wait(&data->rabbits[rabbit_id].sem);
        hunt_mode= 0;
        player = 0;
        
        pthread_mutex_lock(&mutex);

        for(int id=0;id<4;++id){
            if(data[id].state == ONLINE){
                for(int i=-2;i<=2;++i){
                    for(int j=-2;j<=2;++j){
                        if(x == data[id].x+i && y == data[id].y+j){
                            if(data[id].x == data[id].x_spawn && data[id].y == data[id].y_spawn)
                                continue;
                            player_x = data[id].x;
                            player_y = data[id].y;
                            player = 1;
                            break;
                        }
                    }
                    if(player)
                        break;
                }
                if(player)
                    break;
            }
        }
    
        
        *(*(data[0].map+y)+x) = copy;
        if(player)
        {
            hunt_mode=BresenhamLine(x,y,player_x,player_y,data);
        }
        pthread_mutex_unlock(&mutex);
        if(!hunt_mode){
            if(*(*(data[0].bushes+y)+x) == '#' && turn){
                *(*(data[0].map+y)+x) = '*';
                turn=0;
            }
            else{
                do{
                    if(move_y == -1){
                        if(*(*(data[0].map+y)+x+1) != 'H'){
                            move_y=0;
                            move_x=1;
                        }
                        else if(*(*(data[0].map+y-1)+x) == 'H'){
                            move_y=0;
                            move_x=-1;
                        }
                    }
                    else if(move_x == 1){
                        if(*(*(data[0].map+y+1)+x) != 'H'){
                            move_y=1;
                            move_x=0;
                        }
                        else if(*(*(data[0].map+y)+x+1) == 'H'){
                            move_y=-1;
                            move_x=0;
                        }
                    }
                    else if(move_y == 1){
                        if(*(*(data[0].map+y)+x-1) != 'H'){
                            move_y=0;
                            move_x=-1;
                        }
                        else if(*(*(data[0].map+y+1)+x) == 'H'){
                            move_y=0;
                            move_x=1;
                        }
                    }
                    else if(move_x == -1){
                        if(*(*(data[0].map+y-1)+x) != 'H'){
                            move_y=-1;
                            move_x=0;
                        }
                        else if(*(*(data[0].map+y)+x-1) == 'H'){
                            move_y=1;
                            move_x=0;
                        }
                    }
                turn = 1;
                }while(*(*(data[0].map+y+move_y)+x+move_x) == 'H');
                
                pthread_mutex_lock(&mutex);
                if(*(*(data[0].map+y+move_y)+x+move_x) == '*'){
                    move_x/=-1;
                    move_y/=-1;
                }
                x+=move_x;
                y+=move_y;
                if(*(*(data[0].map+y)+x) == 'H'){
                    x-=move_x;
                    y-=move_y;
                    move_x=1;
                    move_y=0;

                }
                else{
                    
                    if(isdigit(*(*(data[0].map+y)+x))){
                        int player_id = *(*(data[0].map+y)+x) - '0' -1;
                        if(data[player_id].carried>0){
                            copy = 'D';
                            struct node_t* node = data[0].ll->head;
                            int is = 0;
                            while(node)
                            {
                                if(node->x == x && node->y == y){
                                    is=1;
                                }
                                node=node->next;
                            }
                            if(!is){
                                int ups =ll_push_back(data[0].ll,data[player_id].carried,x,y);
                                err(ups == 2,"Drop_alloc");
                            }
                        }
                        else
                            copy=' ';
                        data[player_id].x = data[player_id].x_spawn;
                        data[player_id].y = data[player_id].y_spawn;
                        data[player_id].ptr->x = data[player_id].x_spawn;
                        data[player_id].ptr->y = data[player_id].y_spawn;
                        data[player_id].carried = 0;
                        data[player_id].ptr->deaths++; 
                    }
                    else{
                        if(*(*(data[0].map+y)+x) == '*')
                            copy = 'c';
                        else
                            copy = *(*(data[0].map+y)+x);
                    }
                    *(*(data[0].map+y)+x) = '*';
                }

                pthread_mutex_unlock(&mutex);
            }
        }
        else{
            if(y>player_y){
                move_y=-1;
                move_x=0;
                if(*(*(data[0].map+y+move_y)+x+move_x) == 'H'){
                    if(x>player_x){
                    move_y=0;
                    move_x=-1;
                    }
                    else if(x<player_x){
                        move_y=0;
                        move_x=1;
                    }
                    else{
                        move_y=0;
                        move_x=0; 
                    }
                }
            }
            else if(y<player_y){
                move_y=1;
                move_x=0;
                if(*(*(data[0].map+y+move_y)+x+move_x) == 'H'){
                    if(x>player_x){
                    move_y=0;
                    move_x=-1;
                    }
                    else if(x<player_x){
                        move_y=0;
                        move_x=1;
                    }
                    else{
                        move_y=0;
                        move_x=0; 
                    }
                }
            }
            else{
                if(x>player_x){
                    move_y=0;
                    move_x=-1;
                    if(*(*(data[0].map+y+move_y)+x+move_x) == 'H'){
                        if(y>player_y){
                            move_y=-1;
                            move_x=0;
                        }
                        else if(y<player_y){
                            move_y=1;
                            move_x=0;
                        }
                        else{
                            move_y=0;
                            move_x=0; 
                        }
                    }
                }
                else if(x<player_x){
                    move_y=0;
                    move_x=1;
                    if(*(*(data[0].map+y+move_y)+x+move_x) == 'H'){
                        if(y>player_y){
                            move_y=-1;
                            move_x=0;
                        }
                        else if(y<player_y){
                            move_y=1;
                            move_x=0;
                        }
                        else{
                            move_y=0;
                            move_x=0; 
                        }
                    }
                }
                else{
                    move_y=0;
                    move_x=0; 
                }
            }
            x+=move_x;
            y+=move_y;
            pthread_mutex_lock(&mutex);
            if(isdigit(*(*(data[0].map+y)+x))){
                int player_id = *(*(data[0].map+y)+x) - '0' -1;
                if(data[player_id].carried>0){
                    copy = 'D';
                    struct node_t* node = data[0].ll->head;
                    int is = 0;
                    while(node)
                    {
                        if(node->x == x && node->y == y){
                            is=1;
                        }
                        node=node->next;
                    }
                    if(!is){
                        int ups = ll_push_back(data[player_id].ll,data[player_id].carried + data[player_id].carried,data[player_id].x,data[player_id].y);
                        err(ups == 2,"Drop_alloc");
                    }
                }
                else
                    copy=' ';
                data[player_id].x = data[player_id].x_spawn;
                data[player_id].y = data[player_id].y_spawn;
                data[player_id].ptr->x = data[player_id].x_spawn;
                data[player_id].ptr->y = data[player_id].y_spawn;
                data[player_id].carried = 0;
                data[player_id].ptr->deaths++; 
            }
            else
            {
                if(*(*(data[0].map+y)+x) == '*')
                    copy = 'c';
                else
                    copy = *(*(data[0].map+y)+x);
            }
            *(*(data[0].map+y)+x) = '*';
            pthread_mutex_unlock(&mutex);
        }
    }
    return NULL;
}
int BresenhamLine(const int x1, const int y1, const int x2, const int y2, const struct data_t *data)
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
    if(*(*(data[0].map+y)+x) == 'H')
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
    if(*(*(data[0].map+y)+x) == 'H')
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
        if(*(*(data[0].map+y)+x) == 'H')
            return 0;
     }
 }
 return 1;
}
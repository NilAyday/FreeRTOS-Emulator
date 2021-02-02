#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"
#include "TUM_Sound.h"
#include "AsyncIO.h"
#include "tetris.h"

#define STATE_QUEUE_LENGTH 1
#define STATE_COUNT 2
#define STATE_ONE 0
#define STATE_TWO 1
#define STATE_THREE 2

#define NEXT_TASK 0
#define PREV_TASK 1

#define STARTING_STATE STATE_ONE

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#define STATE_DEBOUNCE_DELAY 300

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR

#define LOGO_FILENAME "../resources/images/tetris_logo.jpg"

#define GRID_X 12
#define GRID_Y 23
#define SHAPE_X 6
#define SHAPE_Y 5
#define BLOCK_TYPE 7
#define LIST_SIZE 10
#define STRING_WIDTH 50
#define NUMBER_OF_MAX_SCORE 3
#define KEY_RIGHT 79
#define KEY_LEFT  80
#define KEY_DOWN  81
#define SPEED_CONST1 500
#define SPEED_CONST2 40
#define MAX_LEVEL 9
#define LEVEL_UP_CONST 60
#define QUEUE_SIZE 20
#define QUEUE_WIDTH 20
#define BLOCK_SIZE 20
#define GRID_OFFSET_X 200
#define GRID_OFFSET_Y 20
#define TASK_DELAY 10

#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

static image_handle_t logo_image=NULL;

static TaskHandle_t StateMachine = NULL;
static TaskHandle_t BufferSwap = NULL;
static TaskHandle_t DemoTask = NULL;
static TaskHandle_t MenuTask = NULL;
static TaskHandle_t PausedStateTask = NULL;
static TaskHandle_t GameOverStateTask = NULL;
static TaskHandle_t NoConnectionTask = NULL;
static TaskHandle_t HandleTask= NULL;
static TaskHandle_t HandleUpdateY= NULL;
static TaskHandle_t HandleUpdateX= NULL;

static SemaphoreHandle_t syncSignal=NULL;
static QueueHandle_t     StateQueue = NULL;
static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;

/** AsyncIO related */
#define UDP_BUFFER_SIZE   1024
#define UDP_RECEIVE_PORT  1234
#define UDP_TRANSMIT_PORT 1235

static SemaphoreHandle_t HandleUDP=NULL;
static aIO_handle_t udp_soc_receive=NULL; 
static TaskHandle_t UDPControlTask = NULL;

static QueueHandle_t SendQueue=NULL;
static QueueHandle_t ReceiveQueue=NULL;
static QueueHandle_t ModeReceiveQueue=NULL;
static QueueHandle_t ListQueue=NULL;
static QueueHandle_t SeedQueue=NULL;

struct two_player{
    int flag;
    SemaphoreHandle_t lock;
};
struct two_player instance_two_player={.lock=NULL};

struct my_Y{
    int condition_Y2;
    int condition_Y;
};
struct my_struct_Y{
    SemaphoreHandle_t lock;
    struct my_Y my_Y_instance;
};
struct my_struct_Y my_struct_instance_Y={.lock=NULL};

struct crd{
    int y;
    int py;
    int x;
    int px;
};

struct my_coord{
    SemaphoreHandle_t lock;
    struct crd coord_instance;
};

struct my_coord my_coord_instance={.lock=NULL};

struct board{
    int level;
    int total_line;
    int score;
    int max[NUMBER_OF_MAX_SCORE];
};

struct my_board{
    SemaphoreHandle_t lock;
    struct board board_instance;
};

struct my_board my_board_instance={.lock=NULL};

struct my_struct_tetri{
    SemaphoreHandle_t lock_tetri;
    int Tetri_number;
};

struct my_struct_tetri my_struct_instance_tetri={.lock_tetri=NULL};

struct my_struct_grid{
    SemaphoreHandle_t lock_grid;
    int Grid[GRID_X][GRID_Y];
};

struct my_struct_grid my_struct_instance_grid={.lock_grid=NULL};

struct my_struct_frame{
    SemaphoreHandle_t lock_frame;
    int Frame[GRID_X][GRID_Y];
};

struct my_struct_frame my_struct_instance_frame={.lock_frame=NULL};

struct my_struct_shape{
    SemaphoreHandle_t lock_shape;
    int Shape[SHAPE_X][SHAPE_Y];
};
struct my_struct_shape my_struct_instance_shape={.lock_shape=NULL};

struct next_shape{
    SemaphoreHandle_t lock;
    int NextShape[SHAPE_X][SHAPE_Y];
};
struct next_shape instance_next_shape={.lock=NULL};

struct block{
    SemaphoreHandle_t lock;
    int Block[BLOCK_TYPE];
};
struct block instance_block={.lock=NULL};

struct U_args{
    int i;
    char List[LIST_SIZE][STRING_WIDTH]; 
};

QueueHandle_t Que_cond_Y = NULL;
QueueHandle_t Que_cond_X_R = NULL;
QueueHandle_t Que_cond_X_L = NULL;

typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };

void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

void choosetetri(char tetri)
{
    if(xSemaphoreTake(instance_next_shape.lock,portMAX_DELAY)==pdTRUE)
    {   
        choose_tetri(SHAPE_X, SHAPE_Y, tetri, (int *) &instance_next_shape.NextShape);         
        xSemaphoreGive(instance_next_shape.lock);
    }
}

int movecondition(int x, int y)
{
    int flag=0;
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    { 
            if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
            {
                flag=move_condition(SHAPE_X,SHAPE_Y, GRID_X, GRID_Y,x,y,(int *) &my_struct_instance_shape.Shape, (int *) &my_struct_instance_grid.Grid);  
                xSemaphoreGive(my_struct_instance_grid.lock_grid);
            } 
            xSemaphoreGive(my_struct_instance_shape.lock_shape);
    } 
    return flag;         
}

void rotate_R(void)
{
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)   
    { 
            rotate_tetrimino_R(SHAPE_X,SHAPE_Y, (int *) &my_struct_instance_shape.Shape);    
            xSemaphoreGive(my_struct_instance_shape.lock_shape);
    }
}

void rotate_L(void)
{
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)   
    { 
            rotate_tetrimino_L(SHAPE_X,SHAPE_Y, (int *) my_struct_instance_shape.Shape);    
            xSemaphoreGive(my_struct_instance_shape.lock_shape);
    }
}

void movetetrimino()
{
    if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
    {     
        if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
        {         
            if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
            {       
                move_tetrimino(SHAPE_X,SHAPE_Y,GRID_X,GRID_Y,
                                my_coord_instance.coord_instance.x,my_coord_instance.coord_instance.y,
                                (int *) &my_struct_instance_shape.Shape,(int *) &my_struct_instance_frame.Frame);
                xSemaphoreGive(my_coord_instance.lock);
            }
            xSemaphoreGive(my_struct_instance_shape.lock_shape);
        }
        xSemaphoreGive(my_struct_instance_frame.lock_frame);
    }
}

int linedisappear(void)
{
    int count_line;
    if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
    {
        count_line=line_disappear(GRID_X, GRID_Y,(int *) &my_struct_instance_grid.Grid);
        xSemaphoreGive(my_struct_instance_grid.lock_grid);
    }
    return count_line;
}

void score(int count_line)
{
    int factor=0;

    switch(count_line)
    {
        case 0: 
            factor=0;
            break;
        case 1:
            factor=40;
            break;
        case 2:
            factor=100;
            break;
        case 3:
            factor=300;
            break;
        case 4:
            factor=1200;
            break;
        default:
            break;   
    } 
    if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
    {
        my_board_instance.board_instance.total_line=my_board_instance.board_instance.total_line+count_line;
        my_board_instance.board_instance.score=my_board_instance.board_instance.score+factor*(my_board_instance.board_instance.level+1);

        if((my_board_instance.board_instance.score/LEVEL_UP_CONST)>my_board_instance.board_instance.level)
        {
            my_board_instance.board_instance.level++;
            if (my_board_instance.board_instance.level>MAX_LEVEL) my_board_instance.board_instance.level=MAX_LEVEL;
        }
        xSemaphoreGive(my_board_instance.lock);
    }
}

void copy_frametogrid(void)
{
    if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
    {
        for(int i=0;i<GRID_X; i++)
        {
            for(int j=0; j<GRID_Y; j++)
            {
                if(my_struct_instance_grid.Grid[i][j]==0 && my_struct_instance_frame.Frame[i][j]!=0)
                {
                    my_struct_instance_grid.Grid[i][j]=my_struct_instance_frame.Frame[i][j];
                }
                my_struct_instance_frame.Frame[i][j]=0;
            }
        }
        xSemaphoreGive(my_struct_instance_grid.lock_grid);
    }
}

void copy_nextshapetoshape(void)
{
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    {   
        if(xSemaphoreTake(instance_next_shape.lock,portMAX_DELAY)==pdTRUE)
        { 
            for(int i=0; i<SHAPE_X;i++)
            {
                for(int j=0; j<SHAPE_Y; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=instance_next_shape.NextShape[i][j];
                }
            }
            xSemaphoreGive(instance_next_shape.lock);
        }
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
    }
}

void reset_grid()
{
    if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
    {
        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
        {   
            for(int i=0; i<GRID_X; i++)
            {   
                for(int j=0; j<GRID_Y; j++)
                {
                    my_struct_instance_frame.Frame[i][j]= 0;
                    my_struct_instance_grid.Grid[i][j]= 0;
                    my_struct_instance_grid.Grid[0][j]=1;
                    my_struct_instance_grid.Grid[GRID_X-1][j]=1;
                }
                my_struct_instance_grid.Grid[i][GRID_Y-1]=1;
            }
            xSemaphoreGive(my_struct_instance_grid.lock_grid);
        }              
        xSemaphoreGive(my_struct_instance_frame.lock_frame);
    }
}

void reset_shape(void)
{
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    { 
        for(int i=0; i<SHAPE_X;i++)
        {
            for(int j=0; j<SHAPE_Y;j++)
            {
                my_struct_instance_shape.Shape[i][j]=0;
            }
        }  
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
    } 
}

int Tetrimino(int Tetrimino_number)
{
    static char send_value[STRING_WIDTH];
    static char recv_value[STRING_WIDTH];
    char Tetri;
    int cok=0;                      // 1 communication ok, 0 no communication

    if(xSemaphoreTake(instance_two_player.lock,portMAX_DELAY)==pdTRUE)
    {
        if(instance_two_player.flag==0)
        {
            if(xSemaphoreTake(instance_block.lock,portMAX_DELAY)==pdTRUE)
            {
                Tetri=RGenerator(Tetrimino_number, BLOCK_TYPE, instance_block.Block);
                xSemaphoreGive(instance_block.lock);
            }
        }
        else 
        {
            sprintf(send_value,"NEXT");
            xQueueSend(SendQueue,(void *)&send_value,0);
            if(xQueueReceive(ReceiveQueue,(int *) &recv_value,500)==pdTRUE)
            {
                Tetri=*recv_value;
            }
            else
            {
                cok=1;
                Tetri='O';
            }             
        }
        xSemaphoreGive(instance_two_player.lock);
    }
    copy_nextshapetoshape();
    choosetetri(Tetri);
    return cok;
}

void reset()
{
    if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
    {
        my_board_instance.board_instance.total_line=0;
        my_board_instance.board_instance.score=0;
        xSemaphoreGive(my_board_instance.lock);
    }

    if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
    {
        my_coord_instance.coord_instance.px=GRID_X/2-1;
        my_coord_instance.coord_instance.py=0;
        my_coord_instance.coord_instance.x=GRID_X/2-1;
        my_coord_instance.coord_instance.y=0;
        xSemaphoreGive(my_coord_instance.lock);
    }

    reset_grid();
    reset_shape();

    Tetrimino(0);
    Tetrimino(1);
}

void changeState(volatile unsigned char *state, unsigned char forwards)
{
     switch (forwards) {
        case NEXT_TASK:
            if (*state == STATE_COUNT - 1) {
                *state = 0;
            }
            else {
                (*state)++;
            }
            break;
        case PREV_TASK:
            if (*state == 0) {
                *state = STATE_COUNT - 1;
            }
            else {
                (*state)--;
            }
            break;
        default:
            break;
    }
}

void basicSequentialStateMachine(void *pvParameters)
{
    unsigned char state_changed = 1; // Only re-evaluate state if it has changed
    unsigned char input = 0;

    const int state_change_period = STATE_DEBOUNCE_DELAY;

    int flag_pause=0;
    
    TickType_t last_change = xTaskGetTickCount();

    while (1) {
        if (state_changed) {
            goto initial_state;
        }
        
        // Handle state machine input
        if (StateQueue)
            if (xQueueReceive(StateQueue, &input, portMAX_DELAY) ==
                pdTRUE)
                if (xTaskGetTickCount() - last_change >
                    state_change_period) {
                    //changeState(&current_state, input);
                    state_changed = 1;
                    last_change = xTaskGetTickCount();
                }
         
initial_state:
        // Handle current state
        if (state_changed) {
            switch (input) {
                case STATE_ONE:
                    if (DemoTask)         vTaskSuspend(DemoTask);
                    if (HandleTask)       vTaskSuspend(HandleTask);
                    if (HandleUpdateX)    vTaskSuspend(HandleUpdateX);
                    if (HandleUpdateY)    vTaskSuspend(HandleUpdateY);
                    if (NoConnectionTask) vTaskSuspend(NoConnectionTask);
                    if (UDPControlTask)   vTaskSuspend(UDPControlTask);
                    if (PausedStateTask)  vTaskSuspend(PausedStateTask);
                    if (MenuTask)         vTaskResume(MenuTask);
                    xSemaphoreTake(instance_two_player.lock,0);
                    instance_two_player.flag=0;
                    xSemaphoreGive(instance_two_player.lock); 
                    break;
                case 'O':
                    reset();
                    if (MenuTask)         vTaskSuspend(MenuTask);
                    if (DemoTask)         vTaskResume(DemoTask);
                    if (HandleTask)       vTaskResume(HandleTask);
                    if (HandleUpdateX)    vTaskResume(HandleUpdateX);
                    if (HandleUpdateY)    vTaskResume(HandleUpdateY);
                    break;
                case 'T':
                    if (MenuTask)         vTaskSuspend(MenuTask);
                    if (UDPControlTask)   vTaskResume(UDPControlTask);
                    break;
                case 'E':
                    xSemaphoreTake(instance_two_player.lock,0);
                    instance_two_player.flag=0;
                    xSemaphoreGive(instance_two_player.lock);
                    if (DemoTask)         vTaskSuspend(DemoTask);
                    if (HandleTask)       vTaskSuspend(HandleTask);
                    if (HandleUpdateX)    vTaskSuspend(HandleUpdateX);
                    if (HandleUpdateY)    vTaskSuspend(HandleUpdateY);
                    if (NoConnectionTask) vTaskSuspend(NoConnectionTask);
                    if (UDPControlTask)   vTaskSuspend(UDPControlTask);
                    if (GameOverStateTask) vTaskSuspend(GameOverStateTask);
                    if (MenuTask)         vTaskResume(MenuTask);
                    //reset();
                    break;
                case 'R':
                    reset();
                    break;
                case 'P':
                    if(flag_pause==0)
                    {
                        if (DemoTask)         vTaskSuspend(DemoTask);
                        if (HandleTask)       vTaskSuspend(HandleTask);
                        if (HandleUpdateX)    vTaskSuspend(HandleUpdateX);
                        if (HandleUpdateY)    vTaskSuspend(HandleUpdateY);
                        if (PausedStateTask)  vTaskResume(PausedStateTask);
                        flag_pause=1;
                    }
                    else
                    {
                        if (PausedStateTask)  vTaskSuspend(PausedStateTask);
                        if (DemoTask)         vTaskResume(DemoTask);
                        if (HandleTask)       vTaskResume(HandleTask);
                        if (HandleUpdateX)    vTaskResume(HandleUpdateX);
                        if (HandleUpdateY)    vTaskResume(HandleUpdateY);
                        flag_pause=0;
                    } 
                    break;
                case 'C':
                    if (UDPControlTask)   vTaskSuspend(UDPControlTask);
                    if (DemoTask)         vTaskSuspend(DemoTask);
                    if (HandleTask)       vTaskSuspend(HandleTask);
                    if (HandleUpdateX)    vTaskSuspend(HandleUpdateX);
                    if (HandleUpdateY)    vTaskSuspend(HandleUpdateY);
                    if (NoConnectionTask) vTaskResume(NoConnectionTask);
                    break;
                case 'S':
                    reset();
                    if (NoConnectionTask) vTaskSuspend(NoConnectionTask);
                    if (DemoTask)         vTaskResume(DemoTask);
                    if (HandleTask)       vTaskResume(HandleTask);
                    if (HandleUpdateX)    vTaskResume(HandleUpdateX);
                    if (HandleUpdateY)    vTaskResume(HandleUpdateY); 
                    if (UDPControlTask)   vTaskResume(UDPControlTask);
                    xSemaphoreTake(instance_two_player.lock,0);
                    instance_two_player.flag=1;
                    xSemaphoreGive(instance_two_player.lock);
                    break;
                case 'G':
                    if (DemoTask)         vTaskSuspend(DemoTask);
                    if (HandleTask)       vTaskSuspend(HandleTask);
                    if (HandleUpdateX)    vTaskSuspend(HandleUpdateX);
                    if (HandleUpdateY)    vTaskSuspend(HandleUpdateY);
                    if(GameOverStateTask) vTaskResume(GameOverStateTask);
                    break;
                default:
                    break;
            }
            state_changed = 0;
        }
    }
}

void vSwapBuffers(void *pvParameters)
{
    TickType_t xLastWakeTime;
    xLastWakeTime = xTaskGetTickCount();
    const TickType_t frameratePeriod = 20;

    tumDrawBindThread(); // Setup Rendering handle with correct GL context

    while (1) 
    {
        if (xSemaphoreTake(ScreenLock, portMAX_DELAY) == pdTRUE) {
            tumDrawUpdateScreen();
            tumEventFetchEvents(FETCH_EVENT_BLOCK);
            xSemaphoreGive(ScreenLock);
            xSemaphoreGive(DrawSignal);
            vTaskDelayUntil(&xLastWakeTime,
                            pdMS_TO_TICKS(frameratePeriod));
        }
    }
}

static int vCheckStateInputMenu(void)
{
    unsigned char next_state_signal;
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if (buttons.buttons[KEYCODE(O)]) {
            buttons.buttons[KEYCODE(O)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                next_state_signal='O';
                xQueueSend(StateQueue, &next_state_signal, 0);
                return 0;
            }
            return -1;
        }
        if (buttons.buttons[KEYCODE(T)]) {
            buttons.buttons[KEYCODE(T)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                next_state_signal='T';
                xQueueSend(StateQueue, &next_state_signal, 0);
                return 0;
            }
            return -1;
        }
        xSemaphoreGive(buttons.lock);
    }
    return 0;
}

static int vCheckStateInputTask(void)
{
    unsigned char next_state_signal;
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if (buttons.buttons[KEYCODE(E)]) {
            buttons.buttons[KEYCODE(E)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                next_state_signal='E';
                xQueueSend(StateQueue, &next_state_signal, 0);
                return 0;
            }
            return -1;
        }
        if (buttons.buttons[KEYCODE(R)]) {
            buttons.buttons[KEYCODE(R)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                next_state_signal='R';
                xQueueSend(StateQueue, &next_state_signal, 0);
                return 0;
            }
            return -1;
        }
        if (buttons.buttons[KEYCODE(P)]) {
            buttons.buttons[KEYCODE(P)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                next_state_signal='P';
                xQueueSend(StateQueue, &next_state_signal, 0);
                return 0;
            }
            return -1;
        }
        xSemaphoreGive(buttons.lock);
    }
    return 0;
}

static int vCheckConnectionTask(void)
{

    unsigned char next_state_signal;
    
    if(xSemaphoreTake(instance_two_player.lock,portMAX_DELAY)==pdTRUE)
    {
        if(instance_two_player.flag==3)
        {
            if (StateQueue) {
            xSemaphoreGive(instance_two_player.lock);
            next_state_signal='S';
            xQueueSend(StateQueue, &next_state_signal, 0);
            return 0;
            }
        }
        if(instance_two_player.flag==2)
        {
            if (StateQueue) {
            xSemaphoreGive(instance_two_player.lock);
            next_state_signal='C';
            xQueueSend(StateQueue, &next_state_signal, 0);
            return 0;
            }
        }
        xSemaphoreGive(instance_two_player.lock);
    }
    return 0;
}

void UpdateY()
{
    int delay;
    int Flag_D=1;
    int cx,cy;
    int level;

    while(1)
    {
        xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY);
        my_coord_instance.coord_instance.py=my_coord_instance.coord_instance.y;
        cx=my_coord_instance.coord_instance.x;
        cy=my_coord_instance.coord_instance.y; 
        xSemaphoreGive(my_coord_instance.lock);
       
        if(movecondition(cx,cy+1)==0)
        {
            if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
            {  
                my_coord_instance.coord_instance.y++;
                xSemaphoreGive(my_coord_instance.lock);
            }
            if(xSemaphoreTake(my_struct_instance_Y.lock,portMAX_DELAY)==pdTRUE)
            {
                my_struct_instance_Y.my_Y_instance.condition_Y2=0;  
                xSemaphoreGive(my_struct_instance_Y.lock);
            }        
        }
        else
        {
            if(xSemaphoreTake(my_struct_instance_Y.lock,portMAX_DELAY)==pdTRUE)
            {
                my_struct_instance_Y.my_Y_instance.condition_Y2++;  
                xSemaphoreGive(my_struct_instance_Y.lock);
            }
        }
        
        if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
        {
            if(level!=my_board_instance.board_instance.level)
            {
                level=my_board_instance.board_instance.level;
                Flag_D=1;
            }
            xSemaphoreGive(my_board_instance.lock);
        }

        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
            xGetButtonInput();
            xSemaphoreGive(buttons.lock);
            if (buttons.buttons[KEY_DOWN]) 
            { 
                if(Flag_D==0)
                {
                    Flag_D=1;
                    delay=(SPEED_CONST1-SPEED_CONST2*level)/2; 
                }       
            }
            else 
            {
                if(Flag_D==1)
                {
                    Flag_D=0;
                    delay=SPEED_CONST1-SPEED_CONST2*level;
                }
            } 
        }  
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}

void UpdateX()
{
    int Flag_A=0;
    int Flag_B=0;
    int cx,cy;

    while(1)
    {
        if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
        {
            my_coord_instance.coord_instance.px=my_coord_instance.coord_instance.x;
            cx=my_coord_instance.coord_instance.x;
            cy=my_coord_instance.coord_instance.y;  
            xSemaphoreGive(my_coord_instance.lock);
        }          
        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE)
        {
            xGetButtonInput();
            xSemaphoreGive(buttons.lock);
            if (buttons.buttons[KEY_RIGHT]) 
            { 
                if(movecondition(cx+1,cy)==0)
                {
                    if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                    {  
                        my_coord_instance.coord_instance.x++;
                        xSemaphoreGive(my_coord_instance.lock);
                    }
                }
            }
            if (buttons.buttons[KEY_LEFT]) 
            { 
                if(movecondition(cx-1,cy)==0)
                {
                    if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                    {  
                        my_coord_instance.coord_instance.x--;
                        xSemaphoreGive(my_coord_instance.lock);
                    }
                }
            } 
            
            if ((buttons.buttons[KEYCODE(A)]) & (Flag_A==0))
            { 
                Flag_A=1;
                rotate_R();
                if(movecondition(cx,cy)==1)
                {
                    rotate_L();
                }              
            } 
            else Flag_A=0;
            
            if ((buttons.buttons[KEYCODE(B)]) & (Flag_B==0))
            { 
                Flag_B=1;
                rotate_L();
                if(movecondition(cx,cy)==1)
                {
                    rotate_R(); 
                }            
            }
            else Flag_B=0;
        }  
        vTaskDelay(pdMS_TO_TICKS(10*TASK_DELAY));
    }
}

void Task(void *pvParameters)
{
    unsigned char next_state_signal;
    int Tetrimino_number=1;
    int count_line;
    int flag=0;
    int cok=0;                      // 1 communication ok, 0 no communication

    while(1)
    {
        cok=0;
        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
        {
            if ((my_struct_instance_grid.Grid[GRID_X/2][3]==0) && (my_struct_instance_grid.Grid[GRID_X/2+1][3]==0) && (my_struct_instance_grid.Grid[GRID_X/2+2][3]==0) &&
                (my_struct_instance_grid.Grid[GRID_X/2][4]==0) && (my_struct_instance_grid.Grid[GRID_X/2+1][4]==0) && (my_struct_instance_grid.Grid[GRID_X/2+2][4]==0))
            {
                flag=0;   
            }
            else
            {
                flag=1;
            }              
            xSemaphoreGive(my_struct_instance_grid.lock_grid);
        }  

        if (flag==0)
        {
            movetetrimino();
        }

        if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
        { 
                if(xSemaphoreTake(my_struct_instance_Y.lock,portMAX_DELAY)==pdTRUE)
                {       
                    if(my_struct_instance_Y.my_Y_instance.condition_Y2>=2)
                    {
                        my_struct_instance_Y.my_Y_instance.condition_Y2=0;      

                        if ( (movecondition((GRID_X/2-1),0)==0) && (flag==0))
                        {
                            Tetrimino_number=(Tetrimino_number+1)%BLOCK_TYPE;    
                            cok=Tetrimino(Tetrimino_number);
                            
                            if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                            {
                                my_coord_instance.coord_instance.px=GRID_X/2-1;
                                my_coord_instance.coord_instance.py=0;
                                my_coord_instance.coord_instance.x=GRID_X/2-1;
                                my_coord_instance.coord_instance.y=0;
                                xSemaphoreGive(my_coord_instance.lock);
                            }   
                            copy_frametogrid();
                        }
                        count_line=linedisappear();
                        score(count_line);
                    } 
                    xSemaphoreGive(my_struct_instance_Y.lock);   
                }   
                xSemaphoreGive(my_struct_instance_frame.lock_frame);
         }
        if(flag==1)
        {
             next_state_signal='G';
                    xQueueSendToFront(
                        StateQueue,
                        &next_state_signal,
                        portMAX_DELAY);
        }
        if (cok==1)
        {
            next_state_signal='C';
                    xQueueSendToFront(
                        StateQueue,
                        &next_state_signal,
                        portMAX_DELAY);
        }
        vTaskDelay(pdMS_TO_TICKS(2*TASK_DELAY));      
    }
}

void vGameOverStateTask(void *pvParameters)
{
    unsigned char next_state_signal;
    char str[STRING_WIDTH];
    int flag=0;
    
    static const char *gameover_text = "GAME OVER";
    static int gameover_text_width;
    static int gameover_text_height;

    tumGetTextSize((char *)gameover_text, &gameover_text_width, &gameover_text_height);
  
    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY)==pdTRUE) 
            {
                xGetButtonInput(); // Update global button data

                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(E)]) {
                        xSemaphoreGive(buttons.lock);
                        next_state_signal='E';
                        flag=0;
                        xQueueSendToFront(
                            StateQueue,
                            &next_state_signal,
                            portMAX_DELAY);
                    }
                    xSemaphoreGive(buttons.lock);
                }
                if (xSemaphoreTake(ScreenLock, 0) == pdTRUE) {
                    
                    tumDrawClear(Silver);
                    tumDrawText((char *)gameover_text,
                                SCREEN_WIDTH / 2 -
                                gameover_text_width /
                                2,
                                SCREEN_HEIGHT/4, Red);
                }
                
                if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                {
                    if(flag==0)
                    {
                        max_scores(my_board_instance.board_instance.score,(int *) &my_board_instance.board_instance.max);
                        flag=1;
                    }
                    xSemaphoreGive(my_board_instance.lock);
                }
               
                tumDrawText("[E]xit",50,20,Black);
                tumDrawText("High Scores", SCREEN_WIDTH/2-gameover_text_width /2,SCREEN_HEIGHT/2, White);
                if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                {    
                    sprintf(str,"%d",my_board_instance.board_instance.max[0] );
                    tumDrawText(str, SCREEN_WIDTH/2-gameover_text_width /2,SCREEN_HEIGHT/2+gameover_text_height, White);
                    sprintf(str,"%d",my_board_instance.board_instance.max[1] );
                    tumDrawText(str, SCREEN_WIDTH/2-gameover_text_width /2,SCREEN_HEIGHT/2+2*gameover_text_height, White);
                    sprintf(str,"%d",my_board_instance.board_instance.max[2] );
                    tumDrawText(str, SCREEN_WIDTH/2-gameover_text_width /2,SCREEN_HEIGHT/2+3*gameover_text_height, White);
                    xSemaphoreGive(my_board_instance.lock);
                }
                xSemaphoreGive(ScreenLock);
                vTaskDelay(pdMS_TO_TICKS(10*TASK_DELAY));
            }
        }
    }
}

void UDPHandler(size_t read_size, char *buffer, void *args)
{
    char Tetrimino;
    char sList[STRING_WIDTH];
    char Seed[STRING_WIDTH];
    char *M;
    
    struct U_args *my_args = (struct U_args * ) args;
   

    BaseType_t xHigherPriorityTaskWoken1 = pdFALSE;
    //BaseType_t xHigherPriorityTaskWoken2 = pdFALSE;
    BaseType_t xHigherPriorityTaskWoken3 = pdFALSE;

    if (xSemaphoreTakeFromISR(HandleUDP, &xHigherPriorityTaskWoken1) ==
        pdTRUE) {

        if(strncmp(buffer,"NEXT",strlen("NEXT"))==0)
        {
            Tetrimino=buffer[5];
            xQueueSendFromISR(ReceiveQueue, &Tetrimino,0);
        }
        if(strncmp(buffer,"SEED",strlen("SEED"))==0)
        {
            strcpy(Seed,buffer);
            xQueueSendFromISR(SeedQueue, &Seed,0);
            
        }
        if(strncmp(buffer,"LIST",strlen("LIST"))==0)
        {
            strcpy(sList,buffer);
            M= strtok(sList, "=,");
            my_args->i=0;
            while(M!=NULL)
            {
                strcpy(my_args->List[my_args->i],"MODE=");
                strcat(my_args->List[my_args->i++],M);
                M= strtok(NULL, ",");
            }
           for(int j=1;j<my_args->i;j++)
           {
                xQueueSendFromISR(ListQueue,&my_args->List[j],0);
           }     
        }
        if(strncmp(buffer,"MODE=",strlen("MODE="))==0)
        {
            for(int j=0;j<my_args->i;j++)
            {
                if(strncmp(buffer,my_args->List[j],read_size)==0)
                {
                    xQueueSendFromISR(ModeReceiveQueue,&my_args->List[j],0);
                }
            }
        } 
        xSemaphoreGiveFromISR(HandleUDP, &xHigherPriorityTaskWoken3);

        /*portYIELD_FROM_ISR(xHigherPriorityTaskWoken1 |
                           xHigherPriorityTaskWoken2 |
                           xHigherPriorityTaskWoken3);*/
    }
    else 
    {
        fprintf(stderr, "[ERROR] Overlapping UDPHandler call\n");
    }
}
void vUDPControlTask(void *pvParameters)
{
    static char send_value[STRING_WIDTH];
    char List[LIST_SIZE][STRING_WIDTH];
    int i=0;
    int j=0;
    int com_flag=0;

     char *addr =NULL;

     struct U_args my_args ={0};
     in_port_t port = UDP_RECEIVE_PORT;

    udp_soc_receive = aIOOpenUDPSocket(addr, port, UDP_BUFFER_SIZE, UDPHandler, (void *)&my_args);

    while(1)
    {
        xSemaphoreTake(instance_two_player.lock,0);
        if(instance_two_player.flag==0)
        {
            i=0;
            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, "LIST",strlen("LIST"));
            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, "MODE",strlen("MODE"));
            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, "SEED",strlen("SEED"));
            instance_two_player.flag=4;
            com_flag=0;
        }
        xSemaphoreGive(instance_two_player.lock);
        
        if(xQueueReceive(ListQueue,&List[i],0)==pdTRUE)
        {
            i++;   
            com_flag++;
        }
        if (com_flag==0)
        {
            com_flag=1;
            xSemaphoreTake(instance_two_player.lock,0);
            instance_two_player.flag=2;
            xSemaphoreGive(instance_two_player.lock);
        } 
        if (com_flag==2)
        {
            com_flag=3;
            xSemaphoreTake(instance_two_player.lock,0);
            instance_two_player.flag=3;
            xSemaphoreGive(instance_two_player.lock); 
        } 

        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
            if (buttons.buttons[KEYCODE(M)]) {
                buttons.buttons[KEYCODE(M)] = 0;
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, List[j],strlen(List[j]));
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, "MODE",strlen("MODE"));
                    j=(j+1)%i;
            }
            if (buttons.buttons[KEYCODE(T)]) {
                buttons.buttons[KEYCODE(T)] = 0;
                    aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, "RESET",strlen("RESET"));
            }
            xSemaphoreGive(buttons.lock);
        }
        if(xQueueReceive(SendQueue,&send_value,0)==pdTRUE)
        {
            aIOSocketPut(UDP, NULL, UDP_TRANSMIT_PORT, send_value,strlen(send_value));
        }
        vCheckConnectionTask();
        vTaskDelay(pdMS_TO_TICKS(5*TASK_DELAY));   
    }
}

void vMenuTask(void *pvParameters)
{
    char str[STRING_WIDTH];

    while(1)
    {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
                xGetButtonInput(); // Update global input
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                tumDrawClear(Grey); // Clear screen
                
                static int image_height;
                static int image_width;
                image_height = tumDrawGetLoadedImageHeight(logo_image);
                image_width  = tumDrawGetLoadedImageWidth(logo_image);
                
                tumDrawLoadedImage(logo_image,  SCREEN_WIDTH - image_width -30,
                                     SCREEN_HEIGHT - image_height);

                
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(L)]) {
                        buttons.buttons[KEYCODE(L)] = 0;
                        if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                        {
                            my_board_instance.board_instance.level=(my_board_instance.board_instance.level+1)%10;
                            xSemaphoreGive(my_board_instance.lock);
                        }
                    } 
                xSemaphoreGive(buttons.lock);
                }
            
                if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                {
                    sprintf(str, "[L]evel: %d",  my_board_instance.board_instance.level);
                    tumDrawText(str,
                        DEFAULT_FONT_SIZE * 10,
                        DEFAULT_FONT_SIZE * 16, White);
                    xSemaphoreGive(my_board_instance.lock);
                }
                
                tumDrawText("[O]ne Player",
                    DEFAULT_FONT_SIZE * 10,
                    DEFAULT_FONT_SIZE * 18, White);

                 tumDrawText("[T]wo Player",
                    DEFAULT_FONT_SIZE * 10,
                    DEFAULT_FONT_SIZE * 20, White);
            
                vTaskDelay(pdMS_TO_TICKS(10*TASK_DELAY));
                xSemaphoreGive(ScreenLock);
                vCheckStateInputMenu();
            }      
    }
}

void vDrawFPS(void)
{
    static unsigned int periods[FPS_AVERAGE_COUNT] = { 0 };
    static unsigned int periods_total = 0;
    static unsigned int index = 0;
    static unsigned int average_count = 0;
    static TickType_t xLastWakeTime = 0, prevWakeTime = 0;
    static char str[10] = { 0 };
    static int text_width;
    int fps = 0;
    font_handle_t cur_font = tumFontGetCurFontHandle();

    if (average_count < FPS_AVERAGE_COUNT) {
        average_count++;
    }
    else {
        periods_total -= periods[index];
    }

    xLastWakeTime = xTaskGetTickCount();

    if (prevWakeTime != xLastWakeTime) {
        periods[index] =
            configTICK_RATE_HZ / (xLastWakeTime - prevWakeTime);
        prevWakeTime = xLastWakeTime;
    }
    else {
        periods[index] = 0;
    }

    periods_total += periods[index];

    if (index == (FPS_AVERAGE_COUNT - 1)) {
        index = 0;
    }
    else {
        index++;
    }

    fps = periods_total / average_count;

    tumFontSelectFontFromName(FPS_FONT);

    sprintf(str, "FPS: %2d", fps);

    if (!tumGetTextSize((char *)str, &text_width, NULL))
         tumDrawText(str, SCREEN_WIDTH - text_width -40,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Black);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}



void vDemoTask(void *pvParameters)
{
    char str[STRING_WIDTH];
    char Mode[STRING_WIDTH];
    char Seed[STRING_WIDTH];

    unsigned int color;

    Que_cond_Y  = xQueueCreate(1,sizeof(int));
    Que_cond_X_R= xQueueCreate(1,sizeof(int));
    Que_cond_X_L= xQueueCreate(1,sizeof(int));  
   

    xTaskCreate(Task, "Task", mainGENERIC_STACK_SIZE, NULL,mainGENERIC_PRIORITY+2, &HandleTask);
    vTaskResume(HandleTask);
    xTaskCreate(UpdateY, "UpdateY", mainGENERIC_STACK_SIZE, NULL,mainGENERIC_PRIORITY+1, &HandleUpdateY);
    vTaskResume(HandleUpdateY);
    xTaskCreate(UpdateX, "UpdateX", mainGENERIC_STACK_SIZE, NULL,mainGENERIC_PRIORITY, &HandleUpdateX);
    vTaskResume(HandleUpdateX);
 
    syncSignal= xSemaphoreCreateBinary();

    while (1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
        
                xGetButtonInput(); // Update global input
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                tumDrawClear(Silver);
        
                if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
                {
                    for(int i=0;i<GRID_X;i++)
                    {
                        for(int j=2;j<GRID_Y;j++)
                        {
                            color=switch_color(my_struct_instance_frame.Frame[i][j]);
                            if(my_struct_instance_frame.Frame[i][j]!=0)
                            {
                                tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*j+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,color);
                                tumDrawBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*j+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,Black);
                            }
                            else
                                tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*j+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);
                        }
                    }
                xSemaphoreGive(my_struct_instance_frame.lock_frame);
                }
                if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
                {   
                    for(int i=0;i<GRID_X;i++)
                    {
                        for(int j=2;j<GRID_Y;j++)
                        {
                            color=switch_color(my_struct_instance_grid.Grid[i][j]);
                            if(my_struct_instance_grid.Grid[i][j]!=0)
                            {
                                tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*j+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,color);
                                tumDrawBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*j+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,Black);
                            }
                        }
                    }
                xSemaphoreGive(my_struct_instance_grid.lock_grid);
                }
                /*
                for(int i=12;i<20;i++)
                {
                    for (int j=0;j<23;j++)
                    {
                        tumDrawFilledBox(20*i+200,20*j+20,20,20,Silver);
                        tumDrawFilledBox(20*i-200,20*j+20,20,20,Silver);
                        tumDrawFilledBox(20*i-40,j+20,120,20,Silver);
                    }
                } 
                */
                for(int i=13;i<19;i++)
                {
                    tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*5 +GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);
                    tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*8 +GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);
                    tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*11+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);
                    for(int j=15; j<20;j++)
                    {
                        tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,BLOCK_SIZE*j+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);
                    }       
                }

                xSemaphoreTake(instance_two_player.lock,portMAX_DELAY);
                if(instance_two_player.flag==1)
                {
                    for(int i=13;i<22;i++)
                    {   
                            tumDrawFilledBox(BLOCK_SIZE*i-GRID_OFFSET_X-50,5*BLOCK_SIZE+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);
                            tumDrawFilledBox(BLOCK_SIZE*i-GRID_OFFSET_X-50,8*BLOCK_SIZE+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,White);   
                    }
                }
                xSemaphoreGive(instance_two_player.lock);

                if(xSemaphoreTake(instance_next_shape.lock,portMAX_DELAY)==pdTRUE)
                { 
                    for(int i=0; i<SHAPE_X;i++)
                    {
                        for(int j=0; j<SHAPE_Y; j++)
                        {
                            color=switch_color(instance_next_shape.NextShape[i][j]);
                            if(instance_next_shape.NextShape[i][j]!=0)
                            {
                                tumDrawFilledBox(BLOCK_SIZE*(i+13)+GRID_OFFSET_X,BLOCK_SIZE*(j+15)+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,color);
                                tumDrawBox(BLOCK_SIZE*(i+13)+GRID_OFFSET_X,BLOCK_SIZE*(j+15)+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,Black);
                            }
                        }
                    }
                    xSemaphoreGive(instance_next_shape.lock);
                }
                
                if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                {
                    sprintf(str, "LINES: %d", my_board_instance.board_instance.total_line);
                    tumDrawText(str, BLOCK_SIZE*13+GRID_OFFSET_X+10,5*BLOCK_SIZE+GRID_OFFSET_Y,Black);

                    sprintf(str, "LEVEL: %d", my_board_instance.board_instance.level);
                    tumDrawText(str, BLOCK_SIZE*13+GRID_OFFSET_X+10,8*BLOCK_SIZE+GRID_OFFSET_Y,Black);

                    sprintf(str, "SCORE: %d", my_board_instance.board_instance.score);
                    tumDrawText(str, BLOCK_SIZE*13+GRID_OFFSET_X+10,11*BLOCK_SIZE+GRID_OFFSET_Y,Black);
                    xSemaphoreGive(my_board_instance.lock);
                }
                
                tumDrawText("[E]xit [P]ause [R]estart", GRID_OFFSET_X-160,GRID_OFFSET_Y,Black);
                tumDrawText("[->] Right"      ,GRID_OFFSET_X-160,19*BLOCK_SIZE,Black);
                tumDrawText("[<-] Left"       ,GRID_OFFSET_X-160,20*BLOCK_SIZE,Black);
                tumDrawText(" | "             ,GRID_OFFSET_X-153,21*BLOCK_SIZE-5,Black);
                tumDrawText("[ v ] Faster"    ,GRID_OFFSET_X-160,21*BLOCK_SIZE,Black);
                tumDrawText("[A] Right Rotate",GRID_OFFSET_X-160,22*BLOCK_SIZE,Black);
                tumDrawText("[B] Left Rotate" ,GRID_OFFSET_X-160,23*BLOCK_SIZE,Black);

                xSemaphoreTake(instance_two_player.lock,portMAX_DELAY);
                if(instance_two_player.flag==1)
                {
                    tumDrawText("[M]ode", GRID_OFFSET_X-160 ,4*BLOCK_SIZE+GRID_OFFSET_Y,Black);

                    xQueueReceive(ModeReceiveQueue,&Mode,0);
                    sprintf(str, "%s", Mode);
                    tumDrawText(str, BLOCK_SIZE*13-GRID_OFFSET_X-40,5*BLOCK_SIZE+GRID_OFFSET_Y,Black);
                    

                    xQueueReceive(SeedQueue,&Seed,0);
                    sprintf(str, "%s", Seed);
                    tumDrawText(str, BLOCK_SIZE*13-GRID_OFFSET_X-40,8*BLOCK_SIZE+GRID_OFFSET_Y,Black);
                }
                xSemaphoreGive(instance_two_player.lock);
               
                //tumDrawUpdateScreen(); // Refresh the screen to draw string
                vDrawFPS();
                // Basic sleep of 1000 milliseconds
                xSemaphoreGive(ScreenLock);
                // Get input and check for state change
                vCheckStateInputTask();
        
                vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
            }
    }
}


void vPausedStateTask(void *pvParameters)
{
    static const char *paused_text = "PAUSED";
    static int paused_text_width;
    static int paused_text_height;

    tumGetTextSize((char *)paused_text, &paused_text_width, &paused_text_height);
    unsigned char next_state_signal;
    char str[16];

    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xGetButtonInput(); // Update global button data

                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(P)]) {
                        xSemaphoreGive(buttons.lock);
                        next_state_signal='P';
                        xQueueSendToFront(
                            StateQueue,
                            &next_state_signal,
                            portMAX_DELAY);
                    }
                    xSemaphoreGive(buttons.lock);
                }

                // Don't suspend task until current execution loop has finished
                // and held resources have been released
                // taskENTER_CRITICAL();
                

                if (xSemaphoreTake(ScreenLock, 0) == pdTRUE) {
                    
                    tumDrawClear(Silver);
                    tumDrawText((char *)paused_text,
                                SCREEN_WIDTH / 2 -
                                paused_text_width /
                                2,
                                SCREEN_HEIGHT / 2, Red);
                }

                for(int i=13;i<19;i++)
                {
                    tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,5*BLOCK_SIZE+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,Silver);
                    tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,8*BLOCK_SIZE+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,Silver);
                    tumDrawFilledBox(BLOCK_SIZE*i+GRID_OFFSET_X,11*BLOCK_SIZE+GRID_OFFSET_Y,BLOCK_SIZE,BLOCK_SIZE,Silver);     
                }
                
                if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                {
                    sprintf(str, "LINES: %d", my_board_instance.board_instance.total_line);
                    tumDrawText(str, SCREEN_WIDTH/2-paused_text_width /2,SCREEN_HEIGHT/2+2*paused_text_height,White);

                    sprintf(str, "LEVEL: %d", my_board_instance.board_instance.level);
                    tumDrawText(str, SCREEN_WIDTH/2-paused_text_width /2,SCREEN_HEIGHT/2+3*paused_text_height,White);

                    sprintf(str, "SCORE: %d", my_board_instance.board_instance.score);
                    tumDrawText(str, SCREEN_WIDTH/2-paused_text_width /2,SCREEN_HEIGHT/2+4*paused_text_height,White);
                    xSemaphoreGive(my_board_instance.lock);
                }

                tumDrawText("[P]lay",GRID_OFFSET_X-160,GRID_OFFSET_Y,Black);

                xSemaphoreGive(ScreenLock);

                //taskEXIT_CRITICAL();

                vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
            }
        }
    }
}


void vNoConnectionTask(void *pvParameters)
{
    static const char *connection_text = "NO CONNECTION";
    static int connection_text_width;

    tumGetTextSize((char *)connection_text, &connection_text_width, NULL);
    unsigned char next_state_signal;

    while (1) {
        if (DrawSignal) {
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                xGetButtonInput(); // Update global button data

                 xSemaphoreTake(instance_two_player.lock,0);
                 instance_two_player.flag=0;
                xSemaphoreGive(instance_two_player.lock);

                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(E)]) {
                        xSemaphoreGive(buttons.lock);
                        next_state_signal='E';
                        xQueueSendToFront(
                            StateQueue,
                            &next_state_signal,
                            portMAX_DELAY);
                    }
                    xSemaphoreGive(buttons.lock);
                }

                // Don't suspend task until current execution loop has finished
                // and held resources have been released
                // taskENTER_CRITICAL();

                if (xSemaphoreTake(ScreenLock, 0) == pdTRUE) {
                    tumDrawClear(Silver);

                    tumDrawText((char *)connection_text,
                                SCREEN_WIDTH / 2 -
                                connection_text_width /
                                2,
                                SCREEN_HEIGHT / 2, Red);
                    xSemaphoreGive(ScreenLock);
                }

                tumDrawText("[E]xit",GRID_OFFSET_X-160,GRID_OFFSET_Y,Black);
                xSemaphoreGive(DrawSignal);
                //taskEXIT_CRITICAL();
                vCheckConnectionTask();
                vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
            }
        }
    }
}

void vDrawLogo(void)
{
    static int image_height;
    static int image_width;
    if ((image_height = tumDrawGetLoadedImageHeight(logo_image)) != -1)
    {
        image_width=tumDrawGetLoadedImageWidth(logo_image);
        tumDrawLoadedImage(logo_image, SCREEN_WIDTH -10 - image_width,
                                        SCREEN_HEIGHT - 10 - image_height);
    }    
}

int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

    my_board_instance.lock=xSemaphoreCreateMutex();
    my_struct_instance_grid.lock_grid=xSemaphoreCreateMutex();
    my_struct_instance_frame.lock_frame=xSemaphoreCreateMutex();
    my_struct_instance_shape.lock_shape=xSemaphoreCreateMutex();
    my_struct_instance_tetri.lock_tetri=xSemaphoreCreateMutex();
    my_struct_instance_Y.lock=xSemaphoreCreateMutex();
    my_coord_instance.lock=xSemaphoreCreateMutex();
    instance_next_shape.lock=xSemaphoreCreateMutex();
    instance_block.lock=xSemaphoreCreateMutex();
    
    printf("Initializing: ");

    if (tumDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize drawing");
        goto err_init_drawing;
    }

    if (tumEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }

    if (tumSoundInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize audio");
        goto err_init_audio;
    }

    HandleUDP = xSemaphoreCreateMutex();
    if (!HandleUDP) {
        exit(EXIT_FAILURE);
    }

    buttons.lock = xSemaphoreCreateMutex(); // Locking mechanism
    if (!buttons.lock) {
        PRINT_ERROR("Failed to create buttons lock");
        goto err_buttons_lock;
    }

    DrawSignal = xSemaphoreCreateBinary(); // Screen buffer locking
    if (!DrawSignal) {
        PRINT_ERROR("Failed to create draw signal");
        goto err_draw_signal;
    }

    ScreenLock = xSemaphoreCreateMutex();
    if (!ScreenLock) {
        PRINT_ERROR("Failed to create screen lock");
        goto err_screen_lock;
    }

    StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char));
    if (!StateQueue) {
        PRINT_ERROR("Could not open state queue");
        goto err_state_queue;
    }

    SendQueue       = xQueueCreate(QUEUE_SIZE, QUEUE_WIDTH*sizeof(unsigned char));
    ReceiveQueue    = xQueueCreate(QUEUE_SIZE, QUEUE_WIDTH*sizeof(unsigned char));
    ModeReceiveQueue= xQueueCreate(QUEUE_SIZE, QUEUE_WIDTH*sizeof(unsigned char));
    ListQueue       = xQueueCreate(QUEUE_SIZE, QUEUE_WIDTH*sizeof(unsigned char));
    SeedQueue       = xQueueCreate(QUEUE_SIZE, QUEUE_WIDTH*sizeof(unsigned char));

    instance_two_player.lock=xSemaphoreCreateMutex();
    logo_image= tumDrawLoadScaledImage(LOGO_FILENAME,0.7);

    if (xTaskCreate(basicSequentialStateMachine, "StateMachine",
                    mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES - 1, StateMachine) != pdPASS) {
        //PRINT_TASK_ERROR("StateMachine");
        goto err_statemachine;
    }
    if (xTaskCreate(vSwapBuffers, "BufferSwapTask",
                    mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES,
                    BufferSwap) != pdPASS) {
        //PRINT_TASK_ERROR("BufferSwapTask");
        goto err_bufferswap;
    }

    if (xTaskCreate(vDemoTask, "DemoTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY, &DemoTask) != pdPASS) {
        goto err_demotask;
    }

    if (xTaskCreate(vMenuTask, "MenuTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY, &MenuTask) != pdPASS) {
        goto err_menutask;
    }

    if (xTaskCreate(vPausedStateTask, "PausedStateTask",
                    mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY,
                    &PausedStateTask) != pdPASS) {
        goto err_pausedstate;
    }

    if (xTaskCreate(vGameOverStateTask, "GameOverStateTask",
                    mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY,
                    &GameOverStateTask) != pdPASS) {
    }

    if (xTaskCreate(vNoConnectionTask, "NoConnectionTask",
                    mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY,
                    &NoConnectionTask) != pdPASS) {
    
    }

    xTaskCreate(vUDPControlTask, "UDPControlTask",
                    mainGENERIC_STACK_SIZE*2, NULL, mainGENERIC_PRIORITY+2,
                    &UDPControlTask);

    vTaskSuspend(UDPControlTask);
    vTaskSuspend(NoConnectionTask);
    vTaskSuspend(PausedStateTask);
    vTaskSuspend(GameOverStateTask);
    vTaskSuspend(DemoTask);
    vTaskSuspend(MenuTask);
    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_menutask:
    vTaskDelete(DemoTask);
err_demotask:
    vTaskDelete(PausedStateTask);
err_pausedstate:
    vTaskDelete(BufferSwap);
err_bufferswap:
    vTaskDelete(StateMachine);
err_statemachine:
    vQueueDelete(StateQueue);
err_state_queue:
    vSemaphoreDelete(ScreenLock);
err_screen_lock:
    vSemaphoreDelete(DrawSignal);
err_draw_signal:
    vSemaphoreDelete(buttons.lock);
err_buttons_lock:
    tumSoundExit();
err_init_audio:
    tumEventExit();
err_init_events:
    tumDrawExit();
err_init_drawing:
    return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    /* Makes the process more agreeable when using the Posix simulator. */
    xTimeToSleep.tv_sec = 1;
    xTimeToSleep.tv_nsec = 0;
    nanosleep(&xTimeToSleep, &xTimeSlept);
#endif
}

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize )
{

/* If the buffers to be provided to the Idle task are declared inside this
function then they must be declared static – otherwise they will be allocated on
the stack and so not exists after this function exits. */
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[ configMINIMAL_STACK_SIZE ];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task’s
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task’s stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[ configTIMER_TASK_STACK_DEPTH];
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;
    *pulTimerTaskStackSize =configTIMER_TASK_STACK_DEPTH;
}
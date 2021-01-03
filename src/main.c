#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <inttypes.h>


#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"


#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_Font.h"

#include "AsyncIO.h"


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

#define LOGO_FILENAME "../resources/images/Untitled.bmp"



//const unsigned char next_state_signal = NEXT_TASK;
const unsigned char prev_state_signal = PREV_TASK;

static TaskHandle_t StateMachine = NULL;
static TaskHandle_t BufferSwap = NULL;

static TaskHandle_t DemoTask = NULL;
static TaskHandle_t MenuTask = NULL;
TaskHandle_t PausedStateTask = NULL;
static TaskHandle_t HandleTask= NULL;
static TaskHandle_t HandleUpdateY= NULL;
static TaskHandle_t HandleUpdateX= NULL;

QueueHandle_t BoundrysQueue =NULL;
QueueHandle_t YQueue=NULL;

static SemaphoreHandle_t SignalX=NULL;
static SemaphoreHandle_t syncSignal=NULL;

static QueueHandle_t StateQueue = NULL;
static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;

TimerHandle_t myTimer= NULL;

static image_handle_t logo_image=NULL;

/*
struct my_X{
    int condition_X_Right;
    int condition_X_Left;
};

struct my_struct_X{
    SemaphoreHandle_t lock_X;
    struct my_X my_X_instance;
};
struct my_struct_X my_struct_instance_X={.lock_X=NULL};
*/

struct my_Y{
    int condition_Y2;
    int condition_Y;
};

struct my_struct_Y{
    SemaphoreHandle_t lock_Y;
    struct my_Y my_Y_instance;
};

struct my_struct_Y my_struct_instance_Y={.lock_Y=NULL};

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
    int Grid[12][21];
};

struct my_struct_grid my_struct_instance_grid={.lock_grid=NULL};

struct my_struct_frame{
    SemaphoreHandle_t lock_frame;
    int Frame[12][21];
};

struct my_struct_frame my_struct_instance_frame={.lock_frame=NULL};

struct my_struct_shape{
    SemaphoreHandle_t lock_shape;
    int Shape[6][5];
};
struct my_struct_shape my_struct_instance_shape={.lock_shape=NULL};

struct next_shape{
    SemaphoreHandle_t lock;
    int NextShape[6][5];
};
struct next_shape instance_next_shape={.lock=NULL};



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

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR


void Tetrimino()
{
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    int Tetrimino= my_struct_instance_tetri.Tetri_number;
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
    
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    {   
        if(xSemaphoreTake(instance_next_shape.lock,portMAX_DELAY)==pdTRUE)
        { 
            for(int i=0; i<6;i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=instance_next_shape.NextShape[i][j];
                    instance_next_shape.NextShape[i][j]=0;
                }
            }
            switch(Tetrimino)
                {
                    case 0 : //T
                        instance_next_shape.NextShape[1][2]=2;
                        instance_next_shape.NextShape[2][2]=2;
                        instance_next_shape.NextShape[3][2]=2;
                        instance_next_shape.NextShape[2][3]=2;
                        break;
                    case 1 : //J
                        instance_next_shape.NextShape[1][2]=3;
                        instance_next_shape.NextShape[2][2]=3;
                        instance_next_shape.NextShape[3][2]=3;
                        instance_next_shape.NextShape[3][3]=3;
                        break;
                    case 2 : //Z
                        instance_next_shape.NextShape[1][2]=4;
                        instance_next_shape.NextShape[2][2]=4;
                        instance_next_shape.NextShape[2][3]=4;
                        instance_next_shape.NextShape[3][3]=4;
                        break;
                    case 3 : //O
                        instance_next_shape.NextShape[1][2]=5;
                        instance_next_shape.NextShape[1][3]=5;
                        instance_next_shape.NextShape[2][2]=5;
                        instance_next_shape.NextShape[2][3]=5;
                        break;
                    case 4 : //S
                        instance_next_shape.NextShape[1][3]=6;
                        instance_next_shape.NextShape[2][2]=6;
                        instance_next_shape.NextShape[2][3]=6;
                        instance_next_shape.NextShape[3][2]=6;
                        break;
                    case 5 : //L
                        instance_next_shape.NextShape[1][2]=7;
                        instance_next_shape.NextShape[1][3]=7;
                        instance_next_shape.NextShape[2][2]=7;
                        instance_next_shape.NextShape[3][2]=7;
                        break;
                    case 6 : //I
                        instance_next_shape.NextShape[1][2]=8;
                        instance_next_shape.NextShape[2][2]=8;
                        instance_next_shape.NextShape[3][2]=8;
                        instance_next_shape.NextShape[4][2]=8;
                        break;
                    
                    default:
                        break;
                }
        xSemaphoreGive(instance_next_shape.lock);
        }
    xSemaphoreGive(my_struct_instance_shape.lock_shape);

    }

}

void Grid()
{
    if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
    {
           for(int i=0; i<21;i++) 
           {
               my_struct_instance_grid.Grid[0][i]=1;
               my_struct_instance_grid.Grid[11][i]=1;
           }
           for(int i=0; i<12; i++)
           {
               my_struct_instance_grid.Grid[i][20]=1;
           }
           
    xSemaphoreGive(my_struct_instance_grid.lock_grid);
    }

    if(xSemaphoreTake(instance_next_shape.lock,portMAX_DELAY)==pdTRUE)
    { 
        instance_next_shape.NextShape[1][2]=1;
        instance_next_shape.NextShape[3][2]=1;
        instance_next_shape.NextShape[2][2]=1;
        instance_next_shape.NextShape[4][2]=1;
    xSemaphoreGive(instance_next_shape.lock);  
    }
    
    
}
void reset()
{
    xSemaphoreTake(my_board_instance.lock,portMAX_DELAY);
    my_board_instance.board_instance.total_line=0;
    my_board_instance.board_instance.score=0;
    xSemaphoreGive(my_board_instance.lock);

    xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY);
    my_coord_instance.coord_instance.x=5;
    my_coord_instance.coord_instance.py=0;
    my_coord_instance.coord_instance.x=5;
    my_coord_instance.coord_instance.y=0;
    xSemaphoreGive(my_coord_instance.lock);

    if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
    {
        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
        {   
            for(int i=0; i<12; i++)
            {   
                for(int j=0; j<21; j++)
                {
                    my_struct_instance_frame.Frame[i][j]= 0;
                    my_struct_instance_grid.Grid[i][j]= 0;
                }
            }
        xSemaphoreGive(my_struct_instance_grid.lock_grid);
        }
          
            
    xSemaphoreGive(my_struct_instance_frame.lock_frame);
    }
    Grid(); 
    
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    { 
        for(int i=0; i<6;i++)
        {
            for(int j=0; j<5;j++)
            {
                my_struct_instance_shape.Shape[i][j]=0;
                instance_next_shape.NextShape[i][j]=0;   
            }
        }  

    xSemaphoreGive(my_struct_instance_shape.lock_shape);
    } 
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    my_struct_instance_tetri.Tetri_number=0;
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
    Tetrimino();
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    my_struct_instance_tetri.Tetri_number=1;
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
    Tetrimino();
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
    unsigned char current_state = STARTING_STATE; // Default state
    unsigned char state_changed =
        1; // Only re-evaluate state if it has changed
    unsigned char input = 0;

    const int state_change_period = STATE_DEBOUNCE_DELAY;

    int flag_pause=0;
    
    TickType_t last_change = xTaskGetTickCount();

    while (1) {
        if (state_changed) {
            goto initial_state;
        }
        // printf("1\n");
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
                    if (DemoTask) {
                        vTaskSuspend(DemoTask);
                    }
                    if (MenuTask) {
                        vTaskResume(MenuTask);
                     }   
                    break;
                case 'O':
                    if (MenuTask) {
                        vTaskSuspend(MenuTask);
                    }
                    if(DemoTask){
                        vTaskResume(DemoTask);
                        vTaskResume(HandleTask);
                        vTaskResume(HandleUpdateX);
                        vTaskResume(HandleUpdateY);
                    }
                    break;
                case 'E':
                   if (DemoTask) {
                        vTaskSuspend(DemoTask);
                        vTaskSuspend(HandleTask);
                        vTaskSuspend(HandleUpdateX);
                        vTaskSuspend(HandleUpdateY);
                        reset();
                    }
                    if (MenuTask) {
                        vTaskResume(MenuTask);
                    }   
                    break;
                case 'R':
                    if(DemoTask) {
                        vTaskSuspend(DemoTask);
                        reset();
                        vTaskResume(DemoTask);
                    }
                    break;
                case 'P':
                    if(DemoTask) {
                        if(flag_pause==0)
                        {
                            vTaskSuspend(DemoTask);
                            vTaskSuspend(HandleTask);
                            vTaskSuspend(HandleUpdateX);
                            vTaskSuspend(HandleUpdateY);
                            vTaskResume(PausedStateTask);
                            flag_pause=1;
                        }
                        else
                        {
                            vTaskSuspend(PausedStateTask);
                            vTaskResume(DemoTask);
                            vTaskResume(HandleTask);
                            vTaskResume(HandleUpdateX);
                            vTaskResume(HandleUpdateY);
                            flag_pause=0;
                        } 
                    }
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

    while (1) {
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

void move_tetrimino()
{

    int xmin=6;
    int xmax=0;
    int ymax=0;
    

   if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
    {
          
        if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
        {  
                    
                    for(int i=0; i<6;i++)
                    {
                        for(int j=0; j<5;j++)
                        {
                            if (my_struct_instance_shape.Shape[i][j]!=0)
                            {
                                 if(ymax<j+1)
                                    ymax=j+1;
                            }
                        }
                    }  
                    for(int j=0; j<5;j++)
                    {
                        for(int i=0; i<6;i++)
                        {
                            if (my_struct_instance_shape.Shape[i][j]!=0)
                            {
                                if (xmin>i)
                                    xmin=i;
                                if(i+1>xmax)
                                    xmax=i+1;
                            }
                        }
                    }  
                    
                   

                    for(int i=0; i<12; i++)
                    {   
                        for(int j=0; j<21; j++)
                        {
                            my_struct_instance_frame.Frame[i][j]= 0;
                        }
                    }
                   
                  
                    //printf("-> %d, %d, %d \n",xmin,xmax,ymax);
                    xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY);
                    for(int i=xmin; i<xmax; i++)
                    {
                        for(int j=0; j<ymax; j++)
                        {
                            my_struct_instance_frame.Frame[my_coord_instance.coord_instance.x+i][my_coord_instance.coord_instance.y+j]= my_struct_instance_shape.Shape[i][j];
                        }
                    }    
                    xSemaphoreGive(my_coord_instance.lock);

        xSemaphoreGive(my_struct_instance_shape.lock_shape);            
        }   
    xSemaphoreGive(my_struct_instance_frame.lock_frame);
    } 
}




void rotate_tetrimino_L()
{

    printf("girdi");
    int tmp[5][5];
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    printf("Tetri number : %d",my_struct_instance_tetri.Tetri_number);

    if(1)
    {
        if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
        {
        
            for(int i=0; i<5; i++)
            {
                for(int j=0; j<5; j++)
                {
                    
                    tmp[i][j]=my_struct_instance_shape.Shape[i][j];
                   
                }
            }
            for(int x=0; x<5; x++)
            {
                for(int y=x; y<5-x-1;y++)
                {
                
                    int temp=tmp[x][y];
                    tmp[x][y]=tmp[y][5-1-x];
                    tmp[y][5-1-x]=tmp[5-1-x][5-1-y];
                    tmp[5-1-x][5-1-y]=tmp[5-1-y][x];
                    tmp[5-1-y][x]=temp;
                
                }
            }
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=0;
                }
            }
            for(int i=0; i<5; i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=tmp[i][j];
                }
            }
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
        }

        
    }
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);

    // ------ control_elements();
    
        

          for(int j=0;j<5;j++)
        {
            printf("\n");
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,my_struct_instance_shape.Shape[i][j]);    
            }
        }
        printf("\n");
    
    
    /*
    for(int j=0;j<4;j++)
        {
            printf ( "\n" ); 
            for(int i=0; i<4;i++)
            {
               printf ( "%d " ,tmp[i][j] );    
            }
     }
     printf("\n");
     printf("------------------\n");*/
}

void rotate_tetrimino_R()
{


    int tmp[5][5];
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    printf("Tetri number : %d",my_struct_instance_tetri.Tetri_number);

/*
   if(my_struct_instance_tetri.Tetri_number==0)
    {
        xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
        rotate_I();
    }  */
    if(1)
    {
        if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
        {
        
            for(int i=0; i<5; i++)
            {
                for(int j=0; j<5; j++)
                {
                    tmp[i][j]=my_struct_instance_shape.Shape[i][j];
                }
                    
            }
            for(int i=0; i<5/2; i++)
            {
                for(int j=i; j<5-i-1;j++)
                {
                
                    int temp=tmp[i][j];
                    tmp[i][j]=tmp[5-1-j][i];
                    tmp[5-1-j][i]=tmp[5-1-i][5-1-j];
                    tmp[5-1-i][5-1-j]=tmp[j][5-1-i];
                    tmp[j][5-1-i]=temp;
                }
            }
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=0;
                }
            }
            for(int i=0; i<5; i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=tmp[i][j];
                }
            }
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
        }

        
    }
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);

    // ------ control_elements();
    
        

          for(int j=0;j<5;j++)
        {
            printf("\n");
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,my_struct_instance_shape.Shape[i][j]);    
            }
        }
        printf("\n");
    
    
    /*
    for(int j=0;j<4;j++)
        {
            printf ( "\n" ); 
            for(int i=0; i<4;i++)
            {
               printf ( "%d " ,tmp[i][j] );    
            }
     }
     printf("\n");
     printf("------------------\n");*/
}

int move_condition(int x, int y)
{
    int xmin=6;
    int xmax=0;
    int ymax=0;
    int CFrame[12][21];
    int CFlag;

    CFlag=0;                                

        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
        {   
                if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
                {  
                   
                    for(int i=0; i<6;i++)
                    {
                        for(int j=0; j<5;j++)
                        {
                            if (my_struct_instance_shape.Shape[i][j]!=0)
                            {
                                 if(ymax<j+1)
                                    ymax=j+1;
                            }
                        }
                    }  
                    for(int j=0; j<5;j++)
                    {
                        for(int i=0; i<6;i++)
                        {
                            if (my_struct_instance_shape.Shape[i][j]!=0)
                            {
                                if (xmin>i)
                                    xmin=i;
                                if(i+1>xmax)
                                    xmax=i+1;
                            }
                        }
                    }  
                    
                    
                    for(int i=0; i<12; i++)
                    {   
                        for(int j=0; j<21; j++)
                        {
                            CFrame[i][j]= 0;
                        }
                    }
                   
                  
                    //printf("-> %d, %d, %d \n",xmin,xmax,ymax);
                    for(int i=xmin; i<xmax; i++)
                    {
                        for(int j=0; j<ymax; j++)
                        {
                            CFrame[x+i][y+j]= my_struct_instance_shape.Shape[i][j];
                        }
                    }    
                    
                    for(int i=0; i<12; i++)
                    {   
                        for(int j=0; j<21; j++)
                        {
                            if((CFrame[i][j]!=0) & (my_struct_instance_grid.Grid[i][j]!=0))
                            {
                                CFlag=1;
                            }
                        }
                    }

                }
                xSemaphoreGive(my_struct_instance_shape.lock_shape); 
        }
        xSemaphoreGive(my_struct_instance_grid.lock_grid);  
    return CFlag;
}



void UpdateY()
{
    int delay;
    int Flag_D=1;
    int cx,cy;

    while(1)
    {
        xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY);
        my_coord_instance.coord_instance.py=my_coord_instance.coord_instance.y;
        cx=my_coord_instance.coord_instance.x;
        cy=my_coord_instance.coord_instance.y; 
        xSemaphoreGive(my_coord_instance.lock);

        if(move_condition(cx,cy+1)==0)
        {
                if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                {  
                     my_coord_instance.coord_instance.y++;
                     my_struct_instance_Y.my_Y_instance.condition_Y2=0;
                     xSemaphoreGive(my_coord_instance.lock);
                }
        }
        else
        {
             if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                {  
                     my_struct_instance_Y.my_Y_instance.condition_Y2++;
                     xSemaphoreGive(my_coord_instance.lock);
                }
        }
        
        

        if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
            xGetButtonInput();
            xSemaphoreGive(buttons.lock);

            xSemaphoreTake(my_board_instance.lock,portMAX_DELAY);
            if (buttons.buttons[81]) { 
                    if(Flag_D==0)
                    {
                        Flag_D=1;
                        delay=(600-my_board_instance.board_instance.level*56)/2; 
                    }
                    
                }else 
                {
                    if(Flag_D==1)
                    {
                        Flag_D=0;
                        delay=600-my_board_instance.board_instance.level*56;
                    }
                } 
            xSemaphoreGive(my_board_instance.lock);
         
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
            if (buttons.buttons[79]) 
            { 
                if(move_condition(cx+1,cy)==0)
                {
                        if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                        {  
                            my_coord_instance.coord_instance.x++;
                            xSemaphoreGive(my_coord_instance.lock);
                        }
                }
            }
            if (buttons.buttons[80]) 
            { 
                if(move_condition(cx-1,cy)==0)
                {
                        if(xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY)==pdTRUE)
                        {  
                            my_coord_instance.coord_instance.x--;
                            xSemaphoreGive(my_coord_instance.lock);
                        }
                }
            } 
            if (buttons.buttons[4] & Flag_A==0) 
            { 
                Flag_A=1;
                rotate_tetrimino_R();    
                if(move_condition(cx,cy)==1)
                {
                    rotate_tetrimino_L();
                }
            }else Flag_A=0;

            if (buttons.buttons[5] & Flag_B==0) 
            { 
                Flag_B=1;
                rotate_tetrimino_L();    
                if(move_condition(cx,cy)==1)
                {
                    rotate_tetrimino_R();
                }
            }else Flag_B=0;
        }
                
                 
       vTaskDelay(100);
    }
}

void puf(void)
{
    int flag;
    int count_line=0;
    int factor=0;

    xSemaphoreTake(my_board_instance.lock,portMAX_DELAY);
    if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
    {
        for(int j=19;j>1;j--)
        {
            flag=1;
            for(int i=0; i<12; i++)
            {
                if(my_struct_instance_grid.Grid[i][j]==0)
                    flag=0;
                
            }
            if(flag==1)
            {
                for(int i=1; i<11; i++)
                {
                    my_struct_instance_grid.Grid[i][j]=my_struct_instance_grid.Grid[i][j-1];

                }
                for(int k=j;k>1;k--)
                {
                     for(int i=1; i<11; i++)
                    {
                        my_struct_instance_grid.Grid[i][k]=my_struct_instance_grid.Grid[i][k-1];
                    }
                }
                count_line++;
                my_board_instance.board_instance.total_line++;
                j++;
            }
        }
         printf("line count is %d\n",count_line);
    xSemaphoreGive(my_struct_instance_grid.lock_grid);
    }
    switch(count_line){
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
    my_board_instance.board_instance.score=my_board_instance.board_instance.score+factor*(my_board_instance.board_instance.level+1);
    xSemaphoreGive(my_board_instance.lock);
}

void Task(void *pvParameters)
{


    BoundrysQueue=xQueueCreate(3,sizeof(int));

    
    reset();
    
    while(1)
    {
        //printf("1\n");
        move_tetrimino();

        //xQueueReceive(Que_cond_Y,&condition_Y,0);
       // xQueueReceive(Que_cond_X_R,&condition_X_Right,0);
        //xQueueReceive(Que_cond_X_L,&condition_X_Left,0);
        

        
        
            if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
            {
                
                if(xSemaphoreTake(my_struct_instance_Y.lock_Y,portMAX_DELAY)==pdTRUE)
                {
                    
                    if(my_struct_instance_Y.my_Y_instance.condition_Y2>=2)
                    {
                        my_struct_instance_Y.my_Y_instance.condition_Y2=0;
                       

                        printf("new\n");
                        xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
                        my_struct_instance_tetri.Tetri_number=(my_struct_instance_tetri.Tetri_number+1)%7;
                        xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
                      
                        Tetrimino();
                        xSemaphoreTake(my_coord_instance.lock,portMAX_DELAY);
                        my_coord_instance.coord_instance.x=5;
                        my_coord_instance.coord_instance.py=0;
                        my_coord_instance.coord_instance.x=5;
                        my_coord_instance.coord_instance.y=0;
                        xSemaphoreGive(my_coord_instance.lock);
                        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
                        {
                            for(int i=0;i<12; i++)
                            {
                                for(int j=0; j<21; j++)
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
                        puf();
                       
                    } 
                xSemaphoreGive(my_struct_instance_Y.lock_Y);   
                }   
                
            xSemaphoreGive(my_struct_instance_frame.lock_frame);
          
            }

        vTaskDelay(25);
       
    }
}

void vDrawLogo(void)
{
    static int image_height;
    printf("draw\n");
    if ((image_height = tumDrawGetLoadedImageHeight(logo_image)) != -1)
    {
        printf("height");
        tumDrawLoadedImage(logo_image, 10,
                                     SCREEN_HEIGHT - 10 - image_height);
    }

    
                  
    
}

void vMenuTask(void *pvParameters)
{
     char str[16];
    
    while(1)
    {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
                xGetButtonInput(); // Update global input
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                tumDrawClear(White); // Clear screen
       // printf("menü\n");
                 static int image_height;
                image_height = tumDrawGetLoadedImageHeight(logo_image);
                printf("%d\n",image_height);
                tumDrawLoadedImage(logo_image, 100,
                                     100);

                
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                    if (buttons.buttons[KEYCODE(L)]) {
                        buttons.buttons[KEYCODE(L)] = 0;
                       xSemaphoreTake(my_board_instance.lock,portMAX_DELAY);
                        my_board_instance.board_instance.level=(my_board_instance.board_instance.level+1)%10;
                        xSemaphoreGive(my_board_instance.lock);
                    }
                xSemaphoreGive(buttons.lock);
                }
                
                
                if(xSemaphoreTake(my_board_instance.lock,portMAX_DELAY)==pdTRUE)
                {
                    sprintf(str, "[L]evel: %d",  my_board_instance.board_instance.level);
                    tumDrawText(str,
                        DEFAULT_FONT_SIZE * 2.5,
                        DEFAULT_FONT_SIZE * 4.5, Black);

                    xSemaphoreGive(my_board_instance.lock);
                }
                
                tumDrawText("[O]ne Player",
                    DEFAULT_FONT_SIZE * 2.5,
                    DEFAULT_FONT_SIZE * 6.5, Black);

                 tumDrawText("[T]wo Player",
                    DEFAULT_FONT_SIZE * 2.5,
                    DEFAULT_FONT_SIZE * 8.5, Black);
            
                vTaskDelay((TickType_t)100);

                xSemaphoreGive(ScreenLock);
                vCheckStateInputMenu();
            }
         
    }
}
void vDemoTask(void *pvParameters)
{
    char str[16];

    unsigned int color;
    
    
    Que_cond_Y= xQueueCreate(1,sizeof(int));
    Que_cond_X_R= xQueueCreate(1,sizeof(int));
    Que_cond_X_L= xQueueCreate(1,sizeof(int));

    //YQueue=xQueueCreate(2,sizeof(int));
    
    my_struct_instance_grid.lock_grid=xSemaphoreCreateMutex();
    my_struct_instance_frame.lock_frame=xSemaphoreCreateMutex();
    my_struct_instance_shape.lock_shape=xSemaphoreCreateMutex();
    my_struct_instance_tetri.lock_tetri=xSemaphoreCreateMutex();
    my_struct_instance_Y.lock_Y=xSemaphoreCreateMutex();
    //my_struct_instance_X.lock_X=xSemaphoreCreateMutex();
    instance_next_shape.lock=xSemaphoreCreateMutex();
    my_coord_instance.lock=xSemaphoreCreateMutex();
    

    xTaskCreate(Task, "Task", mainGENERIC_STACK_SIZE, NULL,mainGENERIC_PRIORITY+2, &HandleTask);
    vTaskResume(HandleTask);
    xTaskCreate(UpdateY, "UpdateY", mainGENERIC_STACK_SIZE, NULL,mainGENERIC_PRIORITY+1, &HandleUpdateY);
    vTaskResume(HandleUpdateY);
    xTaskCreate(UpdateX, "UpdateX", mainGENERIC_STACK_SIZE, NULL,mainGENERIC_PRIORITY, &HandleUpdateX);
    vTaskResume(HandleUpdateX);
    //vTaskResume(HandleButtonTask);
    //Signal_X_R= xSemaphoreCreateBinary();
    //Signal_X_L= xSemaphoreCreateBinary();
    syncSignal= xSemaphoreCreateBinary();

    // Needed such that Gfx library knows which thread controlls drawing
    // Only one thread can call tumDrawUpdateScreen while and thread can call
    // the drawing functions to draw objects. This is a limitation of the SDL
    // backend.
    //tumDrawBindThread();

    
    

    while (1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
        
                xGetButtonInput(); // Update global input
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                tumDrawClear(White);
        
                if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
                {
                    for(int i=0;i<12;i++)
                    {
                        for(int j=0;j<21;j++)
                        {
                            switch(my_struct_instance_frame.Frame[i][j]){
                                case 1:
                                    color=Gray;
                                    break;
                                case 2:
                                    color=Red;
                                    break;
                                case 3:
                                    color=Yellow;
                                    break;
                                case 4:
                                    color=Green;
                                    break;
                                case 5:
                                    color=Cyan;
                                    break;
                                case 6:
                                    color=Blue;
                                    break;
                                case 7:
                                    color=Purple;
                                    break;
                                case 8:
                                    color=Pink;
                                    break;
                                default:
                                    break;
                            }
                            if(my_struct_instance_frame.Frame[i][j]!=0)
                            {
                                 tumDrawFilledBox(20*i+200,20*j+20,20,20,color);
                                tumDrawBox(20*i+200,20*j+20,20,20,Black);
                            }
                            else
                                tumDrawFilledBox(20*i+200,20*j+20,20,20,White);
                        }
                    }
                xSemaphoreGive(my_struct_instance_frame.lock_frame);
                }
                if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
                {   
                    for(int i=0;i<12;i++)
                    {
                        for(int j=0;j<21;j++)
                        {
                            switch(my_struct_instance_grid.Grid[i][j]){
                                case 1:
                                    color=Gray;
                                    break;
                                case 2:
                                    color=Red;
                                    break;
                                case 3:
                                    color=Yellow;
                                    break;
                                case 4:
                                    color=Green;
                                    break;
                                case 5:
                                    color=Cyan;
                                    break;
                                case 6:
                                    color=Blue;
                                    break;
                                case 7:
                                    color=Purple;
                                    break;
                                case 8:
                                    color=Pink;
                                    break;
                                default:
                                    break;
                            }
                            if(my_struct_instance_grid.Grid[i][j]!=0)
                            {
                                tumDrawFilledBox(20*i+200,20*j+20,20,20,color);
                                tumDrawBox(20*i+200,20*j+20,20,20,Black);
                            }
                   
                
                        }
                    }
                xSemaphoreGive(my_struct_instance_grid.lock_grid);
                }

                for(int i=12;i<20;i++)
                {
                    for (int j=0;j<21;j++)
                    {
                        tumDrawFilledBox(20*i+200,20*j+20,20,20,Silver);
                    }
            
                } 

                for(int i=13;i<19;i++)
                {
                    tumDrawFilledBox(20*i+200,20*2+20,20,20,White);
                    tumDrawFilledBox(20*i+200,20*5+20,20,20,White);
                    tumDrawFilledBox(20*i+200,20*8+20,20,20,White);
                    for(int j=15; j<20;j++)
                    {
                        tumDrawFilledBox(20*i+200,20*j+20,20,20,White);
                    }       
                }

                if(xSemaphoreTake(instance_next_shape.lock,portMAX_DELAY)==pdTRUE)
                { 
                    for(int i=0; i<6;i++)
                    {
                        for(int j=0; j<5; j++)
                        {
                            switch(instance_next_shape.NextShape[i][j]){
                                case 1:
                                    color=Gray;
                                    break;
                                case 2:
                                    color=Red;
                                    break;
                                case 3:
                                    color=Yellow;
                                    break;
                                case 4:
                                    color=Green;
                                    break;
                                case 5:
                                    color=Cyan;
                                    break;
                                case 6:
                                    color=Blue;
                                    break;
                                case 7:
                                    color=Purple;
                                    break;
                                case 8:
                                    color=Pink;
                                    break;
                                default:
                                    break;
                            }
                            if(instance_next_shape.NextShape[i][j]!=0)
                            {

                                tumDrawFilledBox(20*(i+13)+200,20*(j+15)+20,20,20,color);
                                tumDrawBox(20*(i+13)+200,20*(j+15)+20,20,20,Black);
                            }
                        }
                    }
                xSemaphoreGive(instance_next_shape.lock);
                }
        
                xSemaphoreTake(my_board_instance.lock,portMAX_DELAY);
                sprintf(str, "LINES: %d", my_board_instance.board_instance.total_line);
                tumDrawText(str, 20*13+210,20*2+20,Black);

                sprintf(str, "LEVEL: %d", my_board_instance.board_instance.level);
                tumDrawText(str, 20*13+210,20*5+20,Black);

                sprintf(str, "SCORE: %d", my_board_instance.board_instance.score);
                tumDrawText(str, 20*13+210,20*8+20,Black);
                xSemaphoreGive(my_board_instance.lock);
                

                
                tumDrawText("[E]xit [P]ause [R]estart", 20*11+210,0,Black);
        
    
                

                //tumDrawUpdateScreen(); // Refresh the screen to draw string

        // Basic sleep of 1000 milliseconds
                xSemaphoreGive(ScreenLock);

                // Get input and check for state change
                vCheckStateInputTask();
        
                vTaskDelay((TickType_t)100);
            }
    }
}

static const char *paused_text = "PAUSED";
static int paused_text_width;

void vPausedStateTask(void *pvParameters)
{
    tumGetTextSize((char *)paused_text, &paused_text_width, NULL);
    unsigned char next_state_signal;

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
                taskENTER_CRITICAL();

                if (xSemaphoreTake(ScreenLock, 0) == pdTRUE) {
                    tumDrawClear(White);

                    tumDrawText((char *)paused_text,
                                SCREEN_WIDTH / 2 -
                                paused_text_width /
                                2,
                                SCREEN_HEIGHT / 2, Red);
                }

                xSemaphoreGive(ScreenLock);

                taskEXIT_CRITICAL();

                vTaskDelay(10);
            }
        }
    }
}
int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

    my_board_instance.lock=xSemaphoreCreateMutex();
    logo_image= tumDrawLoadImage(LOGO_FILENAME);


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
        //PRINT_TASK_ERROR("PausedStateTask");
        goto err_pausedstate;
    }

    vTaskSuspend(PausedStateTask);
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
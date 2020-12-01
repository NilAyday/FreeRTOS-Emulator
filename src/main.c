#include <math.h>
#include <stdio.h>
#include <stdlib.h>


#include <SDL2/SDL_scancode.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timers.h"

#include "TUM_Ball.h"
#include "TUM_Draw.h"
#include "TUM_Font.h"
#include "TUM_Event.h"
#include "TUM_Sound.h"
#include "TUM_Utils.h"
#include "TUM_FreeRTOS_Utils.h"
#include "TUM_Print.h"

#include "AsyncIO.h"



#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)

#define STATE_QUEUE_LENGTH 1

#define STATE_COUNT 3

#define STATE_ONE 0
#define STATE_TWO 1
#define STATE_THREE 2

#define NEXT_TASK 0
#define PREV_TASK 1

#define STARTING_STATE STATE_ONE

#define STATE_DEBOUNCE_DELAY 300

#define KEYCODE(CHAR) SDL_SCANCODE_##CHAR
#define CAVE_SIZE_X SCREEN_WIDTH / 2
#define CAVE_SIZE_Y SCREEN_HEIGHT / 2
#define CAVE_X CAVE_SIZE_X / 2
#define CAVE_Y CAVE_SIZE_Y / 2
#define CAVE_THICKNESS 25
#define LOGO_FILENAME "freertos.jpg"
#define UDP_BUFFER_SIZE 2000
#define UDP_TEST_PORT_1 1234
#define UDP_TEST_PORT_2 4321
#define MSG_QUEUE_BUFFER_SIZE 1000
#define MSG_QUEUE_MAX_MSG_COUNT 10
#define TCP_BUFFER_SIZE 2000
#define TCP_TEST_PORT 2222

#ifdef TRACE_FUNCTIONS
#include "tracer.h"
#endif


aIO_handle_t mq_one = NULL;
aIO_handle_t mq_two = NULL;
aIO_handle_t udp_soc_one = NULL;
aIO_handle_t udp_soc_two = NULL;
aIO_handle_t tcp_soc = NULL;

const unsigned char next_state_signal = NEXT_TASK;
const unsigned char prev_state_signal = PREV_TASK;


static TaskHandle_t StateMachine = NULL;
static TaskHandle_t BufferSwap = NULL;
static TaskHandle_t DemoTask1 = NULL;
static TaskHandle_t DemoTask2 = NULL;
static TaskHandle_t DemoTask3 = NULL;


static TaskHandle_t Task1 = NULL;
static TaskHandle_t Task2 = NULL;
static TaskHandle_t Task3 = NULL;
static TaskHandle_t Task4 = NULL;


TaskHandle_t HandleCircleOne = NULL;
TaskHandle_t HandleCircleTwo = NULL;

TaskHandle_t HandleButtonOne =NULL;
TaskHandle_t HandleButtonTwo =NULL;
TaskHandle_t HandleButtonThree =NULL;
TimerHandle_t myTimer = NULL;

StaticTask_t xTaskBuffer;
StackType_t xStack[mainGENERIC_STACK_SIZE];

StaticTask_t xTaskBuffer1;
StackType_t xStack1[mainGENERIC_STACK_SIZE];

static QueueHandle_t StateQueue = NULL;
static SemaphoreHandle_t DrawSignal = NULL;
static SemaphoreHandle_t ScreenLock = NULL;

static SemaphoreHandle_t mySyncSignal = NULL;
static SemaphoreHandle_t mySyncSignalTask = NULL;

QueueHandle_t FlagQueueR = NULL;
QueueHandle_t FlagQueueL = NULL;

QueueHandle_t Button1Queue =NULL;
QueueHandle_t Button2Queue =NULL;
QueueHandle_t Button3Queue =NULL;
QueueHandle_t QueueTasks =NULL;

static image_handle_t logo_image = NULL;

typedef struct buttons_buffer {
    unsigned char buttons[SDL_NUM_SCANCODES];
    SemaphoreHandle_t lock;
} buttons_buffer_t;

static buttons_buffer_t buttons = { 0 };


#define triangle_top_X 300
#define triangle_top_Y 180
#define triangle_high 40
#define triangle_width 40

#define pi 3.1415926
#define middleX 300
#define middleY 200
#define factor 0.1

void TriangleDraw(coord_t *triangle)
{

    triangle[0].x=triangle_top_X +factor*tumEventGetMouseX();
    triangle[0].y=triangle_top_Y +factor*tumEventGetMouseY();
    triangle[1].x=triangle_top_X-triangle_width/2 +factor*tumEventGetMouseX();
    triangle[1].y=triangle_top_Y+triangle_high +factor*tumEventGetMouseY();
    triangle[2].x=triangle_top_X+triangle_width/2+factor*tumEventGetMouseX() ;
    triangle[2].y=triangle_top_Y+triangle_high +factor*tumEventGetMouseY();

}

void checkDraw(unsigned char status, const char *msg)
{
    if (status) {
        if (msg)
            fprints(stderr, "[ERROR] %s, %s\n", msg,
                    tumGetErrorMessage());
        else {
            fprints(stderr, "[ERROR] %s\n", tumGetErrorMessage());
        }
    }
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

/*
 * Example basic state machine with sequential states
 */
void basicSequentialStateMachine(void *pvParameters)
{
    unsigned char current_state = STARTING_STATE; // Default state
    unsigned char state_changed =
        1; // Only re-evaluate state if it has changed
    unsigned char input = 0;

    const int state_change_period = STATE_DEBOUNCE_DELAY;

    
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
                    changeState(&current_state, input);
                    state_changed = 1;
                    last_change = xTaskGetTickCount();
                }

initial_state:
        // Handle current state
        if (state_changed) {
            switch (current_state) {
                case STATE_ONE:
                    if (DemoTask3) {
                        vTaskSuspend(DemoTask3);
                    }
                    if(DemoTask1){
                        vTaskSuspend(DemoTask1);
                     }
                    if (DemoTask2) {
                        vTaskResume(DemoTask2);
                     }   
                    break;
                case STATE_TWO:
                    if (DemoTask2) {
                        vTaskSuspend(DemoTask2);
                    }
                    if(DemoTask3){
                        vTaskSuspend(DemoTask3);
                    }
                    if (DemoTask1) {
                        vTaskResume(DemoTask1);
                    }
                    break;
                case STATE_THREE:
                     if (DemoTask2) {
                        vTaskSuspend(DemoTask2);
                    }
                    if(DemoTask1){
                        vTaskSuspend(DemoTask1);
                    }
                    if (DemoTask3) {
                        vTaskResume(DemoTask3);
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

void xGetButtonInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        xQueueReceive(buttonInputQueue, &buttons.buttons, 0);
        xSemaphoreGive(buttons.lock);
    }
}

#define FPS_AVERAGE_COUNT 50
#define FPS_FONT "IBMPlexSans-Bold.ttf"

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
        checkDraw(tumDrawText(str, SCREEN_WIDTH - text_width - 10,
                              SCREEN_HEIGHT - DEFAULT_FONT_SIZE * 1.5,
                              Skyblue),
                  __FUNCTION__);

    tumFontSelectFontFromHandle(cur_font);
    tumFontPutFontHandle(cur_font);
}

typedef struct Rotate{

    signed short posx;
    signed short posy;
    int r;
    double angle;

}Rotate_t;

void RotatingFiguresInit(Rotate_t *rotate)
{
    (*rotate).posx=0;
    (*rotate).posy =0;
    
    (*rotate).r=100;
    (*rotate).angle=0;
}



void RotatingFiguresDraw(Rotate_t *rotate)
{
    
    (*rotate).angle++;
    if((*rotate).angle>=360)
    {
        (*rotate).angle=0;
    }
    (*rotate).posx=(*rotate).r*cos((*rotate).angle*pi/180);
    (*rotate).posy=(*rotate).r*sin((*rotate).angle*pi/180);

    checkDraw(tumDrawCircle(middleX+(*rotate).posx +factor*tumEventGetMouseX() ,middleY+(*rotate).posy+factor*tumEventGetMouseY(),triangle_high/2,Red),
    __FUNCTION__);
                        

    (*rotate).posx=(*rotate).r*cos((*rotate).angle*pi/180+pi);
    (*rotate).posy=(*rotate).r*sin((*rotate).angle*pi/180+pi);

    checkDraw(tumDrawFilledBox(middleX+(*rotate).posx +factor*tumEventGetMouseX(),middleY+(*rotate).posy+factor*tumEventGetMouseY(),triangle_high,triangle_high,TUMBlue),
    __FUNCTION__);
}


void Mouse(void)
{
    static char str[100] = { 0 };
    sprintf(str, "Axis 1: %5d | Axis 2: %5d", tumEventGetMouseX(),
            tumEventGetMouseY());
    checkDraw(tumDrawText(str, factor*tumEventGetMouseX(), -DEFAULT_FONT_SIZE + factor*tumEventGetMouseY(), Black),
                  __FUNCTION__);
}

typedef struct buttonCounter{
    int ai;
    int bi;
    int ci;
    int di;
    int ei;
    int ni;
    int zi;
    int previous_state_a;
    int previous_state_b;
    int previous_state_c;
    int previous_state_d;
    int previous_state_e;
    int previous_state_n;
    int previous_state_z;

}buttonCounter_t;

void buttonCounterInit(buttonCounter_t *counter)
{
    (*counter).previous_state_a=0;
    (*counter).ai=0;
    (*counter).previous_state_b=0;
    (*counter).bi=0;
    (*counter).previous_state_c=0;
    (*counter).ci=0;
    (*counter).previous_state_d=0;
    (*counter).di=0;
    (*counter).previous_state_e=0;
    (*counter).ei=0;
}

void DrawButtonCount( buttonCounter_t *counter)
{

    static char str[100] = { 0 };

    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {

        
        if(buttons.buttons[KEYCODE(A)]==1)
        {
        
            if((*counter).previous_state_a!=1)
            {
                (*counter).previous_state_a=1;
                (*counter).ai=(*counter).ai+1;
            }
        } else (*counter).previous_state_a=0;
       
       if(buttons.buttons[KEYCODE(B)]==1)
        {
            if((*counter).previous_state_b!=1)
            {
                (*counter).previous_state_b=1;
                (*counter).bi=(*counter).bi+1;
            }
        } else (*counter).previous_state_b=0;
       
       if(buttons.buttons[KEYCODE(C)]==1)
        {
            if((*counter).previous_state_c!=1)
            {
                (*counter).previous_state_c=1;
                (*counter).ci=(*counter).ci+1;
            }
        } else (*counter).previous_state_c=0;
       
       if(buttons.buttons[KEYCODE(D)]==1)
        {
            if((*counter).previous_state_d!=1)
            {
                (*counter).previous_state_d=1;
                (*counter).di=(*counter).di+1;
            }
        } else (*counter).previous_state_d=0;
       
        if(tumEventGetMouseRight()==1)
        {
            (*counter).ai=0;
            (*counter).bi=0;
            (*counter).ci=0;
            (*counter).di=0;
        }
        sprintf(str, "A: %d | B: %d | C: %d | D: %d",
                (*counter).ai,
                (*counter).bi,
                (*counter).ci,
                (*counter).di);
        
        xSemaphoreGive(buttons.lock);
        checkDraw(tumDrawText(str,factor*tumEventGetMouseX(), DEFAULT_FONT_SIZE + factor*tumEventGetMouseY(), Black),
                  __FUNCTION__);
        
    }
}

static int vCheckStateInput(void)
{
    if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
        if (buttons.buttons[KEYCODE(E)]) {
            buttons.buttons[KEYCODE(E)] = 0;
            if (StateQueue) {
                xSemaphoreGive(buttons.lock);
                xQueueSend(StateQueue, &next_state_signal,0);
                return 0;
            }
            return -1;
        }
        xSemaphoreGive(buttons.lock);
    }

    return 0;
}

struct my_struct{
    SemaphoreHandle_t lock;
    int counterTick;
};
struct my_struct my_struct_instance={.lock=NULL};

void vTask1(void *pvParameters)
{
  
    const TickType_t xDelay=1;
    int i=1;
    while (1) {

        
        vTaskDelay(xDelay);
        if(xSemaphoreTake(my_struct_instance.lock,portMAX_DELAY)==pdTRUE)
        {
            
            if(my_struct_instance.counterTick<15)
            {
                my_struct_instance.counterTick= my_struct_instance.counterTick+1;
                xQueueSend(QueueTasks,&i,portMAX_DELAY);
            }
        
             
             xSemaphoreGive(my_struct_instance.lock);
        } 

    }
}
void vTask2(void *pvParameters)
{  
    const TickType_t xDelay=2;
    int i=2;
    while (1) {

        
        vTaskDelay(xDelay);
      
        
        if(xSemaphoreTake(my_struct_instance.lock,portMAX_DELAY)==pdTRUE)
        {
            if(my_struct_instance.counterTick<15)
            {
                xQueueSend(QueueTasks,&i,portMAX_DELAY);
            }
            xSemaphoreGive(my_struct_instance.lock);
        }

        
        xSemaphoreGive(mySyncSignalTask);

    }
}
void vTask3(void *pvParameters)
{
    int i=3;
    while (1) {
        if (mySyncSignalTask)
                   if (xSemaphoreTake(mySyncSignalTask, STATE_DEBOUNCE_DELAY) ==
                  pdTRUE) {
                     
                      if(xSemaphoreTake(my_struct_instance.lock,portMAX_DELAY)==pdTRUE)
                     {
                        if(my_struct_instance.counterTick<15)
                        {
                            xQueueSend(QueueTasks,&i,portMAX_DELAY);
                        }
                        xSemaphoreGive(my_struct_instance.lock);
                     }
                        

        }

    }
}
void vTask4(void *pvParameters)
{
    const TickType_t xDelay=4;
    int i=4;
    while (1) {
        vTaskDelay(xDelay);
       
        if(xSemaphoreTake(my_struct_instance.lock,portMAX_DELAY)==pdTRUE)
        {
            if(my_struct_instance.counterTick<15)
            {
                xQueueSend(QueueTasks,&i,portMAX_DELAY);
            }
            xSemaphoreGive(my_struct_instance.lock);
        }
        

    }
}
void vDemoTask3(void *pvParameters)
{
    

    my_struct_instance.lock=xSemaphoreCreateMutex();
    QueueTasks=xQueueCreate(60,sizeof(int));
    int i;
    int j=0;
    static char str1[100] = { 0 };
    static char str2[100] = { 0 };
    char output[15][15];
   
  
   
    for(int l=0;l<15;l++)
    {
        strcpy(output[l]," ");
    
    }
   

    xTaskCreate(vTask1, "vTask1", mainGENERIC_STACK_SIZE, NULL, 1 , &Task1 );
    xTaskCreate(vTask2, "vTask2", mainGENERIC_STACK_SIZE, NULL, 2 , &Task2);
    xTaskCreate(vTask3, "vTask3", mainGENERIC_STACK_SIZE, NULL, 3 , &Task3 );
    xTaskCreate(vTask4, "vTask4", mainGENERIC_STACK_SIZE, NULL, 4 , &Task4 );

     while (1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                checkDraw(tumDrawClear(White), __FUNCTION__);


                
               if(xSemaphoreTake(my_struct_instance.lock,portMAX_DELAY)==pdTRUE)
                {
                         if(my_struct_instance.counterTick==15)
                         {
                            if(uxQueueMessagesWaiting(QueueTasks))
                            {
                                xQueueReceive(QueueTasks,&i,portMAX_DELAY);
                                
                                sprintf(str2, "%d",i);
                                strcat(str1,str2);
                                if(i==1)
                                {
                          
                                    sprintf(output[j],"Tick %d : ",j+1);
                                    strcat(output[j],str1);
                                  
                                    strcpy(str1,"");
                                    
                                    j++;

                                }
                            }
                           
                         }
                         
                        xSemaphoreGive(my_struct_instance.lock);
                 }
                
                 for(int k=0;k<15;k++)
                {
                    checkDraw(tumDrawText(output[k], 10, 10+20*k, Black),__FUNCTION__);
                }    
                            

                xSemaphoreGive(ScreenLock);
                xGetButtonInput();
                vCheckStateInput();
            }
     }

}

void vDemoTask2(void *pvParameters)
{

    buttonCounter_t counter;
    buttonCounterInit(&counter);

    Rotate_t rotate;

    coord_t triangle[3];

    RotatingFiguresInit(&rotate);

    int moveX=200;
    int direction=0;

    
    while (1) {
        if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
              

                xSemaphoreTake(ScreenLock, portMAX_DELAY);

                // Clear screen
                checkDraw(tumDrawClear(White), __FUNCTION__);


                // Exercise 2.1
                TriangleDraw(triangle);
                tumDrawTriangle(triangle,Black);

                RotatingFiguresDraw(&rotate);

                tumDrawText("this text is under rotating figures!",200+factor*tumEventGetMouseX(), triangle_high/2 +400 +factor*tumEventGetMouseY() ,Black);

                
                if(direction==0)
                {
                    moveX++;
                    if(moveX>400)
                        direction=1;
                }
                else
                {
                    moveX--;
                    if(moveX<200)
                        direction=0;
                }
                
                tumDrawText("Wuhuuuu",moveX+factor*tumEventGetMouseX(),triangle_high+factor*tumEventGetMouseY() ,Black);

                

                 //Exercise 2.2
                xGetButtonInput(); // Update global input
                DrawButtonCount(&counter);


                // Exercise 2.3
                Mouse();

                xSemaphoreGive(ScreenLock);
                vCheckStateInput();
            }
    }
}


void CircleTaskD(void *pvParameters)
{
    const TickType_t xperiod = 250/portTICK_PERIOD_MS;
    int flagR;
    while (1) {
                if(flagR==0)
                    flagR=1;
                else
                    flagR=0;
                
                xQueueSend(FlagQueueR,&flagR, portMAX_DELAY );
                vTaskDelay(xperiod) ;
          
    }
}
void CircleTaskS()
{
    const TickType_t xperiod = 500/portTICK_PERIOD_MS;
    int flagL;
    
    while (1) {
                if(flagL==0)
                    flagL=1;
                else
                    flagL=0;
                xQueueSend(FlagQueueL,&flagL, portMAX_DELAY );
                vTaskDelay(xperiod);
       
    }
}

void myTimerCallback(TimerHandle_t xTimer)
{
    int mi;
    int ni;

    mi=0;
    ni=0;
    xQueueOverwrite(Button1Queue,&ni);
    xQueueOverwrite(Button2Queue,&mi);
}

void Button2(void *pvParameters)
{
    int task_notification;
    
    int mi;

    while(1){
        if((task_notification = ulTaskNotifyTake(pdTRUE,portMAX_DELAY)))
            xQueueReceive(Button2Queue,&mi,0);
            mi=mi+1;
            xQueueSend(Button2Queue,&mi, portMAX_DELAY );

    }
}


void Button1(void *pvParameters)
{
    int ni;
    while (1) {
                if (mySyncSignal)
                   if (xSemaphoreTake(mySyncSignal, STATE_DEBOUNCE_DELAY) ==
                  pdTRUE) {
                   
                 xQueueReceive(Button1Queue,&ni,0);
              
                   ni=ni+1;
                     
                xQueueSend(Button1Queue,&ni, portMAX_DELAY );
               
                }
         }
    }

void Button3(void *pvParameters)
{
     const TickType_t xDelay = 1000/portTICK_PERIOD_MS;
     int ki=0;
     while(1)
     {
        ki=ki+1;
        xQueueSend(Button3Queue,&ki, portMAX_DELAY );
        vTaskDelay(xDelay);
        
       
     }
}

void vDemoTask1(void *pvParameters)
{  

    //Exercise 3.2.2
    HandleCircleTwo = xTaskCreateStatic(CircleTaskS, "CircleTaskS", mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY, xStack, &xTaskBuffer);
    xTaskCreate(CircleTaskD, "CircleTaskD", mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY , &HandleCircleOne );
    vTaskResume(HandleCircleTwo);
    vTaskResume(HandleCircleOne);
    
    //Exercise 3.2.3
    xTaskCreate(Button1, "Button1", mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY+1 , &HandleButtonOne );
    xTaskCreate(Button2, "Button2", mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY+1  , &HandleButtonTwo );
    //Exercise 3.2.4
    xTaskCreate(Button3, "Button3", mainGENERIC_STACK_SIZE, NULL, mainGENERIC_PRIORITY+2  , &HandleButtonThree );

    vTaskResume(HandleButtonOne);
    vTaskResume(HandleButtonTwo);
    vTaskSuspend(HandleButtonThree);
     
    //Exercise 3.2.3.5
    myTimer= xTimerCreate("Mytimer", pdMS_TO_TICKS(15000),pdTRUE,(void*)0,myTimerCallback);
    xTimerStart(myTimer,0);
    
    //Flags of the blinking circles
    FlagQueueR = xQueueCreate(10,sizeof(int));
    FlagQueueL = xQueueCreate(10,sizeof(int));
    int flagR;
    int flagL;

    //int button=0;

    //Queues to get the counter values
    Button1Queue=xQueueCreate(1,sizeof(int));
    Button2Queue=xQueueCreate(1,sizeof(int));
    Button3Queue=xQueueCreate(1,sizeof(int));
    int counterN=0;
    int counterM=0;
    int counterK=0;
    int ni=0;
    int mi;
    int ki;
    static char strN[100] = { 0 };
    static char strM[100] = { 0 };
    static char strK[100] = { 0 };
    int toggleK=0;
    
    while (1) {
       if (DrawSignal)
            if (xSemaphoreTake(DrawSignal, portMAX_DELAY) ==
                pdTRUE) {
                tumEventFetchEvents(FETCH_EVENT_BLOCK |
                                    FETCH_EVENT_NO_GL_CHECK);
    

                xSemaphoreTake(ScreenLock, portMAX_DELAY);
                // Clear screen
                checkDraw(tumDrawClear(White), __FUNCTION__);
                
                
                xQueueReceive(FlagQueueR, &flagR,0);
                xQueueReceive(FlagQueueL, &flagL,0);

                
                if((counterN==0)&&(counterM==0)&&(counterK==0))
                {
                    if (flagR==1)
                    {
                        tumDrawCircle(middleX+100,middleY,50,Black);
                    }
                    if (flagL==1)
                    {
                        tumDrawCircle(middleX-100,middleY,50,Black);
                    }
                }
                if(counterN==1)
                {
                    xQueueReceive(Button1Queue,&ni,0);
                    sprintf(strN, "N: %d",ni);
                    checkDraw(tumDrawText(strN, middleX, middleY-100, Black),__FUNCTION__);
                    xQueueSend(Button1Queue,&ni, portMAX_DELAY );
                }
                if(counterM==1)
                {
                    xQueueReceive(Button2Queue,&mi,0);
                    sprintf(strM, "M: %d",mi);
                    checkDraw(tumDrawText(strM, middleX, middleY, Black),__FUNCTION__);
                    xQueueSend(Button2Queue,&mi, portMAX_DELAY );
                }
                if(counterK==1)
                {
                    xQueueReceive(Button3Queue,&ki,0);
                    sprintf(strK, "K: %d",ki);
                    checkDraw(tumDrawText(strK, middleX, middleY+100, Black),__FUNCTION__);
                   
                }
                
                 
                
               // Draw FPS in lower right corner
                vDrawFPS();


                //Exercise 3.2.3
                xGetButtonInput();
                if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                     if (buttons.buttons[KEYCODE(M)]) {
                        buttons.buttons[KEYCODE(M)]=0;
                    counterM=1;
                    xSemaphoreGive(buttons.lock);
                    xTaskNotifyGive(HandleButtonTwo);
                    }
                    if (buttons.buttons[KEYCODE(N)]) {
                        buttons.buttons[KEYCODE(N)]=0;
                    counterN=1;
                    xSemaphoreGive(buttons.lock);
                    xSemaphoreGive(mySyncSignal);
                    }
                    if (buttons.buttons[KEYCODE(K)]) {
                        buttons.buttons[KEYCODE(K)]=0;
                        if(toggleK==0)
                        {
                            vTaskResume(HandleButtonThree);
                            toggleK=1;
                        }
                        else
                        {
                            vTaskSuspend(HandleButtonThree);
                            toggleK=0;
                        }
                        
                        counterK=1;
                    }
                   
                }
               
               if (mySyncSignal==NULL)
                {
                  printf(" sync signal null");
     
                }
                
                xSemaphoreGive(buttons.lock);
                xSemaphoreGive(ScreenLock);

                vCheckStateInput();
            }
          
            
        }
   
}
#define PRINT_TASK_ERROR(task) PRINT_ERROR("Failed to print task ##task");

int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);
   
    
    
    
    prints("Initializing: ");

    //  Note PRINT_ERROR is not thread safe and is only used before the
    //  scheduler is started. There are thread safe print functions in
    //  TUM_Print.h, `prints` and `fprints` that work exactly the same as
    //  `printf` and `fprintf`. So you can read the documentation on these
    //  functions to understand the functionality.

    if (tumDrawInit(bin_folder_path)) {
        PRINT_ERROR("Failed to intialize drawing");
        goto err_init_drawing;
    }
    else {
        prints("drawing");
    }

    if (tumEventInit()) {
        PRINT_ERROR("Failed to initialize events");
        goto err_init_events;
    }
    else {
        prints(", events");
    }

    if (tumSoundInit(bin_folder_path)) {
        PRINT_ERROR("Failed to initialize audio");
        goto err_init_audio;
    }
    else {
        prints(", and audio\n");
    }

    if (safePrintInit()) {
        PRINT_ERROR("Failed to init safe print");
        goto err_init_safe_print;
    }

    logo_image = tumDrawLoadImage(LOGO_FILENAME);

    atexit(aIODeinit);

    //Load a second font for fun
    tumFontLoadFont(FPS_FONT, DEFAULT_FONT_SIZE);

    
        
     

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

    mySyncSignal= xSemaphoreCreateBinary();
    if (!mySyncSignal)
    {
        PRINT_ERROR("Failed to create sync signal");
        goto err_sync;
    }

    mySyncSignalTask= xSemaphoreCreateBinary();
    if (!mySyncSignalTask)
    {
        PRINT_ERROR("Failed to create sync signal");
        goto err_sync;
    }

    // Message sending
    StateQueue = xQueueCreate(STATE_QUEUE_LENGTH, sizeof(unsigned char));
    if (!StateQueue) {
        PRINT_ERROR("Could not open state queue");
        goto err_state_queue;
    }

    if (xTaskCreate(basicSequentialStateMachine, "StateMachine",
                    mainGENERIC_STACK_SIZE * 2, NULL,
                    configMAX_PRIORITIES - 1, StateMachine) != pdPASS) {
        PRINT_TASK_ERROR("StateMachine");
        goto err_statemachine;
    }
    if (xTaskCreate(vSwapBuffers, "BufferSwapTask",
                    mainGENERIC_STACK_SIZE * 2, NULL, configMAX_PRIORITIES,
                    BufferSwap) != pdPASS) {
        PRINT_TASK_ERROR("BufferSwapTask");
        goto err_bufferswap;
    }

    /** Demo Tasks */
    
    if (xTaskCreate(vDemoTask1, "DemoTask1", mainGENERIC_STACK_SIZE * 2,
                    NULL, mainGENERIC_PRIORITY+3, &DemoTask1) != pdPASS) {
        PRINT_TASK_ERROR("DemoTask1");
        goto err_demotask1;
    }
    if (xTaskCreate(vDemoTask2, "DemoTask2", mainGENERIC_STACK_SIZE * 2,
                    NULL, mainGENERIC_PRIORITY+1, &DemoTask2) != pdPASS) {
        PRINT_TASK_ERROR("DemoTask2");
        goto err_demotask2;
    }
    if (xTaskCreate(vDemoTask3, "DemoTask3", mainGENERIC_STACK_SIZE * 2,
                    NULL, mainGENERIC_PRIORITY+1, &DemoTask3) != pdPASS) {
        PRINT_TASK_ERROR("DemoTask3");
        goto err_demotask3;
    }
    
    
    vTaskSuspend(DemoTask1);
    vTaskSuspend(DemoTask2);
    vTaskSuspend(DemoTask3);

   

    tumFUtilPrintTaskStateList();

    
    vTaskStartScheduler();


    return EXIT_SUCCESS;
err_demotask3:
    vTaskDelete(DemoTask2);
err_demotask2:
    vTaskDelete(DemoTask1);
err_demotask1:
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
    vSemaphoreDelete(mySyncSignal);
err_sync:
    vSemaphoreDelete(buttons.lock);
err_buttons_lock:
    tumSoundExit();
err_init_audio:
    tumEventExit();
err_init_events:
    tumDrawExit();
err_init_drawing:
    safePrintExit();
err_init_safe_print:
    return EXIT_FAILURE;
}

// cppcheck-suppress unusedFunction
__attribute__((unused)) void vMainQueueSendPassed(void)
{
    /* This is just an example implementation of the "queue send" trace hook. */
//}

// cppcheck-suppress unusedFunction
}
__attribute__((unused)) void vApplicationIdleHook(void)
{
#ifdef __GCC_POSIX__
    struct timespec xTimeToSleep, xTimeSlept;
    //Makes the process more agreeable when using the Posix simulator. */
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

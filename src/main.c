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

#define mainGENERIC_PRIORITY (tskIDLE_PRIORITY)
#define mainGENERIC_STACK_SIZE ((unsigned short)2560)
#define STATE_DEBOUNCE_DELAY 300

int level;
int total_line;
int score;


static TaskHandle_t DemoTask = NULL;
static TaskHandle_t HandleTask= NULL;
static TaskHandle_t HandleChangeX= NULL;
static TaskHandle_t HandleChangeY= NULL;
static TaskHandle_t HandleUpdateY= NULL;
static TaskHandle_t HandleUpdateX= NULL;

QueueHandle_t BoundrysQueue =NULL;
QueueHandle_t YQueue=NULL;

static SemaphoreHandle_t SignalX=NULL;
static SemaphoreHandle_t syncSignal=NULL;

TimerHandle_t myTimer= NULL;

struct my_X{
    int condition_X_Right;
    int condition_X_Left
};
struct my_struct_X{
    SemaphoreHandle_t lock_X;
    struct my_X my_X_instance;
};

struct my_struct_X my_struct_instance_X={.lock_X=NULL};

int y;

struct my_Y{
    int condition_Y2;
    int condition_Y;
};

struct my_struct_Y{
    SemaphoreHandle_t lock_Y;
    struct my_Y my_Y_instance;
};

struct my_struct_Y my_struct_instance_Y={.lock_Y=NULL};

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

void move_tetrimino(int x, int y, int px, int py)
{

    int xmin=6;
    int xmax=0;
    int ymax=0;
    int pxmin;
    int pxmax;
    int pymax;

    if(xSemaphoreTake(my_struct_instance_Y.lock_Y,portMAX_DELAY)==pdTRUE)
    {
        my_struct_instance_Y.my_Y_instance.condition_Y=1;
        xSemaphoreGive(my_struct_instance_Y.lock_Y);
    }
                                    
    if(xSemaphoreTake(my_struct_instance_X.lock_X,portMAX_DELAY)==pdTRUE)
    {
         my_struct_instance_X.my_X_instance.condition_X_Right=1;
         my_struct_instance_X.my_X_instance.condition_X_Left=1;
    xSemaphoreGive(my_struct_instance_X.lock_X);
    }

    //xQueueOverwrite(Que_cond_Y,&condition_Y);
    //xQueueOverwrite(Que_cond_X_R,&condition_X_Right);
    

   if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
    {
        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
            {   
                if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
                 {  
                    xQueueReceive(BoundrysQueue,&pxmin,0);
                    xQueueReceive(BoundrysQueue,&pxmax,0);
                    xQueueReceive(BoundrysQueue,&pymax,0);
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
                    
                    
                    xQueueSend(BoundrysQueue,&xmin,portMAX_DELAY);
                    xQueueSend(BoundrysQueue,&xmax,portMAX_DELAY);
                    xQueueSend(BoundrysQueue,&ymax,portMAX_DELAY);
                    /*
                    if(xmin!=pxmin)
                    {
                    printf("current = %d, %d, %d \n", xmin, xmax,ymax);
                    printf("past= %d, %d, %d \n", pxmin, pxmax,pymax);
                    }
                   */
                   // printf("current = %d, %d, %d \n", xmin, xmax,ymax);

                    for(int i=0; i<12; i++)
                    {   
                        for(int j=0; j<21; j++)
                        {
                            my_struct_instance_frame.Frame[i][j]= 0;
                        }
                    }
                   
                  
                    //printf("-> %d, %d, %d \n",xmin,xmax,ymax);

                    for(int i=xmin; i<xmax; i++)
                    {
                        for(int j=0; j<ymax; j++)
                        {
                            my_struct_instance_frame.Frame[x+i][y+j]= my_struct_instance_shape.Shape[i][j];
                        }
                    }    
                    
                    for(int i=0; i<12 ; i++)
                    {
                        for(int j=0; j<21; j++)
                        {
                            if((my_struct_instance_grid.Grid[i][j]==1) & (my_struct_instance_frame.Frame[i][j]!=0) )
                            {
                                if(my_struct_instance_frame.Frame[i][j]==3)
                                {
                                     if(xSemaphoreTake(my_struct_instance_Y.lock_Y,portMAX_DELAY)==pdTRUE)
                                     {
                                         my_struct_instance_Y.my_Y_instance.condition_Y=0;
                                         xSemaphoreGive(my_struct_instance_Y.lock_Y);
                                     }
                                    
                                    //xQueueOverwrite(Que_cond_Y,&condition_Y);
                                }
                                if(xSemaphoreTake(my_struct_instance_X.lock_X,portMAX_DELAY)==pdTRUE)
                                {
                                    if(my_struct_instance_frame.Frame[i][j]==4)
                                    {
                                        my_struct_instance_X.my_X_instance.condition_X_Right=0;
                                    }
                                    if(my_struct_instance_frame.Frame[i][j]==2)
                                    {
                                        my_struct_instance_X.my_X_instance.condition_X_Left=0;
                                    }
                                    //printf("%d, %d \n",my_struct_instance_X.my_X_instance.condition_X_Right, my_struct_instance_X.my_X_instance.condition_X_Left);
                                xSemaphoreGive(my_struct_instance_X.lock_X);
                                }
                            }
                        }
                    }
                }
                xSemaphoreGive(my_struct_instance_shape.lock_shape); 
        }
        xSemaphoreGive(my_struct_instance_grid.lock_grid);  
            
    xSemaphoreGive(my_struct_instance_frame.lock_frame);
    } 
}


void control_elements()
{
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    {
        for(int i=0; i<6;i++)
        {
            for(int j=0;j<5;j++)
            {
                if(my_struct_instance_shape.Shape[i][j]==1)
                {
                    if(my_struct_instance_shape.Shape[i][j+1]!=1)
                        my_struct_instance_shape.Shape[i][j+1]=3;

                    if(my_struct_instance_shape.Shape[i+1][j]==0)
                    {
                        my_struct_instance_shape.Shape[i+1][j]=4;
                    }
                        
                    if(my_struct_instance_shape.Shape[i-1][j]==0)
                    {
                        my_struct_instance_shape.Shape[i-1][j]=2;
                    }

                }
            }
        }
        /*
        for(int i=0; i<6;i++)
        {
            for(int j=0;j<5;j++)
            {
                if(my_struct_instance_shape.Shape[i][j]==3)
                {

                    if(my_struct_instance_shape.Shape[i+1][j]==0 & my_struct_instance_shape.Shape[i+1][j-1]!=3)
                    {
                        my_struct_instance_shape.Shape[i+1][j]=4;
                    }
                        
                    if(my_struct_instance_shape.Shape[i-1][j]==0 & my_struct_instance_shape.Shape[i-1][j-1]!=3)
                    {
                        my_struct_instance_shape.Shape[i-1][j]=2;
                    }

                }
                
            }
        }*/
    
    for(int j=0;j<5;j++)
        {
            printf ( "\n" ); 
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,my_struct_instance_shape.Shape[i][j] );    
            }
     }
     printf ( "\n" );
    xSemaphoreGive(my_struct_instance_shape.lock_shape);
    }
}
void Tetrimino()
{
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    int Tetrimino= my_struct_instance_tetri.Tetri_number;
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
    
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    {   
        for(int i=0; i<6;i++)
        {
            for(int j=0; j<5; j++)
            {
                my_struct_instance_shape.Shape[i][j]=0;
            }
        }
        switch(Tetrimino)
        {
            case 1 : //T
                my_struct_instance_shape.Shape[1][2]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[3][2]=1;
                my_struct_instance_shape.Shape[2][3]=1;
                break;
            case 0 : //J
                my_struct_instance_shape.Shape[1][2]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[3][2]=1;
                my_struct_instance_shape.Shape[3][3]=1;
                break;
            case 2 : //Z
                my_struct_instance_shape.Shape[1][2]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[2][3]=1;
                my_struct_instance_shape.Shape[3][3]=1;
                break;
            case 3 : //O
                my_struct_instance_shape.Shape[1][2]=1;
                my_struct_instance_shape.Shape[1][3]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[2][3]=1;
                break;
            case 4 : //S
                my_struct_instance_shape.Shape[1][3]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[2][3]=1;
                my_struct_instance_shape.Shape[3][2]=1;
                break;
            case 5 : //L
                my_struct_instance_shape.Shape[1][2]=1;
                my_struct_instance_shape.Shape[1][3]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[3][2]=1;
                break;
            case 6 : //I
                my_struct_instance_shape.Shape[1][2]=1;
                my_struct_instance_shape.Shape[3][2]=1;
                my_struct_instance_shape.Shape[2][2]=1;
                my_struct_instance_shape.Shape[4][2]=1;
                break;
            
            default:
                  break;
        }
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
                control_elements();
        
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
    
    
}
void rotate_I()
{
    if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
    {
        if(my_struct_instance_shape.Shape[2][0]==1)
        {
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                
                    my_struct_instance_shape.Shape[i][j]=0;
                }
            }
            my_struct_instance_shape.Shape[1][2]=1;
            my_struct_instance_shape.Shape[3][2]=1;
            my_struct_instance_shape.Shape[2][2]=1;
            my_struct_instance_shape.Shape[4][2]=1;
        }
        else
        {
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                
                    my_struct_instance_shape.Shape[i][j]=0;
                }
            }
            my_struct_instance_shape.Shape[2][0]=1;
            my_struct_instance_shape.Shape[2][1]=1;
            my_struct_instance_shape.Shape[2][2]=1;
            my_struct_instance_shape.Shape[2][3]=1;
            
        }
        
        for(int j=0;j<5;j++)
        {
            printf("\n");
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,my_struct_instance_shape.Shape[i][j]);    
            }
        }
        printf("\n");
    xSemaphoreGive(my_struct_instance_shape.lock_shape);
    }
    //control_elements();
}
void rotate_tetrimino_L()
{
    printf("girdi");
    int tmp[4][4];
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    if(my_struct_instance_tetri.Tetri_number!=6)
    {
        if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
        {
        
            for(int i=0; i<4; i++)
            {
                for(int j=0; j<4; j++)
                {
                    if(my_struct_instance_shape.Shape[i][j]==1)
                        tmp[i][j]=1;
                    else
                        tmp[i][j]=0;
                }
            }
            for(int x=0; x<4; x++)
            {
                for(int y=x; y<4-x-1;y++)
                {
                
                    int temp=tmp[x][y];
                    tmp[x][y]=tmp[y][4-1-x];
                    tmp[y][4-1-x]=tmp[4-1-x][4-1-y];
                    tmp[4-1-x][4-1-y]=tmp[4-1-y][x];
                    tmp[4-1-y][x]=temp;
                
                }
            }
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=0;
                }
            }
            for(int i=0; i<4; i++)
            {
                for(int j=0; j<4; j++)
                {
                    my_struct_instance_shape.Shape[i+1][j]=tmp[i][j];
                }
            }
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
        }
        
    }
    else
    {
            rotate_I();
    }
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);

    control_elements();
        
/*
          for(int j=0;j<5;j++)
        {
            printf("\n");
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,my_struct_instance_shape.Shape[i][j]);    
            }
        }
        printf("\n");*/
    
    
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
    int m=5;
    int n=5;
    int tmp[6][5];
    xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
    if(my_struct_instance_tetri.Tetri_number!=6)
    {   
        if(xSemaphoreTake(my_struct_instance_shape.lock_shape,portMAX_DELAY)==pdTRUE)
        {  
        
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                
                    tmp[i][j]=0;
                }
            }
            for(int i=m-1; i>=0; i--)
            {
                for(int j=0; j<n; j++)
                {
                    if(my_struct_instance_shape.Shape[i][j]==1)
                        tmp[j][m-i-1]=my_struct_instance_shape.Shape[i][j];   
                    else 
                        tmp[j][m-i-1]=0;
                }
            }
            for(int i=0; i<6; i++)
            {
                for(int j=0; j<5; j++)
                {
                    my_struct_instance_shape.Shape[i][j]=tmp[i][j];
                }
            }
        xSemaphoreGive(my_struct_instance_shape.lock_shape);
        }
    }
    else
    {
        rotate_I();
    }
    xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
    control_elements();   

           /* for(int j=0;j<5;j++)
        {
            printf("\n");
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,my_struct_instance_shape.Shape[i][j]);    
            }
        }
        printf("\n");*/
     
    
    
    

    /*for(int j=0;j<5;j++)
        {
            printf ( "\n" ); 
            for(int i=0; i<6;i++)
            {
               printf ( "%d " ,tmp[i][j] );    
            }
     }
     printf("\n");
     printf("------------------\n");*/
     
}


int  py=0;

/*
void TimerY(TimerHandle_t xTimer)
{
    //int y;
    //int py;
    //xQueueReceive(YQueue,&y,0);
    //xQueueReceive(YQueue,&py,0);
   if(xSemaphoreTake(my_struct_instance_Y.lock_Y,portMAX_DELAY)==pdTRUE)
   {
   
    
    //printf("%d\n",condition_Y);
        if(my_struct_instance_Y.my_Y_instance.condition_Y==1)
        {
            py=y;
            y=y+1;
        }   
      
    xSemaphoreGive(my_struct_instance_Y.lock_Y);
   }
    //xQueueSend(YQueue,&y,portMAX_DELAY);
    //xQueueSend(YQueue,&py,portMAX_DELAY);

}*/
int x=5;
    //int y=0;
int px=5;
int delay;

void UpdateY()
{
    delay=600-level*56;
    while(1)
    {
        py=y;
        if(x==px)
        {
            if(xSemaphoreTake(my_struct_instance_Y.lock_Y,portMAX_DELAY)==pdTRUE)
            {
                if(my_struct_instance_Y.my_Y_instance.condition_Y==1)
                {
                    my_struct_instance_Y.my_Y_instance.condition_Y2=0;
                    y=y+1;
                }
                else
                {
                    my_struct_instance_Y.my_Y_instance.condition_Y2++;
                }
            xSemaphoreGive(my_struct_instance_Y.lock_Y);
            }
                
        }
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
}
void UpdateX()
{
    int Flag_R=0;
    int Flag_L=0;
    int Flag_D=0;
    int Flag_A=0;
    int Flag_B=0;

    

    while(1)
    {
       if(xSemaphoreTake(my_struct_instance_X.lock_X,portMAX_DELAY)==pdTRUE)
       {
           
            

            if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
                px=x;  
                xGetButtonInput();
                if (buttons.buttons[79]) { 
                
                    if(my_struct_instance_X.my_X_instance.condition_X_Right==1) 
                    {
                        x=x+1;
                    }
                }
                    
                

                if (buttons.buttons[80]) { 
                    if(my_struct_instance_X.my_X_instance.condition_X_Left==1) 
                    {
                        x=x-1;
                    }  
                }
                
                
                if (buttons.buttons[81]) { 
                    if(Flag_D==0)
                    {
                        Flag_D=1;
                        delay=(600-level*56)/2;
                        
                    }
                    
                }else 
                {
                    if(Flag_D==1)
                    {
                        Flag_D=0;
                        delay=600-level*56;
                    }
                } 
                

                if (buttons.buttons[4]) { 
                    if(Flag_A==0)
                    {
                        xSemaphoreGive(buttons.lock);
                        Flag_A=1;
                        if(my_struct_instance_X.my_X_instance.condition_X_Left==1 & my_struct_instance_X.my_X_instance.condition_X_Right==1)
                            rotate_tetrimino_R();
                    }
                    
                }else {Flag_A=0;}

                if (buttons.buttons[5]) { 
                    if(Flag_B==0)
                    {
                        xSemaphoreGive(buttons.lock);
                        Flag_B=1;
                        if(my_struct_instance_X.my_X_instance.condition_X_Left==1 & my_struct_instance_X.my_X_instance.condition_X_Right==1)
                            rotate_tetrimino_L();
                    }
                    
                }else {Flag_B=0;}

                xSemaphoreGive(buttons.lock); 
                my_struct_instance_X.my_X_instance.condition_X_Right=1;
                my_struct_instance_X.my_X_instance.condition_X_Left=1;
                
              
            }
        xSemaphoreGive(my_struct_instance_X.lock_X);
       }
       vTaskDelay(100);
    }
}

void puf(void)
{
    int flag;
    int count_line=0;
    int factor=0;
  
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
                total_line++;
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
    score=score+factor*(level+1);
}

void Task(void *pvParameters)
{

    //int condition_Y;
    //int condition_X_Right;
    //int condition_X_Left;

    
    

    //Start position of the Tetrimino
    

    

    int flag=0;
    //myTimer = xTimerCreate("MyTimer",pdMS_TO_TICKS(300),pdTRUE,(void*)0,TimerY);
    //xTimerStart(myTimer,0);

    BoundrysQueue=xQueueCreate(3,sizeof(int));
    
    Tetrimino();
    Grid();
    
    while(1)
    {
        //printf("1\n");
        move_tetrimino(x,y,px,py);

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
                       

                        printf("yeni\n");
                        xSemaphoreTake(my_struct_instance_tetri.lock_tetri,portMAX_DELAY);
                        my_struct_instance_tetri.Tetri_number=(my_struct_instance_tetri.Tetri_number+1)%7;
                        xSemaphoreGive(my_struct_instance_tetri.lock_tetri);
                      
                        Tetrimino();
                        px=5;
                        py=0;
                        x=5;
                        y=0;
                        if(xSemaphoreTake(my_struct_instance_grid.lock_grid,portMAX_DELAY)==pdTRUE)
                        {
                            for(int i=0;i<12; i++)
                            {
                                for(int j=0; j<21; j++)
                                {
                                    if(my_struct_instance_grid.Grid[i][j]==0 && my_struct_instance_frame.Frame[i][j]==1)
                                    {
                                        my_struct_instance_grid.Grid[i][j]=1;
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

void vDemoTask(void *pvParameters)
{
    char str[16];
    total_line=0;
    level=5;
    score=0;

    // structure to store time retrieved from Linux kernel
    static struct timespec the_time;
    static char our_time_string[100];
    static int our_time_strings_width = 0;

    
    
    
    
    Que_cond_Y= xQueueCreate(1,sizeof(int));
    Que_cond_X_R= xQueueCreate(1,sizeof(int));
    Que_cond_X_L= xQueueCreate(1,sizeof(int));

    //YQueue=xQueueCreate(2,sizeof(int));
    
    my_struct_instance_grid.lock_grid=xSemaphoreCreateMutex();
    my_struct_instance_frame.lock_frame=xSemaphoreCreateMutex();
    my_struct_instance_shape.lock_shape=xSemaphoreCreateMutex();
    my_struct_instance_tetri.lock_tetri=xSemaphoreCreateMutex();
     my_struct_instance_Y.lock_Y=xSemaphoreCreateMutex();
     my_struct_instance_X.lock_X=xSemaphoreCreateMutex();

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
    tumDrawBindThread();

    for(int i=12;i<20;i++)
    {
        for (int j=0;j<21;j++)
        {
            tumDrawFilledBox(20*i+200,20*j+20,20,20,TUMBlue);
        }
            
    } 
    

    while (1) {
        tumEventFetchEvents(FETCH_EVENT_NONBLOCK); // Query events backend for new events, ie. button presses
        xGetButtonInput(); // Update global input

        // `buttons` is a global shared variable and as such needs to be
        // guarded with a mutex, mutex must be obtained before accessing the
        // resource and given back when you're finished. If the mutex is not
        // given back then no other task can access the reseource.
       /* if (xSemaphoreTake(buttons.lock, 0) == pdTRUE) {
            if (buttons.buttons[KEYCODE(
                                    Q)]) { // Equiv to SDL_SCANCODE_Q
                exit(EXIT_SUCCESS);
            }
            xSemaphoreGive(buttons.lock);
        }*/

        //tumDrawClear(White); // Clear screen

        clock_gettime(CLOCK_REALTIME,
                      &the_time); // Get kernel real time

        // Format our string into our char array
        //sprintf(our_time_string,
        //        "There has been %ld seconds since the Epoch. Press Q to quit",
         //       (long int)the_time.tv_sec);

        //tumDrawBox(199,19,203,403,Black);

        if(xSemaphoreTake(my_struct_instance_frame.lock_frame,portMAX_DELAY)==pdTRUE)
        {
            for(int i=0;i<12;i++)
            {
                for(int j=0;j<21;j++)
                {
                    if(my_struct_instance_frame.Frame[i][j]==1)
                        tumDrawFilledBox(20*i+200,20*j+20,20,20,Red);
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
                    if(my_struct_instance_grid.Grid[i][j]==1)
                        tumDrawFilledBox(20*i+200,20*j+20,20,20,Red);
                    //else
                        //tumDrawFilledBox(20*i+200,20*j+20,20,20,White);
                
                }
            }
            xSemaphoreGive(my_struct_instance_grid.lock_grid);
       }
       /*
       for(int i=12;i<19;i++)
        {
            for (int j=0;j<21;j++)
            {
                 tumDrawFilledBox(20*i+200,20*j+20,20,20,TUMBlue);
            }
            
        } */
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

        
        sprintf(str, "LINES: %d", total_line);
        tumDrawText(str, 20*13+210,20*2+20,Black);

        sprintf(str, "LEVEL: %d", level);
        tumDrawText(str, 20*13+210,20*5+20,Black);

        sprintf(str, "SCORE: %d", score);
        tumDrawText(str, 20*13+210,20*8+20,Black);
        
       // tumDrawFilledBox(20*x+200,20*y+20,20,20,Red);
        // Get the width of the string on the screen so we can center it
        // Returns 0 if width was successfully obtained
        if (!tumGetTextSize((char *)our_time_string,
                            &our_time_strings_width, NULL))
            tumDrawText(our_time_string,
                        SCREEN_WIDTH / 2 -
                        our_time_strings_width / 2,
                        SCREEN_HEIGHT / 2 - DEFAULT_FONT_SIZE / 2,
                        TUMBlue);

        tumDrawUpdateScreen(); // Refresh the screen to draw string

        // Basic sleep of 1000 milliseconds
        //
        vTaskDelay((TickType_t)100);
    }
}

int main(int argc, char *argv[])
{
    char *bin_folder_path = tumUtilGetBinFolderPath(argv[0]);

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

    if (xTaskCreate(vDemoTask, "DemoTask", mainGENERIC_STACK_SIZE * 2, NULL,
                    mainGENERIC_PRIORITY+3, &DemoTask) != pdPASS) {
        goto err_demotask;
    }


    vTaskStartScheduler();

    return EXIT_SUCCESS;

err_demotask:
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
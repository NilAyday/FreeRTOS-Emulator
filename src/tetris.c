
#include <stdlib.h>
#include <time.h>
#include "TUM_Draw.h"

int RGenerator(int number, int nblock, int *Block)
{
    char Tetri;
    time_t t;
    srand((unsigned) time(&t));
    if(number==0)
    {
         for(int i=0; i<nblock; i++)
        {
            Block[i]=i;
        }

        for(int i=0; i<nblock; i++)
        {
            int j= (i+ rand())%nblock;
            int t = Block[j];
            Block[j]=Block[i];
            Block[i]=t;
        }
    }
    switch(Block[number])
    {
        case 0: 
            Tetri= 'T';
            break;
        case 1:
            Tetri='J';
            break;
        case 2:
            Tetri='Z';
            break;
        case 3:
            Tetri='O';
            break;
        case 4:
            Tetri='S';
            break;
        case 5:
            Tetri='L';
            break;
        case 6:
            Tetri='I';
            break;
        default:
            break;

    }
    return Tetri;
}

int switch_color(int x)
{
    int color;
    switch(x)
    {
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
    return color;
}

void choose_tetri(int shapex, int shapey, char tetri, int *array_next)
{
    int shape[shapex][shapey];
    
    for(int i=0; i<shapex ; i++)
        for(int j=0; j<shapey ; j++) 
            shape[i][j]=0; 

    switch(tetri)
    {
        case 'T' : //T
            shape[1][2]=2;
            shape[2][2]=2;
            shape[3][2]=2;
            shape[2][3]=2;
            break;
        case 'J' : //J
            shape[1][2]=3;
            shape[2][2]=3;
            shape[3][2]=3;
            shape[3][3]=3;
            break;
        case 'Z' : //Z
            shape[1][2]=4;
            shape[2][2]=4;
            shape[2][3]=4;
            shape[3][3]=4;
            break;
        case 'O' : //O
            shape[1][2]=5;
            shape[1][3]=5;
            shape[2][2]=5;
            shape[2][3]=5;
            break;
        case 'S' : //S
            shape[1][3]=6;
            shape[2][2]=6;
            shape[2][3]=6;
            shape[3][2]=6;
            break;
        case 'L' : //L
            shape[1][2]=7;
            shape[1][3]=7;
            shape[2][2]=7;
            shape[3][2]=7;
            break;
        case 'I' : //I
            shape[1][2]=8;
            shape[2][2]=8;
            shape[3][2]=8;
            shape[4][2]=8;
            break;
        default:
            break;
    }
    for(int i=0; i<shapex ; i++)
        for(int j=0; j<shapey ; j++) 
            array_next[shapey*i+j]=shape[i][j]; 
}


int line_disappear(int gridx, int gridy, int *array_grid)
{
    int flag;
    int count_line=0;
    int grid[gridx][gridy];
    
    for(int i=0; i<gridx ; i++)
        for(int j=0; j<gridy ; j++) 
            grid[i][j]=array_grid[gridy*i+j]; 

    for(int j=gridy-2;j>1;j--)
    {
        flag=1;
        for(int i=0; i<gridx; i++)
        {
            if(grid[i][j]==0)
                flag=0;
            
        }
        if(flag==1)
        {
            for(int i=1; i<gridx-1; i++)
            {
                grid[i][j]=grid[i][j-1];

            }
            for(int k=j;k>1;k--)
            {
                for(int i=1; i<gridx-1; i++)
                {
                    grid[i][k]=grid[i][k-1];
                }
            }
            count_line++;
            j++;
        }
        for(int i=0; i<gridx ; i++)
            for(int j=0; j<gridy ; j++) 
                array_grid[gridy*i+j]=grid[i][j];      
    }    
    return count_line;
}

int move_condition(int shapex, int shapey, int gridx, int gridy, int x, int y, int *array_shape, int *array_grid)
{
    int xmin=shapex;
    int xmax=0;
    int ymax=0;
    int CFrame[gridx][gridy];
    int CFlag=0;

    int shape[shapex][shapey];
    int grid[gridx][gridy];

    for(int i=0; i<shapex ; i++)
        for(int j=0; j<shapey ; j++) 
            shape[i][j]=array_shape[shapey*i+j]; 

    for(int i=0; i<gridx ; i++)
        for(int j=0; j<gridy ; j++) 
            grid[i][j]=array_grid[gridy*i+j]; 

    for(int i=0; i<shapex;i++)
    {
        for(int j=0; j<shapey;j++)
        {
            if (shape[i][j]!=0)
            {
                    if(ymax<j+1)
                    ymax=j+1;
            }
        }
    }  
    for(int j=0; j<shapey;j++)
    {
        for(int i=0; i<shapex;i++)
        {
            if (shape[i][j]!=0)
            {
                if (xmin>i)
                    xmin=i;
                if(i+1>xmax)
                    xmax=i+1;
            }
        }
    }   
    for(int i=0; i<gridx; i++)
    {   
        for(int j=0; j<gridy; j++)
        {
            CFrame[i][j]= 0;
        }
    }
    for(int i=xmin; i<xmax; i++)
    {
        for(int j=0; j<ymax; j++)
        {
            CFrame[x+i][y+j]= shape[i][j];
        }
    }    
    for(int i=0; i<gridx; i++)
    {   
        for(int j=0; j<gridy; j++)
        {
            if((CFrame[i][j]!=0) & (grid[i][j]!=0))
            {
                CFlag=1;
            }
        }
    }
    return CFlag;
}


void move_tetrimino(int shapex, int shapey, int framex, int framey, int x, int y, int *array_shape, int *array_frame)
{
    int xmin=shapex;
    int xmax=0;
    int ymax=0;

    int shape[shapex][shapey];
    int frame[framex][framey];

    for(int i=0; i<shapex ; i++)
        for(int j=0; j<shapey ; j++) 
            shape[i][j]=array_shape[shapey*i+j]; 

    for(int i=0; i<framex ; i++)
        for(int j=0; j<framey ; j++) 
            frame[i][j]=array_frame[framey*i+j]; 

    for(int i=0; i<shapex;i++)
    {
        for(int j=0; j<shapey;j++)
        {
            if (shape[i][j]!=0)
            {
                    if(ymax<j+1)
                    ymax=j+1;
            }
        }
    }  
    for(int j=0; j<shapey;j++)
    {
        for(int i=0; i<shapex;i++)
        {
            if (shape[i][j]!=0)
            {
                if (xmin>i)
                    xmin=i;
                if(i+1>xmax)
                    xmax=i+1;
            }
        }
    }  
    
    for(int i=0; i<framex; i++)
    {   
        for(int j=0; j<framey; j++)
        {
            frame[i][j]= 0;
        }
    }
    
    for(int i=xmin; i<xmax; i++)
    {
        for(int j=0; j<ymax; j++)
        {
            frame[x+i][y+j]= shape[i][j];
        }
    }
    for(int i=0; i<framex ; i++)
        for(int j=0; j<framey ; j++) 
                array_frame[framey*i+j]=frame[i][j];      
}


void rotate_tetrimino_L(int x, int y, int *array)
{
    int tmp[x-1][y];
    int shape[x][y];

    for(int i=0; i<x ; i++)
        for(int j=0; j<y ; j++) 
            shape[i][j]=array[y*i+j]; 

    if(shape[1][2]!=5)
    {
        for(int i=0; i<x-1; i++)
        {
            for(int j=0; j<y; j++)
            {     
                tmp[i][j]=shape[i][j];
            }
        }
        for(int i=0; i<x-1; i++)
        {
            for(int j=i; j<y-i-1;j++)
            {     
                int temp=tmp[i][j];
                tmp[i][j]=tmp[j][y-1-i];
                tmp[j][y-1-i]=tmp[x-2-i][y-1-j];
                tmp[x-2-i][y-1-j]=tmp[x-2-j][i];
                tmp[x-2-j][i]=temp;
            }
        }
        for(int i=0; i<x; i++)
        {
            for(int j=0; j<y; j++)
            {
                shape[i][j]=0;
            }
        }
        for(int i=0; i<x-1; i++)
        {
            for(int j=0; j<y; j++)
            {
                shape[i][j]=tmp[i][j];
            }
        }
    }
    for(int i=0; i<x ; i++)
        for(int j=0; j<y ; j++) 
            array[y*i+j]=shape[i][j];   
}


void rotate_tetrimino_R(int x, int y, int *array)
{
    int tmp[x-1][y];
    int shape[x][y];

    for(int i=0; i<x ; i++)
        for(int j=0; j<y ; j++) 
            shape[i][j]=array[y*i+j];  

    if(shape[1][2]!=5)
    {
        for(int i=0; i<x-1; i++)
        {
            for(int j=0; j<y; j++)
            {
                tmp[i][j]=shape[i][j];
            }       
        }
        for(int i=0; i<(x-1)/2; i++)
        {
            for(int j=i; j<y-i-1;j++)
            {
            
                int temp=tmp[i][j];
                tmp[i][j]=tmp[x-2-j][i];
                tmp[x-2-j][i]=tmp[x-2-i][y-1-j];
                tmp[x-2-i][y-1-j]=tmp[j][y-1-i];
                tmp[j][x-2-i]=temp;
            }
        }
        for(int i=0; i<x; i++)
        {
            for(int j=0; j<y; j++)
            {
                shape[i][j]=0;
            }
        }
        for(int i=0; i<x-1; i++)
        {
            for(int j=0; j<y; j++)
            {
                shape[i][j]=tmp[i][j];
            }
        }
    }
    for(int i=0; i<x ; i++)
        for(int j=0; j<y ; j++) 
            array[y*i+j]=shape[i][j];   
}


void max_scores(int score, int *array_maxs)
{
    if( score > array_maxs[2])
    {
        if( score > array_maxs[1])
        {
            if( score > array_maxs[0])
            {
                array_maxs[2]=array_maxs[1];
                array_maxs[1]=array_maxs[0];
                array_maxs[0]=score;
            }
            else
            {
                array_maxs[2]=array_maxs[1];
                array_maxs[1]=score;
            }           
        }
        else
        {
            array_maxs[2]=score;
        }    
    }
}
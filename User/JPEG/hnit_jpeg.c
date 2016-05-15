/*********************** HNIT 3103 Application Team **************************
 * �� �� �� ��hnit_jpeg.c
 * ��    �� ��JPEGͼ��ѹ��        
 * ʵ��ƽ̨ ��STM32F407������
 * �� �� �� ��ST1.4.0
 * ʱ    �� ��2016.xx
 * ��    �� ��3103�����Ŷ�
 * �޸ļ�¼ ����
******************************************************************************/
#include "jpeg_data.h"
#include "hnit_jpeg.h"
#include "math.h"
#include "string.h"
#include "stm32_usart.h"
#include "stm32_delay.h"
#include "hnit_lcd.h"
#include "hnit_key.h"
#include "hnit_led.h"

uint32_t size = 0;
uint16_t encode[76800] __attribute__((at(0X68000000)));
uint16_t image[76800] __attribute__((at(0X68000000+sizeof(encode))));;

uint32_t i_size = 0;

//����������ʾ
void printf_array_f(float n[], uint16_t x, uint16_t y)
{
    uint16_t i, j;
    for(i = 0; i < x; i++)
    {
        for(j = 0; j < y; j++)
        {
            printf(" %f ", n[i*y+j]);
        }
        printf("\n");
    }
}

//����������ʾ
void printf_array_d(int8_t n[], uint16_t x, uint16_t y)
{
    uint16_t i, j;
    for(i = 0; i < x; i++)
    {
        for(j = 0; j < y; j++)
        {
            printf(" %d ", n[i*y+j]);
        }
        printf("\n");
    }
}

//�޷�������������ʾ
void printf_array_u(uint16_t n[], uint16_t x, uint16_t y)
{
    uint16_t i, j;
    for(i = 0; i < x; i++)
    {
        for(j = 0; j < y; j++)
        {
            printf(" %d ", n[i*y+j]);
        }
        printf("\n");
    }
}

//��ֵ������ʾ
void printf_array_b(uint16_t n[], uint16_t x)
{
    uint16_t i, j, temp;
    for(i = 0; i <= x; i++)
    {
        temp = n[i];
        for(j = 0; j < 16; j++)
        {
            if(temp&0x8000)
            {
                printf(" %d", 1);
            }
            else
            {
                printf(" %d", 0);
            }
            temp<<=1;
        }
        printf("\n");
    }
}


void rgb_to_yuv(uint16_t rgb[3840], uint16_t yuv[3840])
{
    uint16_t i, temp;
    float red, green, blue;
    
    for(i = 0; i < 3840; i++)
    {
        red = (float)((rgb[i]&0xF800)>>8);   
        green = (float)((rgb[i]&0x07E0)>>3);
        blue = (float)((rgb[i]&0x001F)<<3);
                         
        temp = (uint16_t)(0.299f*red + 0.587f*green + 0.114f*blue); 
        temp >>= 3;
        yuv[i] = (temp<<11 | temp<<6 | temp);
    }
}


//ͼ���ʽת�� ��ȡ����
void rgb565_to_yuv(uint16_t rgb[64], float y[64])
{
    uint16_t i;
    float red, green, blue;
    
    for(i = 0; i < 64; i++)  
    {
        red = (float)((rgb[i]&0xF800)>>8);   
        green = (float)((rgb[i]&0x07E0)>>3);
        blue = (float)((rgb[i]&0x001F)<<3);
                         
        y[i] = 0.299f*red + 0.587f*green + 0.114f*blue -128;        
    }
}


//���������ɻҶ�ͼ
void yuv_to_rgb565(float y[64], uint16_t rgb[64])
{
    uint16_t i, temp;
    for(i = 0; i < 64; i++)
    {        
        temp = (uint16_t)(y[i] + 128);
        temp >>= 3;
        rgb[i] = (temp<<11 | temp<<6 | temp);
    }
}


#define  C1 0.9808f
#define  C2 0.9239f
#define  C3 0.8315f
#define  C4 0.7071f
#define  C5 0.5556f
#define  C6 0.3827f
#define  C7 0.1951f

static void fdctrow(float *blk)
{ 
    float S07,S16,S25,S34,S0734,S1625;
    float D07,D16,D25,D34,D0734,D1625;

    S07 = blk[0] + blk[7];
    S16 = blk[1] + blk[6];
    S25 = blk[2] + blk[5];
    S34 = blk[3] + blk[4];
    S0734 = S07 + S34;
    S1625 = S16 + S25;

    D07 = blk[0] - blk[7]; 
    D16 = blk[1] - blk[6];
    D25 = blk[2] - blk[5];
    D34 = blk[3] - blk[4];
    D0734 = S07 - S34;
    D1625 = S16 - S25;

    blk[0] = 0.5f*(C4 * (S0734+S1625));
    blk[1] = 0.5f*(C1*D07 + C3*D16 + C5*D25 + C7*D34);
    blk[2] = 0.5f*(C2*D0734 + C6*D1625);
    blk[3] = 0.5f*(C3*D07 - C7*D16 - C1*D25 - C5*D34);
    blk[4] = 0.5f*(C4 * (S0734-S1625));
    blk[5] = 0.5f*(C5*D07 - C1*D16 + C7*D25 + C3*D34);
    blk[6] = 0.5f*(C6*D0734 - C2*D1625);
    blk[7] = 0.5f*(C7*D07 - C5*D16 + C3*D25 - C1*D34);
}

static void fdctcol(float *blk)
{
    float S07,S16,S25,S34,S0734,S1625;
    float D07,D16,D25,D34,D0734,D1625;

    S07 = blk[0*8] + blk[7*8];
    S16 = blk[1*8] + blk[6*8];
    S25 = blk[2*8] + blk[5*8];
    S34 = blk[3*8] + blk[4*8];
    S0734 = S07 + S34;
    S1625 = S16 + S25;

    D07 = blk[0*8] - blk[7*8]; 
    D16 = blk[1*8] - blk[6*8];
    D25 = blk[2*8] - blk[5*8];
    D34 = blk[3*8] - blk[4*8];
    D0734 = S07 - S34;
    D1625 = S16 - S25;

    blk[0*8] = 0.5f*(C4 * (S0734+S1625));
    blk[1*8] = 0.5f*(C1*D07 + C3*D16 + C5*D25 + C7*D34);
    blk[2*8] = 0.5f*(C2*D0734 + C6*D1625);
    blk[3*8] = 0.5f*(C3*D07 - C7*D16 - C1*D25 - C5*D34);
    blk[4*8] = 0.5f*(C4 * (S0734-S1625));
    blk[5*8] = 0.5f*(C5*D07 - C1*D16 + C7*D25 + C3*D34);
    blk[6*8] = 0.5f*(C6*D0734 - C2*D1625);
    blk[7*8] = 0.5f*(C7*D07 - C5*D16 + C3*D25 - C1*D34);
}

//����DCT�任
void jpeg_dct2(float *block)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        fdctrow(block + 8*i);
    }
    for (i = 0; i < 8; i++)
    {
        fdctcol(block + i);
    }
}


static void idctrow(float *blk)
{ 
    float tmp[16];

    //first step
    tmp[0] = blk[0]*C4 + blk[2]*C2;
    tmp[1] = blk[4]*C4 + blk[6]*C6;
    tmp[2] = blk[0]*C4 + blk[2]*C6;
    tmp[3] = -blk[4]*C4 - blk[6]*C2;
    tmp[4] = blk[0]*C4 - blk[2]*C6;
    tmp[5] = -blk[4]*C4 + blk[6]*C2;
    tmp[6] = blk[0]*C4 - blk[2]*C2;
    tmp[7] = blk[4]*C4 - blk[6]*C6;

    tmp[8] = blk[1]*C7 - blk[3]*C5;
    tmp[9] = blk[5]*C3 - blk[7]*C1;
    tmp[10] = blk[1]*C5 - blk[3]*C1;
    tmp[11] = blk[5]*C7 + blk[7]*C3;
    tmp[12] = blk[1]*C3 - blk[3]*C7;
    tmp[13] = -blk[5]*C1 - blk[7]*C5;
    tmp[14] = blk[1]*C1 + blk[3]*C3;
    tmp[15] = blk[5]*C5 + blk[7]*C7;

    //second step
    tmp[0] = 0.5f *  (tmp[0]+tmp[1]);
    tmp[1] = 0.5f * (tmp[2]+tmp[3]);
    tmp[2] = 0.5f * (tmp[4]+tmp[5]);
    tmp[3] = 0.5f * (tmp[6]+tmp[7]);
    tmp[4] = 0.5f * (tmp[8]+tmp[9]);
    tmp[5] = 0.5f * (tmp[10]+tmp[11]);
    tmp[6] = 0.5f * (tmp[12]+tmp[13]);
    tmp[7] = 0.5f * (tmp[14]+tmp[15]);

    //third step
    blk[0] = tmp[0] + tmp[7];
    blk[1] = tmp[1] + tmp[6];
    blk[2] = tmp[2] + tmp[5];
    blk[3] = tmp[3] + tmp[4];
    blk[4] = tmp[3] - tmp[4];
    blk[5] = tmp[2] - tmp[5];
    blk[6] = tmp[1] - tmp[6];
    blk[7] = tmp[0] - tmp[7];
}


static void idctcol(float *blk)
{
    float tmp[16];

    //first step
    tmp[0] = blk[0*8]*C4 + blk[2*8]*C2;
    tmp[1] = blk[4*8]*C4 + blk[6*8]*C6;
    tmp[2] = blk[0*8]*C4 + blk[2*8]*C6;
    tmp[3] = -blk[4*8]*C4 - blk[6*8]*C2;
    tmp[4] = blk[0*8]*C4 - blk[2*8]*C6;
    tmp[5] = -blk[4*8]*C4 + blk[6*8]*C2;
    tmp[6] = blk[0*8]*C4 - blk[2*8]*C2;
    tmp[7] = blk[4*8]*C4 - blk[6*8]*C6;

    tmp[8] = blk[1*8]*C7 - blk[3*8]*C5;
    tmp[9] = blk[5*8]*C3 - blk[7*8]*C1;
    tmp[10] = blk[1*8]*C5 - blk[3*8]*C1;
    tmp[11] = blk[5*8]*C7 + blk[7*8]*C3;
    tmp[12] = blk[1*8]*C3 - blk[3*8]*C7;
    tmp[13] = -blk[5*8]*C1 - blk[7*8]*C5;
    tmp[14] = blk[1*8]*C1 + blk[3*8]*C3;
    tmp[15] = blk[5*8]*C5 + blk[7*8]*C7;

    //second step
    tmp[0] = 0.5f*(tmp[0] + tmp[1]);
    tmp[1] = 0.5f*(tmp[2] + tmp[3]);
    tmp[2] = 0.5f*(tmp[4] + tmp[5]);
    tmp[3] = 0.5f*(tmp[6] + tmp[7]);
    tmp[4] = 0.5f*(tmp[8] + tmp[9]);
    tmp[5] = 0.5f*(tmp[10] + tmp[11]);
    tmp[6] = 0.5f*(tmp[12] + tmp[13]);
    tmp[7] = 0.5f*(tmp[14] + tmp[15]);

    //third step
    blk[0*8] = tmp[0] + tmp[7];
    blk[1*8] = tmp[1] + tmp[6];
    blk[2*8] = tmp[2] + tmp[5];
    blk[3*8] = tmp[3] + tmp[4];
    blk[4*8] = tmp[3] - tmp[4];
    blk[5*8] = tmp[2] - tmp[5];
    blk[6*8] = tmp[1] - tmp[6];
    blk[7*8] = tmp[0] - tmp[7];
}

//����DCT��任
void jpeg_inv_dct2(float *block)
{
    int i;
    for (i=0; i<8; i++)
    {
        idctrow(block+8*i);
    }
    for (i=0; i<8; i++)
    {
        idctcol(block+i);
    }
}

//����
void jpeg_quantify(float in[64], int8_t out[64])
{
    uint16_t i, j;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            out[i*8+j] = (int8_t)(in[i*8+j] / bright_code[i][j]);
        }
    }
}
void jpeg_inv_quantify(int8_t in[64], float  out[64])
{
    uint16_t i, j;
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            out[i*8+j] = (float)in[i*8+j] * bright_code[i][j];
        }
    }
}

void jpeg_zigzag(int8_t data[64])
{
    uint16_t i, j;
    int8_t temp[64];
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            temp[zigzag_code[i][j]] = data[i*8+j]; 
        }
    }
    for(i = 0; i < 64; i++)
    {
        data[i] = temp[i];
    }    
}

void jpeg_inv_zigzag(int8_t data[64])
{
    uint16_t i, j;
    int8_t temp[64];
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            temp[i*8+j] = data[zigzag_code[i][j]];  
        }
    }
    for(i = 0; i < 64; i++)
    {
        data[i] = temp[i];
    }      
}

//�γ̱���
void jpeg_rle(int8_t in[64], int8_t out[60])
{
    uint16_t i, j;
    int8_t m = 0, n, count = 0, group = 0;

    for(i = 0; i < 60; i++)
    {
        out[i] = 0;
    }
    
    for(i = 0; i < 8; i++)
    {
        for(j = 0; j < 8; j++)
        {
            if(in[i*8+j] == 0)
            {
                count++;
                if(count>15)
                {
                    out[m*3] = 15;
                    out[m*3+1] = 0;
                    out[m*3+2] = 0; 
                    count = 0;
                    m++;
                }               
            }
            else
            {
                n = in[i*8+j];
                while(n != 0)
                {
                    n = n / 2;
                    group++;
                }
                out[m*3] = count;
                out[m*3+1] = group;
                out[m*3+2] = in[i*8+j];
                count = 0;
                group = 0;
                m++;
            }
            if(m == 19)
            {
                j = 8;
                i = 8;
            }
        }
    }
    while(out[m*3+2] == 0)
    {
        m--;
    }
    m++;
    out[m*3] = 0;
    out[m*3+1] = 0;
    out[m*3+2] = 0;
}

void jpeg_inv_rle(int8_t in[60], int8_t out[64])
{
    uint16_t i, n = 0, count;

    for(i = 0; i < 64; i++)
    {
        out[i] = 0;
    }
    
    for(i = 0; i < 20; i++)
    {
        count = in[i*3];
        while(count--)
        {
            out[n++] = 0;
        }
        out[n++] = in[i*3+2];
    }
}

//����������
void jpeg_huffman(int8_t in[60], uint32_t *c, uint16_t *out)
{
    uint16_t i;
    uint16_t zeros, length, huffman;
    uint32_t move, count = (*c)%16;
    uint16_t num1, mask = 0;
    for(i = 0; i < 20; i++)
    {
        zeros = in[i*3];
        length = in[i*3+1];
        
        if(in[i*3+2] < 0)
        {
            num1 = (uint16_t)(-in[i*3+2]);
            mask = 0x0001;
            mask = ~(mask << (length-1));
            num1 = num1 & mask;
        }
        else
        {
            num1 = (uint16_t)in[i*3+2];
        }
            
        huffman = huffman_ac_code[zeros*10+length][1];
        move = huffman_ac_code[zeros*10+length][0];
            
        count += move;          
        if(count > 15)
        {
//TODO������            printf("move:%d  %d  %d\n",move, zeros, length);
            count = count -16;             
            *out = *out << (move-count) | (huffman >> count);                             
            *(++out) = 0;                
            mask = 0xFFFF;
            mask = ~(mask <<= count);
            *out = *out | (huffman & mask);
            (*c)+=16;
        }
        else
        {
            *out = *out << move | huffman; 
        }  
        
        if(length == 0 && zeros != 15)
        {
            *c/=16;
            *c*=16;
            *c += count;
            return;
        }
       
        count += length;
        if(count > 15)
        {
            count = count -16;             
            *out = *out << (length-count) | (num1 >> count);                             
            *(++out) = 0;                
            mask = 0xFFFF;
            mask = ~(mask <<= count);
            *out = *out | (num1 & mask);
            (*c)+=16;
        }
        else
        {
            *out = *out << length | num1; 
        }        
    }   
}

//����������
void jpeg_inv_huffman(uint16_t *in, uint32_t *c, uint32_t *s, int8_t out[60])
{
    uint16_t n, m = 0, k = 0;
    uint16_t zeros, length, huffman;
    uint16_t temp = 0;
    uint32_t count = *c, sum = *s;
    
    while(sum <= count)
    {      
        temp <<= 1;
        if((*in & 0x8000) == 0x8000)
        {              
            temp |= 0x0001;
        }              
        *in <<= 1;
        sum++;
        if(sum%16 == 0)
        {
            in++; 
        }
        k++; 
        for(n = 0; n < 161; n++)
        {
            if(k == huffman_ac_code[n][0] && temp == huffman_ac_code[n][1])
            {
                k = 0;
                break;
            }
        }     
        if(n == 0)
        {
            *c = count;
            *s = sum;
            return;
        }      
        if(n != 161)
        {           
            zeros = n / 10;
            out[3*m] = zeros;
            length = n % 10; 
            out[3*m+1] = length;
                                     
            if(length == 0)
            {
                huffman = 0;
            }
            else
            {
                if((*in & 0x8000) == 0x8000)
                {
                    out[3*m+2] = 1;
                }
                else
                {
                    out[3*m+2] = -1;
                }
                *in <<= 1;
                sum++;
                if(sum%16 == 0)
                {
                    in++;
                }            
                huffman = 0x0001;
                
                length--;
                while(length--)
                {
                    huffman <<= 1;
                    if((*in & 0x8000) == 0x8000)
                    {
                        huffman |= 0x0001;
                    }
                    *in <<= 1;
                    sum++;
                    if(sum%16 == 0)
                    {
                        in++;
                    }
                }                  
            }                
                                         
            out[3*m+2] *= (int8_t)huffman;               
            m++;
            temp = 0;           
        }
    }
}    

//#define IDEBUG  3
//#define JDEBUG  1
#define IDEBUG  39
#define JDEBUG  29

void jpeg_encode(uint32_t *num)
{   
    int8_t dct[64] = {0}, rle[60] = {0}; 
    uint16_t i, j, m, n, temp[64];
    float bright[64];
     
    for(i = 0; i < 40; i++)
    {
        for(j = 0; j < 30; j++)
        {                   
            for(m = 0; m < 8; m++)
            {
                for(n = 0; n < 8; n++)
                {
                    temp[m*8+n] = test_image[(i*8+m)*240+j*8+n];
                }
            }
  
//            printf(" %d  %d \n", i, j);
            
            rgb565_to_yuv(temp, bright);
            if(i == IDEBUG && j == JDEBUG)
            {  
                printf("\n ��ɫ��ʽ�任�����\n");
                printf_array_f(bright, 8, 8);            
            }
            
            jpeg_dct2(bright); 
            if(i == IDEBUG && j == JDEBUG)
            {  
                printf("\n DCT�任�����\n");
                printf_array_f(bright, 8, 8);
            }
            jpeg_quantify(bright, dct); 
            if(i == IDEBUG && j == JDEBUG)
            {  
                printf("\n ���������\n");             
                printf_array_d(dct, 8, 8);
            }
            jpeg_zigzag(dct); 
            if(i == IDEBUG && j == JDEBUG)
            {            
                printf("\n Zigzag��������\n");
               printf_array_d(dct, 8, 8);
            }
            
            jpeg_rle(dct, rle);       
            if(i == IDEBUG && j == JDEBUG)
            {
                printf("\n �γ̱�������%d\n", j);
               printf_array_d(rle, 20, 3);                
            }
//            printf("\n");
            
            
            jpeg_huffman(rle, num, encode+*num/16);            
            if(i == IDEBUG && j == JDEBUG)
            {
//                printf("\n ��������������\n");    
//               printf_array_b(encode, size/16); 
                encode[size/16] <<= (16-size%16);
                return;  
            }
        }              
    }
    encode[size/16] <<= (16-size%16);
}

void jpeg_decode(uint32_t *num)
{
    uint32_t i = 0, j = 0, m, n;
    uint32_t i_num = 0;
    int8_t i_rle[60] = {0};
    int8_t i_dct[64] = {0}; 
    float temp[64] = {0};
    uint16_t rec[64] = {0};
    
    //while(i_num/16 < *num/16)        
    //TODO ͣ��������
    while(j <= IDEBUG*30+JDEBUG)
    {
//        printf("%d  %d\n", j/30, j%30);
        
        jpeg_inv_huffman(encode+i_num/16, num, &i_num, i_rle);       

        if(j == IDEBUG*30+JDEBUG)
        {
            printf("\n ���룺\n");
            printf_array_d(i_rle, 20, 3);                
        }
              
        jpeg_inv_rle(i_rle, i_dct);
        if(j == IDEBUG*30+JDEBUG)
        {
            printf("\n �γ��棺\n");
            printf_array_d(i_dct, 8, 8);                
        }
        
        jpeg_inv_zigzag(i_dct);
        
        jpeg_inv_quantify(i_dct, temp);
        
        if(j == IDEBUG*30+JDEBUG)
        {                   
            printf("\n �����������\n");
            printf_array_f(temp, 8, 8);  
        }       
        jpeg_inv_dct2(temp); 
        
        if(j == IDEBUG*30+JDEBUG)
        {
            printf("\n DCT��任�����\n");
            printf_array_f(temp, 8, 8);                
        }       
        
        yuv_to_rgb565(temp, rec);
                
        for(m = 0; m < 8; m++)
        {
            for(n = 0; n < 8; n++)
            {
                image[((j/30)*8+m)*240+(j%30)*8+n] = rec[m*8+n];
            }
        }
                               
        for(i = 0; i < 60;i++)
        {
            i_rle[i] = 0;
        }
        j++;       
    } 
}

void lcd_show_image2(uint16_t m[76800])
{
    uint32_t i;
    lcd_scan_dir(3);
    lcd_set_cursor(0, 0);  // ���ù��λ��                   
    lcd_write_ram_prepare();   // ��ʼд��GRAM
    for(i = 0; i < 76800; i++)
    {
        LCD->LCD_RAM = m[i];
    }
}

void jpeg_test(void)
{
    memset(encode, 0, 153600);
         
    memset(image, 0, 153600);
     
    size = 0;
    
    jpeg_encode(&size);   
    
    printf("\n %d \n", size);
    
    jpeg_decode(&size); 
    
    lcd_show_image2(image);    
}

/******************************* END OF FILE *********************************/

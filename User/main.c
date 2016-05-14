/*********************** HNIT 3103 Application Team **************************
 * �� �� �� ��main.c
 * ��    �� ��Һ����ʾ    
 * ʵ��ƽ̨ ��STM32F407������
 * �� �� �� ��ST1.4.0
 * ʱ    �� ��2016.3.19
 * ��    �� ��3103�����Ŷ� ������
 * �޸ļ�¼ ����
******************************************************************************/
#include "stm32_sys.h"
#include "stm32_delay.h"
#include "stm32_usart.h"
#include "hnit_led.h" 
#include "hnit_key.h"
#include "hnit_lcd.h"
#include "hnit_jpeg.h"
#include "hnit_sram.h"

/*****************************************************************************
*	�� �� ��: main
*	��    ��: c�������
*   ���ú�������
*	��    �Σ���
*	�� �� ֵ: �������
******************************************************************************/	
int main(void)
{
    u32 t = 0;
    
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    delay_init(168);
    usart1_init(115200);		
    fsmc_lcd_init();
    fsmc_sram_init();
    led_gpio_config();
    key_gpio_config();
    
    lcd_clear(BLACK);   
    POINT_COLOR = BLACK;	
    BACK_COLOR = WHITE;
    
    jpeg_test();
    
    while(1)
    {
       
        LED0 = ~LED0;
        delay_ms(500);
        t++;
    }
}

/******************************* END OF FILE *********************************/

/*********************** HNIT 3103 Application Team **************************
 * @file    hnit_key.c
 * @brief   �����������������         
 * @board   STM32F407������
 * @author  ������
 * @date    2016.3.19
 * @history �� 
 *****************************************************************************/
#include "hnit_key.h"

/**
  * @brief  ����GPIO��ʼ��
  * @param  ��
  * @retval ��
  */
void key_gpio_config(void)
{    	 
    GPIO_InitTypeDef  GPIO_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);  //ʹ��GPIOEʱ��

    // GPIOE3, E4��ʼ������
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;         //��ͨ����ģʽ
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;        //�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;    //100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;          //����
    GPIO_Init(GPIOE, &GPIO_InitStructure);                //��ʼ��	
}

/******************************* END OF FILE *********************************/

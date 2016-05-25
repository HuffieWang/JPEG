#简介
##硬件
&emsp;&emsp;主控芯片：STM32F407ZET6

&emsp;&emsp;拓展内存：IS62WV51216BLL(1Mbyte)

&emsp;&emsp;液晶显示：ILI9341彩屏(320*240像素)

&emsp;&emsp;原图信息：320*240像素、RGB565模式的lenna局部图
##算法
&emsp;&emsp;颜色模式：YUV444

&emsp;&emsp;压缩算法：DCT变换

&emsp;&emsp;编码方法：行程编码、霍夫曼编码

#算法原理及实现
##数据分块
###说明
&emsp;&emsp;将320*240图像数据划分为1200个8x8的数据块。
###原因
&emsp;&emsp;JPEG是以每8x8个点为一个单元进行处理的，即将一个单元做完格式转换、DCT变换、量化、编码等全部处理后，再操作下一个单元。所以需要将图像进行分块。
###实现
&emsp;&emsp;需按从左到右、从上到下的顺序从图层数据中取出8x8的数据，依次存放在64长度的数组中。
```C
    for(m = 0; m < 8; m++)
        for(n = 0; n < 8; n++)
            temp[m*8+n] = image[(i*8+m)*240+j*8+n];
```
&emsp;&emsp;对全图进行8x8的分块处理，暂时左上角部分的8x8块为后面的处理作准备。

![Alt Text](https://github.com/HuffieWang/JPEG/blob/master/DATA/图11分块结果.png)

##颜色模式转换
###说明
&emsp;&emsp;将RGB565模式的原图像转换为YUV444模式。RGB565模式由Red（红）、Green（绿）、Blue（蓝）三个分量按5：6：5共16bit组成，YUV444模式由Y（亮度）、U（色度）、V（饱和度）三个分量按8：8：8共24bit组成。
###原因
&emsp;&emsp;研究发现，人眼对亮度变换的敏感度要比对色彩（色度和饱和度）变换的敏感度高出很多。利用这个特性，我们可以对YUV模式的图像进行亮度权重较高、色度权重较低的采样，得到数据量更小的图片，同时损失的精度又不易被人眼察觉。YUV常用的采样比例有4：1：1和4：2：2等，下图为YUV411采样。

![Alt Text](https://github.com/HuffieWang/JPEG/blob/master/DATA/图1格式转换.png)

&emsp;&emsp;RGB模式显然不具备YUV这种采样优势，所以需要对其进行转换。本设计中因为在4：4：4采样时的压缩率已达成设计要求，所以暂时采用YUV444来保证最好的图像质量。
###实现
&emsp;&emsp;因原始图像为RGB565格式，所以需先将其分离为8位的red、green、blue分量，C语言实现方法如下。
```C
     //RGB565转RGB888
     red = (rgb565 & 0xF800) >> 8;   
     green = (rgb565 & 0x07E0) >> 3;
     blue = (rgb565 & 0x001F) << 3;
```
&emsp;&emsp;又因后续的DCT变换所能处理的数值范围为-127~+127，所以需将RGB转YUV得的到数值均减去128。
```C
     //RGB888转YUV
     Y = 0.299f*red + 0.587f*green + 0.114f*blue - 128; 
     U = -0.1687f*red - 0.3313f*green + 0.5f*blue;             
     V = 0.5f*red - 0.418f*green - 0.0813f*blue; 
```

&emsp;&emsp;对图像的第一个8x8数据块进行上述格式转换，得到其亮度、色度、饱和度三层数据，这里只选取其亮度层作为展示，后续的一系列处理均以此8x8亮度层数据为例。

![Alt Text](https://github.com/HuffieWang/JPEG/blob/master/DATA/图2格式转换结果.png)

##DCT变换
###说明
&emsp;&emsp;将8x8的图像数据变换为8x8的频域系数矩阵。
###原因
&emsp;&emsp;DCT变换是JEPG图像压缩的核心部分。由于大多数图像的高频分量比较小，相应的图像高频分量的DCT系数经常接近于0，再加上高频分量中只包含了图像的细微的细节变化信息，而人眼对这种高频成分的失真不太敏感，所以，可以考虑将这一些高频成分予以抛弃，从而降低需要传输的数据量。这样一来，传送DCT变换系数的所需要的编码长度要远远小于传送图像像素的编码长度。到达接收端之后通过反离散余弦变换就可以得到原来的数据，虽然这么做存在一定的失真，但人眼是可接受的，而且对这种微小的改变是不敏感的。
###实现
&emsp;&emsp;DCT变换公式：

&emsp;&emsp;DCT变换相关代码见本项目的User\JPEG\hnit_jpeg.c\"jpeg_dct2"

&emsp;&emsp;图像信号通过DCT被分解为直流成分和一些从低频到高频的各种余弦成分。而DCT系数只表示了该种成分所占原图像信号的份额大小。例如，U=0，V=0时的F（0,0）是原来的64个数据的均值，相当于直流分量，也有人称之为DC系数或者直流系数。随着U，V的增加，相另外的63个系数则代表了水平空间频率和垂直空间频率分量（高频分量）的大小，多半是一些接近于0的正负浮点数，我们称之为交流系数AC。DCT变换后的8x8的系数矩阵中，低频分量集中在矩阵的左上角。高频成分则集中在右下角。

![Alt Text](https://github.com/HuffieWang/JPEG/blob/master/DATA/图3DCT变换结果.png)

##量化
###说明
&emsp;&emsp;将8x8的频域系数矩阵按照量化表进行量化。
###原因
&emsp;&emsp;原因有三点。其一，去掉部分高频分量，因为我们要保留低频分量，适当去掉高频分量来压缩数据；其二，减小数据的数值大小，在压缩方面，小数值的数据一般比大数值的更有优势；其三，化浮点型为整型，方便后面的编码处理。
###实现
&emsp;&emsp;首先我们需要一张8x8的量化矩阵，以下是常用的一张根据心理阀制作的量化表，当然也可以按照需要自行编写。

![Alt Text](https://github.com/HuffieWang/JPEG/blob/master/DATA/图9量化表.png)

&emsp;&emsp;将频域系数矩阵点除量化矩阵，即可完成量化，结果如下。

![Alt Text](https://github.com/HuffieWang/JPEG/blob/master/DATA/图4量化结果.png)

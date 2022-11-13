# ComputerGraph-ImageEditing

#### Libraries
 1. fltk-1.3.2

#### 指令
 1. load \<filename\>
>    開啟tga格式圖片檔案
>
>    ![](https://i.imgur.com/UcbCWYJ.png)
 2. gray
>    RGB轉灰階(I = 0.299R + 0.587G + 0.114B)
>
>    ![](https://i.imgur.com/Cf2uCGd.png)
 3. quant-unif
>    Uniform Quantization，24bits轉8bits
>
>    ![](https://i.imgur.com/XWY9W5Q.png)
 4. quant-pop
>    Populosity Quantization，24bits轉8bits
>
>    ![](https://i.imgur.com/pBCKPvI.png)
 5. dither-thresh
>    ![](https://i.imgur.com/mUbIvsj.png)
 7. dither-bright
>    平均亮度
>
>    ![](https://i.imgur.com/HIn81q7.png)
 7. dither-rand
>   使用uniform(-0.2, 0.2)
>
>    ![](https://i.imgur.com/n4ArUZx.png)
 8. dither-cluster
>   mask = \[180 90 150 60; 15 240 210 105; 120 195 225 80; 45 135 75 165\]
>
>    ![](https://i.imgur.com/AT043Xt.png)
 9. dither-fs
>   Floyd-Steinberg Dithering
>
>    ![](https://i.imgur.com/hqMypJi.png)
 10. dither-color 
>   Floyd-Steinberg，24bits轉8bits
>
>    ![](https://i.imgur.com/XDrlYwa.png)

 11.filter-box
>    ![](https://i.imgur.com/FDYLGTc.png)

 12.filter-bartlett
>    
 13.filter-gauss
>    
 14.filter-gauss-n <kernel size>
>    
 15.filter-edge
>    
 16.filter-enhance
>    
 17.half
>    
 18.double
>    
 19.scale <n>
>    
 20.rotate <n>
>    
 21.npr-paint
>    ![](https://i.imgur.com/iezFnhY.png)

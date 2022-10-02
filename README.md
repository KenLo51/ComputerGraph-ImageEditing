# NTUST-1111-ComputerGraph-Project1
NTUST 111-1-賴祐吉-電腦圖學導論  Project1(影像處理)

#### 指令
 1. load <filename>
    開啟tga格式圖片檔案
 2. gray
    RGB轉灰階(I = 0.299R + 0.587G + 0.114B)
 3. quant-unif
    Uniform Quantization，24bits轉8bits
 4. quant-pop
    Populosity Quantization，24bits轉8bits
 5. dither-thresh
 6. dither-bright
    平均亮度
 7. dither-rand
    使用uniform(-0.2, 0.2)
 8. dither-cluster
    mask = [180 90 150 60; 15 240 210 105; 120 195 225 80; 45 135 75 165]
 9. dither-fs
    Floyd-Steinberg Dithering
 10.dither-color
    Floyd-Steinberg，24bits轉8bits
 11.filter-box
 
 12.filter-bartlett
 
 13.filter-gauss
 
 14.filter-gauss-n <kernel size>
 
 15.filter-edge
 
 16.filter-enhance
 
 17.half
 
 18.double
 
 19.scale <n>
 
 20.rotate <n>
 
 21.npr-paint
 

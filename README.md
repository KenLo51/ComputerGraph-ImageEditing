# ComputerGraphic-ImageEditing

1. FLTK 練習。  
2. 基礎2D影像處理(Convolution, Warping, Dithering, ...)。  

# Libraries
 1. fltk-1.3.2

# 運行結果
|  1. load \<filename\> | 2. gray |
| :- | :- | 
| 開啟tga格式圖片檔案 | RGB轉灰階(I = 0.299R + 0.587G + 0.114B) |
| <img src="https://i.imgur.com/UcbCWYJ.png" width="300" height="275" /> | <img src="https://i.imgur.com/Cf2uCGd.png" width="300" height="275" /> |
## Quantization  
|  3. quant-unif | 4. quant-pop | 
| :- | :- |
| Uniform Quantization，24bits轉8bits<br>直接捨去個顏色LSB資料 | Populosity Quantization，24bits轉8bits<br>histogram統計後使用最常出現顏色，<br>細部物體顏色可能被捨去 |
| <img src="https://i.imgur.com/XWY9W5Q.png" width="300" height="275" /> | <img src="https://i.imgur.com/pBCKPvI.png" width="300" height="275" /> |
## Dithering  
|  5. dither-thresh | 6. dither-bright | 7. dither-rand |
| :- | :- | :- |
| 使用固定值(通常為0.5)判斷輸出顏色<br>可能使圖片轉為全白(或黑) | 使用平均亮度判斷輸出顏色 | 使用uniform(-0.2, 0.2)判斷輸出顏色，<br>過深(淺)部分變化無法保留 |
| <img src="https://i.imgur.com/mUbIvsj.png" width="300" height="275" /> | <img src="https://i.imgur.com/HIn81q7.png" width="300" height="275" /> | <img src="https://i.imgur.com/n4ArUZx.png" width="300" height="275" /> |
|  8. dither-cluster | 9. dither-fs | 10. dither-color  |
| 使用一固定n\*n個不同threshold判斷 | Floyd-Steinberg Dithering<br>結果較佳但無法平行處理 | Floyd-Steinberg，24bits轉8bits |
| <img src="https://i.imgur.com/AT043Xt.png" width="300" height="275" /> | <img src="https://i.imgur.com/hqMypJi.pngg" width="300" height="275" /> | <img src="https://i.imgur.com/XDrlYwa.png" width="300" height="275" /> |

## Filtering
|  11.filter-box | 12.filter-bartlett | 13.filter-gauss |
| :- | :- | :- |
|  |  |  |
| <img src="https://i.imgur.com/FDYLGTc.png" width="300" height="275" /> |  |  |
|  14.filter-gauss-n <kernel size> | 15.filter-edge | 16.filter-enhance |
|  |  |  |
|  |  |  |
|  17.half | 18.double | 19.scale <n> |
|  |  |  |
|  |  |  |
|  20.rotate <n> | 21.npr-paint | 19.scale <n> |
|  |  |  |
|  | <img src="https://i.imgur.com/iezFnhY.png" width="300" height="275" /> |  |

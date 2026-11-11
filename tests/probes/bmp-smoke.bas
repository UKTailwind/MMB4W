Mode 7,8
imagePath$=MM.INFO$(PATH)
Cls RGB(0,0,0)
Load BMP imagePath$+"bmp-24.bmp",10,20
If Pixel(10,20)<>RGB(255,0,0) Then Error "BMP 24 RED TOP LEFT"
If Pixel(11,20)<>RGB(0,255,0) Then Error "BMP 24 GREEN TOP RIGHT"
If Pixel(10,21)<>RGB(0,0,255) Then Error "BMP 24 BLUE BOTTOM LEFT"
If Pixel(11,21)<>RGB(255,255,255) Then Error "BMP 24 WHITE BOTTOM RIGHT"
Load BMP imagePath$+"bmp-indexed.bmp",30,40
If Pixel(30,40)<>RGB(240,10,20) Then Error "BMP PALETTE TOP LEFT"
If Pixel(31,40)<>RGB(5,210,30) Then Error "BMP PALETTE TOP RIGHT"
If Pixel(30,41)<>RGB(7,20,190) Then Error "BMP PALETTE BOTTOM LEFT"
If Pixel(31,41)<>RGB(170,180,190) Then Error "BMP PALETTE BOTTOM RIGHT"
Cls RGB(0,0,0)
Load BMP imagePath$+"bmp-24.bmp",-1,-1
If Pixel(0,0)<>RGB(255,255,255) Then Error "BMP NEGATIVE ORIGIN"
If Pixel(1,0)<>RGB(0,0,0) Then Error "BMP HORIZONTAL CLIP"
If Pixel(0,1)<>RGB(0,0,0) Then Error "BMP VERTICAL CLIP"
On Error Skip 1
Load BMP imagePath$+"bmp-malformed.bmp"
If MM.Errno=0 Then Error "BMP TRUNCATED INPUT ACCEPTED"
On Error Abort
Load BMP imagePath$+"bmp-indexed.bmp",10,10
If Pixel(10,10)<>RGB(240,10,20) Then Error "BMP ERROR RECOVERY"
Open imagePath$+"bmp-marker.txt" For Output As #1
Print #1,"WINDOWS BMP TRUECOLOUR PALETTE CLIP ERROR PASS"
Close #1
Quit

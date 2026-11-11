Mode 7,8
Framebuffer Create 640,640
Framebuffer Write
Cls RGB(0,0,0)
Pixel 639,639,RGB(255,0,0)
Pixel 300,301,RGB(0,255,0)
expectedLast=Pixel(639,639):expectedRect=Pixel(300,301)
Framebuffer Backup
Pixel 639,639,RGB(0,0,255)
Framebuffer Restore
If Pixel(639,639)<>expectedLast Then Error "FULL FRAMEBUFFER RESTORE"
Pixel 300,301,RGB(0,0,255)
Framebuffer Restore 299,300,4,4
If Pixel(300,301)<>expectedRect Then Error "RECT FRAMEBUFFER RESTORE"
Page Write 3
Page Copy 1 To 2,I
Pixel 1,1,RGB(255,255,255)
If Pixel(1,1)<>RGB(255,255,255) Then Error "PAGE COPY WRITE STATE"
Open "framebuffer-marker.txt" For Output As #1
Print #1,"FRAMEBUFFER FULL AND RECT RESTORE PASS"
Close #1
Quit

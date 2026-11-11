' INTERACTIVE INPUT MONITOR FOR THE WINDOWS EMULATOR
' Optional BASIC argument SELFTEST draws 100 frames, writes a marker, and exits.
Option Base 0
Dim monitorKeys(5)
Mode 7,8
monitorFrame=0:monitorLastKey=0:monitorWheel=0:monitorLastWheel=0
monitorSelfCheck=Instr(UCase$(MM.CMDLINE$),"SELFTEST")>0
If monitorSelfCheck Then
 On Error Skip 1
 monitorDiscard=Mouse(W,0)
 monitorWheelError=MM.Errno
 On Error Abort
 If monitorWheelError=0 Then Error "MOUSE WHEEL ZERO SCALE ACCEPTED"
EndIf
Do
 monitorCount=KeyDown(0)
 For monitorSlot=1 To 5
  monitorKeys(monitorSlot)=KeyDown(monitorSlot)
  If monitorKeys(monitorSlot)>0 Then monitorLastKey=monitorKeys(monitorSlot)
  If monitorKeys(monitorSlot)=27 Then Quit
 Next monitorSlot
 monitorX=Mouse(X):monitorY=Mouse(Y)
 monitorLeft=Mouse(L):monitorRight=Mouse(R):monitorMiddle=Mouse(M)
 monitorStep=Mouse(W)
 monitorWheel=monitorWheel+monitorStep
 If monitorStep<>0 Then monitorLastWheel=monitorStep
 Page Write 1
 Cls RGB(8,12,20)
 Text 12,10,"MMB4W INPUT MONITOR","LT",1,1,RGB(255,210,64)
 Text 12,34,"HELD KEYS: "+Str$(monitorCount),"LT",1,1,RGB(200,220,240)
 monitorText$=""
 For monitorSlot=1 To 5
  monitorText$=monitorText$+Str$(monitorKeys(monitorSlot))+"  "
 Next monitorSlot
 Text 12,52,monitorText$,"LT",1,1,RGB(255,255,255)
 Text 12,80,"MOUSE X "+Str$(monitorX)+"  Y "+Str$(monitorY),"LT",1,1,RGB(200,220,240)
 monitorText$="L "+Str$(monitorLeft)+"  R "+Str$(monitorRight)+"  M "+Str$(monitorMiddle)
 Text 12,98,monitorText$,"LT",1,1,RGB(255,255,255)
 Text 12,126,"WHEEL LAST "+Str$(monitorLastWheel),"LT",1,1,RGB(200,220,240)
 Text 12,144,"WHEEL TOTAL "+Str$(monitorWheel),"LT",1,1,RGB(255,255,255)
 Text 12,170,"LAST KEY CODE "+Str$(monitorLastKey),"LT",1,1,RGB(80,230,190)
 Text 12,188,"ARROWS 128-131  SPACE 32","LT",1,1,RGB(160,175,195)
 Text 12,208,"ESC QUITS THE MONITOR","LT",1,1,RGB(160,175,195)
 Text 12,228,"FRAME "+Str$(monitorFrame)+" T "+Str$(Int(Timer)),"LT",1,1,RGB(160,175,195)
 If monitorX>=0 And monitorX<320 And monitorY>=0 And monitorY<240 Then
  Circle monitorX,monitorY,4,1,1,RGB(80,230,190)
 EndIf
 Page Copy 1 To 0,I
 monitorFrame=monitorFrame+1
 If monitorSelfCheck And monitorFrame>=100 Then
  Open MM.INFO$(PATH)+"input-monitor-marker.txt" For Output As #1
  Print #1,"INPUT MONITOR STARTUP PASS"
  Close #1
  Quit
 EndIf
 Pause 20
Loop

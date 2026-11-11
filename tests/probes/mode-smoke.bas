Open "mode-marker.txt" For Output As #1
Print #1,"BEGIN MODE PROBE"
Close #1
For phase=1 To 4
 Mode 7,16
 Open "mode-marker.txt" For Append As #1
 Print #1,"MODE7 ";phase
 Close #1
 Pause 20
 Mode 1
 Open "mode-marker.txt" For Append As #1
 Print #1,"MODE1 ";phase
 Close #1
 Pause 20
Next phase
Open "mode-marker.txt" For Append As #1
Print #1,"MODE TRANSITIONS PASS"
Close #1
Quit

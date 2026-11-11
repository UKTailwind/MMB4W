On Error Skip 1
Turtle Reset
If MM.Errno=0 Then Error "TURTLE EXCLUSION NOT ENFORCED"
On Error Abort
Open MM.INFO$(PATH)+"turtle-marker.txt" For Output As #1
Print #1,"TURTLE PREVIEW SCOPE PASS"
Close #1
Quit

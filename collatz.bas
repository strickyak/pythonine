10 REM  Prove the Collatz Conjecture 
30 LET A = 0 
50 LET A = (A+1) 
80 PRINT  
100 LET B = A 
110 PRINT B 
120 IF (B<2) THEN 50 
130 IF ((B%2)=0) THEN 500 
200 LET B = ((3*B)+1) 
220 GOTO 110 
500 LET B = (B/2) 
520 GOTO 110 

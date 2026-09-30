if exist %1 (
if not exist %~n1.s cc %1 & goto compile
::newer %~n1.s %1 cc.exe stdio.h string.h stdlib.h
::@echo %errorlevel%
newer %~n1.s %1 cc.exe stdio.h string.h stdlib.h
if %errorlevel% geq 1 cc %1)
newer %~n1.exe %~n1.s
if %errorlevel% == 1  goto exec
:compile
gcc -m32 -Og -g -g3 -o %~n1.exe %~n1.s
@if errorlevel 1 goto :eof
:exec
%~n1.exe
@echo Ergebnis: %errorlevel%
@echo %~n1 - %errorlevel% >> summe.txt

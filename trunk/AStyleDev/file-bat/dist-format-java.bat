@REM formats the astylex Java source code
@echo off

set progdir=..\..\AStyle\build\vs2008\bin
set srcdir1=..\src-j
set srcdir2=..\src-jx


%progdir%\AStyle  -l  %srcdir1%\*.java 
%progdir%\AStyle  -l  %srcdir2%\*.java 

echo -
echo -
echo * * * *  end of Format Java  * * * *
pause

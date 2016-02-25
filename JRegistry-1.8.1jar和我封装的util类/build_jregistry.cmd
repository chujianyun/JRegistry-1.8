@echo off
rem %1 is either 32 or 64 for specific architecture bit
rem %2 is the name of the dll file
rem %3 is the name of the input resource file
rem %4 is the name of the output resource file compilation
rem %* everything else which is the source code to compile

if [%1]==[] goto help

if not [%1]==[32] (
  if not [%1]==[64] goto help
)

set bit=%1

if [%2]==[] goto help

set dll=%2

shift
shift

if [%1]==[] goto help
if [%2]==[] goto help

set resobj=%2

echo Compiling the resource file...
C:\mingw%bit%\bin\windres.exe -i %1 -o %resobj%
echo done
echo.

shift
shift

set code=%1
:loop
shift
if [%1]==[] goto continue
set code=%code% %1
goto loop

if [%code%]==[] goto help

:continue
echo Creating the final dll file...
C:\mingw%bit%\bin\gcc.exe -mno-cygwin -I"C:\Program Files\Java\jdk1.7.0_07\include" -I"C:\Program Files\Java\jdk1.7.0_07\include\win32" -Wl,--add-stdcall-alias -Wl,--strip-all -municode -shared -W -Wall -o %dll%.dll %code% %resobj%
del %resobj%
echo done
goto exit

:help
echo Params list:
echo  1. 32 or 64 indicating the type of bit format to compile to
echo  2. Name of the final .dll file (no extension)
echo  3. Name of the resource file
echo  4. Name of the compiled resource file
echo  *. Everything else is the source code to compile

:exit
@echo on
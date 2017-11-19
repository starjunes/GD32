@echo off

if "%1"=="" ( 
  goto :AFTER 
) 

if "%1"=="before" Goto :BEFORE
if "%1"=="after"  Goto :AFTER

:BEFORE
binchk.exe -creatautofile ..\source\app\common\h\yx_auto_bulid.h -tool yes
goto End

:AFTER
fromelf .\object\cortex-m3.axf -bin -output .\object\cortex-m3.bin
copy .\object\cortex-m3.bin cortex-m3.bin
copy .\object\cortex-m3.hex cortex-m3.hex
binchk.exe -u m0\cortex-m0.bin -in cortex-m3 -out cortex-m3 -jflash STM32F105RC.jflash -outtime yes
goto End
:End 

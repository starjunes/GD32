@echo off

if "%1"=="" ( 
  goto :AFTER 
) 

if "%1"=="before" Goto :BEFORE
if "%1"=="after"  Goto :AFTER

:BEFORE
del .\Objects\yx_version.o
del .\Objects\yx_version.d
if exist .\生成烧录文件 (
echo "文件夹已存在"
) else (
md .\生成烧录文件
)
goto End

:AFTER
fromelf .\Objects\ae64m4.axf --bin --output .\Objects\ae64m4.bin
copy .\Objects\ae64m4.bin ae64m4.bin
copy .\Objects\ae64m4.hex .\生成烧录文件\ae64m4.hex
copy /b ..\boot-ae64m4-dy.hex+.\Objects\ae64m4.hex .\生成烧录文件\ae64m4_Jlink.hex
check_bin_app .\Objects\ae64m4.bin ae64gdm4 ae64gdm4 .\生成烧录文件\cortex-m4.bin
goto End
:End 
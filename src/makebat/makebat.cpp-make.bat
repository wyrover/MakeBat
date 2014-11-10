@rem MakeBat-Template: "cl con xp"
@echo off

:: Base
path %ProgramFiles%\microsoft visual studio 12.0\vc;%PATH%
set CL_PARAMS=/W3 /O2 /Oi /GL /GF /GS /MT /EHsc /D "NDEBUG" /D "WIN32" /D "_WINDOWS"
set LINK_PARAMS=/link /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /SUBSYSTEM:CONSOLE,5.01 /SAFESEH

:: XP
path %ProgramFiles%\Microsoft SDKs\Windows\7.1A\Bin;%PATH%
set CL_PARAMS=%CL_PARAMS% /D_USING_V110_SDK71_ /I "%ProgramFiles%\Microsoft SDKs\Windows\7.1A\Include"
set LINK_PARAMS=%LINK_PARAMS% /LIBPATH:"%ProgramFiles%\Microsoft SDKs\Windows\7.1A\Lib"

:: Prepare compiler
call vcvarsall.bat x86
if errorlevel 1 goto :EOF

:: Resources
FOR %%a IN ("makebat.rc") DO (
rc.exe /D NDEBUG %%a
if errorlevel 1 goto :EOF
)

::Compile
cl.exe %CL_PARAMS%  "makebat.cpp" "makebat.res" "shell32.lib" %LINK_PARAMS%  && del *.obj && del *.res

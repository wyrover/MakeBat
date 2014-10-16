:: MakeBat-Template: "VC++ 2013 32bit Windows Unicode XP"
@echo off

:: Base
path %ProgramFiles%\microsoft visual studio 12.0\vc;%PATH%
set CL_PARAMS=/W3 /O2 /Oi /GL /GF /GS /MT /EHsc /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE"
set LINK_PARAMS=/link /LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /SUBSYSTEM:WINDOWS,5.01 /SAFESEH /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed

:: XP
path %ProgramFiles%\Microsoft SDKs\Windows\7.1A\Bin;%PATH%
set CL_PARAMS=%CL_PARAMS% /D_USING_V110_SDK71_ /I "%ProgramFiles%\Microsoft SDKs\Windows\7.1A\Include"
set LINK_PARAMS=%LINK_PARAMS% /LIBPATH:"%ProgramFiles%\Microsoft SDKs\Windows\7.1A\Lib"

:: Prepare compiler
call vcvarsall.bat x86
if errorlevel 1 goto :EOF

:: Resources
FOR %%a IN ("selector.rc") DO (
rc.exe /D NDEBUG %%a
if errorlevel 1 goto :EOF
)

::Compile
cl.exe %CL_PARAMS% /I "wtl" "selector.cpp" "selector.res"  %LINK_PARAMS%  && del *.obj && del *.res

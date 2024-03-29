@rem MakeBat-Template: "Visual C++ 2013 Windows Unicode XP"
@echo off

:: Base
set CL_PARAMS=/W3 /O2 /Oi /GL /GF /GS /MT /EHsc /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_UNICODE" /D "UNICODE"
set LINK_PARAMS=/LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /SUBSYSTEM:WINDOWS,5.01 /SAFESEH /MANIFEST /MANIFESTUAC:"level='asInvoker' uiAccess='false'" /manifest:embed /manifestdependency:"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'"

:: XP
set SDK71PATH=%ProgramFiles%\Microsoft SDKs\Windows\7.1A
path %SDK71PATH%\Bin;%PATH%
set CL_PARAMS=%CL_PARAMS% /D_USING_V110_SDK71_ /I "%SDK71PATH%\Include"
set LINK_PARAMS=%LINK_PARAMS% /LIBPATH:"%SDK71PATH%\Lib"

:: Prepare compiler
call "%VS120COMNTOOLS%\vsvars32.bat"
if errorlevel 1 goto :EOF

:: Resources
FOR %%a IN ("selector.rc") DO (
rc.exe /D NDEBUG %%a
if errorlevel 1 goto :EOF
)

::Compile
cl.exe %CL_PARAMS% "/O1" /I "3rd\wtl" "selector.cpp" "selector.res" /link %LINK_PARAMS%    && del *.obj && del *.res
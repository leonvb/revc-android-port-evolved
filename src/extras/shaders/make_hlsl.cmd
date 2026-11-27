@echo off
for %%f in (*PS.hlsl) do if /I not "%%~nf"=="modernAA_PS" "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_2_0 /nologo /E main /Fo obj\%%~nf.cso %%f
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_3_0 /nologo /E main /Fo obj\modernAA_PS.cso modernAA_PS.hlsl
for %%f in (*VS.hlsl) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E main /Fo obj\%%~nf.cso %%f

@echo off
for %%f in (*PS.hlsl) do if /I not "%%~nf"=="modernAA_PS" if /I not "%%~nf"=="modernBloom_PS" if /I not "%%~nf"=="modernFXAA_PS" if /I not "%%~nf"=="modernWorld_PS" "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_2_0 /nologo /E main /Fo obj\%%~nf.cso %%f
for %%f in (modernAA_PS modernBloom_PS modernFXAA_PS modernWorld_PS) do "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T ps_3_0 /nologo /E main /Fo obj\%%f.cso %%f.hlsl
for %%f in (*VS.hlsl) do if /I not "%%~nf"=="modernWorld_VS" "%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_2_0 /nologo /E main /Fo obj\%%~nf.cso %%f
"%DXSDK_DIR%\Utilities\bin\x86\fxc.exe" /T vs_3_0 /nologo /E main /Fo obj\modernWorld_VS.cso modernWorld_VS.hlsl

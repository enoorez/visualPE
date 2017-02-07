cd /d "E:\project\visualPE" &msbuild "visualPE.vcxproj" /t:sdvViewer /p:configuration="Release" /p:platform=Win32
exit %errorlevel% 
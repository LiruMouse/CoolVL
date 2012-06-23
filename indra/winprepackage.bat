rem This script puts all the files needed to package the viewer inside the viewer-win32-release directory

md viewer-win32-release
md viewer-win32-release\app_settings
md viewer-win32-release\character
md viewer-win32-release\fonts
md viewer-win32-release\llplugin
md viewer-win32-release\skins

xcopy /y /s newview\app_settings\* viewer-win32-release\app_settings\
xcopy /y build-vc80\newview\release\app_settings\* viewer-win32-release\app_settings\
xcopy /y /s newview\character\* viewer-win32-release\character\
xcopy /y /s newview\fonts\* viewer-win32-release\fonts\
xcopy /y /s build-vc80\newview\release\llplugin\* viewer-win32-release\llplugin\
xcopy /y /s newview\skins\* viewer-win32-release\skins\

xcopy /y build-vc80\newview\release\*.dll viewer-win32-release\
copy /y newview\*.dll viewer-win32-release\
copy /y newview\Microsoft.VC80.CRT.manifest viewer-win32-release\
copy /y newview\featuretable.txt viewer-win32-release\
copy /y newview\gpu_table.txt viewer-win32-release\
copy /y build-vc80\newview\release\secondlife-bin.exe viewer-win32-release\CoolVLViewer.exe
copy /y build-vc80\newview\release\SLPlugin.exe viewer-win32-release\
copy /y build-vc80\newview\release\SLVoice.exe viewer-win32-release\

copy /y build-vc80\win_crash_logger\release\*.exe viewer-win32-release\

copy /y newview\installers\windows\7z.* viewer-win32-release\

copy /y ..\doc\licenses.txt viewer-win32-release\
copy /y ..\doc\CoolVLViewerReadme.txt viewer-win32-release\
copy /y ..\doc\RestrainedLoveReadme.txt viewer-win32-release\

echo Done.

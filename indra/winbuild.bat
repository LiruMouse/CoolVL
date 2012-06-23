@rem Creating the Win32 libraries and headers directory tree:
md ..\libraries
md ..\libraries\include
md...\libraries\i686-win32
md ..\libraries\i686-win32\lib
md ..\libraries\i686-win32\lib\release
md ..\libraries\i686-win32\lib\debug
md ..\libraries\i686-win32\include
md ..\libraries\i686-win32\include\quicktime

@rem Copying fmod headers and libs:
xcopy /y "c:\Program Files\fmodapi375win\api\inc\*" ..\libraries\include\
copy /y "c:\Program Files\fmodapi375win\api\lib\fmodvc.lib" ..\libraries\i686-win32\lib\release\
copy /y "c:\Program Files\fmodapi375win\api\lib\fmodvc.lib" ..\libraries\i686-win32\lib\debug\
copy /y "c:\Program Files\fmodapi375win\api\fmod.dll" newview\

@rem Copying Quicktime headers and libs:
xcopy /y /s "c:\Program Files\QuickTime SDK\CIncludes\*" ..\libraries\i686-win32\include\quicktime\
copy /y "c:\Program Files\QuickTime SDK\Libraries\QTMLClient.lib" ..\libraries\i686-win32\lib\release\
copy /y "c:\Program Files\QuickTime SDK\Libraries\QTMLClient.lib" ..\libraries\i686-win32\lib\debug\

python develop.py -G VC80
@rem python develop.py build

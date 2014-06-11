set x86dir=c:\program files (x86)
if not exist "c:\program files (x86)" (
  set pf=c:\program files
) else (
  set pf=%x86dir%
)

net stop GoogleInputService
"%pf%\google\google input tools\GoogleInputService.exe" /UnregServer
taskkill /im googleinputhandler.exe
%~dp0\Debug\win32_register.exe --action=unregister_ime --languageid=0x0804 --name=InputTools --filename=input.ime
del c:\windows\system32\input.ime
rmdir "c:\programdata\google\Google Input Tools"
del "%pf%\google\google input tools\GoogleInputHandler.exe"
del "%pf%\google\google input tools\GoogleInputService.exe"
pause
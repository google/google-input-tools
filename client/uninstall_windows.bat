set outdir=%~dp0\out\bin\Debug
if %PROCESSOR_ARCHITECTURE% == AMD64 (
  set pf="c:\program files (x86)"
) else (
  set pf="c:\program files"
)
net stop GoogleInputService
%pf%"\google\google input tools\GoogleInputService.exe" /UnregServer
taskkill /f /im googleinputhandler.exe
%outdir%\win32_register.exe --action=unregister_ime --languageid=0x0804 --name=InputTools --filename=input.ime
del c:\windows\system32\input.ime
del c:\windows\syswow64\input.ime
rmdir "c:\programdata\google\Google Input Tools"
del %pf%"\google\google input tools\GoogleInputHandler.exe"
del %pf%"\google\google input tools\GoogleInputService.exe"
pause
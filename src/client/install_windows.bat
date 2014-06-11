set x86dir=c:\program files (x86)
if not exist "c:\program files (x86)" (
  set pf=c:\program files
) else (
  set pf=%x86dir%
)
copy %~dp0\Debug\frontend_component.dll c:\windows\system32\input.ime
%~dp0\Debug\win32_register.exe --action=register_ime --languageid=0x0804 --name=InputTools --filename=input.ime
regedit %~dp0\installer\registry.reg
md "c:\programdata\google"
mklink /D "c:\programdata\google\Google Input Tools" %~dp0\resources\data
md %pf%\google
md "%pf%\google\google input tools"
copy %~dp0\Debug\ipc_console.exe "%pf%\google\google input tools\GoogleInputHandler.exe"
copy %~dp0\Debug\ipc_service.exe "%pf%\google\google input tools\GoogleInputService.exe"
"%pf%\google\google input tools\GoogleInputService.exe" /RegServer
"%pf%\google\google input tools\GoogleInputService.exe" /Service
net start GoogleInputService
pause
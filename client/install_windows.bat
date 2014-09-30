set outdir=%~dp0\out\bin\Debug
if %PROCESSOR_ARCHITECTURE% == AMD64 (
  set pf="c:\program files (x86)"
  copy %outdir%\frontend_component_x64.dll c:\windows\system32\input.ime
  copy %outdir%\frontend_component.dll c:\windows\syswow64\input.ime
) else (
  set pf="c:\program files"
  copy %outdir%\frontend_component.dll c:\windows\system32\input.ime
)
%outdir%\win32_register.exe --action=register_ime --languageid=0x0804 --name=InputTools --filename=input.ime
regedit %~dp0\installer\registry.reg
md "c:\programdata\google"
mklink /D "c:\programdata\google\Google Input Tools" %~dp0\resources\data
md %pf%"\google"
md %pf%"\google\google input tools"
copy %outdir%\ipc_console.exe %pf%"\google\google input tools\GoogleInputHandler.exe"
copy %outdir%\ipc_service.exe %pf%"\google\google input tools\GoogleInputService.exe"
%pf%"\google\google input tools\GoogleInputService.exe" /RegServer
%pf%"\google\google input tools\GoogleInputService.exe" /Service
net start GoogleInputService
pause
name = "mfc3"
path = "..\..\out\build\x64-Release\xmoser.exe"
arguments = "mfc3.msr"

set WshShell = CreateObject("WScript.Shell")
set oMyShortCut = WshShell.CreateShortcut(name+".lnk")
oMyShortCut.RelativePath = path

'oMyShortCut.WindowStyle = 7 &&Minimized 0=Maximized 4=Normal
'oMyShortcut.IconLocation = home()+"wizards\graphics\builder.ico"
'oMyShortCut.Target = target
'oMyShortCut.Arguments = arguments
oMyShortCut.WorkingDirectory = "."
oMyShortCut.Save

Function FileInUse(file)

	'WScript.Echo file

	Set fso=createobject("scripting.filesystemobject")

	on error resume next
	fso.MoveFile file, file & ".2"

	if err.number <> 0 then
   		FileInUse = True
	else 
   		FileInUse = False
		fso.MoveFile  file & ".2", file
	end if

	Set fso = Nothing
	on error goto 0

End Function


Dim WshShell
Set WshShell = WScript.CreateObject("WScript.Shell")
'WScript.Echo WshShell.CurrentDirectory

Set objShell = CreateObject("Shell.Application")
Set fso = CreateObject("Scripting.FileSystemObject") 


Set objArgs = WScript.Arguments
For I = 0 to objArgs.Count - 1
   WScript.Echo objArgs(I)
Next

zip = ""
src = ""

if objArgs.Count > 0 then
  zip = objArgs(0)
end if

if objArgs.Count > 1 then
  src = objArgs(1)
end if

isSrcFolder = fso.FolderExists(src) 

'WScript.Echo "IsSrcFolder: " & isSrcFolder

Const FOF_CREATEPROGRESSDLG = &H0&

MyZip = WshShell.CurrentDirectory & "\" & zip

Target = WshShell.CurrentDirectory & "\" &  src 

if isSrcFolder then
	Set MySrc = objShell.NameSpace(WshShell.CurrentDirectory & "\" &  src )
else
	MySrc = Target
end if
 

'WshShell.Popup(MyZip & " : " & Target)


'-------------- create empty zip file ---------

'Create the basis of a zip file.

set f = fso.CreateTextFile(MyZip, True)
f.Write "PK" & Chr(5) & Chr(6) & String(18, vbNullChar)
f.Close

'-------------- zip ---------------------------

'get ready to add files to zip

'add files
objShell.NameSpace(MyZip).CopyHere MySrc, FOF_CREATEPROGRESSDLG

wScript.Sleep 1000

while( FileInUse(MyZip) ) 
	wScript.Sleep 1000
wend

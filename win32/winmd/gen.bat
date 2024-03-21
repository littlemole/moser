set winmidl="C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\winmdidl.exe"

if not exist "%winmidl%" echo "winmidl.exe not found, check the path"; exit 1

%winmidl% /metadata_dir:. /outdir:../idl Microsoft.Foundation.winmd
%winmidl% /metadata_dir:. /outdir:../idl Microsoft.Graphics.winmd
%winmidl% /metadata_dir:. /outdir:../idl Microsoft.UI.winmd
%winmidl% /metadata_dir:. /outdir:../idl Microsoft.UI.Text.winmd
%winmidl% /metadata_dir:. /outdir:../idl Microsoft.UI.Xaml.winmd
%winmidl% /metadata_dir:. /outdir:../idl Microsoft.Web.Webview2.Core.winmd
%winmidl% /metadata_dir:. /outdir:../idl Microsoft.Windows.System.winmd

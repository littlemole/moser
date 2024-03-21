## windows dependencies

pretty all these dependencies are optional.

# moxaml

this is a winrt cpp project that provides the moxaml winrt runtime object that effectively bootstraps XAML island support,
allowing to host a xaml window inside a classic win32 window. The embedded edge webrowser support relies on XAML.

the binary results (xamll.dll and resources) are copied over to the moser build folder alongside the moser.exe.

# winuidep

this is actually a dummy winui cpp project in standalone format (not using msix packaging). It is simmply used to produce
all the runtime requirements, which are copied over to the moser build folder alongside the moser.exe.

the only ever feature used is the "expand into title bar" api, and that even needs a hack ;-(

# mocres

minimal win32 resource - only dll for example purposes. show how to put win32 classic dialog templates, icons and images into a dll so you can use it from moser.

the dll will be copied over to the moser build folder alongside the moser.exe. 

# winmeta

contains hacky solutions to read windmed files for Windows Runtime (metamoc) and plain Win32 platform (winmetamoc). this is rather low 
quality code, but it works.a

these are used to project selected windows runtime components to moser (placed as .lib files into lib/win/rt) and to generate classic win32 imports (placed as .lib files into lib/win/win32).

# winmd

create idl files from winmd files. output in ../idl folder

# idl

generates cpp header files (.h) from idl

# wix

a wix based installer project. 



## how does moser.exe know about this dependencies?

look at moser.manifest - registry free COM is what does the trick



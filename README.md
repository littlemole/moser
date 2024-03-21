# MOSER

The Mole's own Script execution runtime (M.O.S.E.R.).

<p align=right><i><small>dedicated to E.M., for everything.</small></i></p>

Moser is an academic study of [Crafting Interpreters](https://craftinginterpreters.com/) fame. Technically Moser is a C++ port of the VM based Interpreter Lox which is implemented in the second part of this wonderful book. Like Lox, Moser is a Javascript/Python hybrid scripting language powered by running bytecode in a virtual machine. Moser supports a large subset of Lox and for the most part, Syntax follows the [Lox](https://craftinginterpreters.com/the-lox-language.html) example.

## Warning: MOSER is not intended for production usage.

<i>did I say it is an academic exercise?</i>

<br>
<br>

# Major Aims of this Exercise

- Implementation of a scripting VM based on bytecode
- Native interop in the spirit of CLRs Platform Invoke (which itself of course is in the spirit of VB6's Declare Function), including event driven async programming
- Displaying native Desktop UI from script
- Some helpful Basics like Arrays and Maps (like Javascript Object), JSON support, Regular Expressions, Metadata (Annotations), runtime import and eval.

# Basic Moser Syntax

Here is fizzbuzz written in MOSER:

```js

fun fizzbuzz(i)
{
    var dividableBy3 = i % 3 == 0;
    var dividableBy5 = i % 5 == 0;

    if( dividableBy3 && dividableBy5 )
    {
        print "fizz buzz";
    }
    else if( dividableBy3 )
    {
        print "fizz";
    }
    else if( dividableBy5 )
    {
        print "buzz";
    }
    else print i;
}

var n = 20;
if( sys.args().length > 2) n = int( sys.args()[2] );

for( var i = 1; i <= n; i++)
{
    fizzbuzz(i);
}

```

# Major Changes to Nox Syntax

Moser follows Lox [syntax](https://craftinginterpreters.com/appendix-i.html) with the exception of indiciting inheritance: While Nox uses '>' as a separator in Class declarations to indicate the base (super) class, Moser uses ':' like Java or C##.

Moser has support for for and while loops, including break and continue functionality. Moser has a rudimentary throw exceptions mechanism, including catch and finally constructs.

Moser comes with two major builtin data structures - Array and Map (Object), allowing for a native JSON implementation.

Moser comes with a handful of builtin objects and functions for some fundamental tasks, and to allow for native OS interop. see [builtins](doc/builtin_objects.md) and [native lib](doc/native_lib.md)

Moser also allows for variadic functions with optional arguments.

[link to stdlib]

<br>
<br>

# Native OS Integration

Moser supports native OS integration by providing a foreign function interface to call into system libraries based on libffi.

- import C functions, including variadic ones
- declare and use C style structs
- declare and use C style callback functions and function pointers
- call a COM interface function (Win only)
- use COM IDispatch based interfaces (Win only)

[link to native code decl]

<br>
<br>

## Example C stdio code

  ```js
extern {
    ptr fopen( str, str );
    ulong fread( ptr, ulong, ulong, ptr );
    int fclose( ptr );
}

var filename = io.self(); 
var file = fopen(filename, "rb" );

var buf = sys.buffer(1024);
var content = "";

var r = 1;
while(r > 0)
{
    r = fread( buf, 1, 1024, file);
    content = content + buf.asString(r);
}

print content;

fclose(file);
```

## Building MOSER

see [Linux Build](doc/build_linux.md) and
    [Windows Build](doc/build_win.md)
for build and pre-requisite instructions.

## Linux Examples

see directory linux

### curl

demonstrates nedtworking using libcurl.

### gtk

demonstrates simple ad-hoc usage of GTK3 libs.

 !["gtk"](./doc/gtk_simple.png) 

- gtk_simple.msr : demonstrates a minimal GTK3 Hello World example
- md5.msr : UI to compute md5 hash of chosen file using shell-out (popen)
- hash.msr : computes common hashes of chosen files using openssl
- builder.msr : demonstrates using GtkBuilder to construct a simple UI from xml markaup.
- ui.msr : demonstrates a simple GTK3 UI using GtkBuilder and the local "gtk" lib

### gtk3

demonstrates using gtk via c-style bindings generated with gnome object introspection (gir).

### gtk4

same as gtk3 but for GTK version 4

### diff

Git diff viewer using runtime gnome object introspection similar to pgtk or vala bindings
 !["diff"](./doc/diff.png) 

### dot

 Dotfiles editor using runtime gnome object introspection similar to pgtk or vala bindings
 and demonstrating creating new GObject objects from moser.
 !["dot"](./doc/dot.png) 

### soup

Gnome soup networking examples, including async io.

### eve
async io using libevent

### openssl
crypto with openssl

## windows examples

see directory win32/examples

### hash.msr
cli file hashing using ms crypto libs

### petzold.msr
simple Petzold style win32 api windowing

 !["petzold"](./doc/petzold.png) 

### stdin.msr
read from stdin

###  stdio.msr
c style stdio on win32

### stdmath.msr
using c stdlib math functions in moser

### winrttest.msr
some examples / playground for winRT invocations

### win32

- encDlg.msr shows using win32 dialog resources from a resource only DLL  !["enc"](./doc/encDlg.png) 
- hash32.msr computes file hashes in a Petzold style Win32 UI  !["hash32"](./doc/hash32.png) 
- hex32.msr shows file contents in a classic hexview, Petzold style UI  !["hex32"](./doc/hex32.png) 
- hmac32.msr computes HMACs using Win32 UI
- iconWnd.msr just prints icons from a resource DLL  !["icon"](./doc/iconWnd.png) 
- mocpad.msr demonstrates basic mini text editor features  !["mocpad"](./doc/mocpad32.png) 

### disp
demonstrates using IDispatch based COM Apis from moser.

- fso.msr uses FileSystemObject from WIndows Scripting Host
- msxml.msr uses msxml for xml parsing
- xhr.msr uses XmlHttpRequest for HTTP requets similar to JavaScript
- excel.msr demonstrates Office automation

### xaml

- dot implements a simple dot file (graphviz) editor in XAML
 !["dot"](./doc/dot-xaml.png) 
- xpad implements a basic XAML editor with preview  !["xpad"](./doc/xamlpad.png) 
- mopad implements most basic notepad functionality in XAML
- hash.msr computes file hashes offering a XAML UI
 !["hash"](./doc/hash-xaml.png) 
- mochrome.msr and mohtml.msr demonstrate using WebView2 in a moser XAML app
- simple_xaml.msr just shows basic XAML embedding

# Windows impl extra deps

see directory win32

- idl is used to extract c/c++ headers from winrt idl

- mocres demonstrates an example resource only DLL used by win32 examples (dialogs, icons)

- moxaml brings in native support for XAML hosting

- test includes integration tests for win32

- winmd re-generates idl from winmd files

- winmeta has cupport for reading winmd meta files and create moser bindings for
    * metamoc generates WinRT bindings
    * winmetamoc generates Win32 bindings

 - winui is a simple WinUI example app used to ripp all the dependencies that then can be used from moser to play with WinUI. the only feature ever used is to expand xaml into the window titlebar ...

 - wix contains code for creating a MSI installer using the WIX toolkit
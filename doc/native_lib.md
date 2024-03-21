# the moser native lib

MOSER comes with a set of builtin library functions and objects.

## global functions

<table>
 <tr>
  <td>typeof(arg)</td>
  <td>prints the type of an object. for pimitices, the values are nil, boolean, int and double. heap objects will print their type.</td>
 </tr>
 <tr>
  <td>int(arg)</td>
  <td>convert variable to integer</td>
 </tr>
 <tr>
  <td>float(arg)</td>
  <td>convert variable to a floating point double.</td>
 </tr>
 <tr>
  <td>string(arg)</td>
  <td>convert variable to string</td>
 </tr>
 <tr>
  <td>char(arg)</td>
  <td>convert variable to a single char. for number types, the number will be converted to char. for strings, it will just be the value of the first character.</td>
 </tr>
 <tr>
  <td>regex(rgx,optional:options)</td>
  <td>constructs a REGEX object</td>
 </tr>
 <tr>
  <td>mustache(tmpl,json)</td>
  <td>takes two arguments, the first being a mustache template string, the second the (JSON like) data strcuture to expand. returns the transformed string.</td>
 </tr>
 <tr>
  <td>import(path)</td>
  <td>imports a module with code</td>
 </tr>
 <tr>
  <td>arguments()</td>
  <td>returns an array with the arguments to a function. must be called in the first in a function.</td>
 </tr>
 <tr>
  <td>strcmp(str1,str2)</td>
  <td>compares two strings like C's strcmp</td>
 </tr>
</table>

# runtime object

these functions are provided on the global "runtime" object.

<table>
 <tr>
  <td>runtime.fn(func)</td>
  <td>returns the string name of a function object passed.</td>
 </tr>
 <tr>
  <td>runtime.invoke(func,[args]) or invoke(this,name,[args])</td>
  <td>invoke function dynamically. can have two or three arguments. with two arguments, first parameter is a function object, the second an array of arguments. in the three argument form it takes an object, the name of a function, and an array of arguments.</td>
 </tr>
 <tr>
  <td>runtime.Decorator(func)</td>
  <td>creates a Python like decorator. first parameter is the target function, second is the decorator wrapper.</td>
 </tr>
 <tr>
  <td>runtime.RuntimeProxy(obj)</td>
  <td>creates a Java style Runtime Proxy. first parameter is the target object, second is the decorator wrapper.</td>
 </tr>
 <tr>
  <td>runtime.global(name) or global(name,value)</td>
  <td>accessor for global variable. called with one parameter, it will return the global variable of that name if it exists, or nil. called with two arguments, it will set the global variable named in param one with the contents of param two.</td>
 </tr>
  <tr>
  <td>runtime.eval(codetxt)</td>
  <td>yup, eval as a function. pass code as string argument for runtime evalution.</td>
 </tr>
 <tr>
  <td>runtime.meta(target)</td>
  <td>return the metadata associated with that function, or nil.</td>
 </tr>
 <tr>
  <td>runtime.gc_run()</td>
  <td>force a garbage collector run for debugging purposes.</td>
 </tr>
 <tr>
  <td>runtime.add_lib_path(path)</td>
  <td>adds a path to the VM to search for modules when calling "import"</td>
 </tr>
</table>

# io object

<table>
 <tr> 
  <td>io.self()</td>
  <td>path to MOSER executable</td>
 </tr>
 <tr> 
  <td>io.is_directory(path)</td>
  <td>returns true if passed path points to a file system directory</td>
 </tr>
 <tr> 
  <td>io.dirname(path)</td>
  <td>returns directox full name of passed file system path</td>
 </tr>
 <tr> 
  <td>io.basename(path)</td>
  <td>returns the filename of a full path</td>
 </tr>
 <tr> 
  <td>io.ext(path)</td>
  <td>returns file extension of passed path</td>
 </tr>
 <tr> 
  <td>io.glob(path)</td>
  <td>enumarates a file system directory</td>
 </tr>
 <tr> 
  <td>io.readline()</td>
  <td>reads a line from stdin</td>
 </tr>
 <tr> 
  <td>io.cwd()</td>
  <td>return current working directory</td>
 </tr>
 <tr> 
  <td>io.slurp(path)</td>
  <td>slurp a file as string assuming content in utf-8</td>
 </tr>
 <tr> 
  <td>io.flush(path,content)</td>
  <td>save argument 2 to the file in argument 1</td>
 </tr>
 <tr> 
  <td>io.toHex(binaryString)</td>
  <td>convert (binary) string to hex representation</td>
 </tr>
 <tr> 
  <td>io.fromHex(hexString)</td>
  <td>convert a hex representation to (binary) string</td>
 </tr>
</table>

# sys object

<table>
 <tr>
  <td>sys.clock()</td>
  <td>calls C time(0)</td>
 </tr>
 <tr>
  <td>sys.ime()</td>
  <td>ms since epoch</td>
 </tr>
 <tr>
  <td>sys.args()</td>
  <td>script arguments as array. note arg[0] is always MOSER, and arg[1] is the name of the script. </td>
 </tr>
 <tr>
  <td>sys.popen(path)</td>
  <td>run the specified cli command and return output as string.</td>
 </tr>
 <tr>
  <td>sys.buffer(size)</td>
  <td>create a C memory buffer of desired size</td>
 </tr>
 <tr>
  <td>sys.pointer() or pointer(addr)</td>
  <td>creates a C void* pointer</td>
 </tr>
 <tr>
  <td>sys.rand() or rand(upper) or rand(lower,upper)</td>
  <td>random number like C's rand</td>
 </tr>
 <tr>
  <td>sys.errno()</td>
  <td>return current C errno</td>
 </tr>
</table>

# os object

<table>
 <tr>
  <td>os.malloc(size)</td>
  <td>as in C, returns sys.pointer</td>
 </tr>
 <tr>
  <td>os.free(ptr)</td>
  <td>call C free on a sys.pointer</td>
 </tr>
 <tr>
  <td>os.toDos(string)</td>
  <td>replace \n with \r\n in a string</td>
 </tr>
 <tr>
  <td>os.toUnix(string)</td>
  <td>replace \r\n with \n in a string</td>
 </tr>
 <tr>
  <td>os.system(path)</td>
  <td>call C's system function</td>
 </tr>
 <tr>
  <td>os.linesep<()/td>
  <td>return the Os' line seperator</td>
 </tr>
 <tr>
  <td>os.pathsep()</td>
  <td>return the Os' path separator</td>
 </tr>
</table>

# JSON object

following JavaScript:

<table>
 <tr>
  <td>JSON.stringify(obj)</td>
  <td>return JSON represenation as a string</td>
 </tr>
 <tr>
  <td>JSON.flatten(obj)</td>
  <td>return JSON represenation as a string in a single line</td>
 </tr>
 <tr>
  <td>JSON.parse(string)</td>
  <td>parse a JSON string into object representation (MOSER Maps/Arrays)</td>
 </tr>
</table>

# foreign object

<table>
 <tr>
  <td>foreign.native(lib,retType,name,[argTypes])</td>
  <td>declare a new native C function</td>
 </tr>
 <tr>
  <td>foreign.variadic(lib,retType,name,[argType,"..."])</td>
  <td>declare  a new native variadic C function</td>
 </tr>
 <tr>
  <td>foreign.struct("membername:membertype", ...) or struct(["membername:membertype"])</td>
  <td>create a new native C struct at runtime</td>
 </tr>
 <tr>
  <td>foreign.named_struct(structname, "membername:membertype", ...) or named_struct(structname,["membername:membertype"])</td>
  <td>create a new named native C struct at runtime - will be put into named struct cache so it can be reused, including as member of other new native struct definitions.</td>
 </tr>
 <tr>
  <td>foreign.callback(retType,[argTypes]</td>
  <td>a new C function callback definition</td>
 </tr>
 <tr>
  <td>foreign.offset_of(struct,name)</td>
  <td>return the offset of a member in a struct</td>
 </tr>
</table>

# win32 only

## globals

<table>
 <tr>
  <td>wstring(str)</td>
  <td>converts variable to wstring for Wind32 API calls. convert a wstring back to string using "string" function.</td>
 </tr>
</table>

## runtime object

<table>
 <tr>
  <td>runtime.menue([nested entries])</td>
  <td>create a Win32 Menu described by a MOSER (nested) array</td>
 </tr>
 <tr>
  <td>runtime.encode(str,codePage) / runtime.decode(str,codePage) </td>
  <td>wrappers around Win32 WideCharToMultiByte and MultiByteToWideChar </td>
 </tr>
 <tr>
  <td>runtime.MAKEWORD(int) / runtime.MAKELONG(int) / runtime.LOWORD(int) / runtime.HIWORD(int) / runtime.LOBYTE(int) / runtime.HIBYTE(int) / runtime.MAKEINTRESOURCE(string)</td>
  <td>functions to replace the corresponding Win32 C macros</td>
 </tr>
</table>

## com object

<table>
 <tr>
  <td>com.Ptr(addr)</td>
  <td>create new COM object from void*</td>
 </tr>
 <tr>
  <td>com.DispatchPtr(addr)</td>
  <td>create new COM Dispatch object from void*</td>
 </tr>
 <tr>
  <td>com.createObject(string)</td>
  <td>create new COM Dispatch object from ProgID</td>
 </tr>
 <tr>
  <td>com.enumerate(IEnumerator)</td>
  <td>create MOSER array from IEnumerator</td>
 </tr>
 <tr>
  <td>com.init()</td>
  <td>call CoInitialize(0)</td>
 </tr>
</table>

## winrt object

<table>
 <tr>
  <td>winrt.activationFactory(string)</td>
  <td>create new WinRT factory for named WinRT object</td>
 </tr>
 <tr>
  <td>winrt.activate(string)</td>
  <td>create new named WinRT object, default c'tor
 <tr>
  <td>winrt.Delegate([argTypes],handlerIID,callbackFunction)</td>
  <td>create new COM WinRT callback delegate</td>
 </tr>
 <tr>
  <td>winrt.init()</td>
  <td>call RoInitialize (singlethead)</td>
 </tr>
</table>


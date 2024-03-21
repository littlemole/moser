# MOSER native function declarative support

to declare a native function to be called from MOSER script, declare it as such:

``` C
extern from "libc.so" {
    long stat(str,ptr) as fileinfo;
}
```

note if you call functions from libc, the from "lib" part can be ommited.

declare a variadic C function like this:

``` C
extern {
   int printf( str, ...);
}
``` 

a C struct can be described as follows:


``` C
extern struct STAT {

    ulong st_dev,
    ulong st_ino,
    ulong st_nlink,
    uint  st_mode,
    uint  st_uid,
    uint  st_gid,
    int   __pad0,
    ulong st_rdev,
    long  st_size,
    long  blksize,
    long  blocks,
    long  st_atim_sec,
    long  st_atim_nsec,
    long  st_mtim_sec,
    long  st_mtim_nsec,
    long  st_ctim_sec,
    long  st_ctim_nsec,
    long glibcreserved1,
    long glibcreserved2,
    long glibcreserved3
}
``` 

a C callback function can be declared as such:

``` C
extern callback long WINDOWPROC(ptr, uint, ptr, ptr);
```

and used by providing a MOSER function:

```C
var cb = WINDOWPROC( fun( hwnd, msg, wparam, lparam) {
  // callback handler code here  
});
```

cb can then be passed to a native function expecting a function ptr of type WINDOWPROC.

# recongized types for native declarations

<table>
 <tr>
  <td>byte</td><td>ffi_type_sint8</td>
 </tr>
 <tr>
  <td>short</td><td>ffi_type_sint16</td>
 </tr>
 <tr>
  <td>int</td><td>ffi_type_sint32</td>
 </tr>
 <tr>
  <td>long</td><td>ffi_type_sint64</td>
 </tr>
 <tr>
  <td>ubyte</td><td>ffi_type_uint8</td>
 </tr>
 <tr>
  <td>ushort</td><td>ffi_type_uint16</td>
 </tr>
 <tr>
  <td>uint</td><td>ffi_type_uint32</td>
 </tr>
 <tr>
  <td>ulong</td><td>ffi_type_uint64</td>
 </tr>
 <tr>
  <td>float</td><td>ffi_type_float</td>
 </tr>
 <tr>
  <td>double</td><td>ffi_type_double</td>
 </tr>
 <tr>
  <td>ptr</td><td>ffi_type_pointer</td>
 </tr>
 <tr>
  <td>str</td><td>ffi_type_pointer</td>
 </tr>
 <tr>
  <td>wstr</td><td>ffi_type_pointer</td>
 </tr>
 <tr>
  <td>void</td><td>ffi_type_pointer</td>
 </tr>
 <tr>
  <td>ptr</td><td>ffi_type_pointer</td>
 </tr>
</table>

note when "converting" C headers: watch the ABI, especially the differences between Linux and Win64. see
[ABI](https://en.cppreference.com/w/cpp/language/types) especially LLP64 (windows) and LP64 data models.

in short:

# linux
<table>
 <tr>
  <th>C type</th>
  <th>size</th>
  <th>MOSER native type</th>
 </tr>
 <tr>
  <td>int</td>
  <td>32 bit</td>
  <td>int</td>
 </tr>
 <tr>
  <td>long</td>
  <td>64 bit</td>
  <td><b>long</b></td>
 </tr>
 <tr>
  <td>void*</td>
  <td>64 bit</td>
  <td>ptr</td>
 </tr>
</table>

# windows
<table>
 <tr>
  <th>C type</th>
  <th>size</th>
  <th>MOSER native type</th>
 </tr>
 <tr>
  <td>int</td>
  <td>32 bit</td>
  <td>int</td>
 </tr>
 <tr>
  <td>long</td>
  <td>32 bit</td>
  <td><b>int</b></td>
 </tr>
 <tr>
  <td>void*</td>
  <td>64 bit</td>
  <td>ptr</td>
 </tr>
</table>
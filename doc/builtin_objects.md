# MOSER builtin objects

## string

string of characters (usually utf-8)

<table>
 <tr>
  <td>length</td>
  <td>field holding the length of the string, in bytes</td>
 </tr>
 <tr>
  <td>add(str,...)</td>
  <td>appends all strings passed as arguments to this string</td>
 </tr>
 <tr>
  <td>substr(start) or substr(start,n)</td>
  <td>returns the substring starting at zero-based index start. if n is given, return max n bytes of that string</td>
 </tr>
 <tr>
  <td>find(str)</td>
  <td>return the zero-based index of the first occurence of str, or -1</td>
 </tr>
 <tr>
  <td>split(str)</td>
  <td>splits the string on str boundaries and returns an array of the split result.</td>
 </tr>
 <tr>
  <td>replace_all(rgxwhat,with)</td>
  <td>return a new string where all occurences of regex what are replaced by with.</td>
 </tr>
 <tr>
  <td>replace(rgxwhat,with)</td>
  <td>return a new string where the first occurence of regex what is replaced by with.</td>
 </tr>
</table>

## class

a class is a global object that can be used to procude new object instances of that class.

<table>
 <tr>
  <td>keys()</td>
  <td>returns an array with all the properties of this class. class properties are basically <i>static</i> properties belonging to the class instance, not an object instance.</td>
 </tr>
 <tr>
  <td>methods()</td>
  <td>returns an array with all the member function names.</td>
 </tr>
</table>

## object instance
<table>
 <tr>
  <td>className</td>
  <td>field holding the name of the class this object was constructed from.</td>
 </tr>
 <tr>
  <td>type</td>
  <td>field holding a reference to the class instance this object was constructed from</td>
 </tr>
 <tr>
  <td>keys()</td>
  <td>returns an array with all this object's property names.</td>
 </tr>
 <tr>
  <td>methods()</td>
  <td>returns an array with all this object's member function names.</td>
 </tr>
</table>

## map

Map is like a JavaScript Object.

<table>
 <tr>
  <td>length</td>
  <td>a field holding the number of keys in this map</td>
 </tr>
 <tr>
  <td>exists(key)</td>
  <td>returns true if key exists in this map, otherwise false.</td>
 </tr>
 <tr>
  <td>keys()</td>
  <td>returns an array with all the key names of this map.</td>
 </tr>
 <tr>
  <td>forEach(visitor)</td>
  <td>calls function visitor(key,value) callback for each key and value of this map</td>
 </tr>
 <tr>
  <td>filter(visitor)</td>
  <td>calls function visitor(key,value) callback for each key and value of this map. if visitor returns true, this key:value pair will be added to the result map returned by filter.</td>
 </tr>
 <tr>
  <td>transform(visitor)</td>
  <td>calls function visitor(key,value) callback for each key and value of this map. if visitor does not return nil, the return value will be added to the result map.</td>
 </tr>
 <tr>
  <td>sort() or sort(compare)</td>
  <td>returns a new, sorted array. if compare(rhs,lhs) function is specified, it will be called to compare two elements. the compare function should return 0 for equality or -1 and 1 for lower / greater.</td>
 </tr>
 </table>



## array

<table>
 <tr>
  <td>length</td>
  <td>fielding holding the number of elements in this array</td>
 </tr>
 <tr>
  <td>contains(element)</td>
  <td>return true if the array contains that element</td>
 </tr>
 <tr>
  <td>join(array)</td>
  <td>adds all elements from array to this array.</td>
 </tr>
 <tr>
  <td>pop()</td>
  <td>pops the last element from this array and returns it</td>
 </tr>
 <tr>
  <td>back()</td>
  <td>returns the last element of the array</td>
 </tr>
 <tr>
  <td>add(value) and push(value)</td>
  <td>adds value to the end of the array</td>
 </tr>
 <tr>
  <td>forEach(visitor)</td>
  <td>calls function visitor(value) callback for each value of this array</td>
 </tr>
 <tr>
  <td>filter(visitor)</td>
  <td>calls function visitor(value) callback for each key and value of this map. if visitor returns true, this value will be added to the result array returned by filter.</td>
 </tr>
 <tr>
  <td>transform(visitor)</td>
  <td>calls function visitor(value) callback for each kvalue of this array. if visitor does not return nil, the return value will be added to the result array.</td>
 </tr>
</table>

## regex

simple wrapper around c++ regex
 <table>
 <tr>
  <td>suffix</td>
  <td>field holding the regex suffix</td>
 </tr>
 <tr>
  <td>match(str)</td>
  <td>return nil if this regex does not match str. otherwise it returns an array where the first element is the matched string, and the following elements hold each regex capturing group as a substring</td>
 </tr>
 <tr>
  <td>search(str)</td>
  <td>like match but to be called more than once to find all occurences of rgx in str.</td>
 </tr>
 <tr>
  <td>replace(str,with)</td>
  <td>replacde first occurences of this rgx in str by with.</td>
 </tr>
 <tr>
  <td>replace:all(str,with)</td>
  <td>replacde all occurences of this rgx in str by with.</td>
 </tr>
</table>

## decorator

a python like decorator around a function.

see [here](../test/decorator.msr) for basic decorator usage.

## runtimeproxy

a Java like runtime proxy around a target object.
see [here](../test/proxy.msr) for basic decorator usage.

# win32 only:

## wstring

like string but for Win32 16-bit unicode.
wstring does not offer methods or properties, it just exists to call Win32 API functions expecting wide strings.

create a wstring from a string by calling the global wstring(str) function. to convert back, call the global string(wstring) function.

## com objects

not meant to be called directly, used by WinRT projection to call WinRT COM objects

see [here](../win32/examples/winrttests.msr) for basic winRT COM usage.

## dispatch object

encapsulates a classic COM Dispatch object aka OLE Automation.

see [here](../win32/examples/disp) for basic COM Dispatch usage



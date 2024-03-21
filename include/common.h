#ifndef moc_common_h
#define moc_common_h

#include <string>
#include <vector>
#include <cstddef>
#include <stdint.h>

// result of interpreting source code

enum class InterpretResult 
{
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
};

// forwards 

class Obj;
class ObjString;
class ObjPointer;
class Compiler;


// string helpers
std::string trim(const std::string& input);
std::string fixNewline(const std::string& s);
std::string toDos(const std::string& s);
std::string toUnix(const std::string& s);
std::string unescape(const std::string& str);
std::string toHex(const std::string& input);
std::string fromHex(const std::string& hex);

std::vector<std::string> split(const std::string& s, std::string delim);
std::string join(std::vector<std::string>& items, const std::string& glue);

// simple IO helpers
std::string slurp(const char* path);
void flush(const std::string& fp, const std::string& content);

long get_mtime(const char *path);

std::string path_to_self();
std::string current_working_directory();
std::string path_to_self_directory(const char* postfix = 0);


// helper to read a 16 bit uint from an uint8 pointer
inline uint16_t read_short( uint8_t* p)
{
    uint16_t result = (uint16_t)(*p <<8) | *(p+1);
    return result;
}

#endif

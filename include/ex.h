#ifndef moc_ex_h
#define moc_ex_h

#include "common.h"


class Exception : public std::exception
{
public:
    std::string msg;

    Exception() {};
    Exception(const std::string& m) : msg(m) {}

    virtual const char* what() const noexcept override
    {
        return msg.c_str();
    }
};

class RuntimeException : public Exception
{
public:
    RuntimeException() {}
    RuntimeException(const std::string& m) : Exception(m) {}
};


#endif



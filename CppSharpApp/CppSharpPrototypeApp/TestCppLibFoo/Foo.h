// Foo.h
#pragma once

#ifdef FOO_EXPORTS
#  define FOO_API __declspec(dllexport)
#else
#  define FOO_API __declspec(dllimport)
#endif

#include <string>
#include <vector>

class FOO_API Foo {
public:         
    std::string Name() const;                            
    void SetName(const std::string& name);
    std::vector<int> GetSomeNumbers() const;
    void SetSomeNumbers(std::vector<int> nums);
private:
    std::string m_Name = "";
    std::vector<int> m_SomeNumbers = {};
};
// Foo.cpp

#include "Foo.h"

#include <utility>
      
std::string Foo::Name() const
{
    return m_Name;
}        
void Foo::SetName(const std::string& name)
{
    m_Name = name;
}
std::vector<int> Foo::GetSomeNumbers() const
{
    return m_SomeNumbers;
}
void Foo::SetSomeNumbers(std::vector<int> nums)
{
    m_SomeNumbers = std::move(nums);
}

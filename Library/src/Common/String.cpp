/*
 * Copyright 2023 Magnopus LLC

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "CSP/Common/String.h"

#include "Memory/Memory.h"

#include <algorithm>
#include <sstream>

namespace csp::common
{

/**
		Custom string class that we can use safely across a DLL boundary
 */

/// @brief Internal implementation for DLL safe string class
class String::Impl
{
public:
	~Impl()
	{
		CSP_DELETE_ARRAY(Text);
	}

	explicit Impl(char const* const InText)
	{
		const size_t Len = strlen(InText);

		char* NewText = CSP_NEW char[Len + 1];
		memcpy((void*) NewText, InText, Len * sizeof(char));
		NewText[Len] = 0;

		Text   = NewText;
		Length = Len;
	}

	Impl(char const* const InText, size_t Len)
	{
		char* NewText = CSP_NEW char[Len + 1];
		memcpy((void*) NewText, InText, Len * sizeof(char));
		NewText[Len] = 0;

		Text   = NewText;
		Length = Len;
	}

	explicit Impl(size_t Len)
	{
		char* NewText = CSP_NEW char[Len + 1];
#if DEBUG
		memset((void*) NewText, 0, sizeof(NewText));
#endif
		NewText[Len] = 0;
		Text		 = NewText;
		Length		 = Len;
	}

	Impl* Clone() const
	{
		return CSP_NEW Impl(Text, Length);
	}

	inline void Append(const char* Other, size_t OtherLength)
	{
		auto NewLength = Length + OtherLength;
		auto NewText   = CSP_NEW char[NewLength + 1];
		memcpy(NewText, Text, Length);
		memcpy(NewText + Length, Other, OtherLength);
		NewText[NewLength] = '\0';

		CSP_DELETE_ARRAY(Text);
		Text   = NewText;
		Length = NewLength;
	}

	void Append(const Impl& Other)
	{
		Append(Other.Text, Other.Length);
	}

	void Append(const char* Other)
	{
		auto OtherLength = strlen(Other);
		Append(Other, OtherLength);
	}

	List<String> Split(char Separator)
	{
		List<String> Parts;

		// NOTE: Don't use strtok here because it ignores empty entries!
		auto Index = strchr(Text, Separator);

		if (Index == nullptr)
		{
			Parts.Append(Text);

			return Parts;
		}

		auto Start = Text;

		for (;;)
		{
			Parts.Append(String(Start, Index - Start));
			Start = Index + 1;
			Index = strchr(Start, Separator);

			// Also look for null-terminator
			if (Index == nullptr)
			{
				Index = strchr(Start, '\0');
				Parts.Append(String(Start, Index - Start));
				break;
			}
		}

		return Parts;
	}

	char* Text;
	size_t Length;
};


String::String() : ImplPtr(CSP_NEW Impl(""))
{
}

String::String(char const* const Text, size_t Length) : ImplPtr(CSP_NEW Impl(Text, Length))
{
}

String::String(size_t Length) : ImplPtr(CSP_NEW Impl(Length))
{
}

String::String(const char* Text) : ImplPtr(CSP_NEW Impl(Text))
{
}

String::String(String const& Other) : ImplPtr(Other.ImplPtr->Clone())
{
}

List<String> String::Split(char Separator) const
{
	return ImplPtr->Split(Separator);
}

List<String> String::Split(const char* Delimiters) const
{
	std::stringstream stringStream(ImplPtr->Text);
	List<String> StringList;
	std::string InputLine;

	while (std::getline(stringStream, InputLine))
	{
		std::size_t Prev = 0, Pos;
		while ((Pos = InputLine.find_first_of(Delimiters, Prev)) != std::string::npos)
		{
			if (Pos > Prev)
				StringList.Append(InputLine.substr(Prev, Pos - Prev).c_str());
			Prev = Pos + 1;
		}
		if (Prev < InputLine.length())
			StringList.Append(InputLine.substr(Prev, std::string::npos).c_str());
	}

	return StringList;
}

String& String::swap(String& Other)
{
	std::swap(ImplPtr, Other.ImplPtr);
	return *this;
}

String& String::operator=(const String& Rhs)
{
	ImplPtr = Rhs.ImplPtr->Clone();
	return *this;
}

String& String::operator=(char const* const Text)
{
	return *this = String(Text);
}

const char* String::Get() const
{
	return ImplPtr->Text;
}

size_t String::Length() const
{
	return ImplPtr->Length;
}

size_t String::AllocatedMemorySize() const
{
	return ImplPtr->Length + 1;
}

bool String::IsEmpty() const
{
	return ImplPtr->Length == 0;
}

bool String::operator==(const String& Other) const
{
	return strcmp(Get(), Other.Get()) == 0;
}

bool String::operator==(const char* Other) const
{
	return strcmp(Get(), Other) == 0;
}

bool String::operator!=(const String& Other) const
{
	return !(*this == Other);
}

bool String::operator!=(const char* Other) const
{
	return !(*this == Other);
}

bool String::operator<(const String& Other) const
{
	return strcmp(Get(), Other.Get()) < 0;
}

String::~String()
{
	CSP_DELETE(ImplPtr);
}

void String::Append(const String& Other)
{
	ImplPtr->Append(*Other.ImplPtr);
}

void String::Append(const char* Other)
{
	ImplPtr->Append(Other);
}

String& String::operator+=(const String& Other)
{
	Append(Other);

	return *this;
}

String& String::operator+=(const char* Other)
{
	Append(Other);

	return *this;
}

String String::Trim()
{
	static char Whitespace[] = {' ', '\r', '\n', '\t'};

	auto Length = ImplPtr->Length;
	auto Text	= ImplPtr->Text;

	// Trim leading whitespace
	while (Length > 0)
	{
		auto IsWhitespace = std::find(std::begin(Whitespace), std::end(Whitespace), Text[0]) != std::end(Whitespace);

		if (!IsWhitespace)
			break;

		++Text;
		--Length;
	}

	// Trim trailing whitespace
	while (Length > 0)
	{
		auto IsWhitespace = std::find(std::begin(Whitespace), std::end(Whitespace), Text[Length - 1]) != std::end(Whitespace);

		if (!IsWhitespace)
			break;

		--Length;
	}

	return String(Text, Length);
}

String String::Join(const List<String>& Parts)
{
	return Join('\0', Parts);
}

String String::Join(const std::initializer_list<String>& Parts)
{
	return Join('\0', Parts);
}

String String::Join(char Separator, const List<String>& Parts)
{
	size_t Length = 0;

	for (int i = 0; i < Parts.Size(); ++i)
	{
		Length += Parts[i].Length();
	}

	if (Separator != '\0' && Length != 0)
	{
		Length += Parts.Size() - 1;
	}

	auto Buffer = CSP_NEW char[Length + 1]();
	size_t Pos	= 0;

	for (size_t i = 0; i < Parts.Size(); ++i)
	{
		auto PartLength = Parts[i].Length();
		memcpy(Buffer + Pos, Parts[i].c_str(), PartLength);
		Pos += PartLength;

		Buffer[Pos++] = Separator;
	}

	Buffer[Length] = '\0';

	String JoinedString(Buffer);

	CSP_DELETE_ARRAY(Buffer);

	return JoinedString;
}

String String::Join(char Separator, const std::initializer_list<String>& Parts)
{
	size_t Length = 0;

	for (int i = 0; i < Parts.size(); ++i)
	{
		Length += (Parts.begin() + i)->Length();
	}

	if (Separator != '\0')
	{
		Length += Parts.size() - 1;
	}

	auto Buffer = CSP_NEW char[Length + 1]();
	size_t Pos	= 0;

	for (size_t i = 0; i < Parts.size(); ++i)
	{
		auto PartLength = (Parts.begin() + i)->Length();
		memcpy(Buffer + Pos, (Parts.begin() + i)->c_str(), PartLength);
		Pos += PartLength;

		Buffer[Pos++] = Separator;
	}

	Buffer[Length] = '\0';

	String JoinedString(Buffer);

	CSP_DELETE_ARRAY(Buffer);

	return JoinedString;
}

} // namespace csp::common


#include <iostream>
#include <cwctype>
#include <clocale>

#include <string>
#include <locale>
#include <codecvt>
#include <cassert>

void print(const std::string &s)
{
    for (char const &c: s) {
        std::cout << std::hex << (int)((unsigned char)c) << ' ';
    }
}

int main()
{
    std::cout << std::hex << std::showbase << std::boolalpha;
    std::setlocale(LC_ALL, "en_US.utf8");

    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;


    for (wchar_t c = 257; c < 0xFFFF; c++)
      if ((bool)std::iswpunct(c)) {
        std::string u8str = converter.to_bytes(c);
	std::cout << (std::wint_t)c  << "\t" << u8str << "\t";
        print( u8str );
        std::cout << "\n";

      }
}

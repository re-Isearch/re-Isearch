
#include "base64.h"

#include <algorithm>
#include <stdexcept>

 //
 // Depending on the url parameter in base64_chars, one of
 // two sets of base64 characters needs to be chosen.
 // They differ in their last two characters.
 //
inline static const char* base64_chars[2] = {
  // Non-URL
  // Base64 table from RFC 4648
  "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz"
  "0123456789"
  "+/", 
   // URL 
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
   "abcdefghijklmnopqrstuvwxyz"
   "0123456789"
   "-_"
};


// Like base64 but starts with numbers and removes 
// some characters that may be mistaken by readers
inline static const char base58_chars[] = {
	'1', '2', '3', '4', '5', '6', '7', '8',
	'9', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
	'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q',
	'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y',
	'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
	'h', 'i', 'j', 'k', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
	'y', 'z' 
};

std::string EncodeBase58(const std::vector<uint8_t>& data, const uint8_t* mapping)
{
	std::vector<uint8_t> digits((data.size() * 138 / 100) + 1);
	size_t digitslen = 1;
	for (size_t i = 0; i < data.size(); i++)
	{
		uint32_t carry = static_cast<uint32_t>(data[i]);
		for (size_t j = 0; j < digitslen; j++)
		{
			carry = carry + static_cast<uint32_t>(digits[j] << 8);
			digits[j] = static_cast<uint8_t>(carry % 58);
			carry /= 58;
		}
		for (; carry; carry /= 58)
			digits[digitslen++] = static_cast<uint8_t>(carry % 58);
	}
	std::string result;
	for (size_t i = 0; i < (data.size() - 1) && !data[i]; i++)
		result.push_back(mapping[0]);
	for (size_t i = 0; i < digitslen; i++)
		result.push_back(mapping[digits[digitslen - 1 - i]]);
	return result;
}


static unsigned int pos_of_char(const unsigned char chr) {
 //
 // Return the position of chr within base64_encode()
 //

    if      (chr >= 'A' && chr <= 'Z') return chr - 'A';
    else if (chr >= 'a' && chr <= 'z') return chr - 'a' + ('Z' - 'A')               + 1;
    else if (chr >= '0' && chr <= '9') return chr - '0' + ('Z' - 'A') + ('z' - 'a') + 2;
    else if (chr == '+' || chr == '-') return 62; // Be liberal with input and accept both url ('-') and non-url ('+') base 64 characters (
    else if (chr == '/' || chr == '_') return 63; // Ditto for '/' and '_'
    else return -1; // ERROR
}

static std::string insert_linebreaks(std::string str, size_t distance) {
 //
 // Provided by https://github.com/JomaCorpFX, adapted by me.
 //
    if (!str.length()) {
        return "";
    }

    size_t pos = distance;

    while (pos < str.size()) {
        str.insert(pos, "\n");
        pos += distance + 1;
    }

    return str;
}

template <typename String, unsigned int line_length>
static std::string encode_with_line_breaks(String s) {
  return insert_linebreaks(base64_encode(s, false), line_length);
}

template <typename String>
static std::string encode_pem(String s) {
  return encode_with_line_breaks<String, 64>(s);
}

template <typename String>
static std::string encode_mime(String s) {
  return encode_with_line_breaks<String, 76>(s);
}

template <typename String>
static std::string encode(String s, bool url) {
  return base64_encode(reinterpret_cast<const unsigned char*>(s.data()), s.length(), url);
}

std::string base64_encode(unsigned char const* bytes_to_encode, size_t in_len, bool url) {

    size_t len_encoded = (in_len +2) / 3 * 4;

    unsigned char trailing_char = url ? '.' : '=';

 //
 // Choose set of base64 characters. They differ
 // for the last two positions, depending on the url
 // parameter.
 // A bool (as is the parameter url) is guaranteed
 // to evaluate to either 0 or 1 in C++ therefore,
 // the correct character set is chosen by subscripting
 // base64_chars with url.
 //
    const char* base64_chars_ = base64_chars[url];

    std::string ret;
    ret.reserve(len_encoded);

    unsigned int pos = 0;

    while (pos < in_len) {
        ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0xfc) >> 2]);

        if (pos+1 < in_len) {
           ret.push_back(base64_chars_[((bytes_to_encode[pos + 0] & 0x03) << 4) + ((bytes_to_encode[pos + 1] & 0xf0) >> 4)]);

           if (pos+2 < in_len) {
              ret.push_back(base64_chars_[((bytes_to_encode[pos + 1] & 0x0f) << 2) + ((bytes_to_encode[pos + 2] & 0xc0) >> 6)]);
              ret.push_back(base64_chars_[  bytes_to_encode[pos + 2] & 0x3f]);
           }
           else {
              ret.push_back(base64_chars_[(bytes_to_encode[pos + 1] & 0x0f) << 2]);
              ret.push_back(trailing_char);
           }
        }
        else {

            ret.push_back(base64_chars_[(bytes_to_encode[pos + 0] & 0x03) << 4]);
            ret.push_back(trailing_char);
            ret.push_back(trailing_char);
        }

        pos += 3;
    }


    return ret;
}

template <typename String>
static std::string decode(String encoded_string, bool remove_linebreaks) {
 //
 // decode(…) is templated so that it can be used with String = const std::string&
 // or std::string_view (requires at least C++17)
 //

    if (encoded_string.empty()) return std::string();

    if (remove_linebreaks) {

       std::string copy(encoded_string);

       copy.erase(std::remove(copy.begin(), copy.end(), '\n'), copy.end());

       return base64_decode(copy, false);
    }

    size_t length_of_string = encoded_string.length();
    size_t pos = 0;

 //
 // The approximate length (bytes) of the decoded string might be one or
 // two bytes smaller, depending on the amount of trailing equal signs
 // in the encoded string. This approximation is needed to reserve
 // enough space in the string to be returned.
 //
    size_t approx_length_of_decoded_string = length_of_string / 4 * 3;
    std::string ret;
    ret.reserve(approx_length_of_decoded_string);

    while (pos < length_of_string) {
    //
    // Iterate over encoded input string in chunks. The size of all
    // chunks except the last one is 4 bytes.
    //
    // The last chunk might be padded with equal signs or dots
    // in order to make it 4 bytes in size as well, but this
    // is not required as per RFC 2045.
    //
    // All chunks except the last one produce three output bytes.
    //
    // The last chunk produces at least one and up to three bytes.
    //

       size_t pos_of_char_1 = pos_of_char(encoded_string[pos+1] );

    //
    // Emit the first output byte that is produced in each chunk:
    //
       ret.push_back(static_cast<std::string::value_type>( ( (pos_of_char(encoded_string[pos+0]) ) << 2 ) + ( (pos_of_char_1 & 0x30 ) >> 4)));

       if ( ( pos + 2 < length_of_string  )       &&  // Check for data that is not padded with equal signs (which is allowed by RFC 2045)
              encoded_string[pos+2] != '='        &&
              encoded_string[pos+2] != '.'            // accept URL-safe base 64 strings, too, so check for '.' also.
          )
       {
       //
       // Emit a chunk's second byte (which might not be produced in the last chunk).
       //
          unsigned int pos_of_char_2 = pos_of_char(encoded_string[pos+2] );
          ret.push_back(static_cast<std::string::value_type>( (( pos_of_char_1 & 0x0f) << 4) + (( pos_of_char_2 & 0x3c) >> 2)));

          if ( ( pos + 3 < length_of_string )     &&
                 encoded_string[pos+3] != '='     &&
                 encoded_string[pos+3] != '.'
             )
          {
          //
          // Emit a chunk's third byte (which might not be produced in the last chunk).
          //
             ret.push_back(static_cast<std::string::value_type>( ( (pos_of_char_2 & 0x03 ) << 6 ) + pos_of_char(encoded_string[pos+3])   ));
          }
       }

       pos += 4;
    }

    return ret;
}

std::string base64_decode(std::string const& s, bool remove_linebreaks) {
   return decode(s, remove_linebreaks);
}

std::string base64_encode(std::string const& s, bool url) {
   return encode(s, url);
}

std::string base64_encode_pem (std::string const& s) {
   return encode_pem(s);
}

std::string base64_encode_mime(std::string const& s) {
   return encode_mime(s);
}

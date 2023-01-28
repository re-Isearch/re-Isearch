
/ Like base64 but starts with numbers and removes
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

static char *encode64(char *ptr, size_t siz, unsigned long num)
{
  char tmp[13];
  const size_t modulo = sizeof(base58_chars)/sizeof(char) - 1;
  const size_t pad    = siz - 1;
  unsigned long val = num;
  size_t i = 0;

  do {
    tmp[i++] = chars[ val % modulo ];
    val /= modulo;
  } while (val);
  for (size_t j =i; j < pad; j++)
    tmp[j] = '0';
  for (size_t j = 0; j < pad; j++)
    ptr[j] = tmp[pad -  j - 1];
  ptr[j] = '\0';
  return ptr;
}


STRING encodeBase58(const STRING& data, const uint8_t* mapping)
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


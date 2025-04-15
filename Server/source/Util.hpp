#pragma once

#include <string>
#include <iostream>
#include <vector>

class Util
{
public:
    // 字符串分割函数,将src字符串按照sep字符进⾏分割，得到的各个字串放到arry中，最终返回字串的数量
    static size_t Split(const std::string &src, const std::string &sep, std::vector<std::string> *arry)
    {
        int offset = 0;
        while (offset < src.size())
        {
            ssize_t pos = src.find(sep, offset);
            // 没找到分割字符串
            if (pos == std::string::npos)
            {
                // 直接把剩余部分当作一个字串，放入arry中
                arry->push_back(src.substr(offset));
                return arry->size();
            }

            // 当前字串为空
            if (offset == pos)
            {
                offset = pos + sep.size();
                continue;
            }

            // 找到了分割字符串
            arry->push_back(src.substr(offset, pos - offset));
            offset = pos + sep.size();
        }

        return arry->size();
    }
};
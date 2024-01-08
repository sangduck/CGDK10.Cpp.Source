#if defined(_MSC_VER)
    #define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <stdio.h>
#include <cstring>
#include <fstream>
#include <string_view>

int main(int _argc, char* _argv[], char* /*_envp*/[])
{
    // declare) 
    const char* str_head_1 = "#include \"pch.h\"\nstd::string_view ";
    const char* str_head_2 = "=\n";
    const char* str_para_end_1 = "\\n\"\n";

    // check)
    if (_argc < 2)
    {
        std::cout << "invalid paramenter" << std::endl;
        return 0;
    }

    // 1) file open
    std::ifstream file_read(_argv[2]);

    // check)
    if (!file_read)
    {
        std::cout << "can't open file " << _argv[2] << "." << std::endl;
        return 1;
    }

    // 2) file open
    auto fp_write = fopen(_argv[1], "wb");

    // check)
    if (fp_write == nullptr)
    {
        std::cout << "can't open file " << _argv[1] << "." << std::endl;
        return 0;
    }

    // 3) write head
    fwrite(str_head_1, 1, strlen(str_head_1), fp_write);
    fwrite(_argv[3], 1, strlen(_argv[3]), fp_write);
    fwrite(str_head_2, 1, strlen(str_head_2), fp_write);

    // declare)
    char* buf_source = new char[65536];
    char* buf_dest = new char[65536];

    // 4) write
    while (file_read)
    {
        // - read line
        file_read.getline(buf_source, 65536);  // delim defaults to '\n'

        // check) 
        if (!file_read)
            continue;

        // - copy (add \ before ")
        {
            auto iter_source = buf_source;
            auto iter_dest = buf_dest;

            while (*iter_source != 0)
            {
                auto value = *(iter_source++);
                if (value == '\"' || value == '\\')
                    *(iter_dest++) = '\\';

                *(iter_dest++) = value;
            }
            *(iter_dest++) = 0;
        }

        std::string_view temp(buf_dest);
        if (temp.size() != 0)
        {
            auto pos_find = temp.find_first_not_of(" ");

            if (pos_find < temp.size())
            {
                temp.remove_prefix(pos_find);
            }
            else
            {
                temp = std::string_view();
            }
        }

        // check)
        // - write
        fputc('\"', fp_write);
        if (temp.size() != 0)
        {
            fwrite(temp.data(), 1, temp.size(), fp_write);
        }
        fwrite(str_para_end_1, 1, strlen(str_para_end_1), fp_write);
    }
    fputc(';', fp_write);

    delete [] buf_source;
    delete [] buf_dest;

    // 5)
    file_read.close();
    fclose(fp_write);

    // return)
    return 0;
}

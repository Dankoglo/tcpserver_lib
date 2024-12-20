#include "utils.hpp"

std::vector<std::string> chunks(const std::string& str, int chunk_size)
{
    int chunks = std::ceil( (double) str.size() / chunk_size );
    std::vector<std::string> v(chunks, "");

    int i = 0;
    for(int j = 0; j < str.size(); j++) {
        if(j == chunk_size * (i+1)) {
            i++;
        }
        v[i] += str[j];
    }

    return v;
}
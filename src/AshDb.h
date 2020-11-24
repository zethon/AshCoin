#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <istream>

namespace ashdb
{

using ValueType = unsigned char;
using Container = std::vector<ValueType>;
using PointerType = char*;
using StrLenType = std::uint32_t;

template <typename T,
    typename = typename std::enable_if<(std::is_integral<T>::value)>::type>
inline void write_data(std::ostream& stream, T value)
{
    stream.write(reinterpret_cast<PointerType>(&value), sizeof(value));
}

inline void write_data(std::ostream& stream, std::string_view data)
{
    StrLenType size = data.size();
    write_data<StrLenType>(stream, size);
    stream.write(data.data(), size);
}

template <typename T,
    typename = typename std::enable_if<(std::is_integral<T>::value)>::type>
inline void read_data(std::istream& stream, T& value)
{
    stream.read(reinterpret_cast<PointerType>(&value), sizeof(value));
}

inline void read_data(std::istream& stream, std::string& data)
{
    StrLenType len;
    ashdb::read_data(stream, len);

    data.resize(len);
    stream.read(reinterpret_cast<PointerType>(data.data()), len);
}

} // namespace ashdb
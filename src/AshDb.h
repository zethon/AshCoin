#pragma once
#include <string>
#include <vector>
#include <algorithm>

namespace ashdb
{

using AshDbPtr = unsigned char;
using Container = std::vector<AshDbPtr>;

class AshBuffer
{
    Container              _data;
    Container::iterator    _current;

public:
    auto begin() -> decltype(_data.begin())
    {
        return _data.begin();
    }

    auto end() -> decltype(_data.end())
    {
        return _data.end();
    }

    template <typename T,
        typename = typename std::enable_if<(std::is_integral<T>::value)>::type>
    void push(T value)
    {
        auto offset = _data.size();

        auto size = sizeof(value);
        _data.resize(_data.size() + size);
        
        auto current = &(*(std::next(_data.begin(), offset)));
        std::memcpy(&(*current), &value, size);
    }

    void push(std::string_view data)
    {
        std::uint32_t size = static_cast<std::uint32_t>(data.size());
        push(size);

        auto offset = _data.size();
        _data.resize(offset + size);

        auto current = &(*(std::next(_data.begin(), offset)));
        std::memcpy(&(*current), data.data(), size);
    }
};

} // namespace ashdb
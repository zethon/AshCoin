#pragma once
#include <optional>
#include <functional>
#include <string>


namespace ash
{

class IChainDatabase
{
    std::string _datafolder;

public:
    using GenesisCallback = std::function<Block(void)>;

    IChainDatabase(const std::string& datafolder)
        : _datafolder(datafolder)
    {
        // nothing to do
    }

    virtual  ~IChainDatabase() = default;

    std::string datafolder() const { return _datafolder; }

    virtual void initialize(Blockchain& chain) = 0;
    virtual bool opened() const = 0;

    virtual std::optional<Block> read(std::size_t index) const = 0;
    virtual std::size_t size() = 0;

    virtual void write(const Block& block) = 0;
    virtual void writeChain(const Blockchain& chain) = 0;
};

} // namespace

// Ash Crypto
// Copyright (c) 2017-2020, Adalid Claure <aclaure@gmail.com>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <ctime>
#include <chrono>
#include <cmath>
#include <iostream>
#include <sstream>
#include <random>
#include <cctype>

#ifdef _WINDOWS
#   include <windows.h>
#   include <shellapi.h>
#   include <Shlobj.h>
#   include <codecvt>
#else
#   include <unistd.h>
#   include <sys/types.h>
#   include <pwd.h>
#   include <boost/process.hpp>
#endif

#ifdef __APPLE__
#   include <CoreFoundation/CFBundle.h>
#   include <ApplicationServices/ApplicationServices.h>
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/join.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <fmt/core.h>

#include "utils.h"

// There's some weirdness going on in Ubuntu where using the / operator
// on Ubuntu was throwing an error in some instances. Instead I set out
// to use boost::filesystem::path::seperator but that turned out to be
// a pain since it is multibyte on Windows! So I did this manually.
#ifdef _WINDOWS
#   define PATH_SEPERATOR   '\\'
#else
#   define PATH_SEPERATOR   '/'
#endif

namespace utils
{

NotImplementedException::NotImplementedException(const std::string& funcname)
    : std::logic_error(fmt::format("Function '{}' not yet implemented.", funcname))
{
    // nothing to do
}

std::string getOsString()
{
#ifdef _WINDOWS
    return "windows";
#elif defined(__APPLE__)    
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown"
#endif
}

#ifdef _WINDOWS
std::string getWindowsFolder(int csidl)
{
    std::string retval;

    WCHAR path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, csidl, NULL, 0, path)))
    {
        std::wstring temp(path);
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;
        retval = convert.to_bytes(temp);
    }
    else
    {
        throw std::runtime_error("could not retrieve user folder");
    }

    return retval;
}
#endif

std::string getUserFolder()
{
    std::string retval;

#ifdef _WINDOWS
    retval = getWindowsFolder(CSIDL_PROFILE);
#else    
struct passwd *pw = getpwuid(getuid());
retval = pw->pw_dir;
#endif

    return retval;
}

std::string getDataFolder()
{
    std::string retval;

#ifdef _WINDOWS
    retval = getWindowsFolder(CSIDL_COMMON_APPDATA);
#else    
    struct passwd *pw = getpwuid(getuid());
    retval = pw->pw_dir;
#endif

    return retval;
}
    
void openBrowser(const std::string& url_str)
{
    if (url_str.empty()) return;

#ifdef _WINDOWS
    ShellExecute(0, 0, url_str.c_str(), 0, 0, SW_SHOWNORMAL);
#elif defined(__APPLE__)
    // only works with `http://` prepended
    CFURLRef url = CFURLCreateWithBytes (
        // allocator
        nullptr,

        // URLBytes
        (UInt8*)url_str.c_str(),     // URLBytes

        // length
        static_cast<std::int32_t>(url_str.length()),

        // encoding
        kCFStringEncodingASCII,

        // baseURL
        NULL
    );

    LSOpenCFURLRef(url, nullptr);
    CFRelease(url);
#elif defined(__linux__)
    boost::process::system("/usr/bin/xdg-open", url_str,
        boost::process::std_err > boost::process::null,
        boost::process::std_out > boost::process::null);
#else
    throw NotImplementedException("openBrowser");
#endif
}

bool isNumeric(const std::string_view& s)
{
    return !s.empty() 
        && std::find_if(s.begin(), s.end(), 
            [](char c) 
            { 
                return !std::isdigit(c); 
            }) == s.end();
}

static const std::vector<std::string> trueStrings = { "true", "on", "1" };
static const std::vector<std::string> falseStrings = { "false", "off", "0" };

bool isBoolean(const std::string_view s)
{
    auto temp = boost::range::join(trueStrings, falseStrings);
    return std::find_if(std::begin(temp), std::end(temp),
        [s](const std::string& val) -> bool
        {
            return boost::iequals(val,s);
        })
        != std::end(temp);
}

bool convertToBool(const std::string_view s)
{
    if (std::find_if(
        std::begin(trueStrings), 
        std::end(trueStrings),
        [&s](const std::string& data)
        {
            return boost::iequals(data, s);
        }) != std::end(trueStrings))
    {
        return true;
    }
    else if (std::find_if(
        std::begin(falseStrings),
        std::end(falseStrings),
        [&s](const std::string& data)
        {
            return boost::iequals(data, s);
        }) != std::end(falseStrings))
    {
        return false;
    }

    throw std::runtime_error(fmt::format("invalid value '{}'", s));
}

std::string DoDictionary(const std::string& source, const Dictionary& valmap)
{
    std::string retval { source };
    for (const auto& [key, value] : valmap)
    {
        boost::replace_all(retval, key, value);
    }
    return retval;
}

std::string getDefaultConfigFile()
{
    return fmt::format("{}{}{}",
        utils::getUserFolder(), PATH_SEPERATOR, ".ash_config");
}

std::string getDefaultDatabaseFolder()
{
    return fmt::format("{}{}{}",
        utils::getDataFolder(), PATH_SEPERATOR, "AshChain");
}

std::string getDefaultPeersFile()
{
    return fmt::format("{}{}{}",
        utils::getDefaultDatabaseFolder(), PATH_SEPERATOR, "peers.txt");
}


} // namespace

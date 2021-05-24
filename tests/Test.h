#pragma once
#include <iostream>

#include <boost/test/data/test_case.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <test-config.h>

namespace ash
{

std::string LoadFile(std::string_view filename)
{
    std::ifstream t(filename.data());
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    return str;
}

ash::Blockchain LoadBlockchain(std::string_view chainfile)
{
    const std::string filename = fmt::format("{}/tests/data/{}", ASH_SRC_DIRECTORY, chainfile);
    const std::string rawjson = LoadFile(filename);
    nl::json json = nl::json::parse(rawjson, nullptr, false);
    BOOST_TEST(!json.is_discarded());
    return json["blocks"].get<ash::Blockchain>();
}

boost::filesystem::path tempFolder()
{
    auto temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("ash%%%%%%");
    temp /= std::to_string(boost::unit_test::framework::current_test_case().p_id);
    boost::filesystem::create_directories(temp);
    return temp;
}

boost::filesystem::path tempFolder(const std::string& subfolder)
{
    auto temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path("ash%%%%%%");
    temp /= subfolder;
    temp /= std::to_string(boost::unit_test::framework::current_test_case().p_id);
    boost::filesystem::create_directories(temp);
    return temp;
}

}
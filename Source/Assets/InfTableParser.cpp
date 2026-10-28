#include "Assets/InfTableParser.h"

#include <fstream>
#include <sstream>

namespace phoenix::assets
{
    namespace
    {
        LegacyReadIssue MakeInfIssue(const std::filesystem::path& path, std::wstring message)
        {
            return LegacyReadIssue{
                LegacyReadSeverity::Error,
                LegacyAssetFormat::InfTable,
                path,
                0,
                std::move(message)
            };
        }

        std::vector<std::string> SplitFields(const std::string& line)
        {
            std::vector<std::string> fields;
            std::string field;
            std::stringstream stream(line);

            while (std::getline(stream, field, '\t'))
            {
                if (!field.empty())
                {
                    fields.push_back(field);
                }
            }

            if (fields.size() <= 1)
            {
                fields.clear();
                std::stringstream commaStream(line);
                while (std::getline(commaStream, field, ','))
                {
                    if (!field.empty())
                    {
                        fields.push_back(field);
                    }
                }
            }

            return fields;
        }
    }

    LegacyReadResult<InfTable> InfTableParser::Read(const std::filesystem::path& path) const
    {
        LegacyReadResult<InfTable> result;
        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            result.issues.push_back(MakeInfIssue(path, L"INF table could not be opened."));
            return result;
        }

        InfTable table;
        table.sourcePath = path;

        std::string line;
        std::uint32_t lineNumber = 0;
        while (std::getline(file, line))
        {
            ++lineNumber;
            if (line.empty() || line.front() == '#')
            {
                continue;
            }

            auto fields = SplitFields(line);
            if (!fields.empty())
            {
                table.records.push_back(InfRecord{ lineNumber, std::move(fields) });
            }
        }

        result.value = std::move(table);
        return result;
    }
}

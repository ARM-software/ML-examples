//
// Copyright © 2017 Arm Ltd. All rights reserved.
// SPDX-License-Identifier: MIT
//
#include <algorithm>
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <string>
#include <vector>


using CategoryNames = std::vector<std::string>;

/** Split a string into tokens by a delimiter
 *
 * @param[in] originalString    Original string to be split
 * @param[in] delimiter         Delimiter used to split \p originalString
 * @param[in] includeEmptyToekn If true, include empty tokens in the result
 * @return A vector of tokens split from \p originalString by \delimiter
 */
std::vector<std::string>
    SplitBy(const std::string& originalString, const std::string& delimiter = " ", bool includeEmptyToken = false)
{
    std::vector<std::string> tokens;
    size_t cur  = 0;
    size_t next = 0;
    while ((next = originalString.find(delimiter, cur)) != std::string::npos)
    {
        // Skip empty tokens, unless explicitly stated to include them.
        if (next - cur > 0 || includeEmptyToken)
        {
            tokens.push_back(originalString.substr(cur, next - cur));
        }
        cur = next + delimiter.size();
    }
    // Get the remaining token
    // Skip empty tokens, unless explicitly stated to include them.
    if (originalString.size() - cur > 0 || includeEmptyToken)
    {
        tokens.push_back(originalString.substr(cur, originalString.size() - cur));
    }
    return tokens;
}

/** Remove any preceding and trailing character specified in the characterSet.
 *
 * @param[in] originalString    Original string to be stripped
 * @param[in] characterSet      Set of characters to be stripped from \p originalString
 * @return A string stripped of all characters specified in \p characterSet from \p originalString
 */
std::string Strip(const std::string& originalString, const std::string& characterSet = " ")
{
    const std::size_t firstFound = originalString.find_first_not_of(characterSet);
    const std::size_t lastFound  = originalString.find_last_not_of(characterSet);
    // Return empty if the originalString is empty or the originalString contains only to-be-striped characters
    if (firstFound == std::string::npos || lastFound == std::string::npos)
    {
        return "";
    }
    return originalString.substr(firstFound, lastFound + 1 - firstFound);
}

/** Load and parse model output labels file
 *
 * @param[in] modelOutputLabelsPath   Path to model output labels file
 * @return A vector of category names, corresponding to each output node
 */
std::vector<CategoryNames> LoadModelOutputLabels(const std::string& modelOutputLabelsPath)
{
    BOOST_ASSERT(!modelOutputLabelsPath.empty() &&
            boost::filesystem::exists(modelOutputLabelsPath) &&
            boost::filesystem::is_regular_file(modelOutputLabelsPath));

    std::vector<CategoryNames> modelOutputLabels;
    std::ifstream modelOutputLablesFile(modelOutputLabelsPath);
    std::string line;
    while (std::getline(modelOutputLablesFile, line))
    {
        CategoryNames tokens                  = SplitBy(line, ":");
        CategoryNames predictionCategoryNames = SplitBy(tokens.back(), ",");
        std::transform(predictionCategoryNames.begin(), predictionCategoryNames.end(), predictionCategoryNames.begin(),
                       [](const std::string& category) { return Strip(category); });
        modelOutputLabels.push_back(predictionCategoryNames);
    }
    return modelOutputLabels;
}

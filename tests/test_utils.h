#ifndef EOPFZARR_TEST_UTILS_H
#define EOPFZARR_TEST_UTILS_H

#include <string>

// Forward declarations for functions we want to test
// These will be linked from the main driver code

/**
 * Parse a subdataset path from EOPFZARR format
 * This function is exposed for unit testing
 */
std::string ParseSubdatasetPath(const std::string& path);

/**
 * Test helper to validate URL format detection
 */
bool IsVirtualPath(const std::string& path);

/**
 * Test helper to check if path is quoted
 */
bool IsQuotedPath(const std::string& path);

#endif  // EOPFZARR_TEST_UTILS_H

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cu_test/cu_test.h"

#include "util.h"
#include "logproto/logproto.pb-c.h"

void TestUtilTrimWhiteSpaceEmpty(CuTest *tc)
{
    size_t len = 0;
    char *s = "";
    char *result;

    s = trim_whitespace(s, &len);
    result = malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    CuAssertStrEquals(tc, "", result);
    free(result);

    s = "  \n";
    s = trim_whitespace(s, &len);
    result = malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    CuAssertStrEquals(tc, "", result);
    free(result);

}

void TestUtilTrimWhiteSpaceSimple(CuTest *tc)
{
    size_t len = 0;
    char *s = " a  aa ";
    char *result;

    s = trim_whitespace(s, &len);
    result = malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    CuAssertStrEquals(tc, "a  aa", result);
    free(result);

    s = "a aa";
    s = trim_whitespace(s, &len);
    result = malloc(len + 1);
    strncpy(result, s, len);
    result[len] = '\0';
    CuAssertStrEquals(tc, "a aa", result);
    free(result);

}

/*-------------------------------------------------------------------------*
 * main
 *-------------------------------------------------------------------------*/

CuSuite *CuUtilGetSuite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestUtilTrimWhiteSpaceEmpty);
    SUITE_ADD_TEST(suite, TestUtilTrimWhiteSpaceSimple);

    return suite;
}

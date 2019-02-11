#include <assert.h>
#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "cu_test/cu_test.h"

#include "input_logread.h"
#include "util.h"
#include "logproto/logproto.pb-c.h"

void TestInputLogreadParseLineEmpty(CuTest *tc)
{
    Logproto__Entry entry;

    logproto__entry__init(&entry);
    input_logread_parse_line(
        "   ",
        &entry
    );
    CuAssertStrEquals(tc, "", entry.line);
    logproto__entry__free(&entry);

    logproto__entry__init(&entry);
    input_logread_parse_line(
        "",
        &entry
    );
    CuAssertStrEquals(tc, "", entry.line);
    logproto__entry__free(&entry);
}

void TestInputLogreadParseLineOpenWRT(CuTest *tc)
{
    Logproto__Entry entry;
    char *line = "daemon.notice hostapd: wlan0: AP-STA-DISCONNECTED xx:yy:zz:42:a2:2c";

    logproto__entry__init(&entry);
    input_logread_parse_line(
        "Sun Feb 10 14:39:20 2019 daemon.notice hostapd: wlan0: AP-STA-DISCONNECTED xx:yy:zz:42:a2:2c ",
        &entry
    );
    CuAssertStrEquals(tc, line, entry.line);
    CuAssertIntEquals(tc, 1549809560, entry.timestamp->seconds);
    logproto__entry__free(&entry);

    logproto__entry__init(&entry);
    input_logread_parse_line(
        "Sun Feb 10 14:39:20 2019 daemon.notice hostapd: wlan0: AP-STA-DISCONNECTED xx:yy:zz:42:a2:2c\n",
        &entry
    );
    CuAssertStrEquals(tc, line, entry.line);
    CuAssertIntEquals(tc, 1549809560, entry.timestamp->seconds);
    logproto__entry__free(&entry);
}

void TestInputLogreadParseLineSystemd(CuTest *tc)
{
    Logproto__Entry entry;
    char *line = "christian-xps kernel: wlp58s0: associated";
    struct tm *tm = NULL;

    logproto__entry__init(&entry);
    input_logread_parse_line(
        "Feb 11 08:32:41 christian-xps kernel: wlp58s0: associated",
        &entry
    );
    CuAssertStrEquals(tc, line, entry.line);
    tm = localtime(&entry.timestamp->seconds);
    CuAssertIntEquals(tc, tm->tm_mon, 1);
    CuAssertIntEquals(tc, tm->tm_mday, 11);
    CuAssertIntEquals(tc, tm->tm_hour, 8);
    CuAssertIntEquals(tc, tm->tm_min, 32);
    CuAssertIntEquals(tc, tm->tm_sec, 41);
    logproto__entry__free(&entry);

    logproto__entry__init(&entry);
    input_logread_parse_line(
        "Feb 11 08:32:41 christian-xps kernel: wlp58s0: associated\n",
        &entry
    );
    CuAssertStrEquals(tc, line, entry.line);
    tm = localtime(&entry.timestamp->seconds);
    CuAssertIntEquals(tc, tm->tm_mon, 1);
    CuAssertIntEquals(tc, tm->tm_mday, 11);
    CuAssertIntEquals(tc, tm->tm_hour, 8);
    CuAssertIntEquals(tc, tm->tm_min, 32);
    CuAssertIntEquals(tc, tm->tm_sec, 41);
    logproto__entry__free(&entry);
}

/*-------------------------------------------------------------------------*
 * main
 *-------------------------------------------------------------------------*/

CuSuite *CuInputLogreadGetSuite(void)
{
    CuSuite *suite = CuSuiteNew();

    SUITE_ADD_TEST(suite, TestInputLogreadParseLineEmpty);
    SUITE_ADD_TEST(suite, TestInputLogreadParseLineOpenWRT);
    SUITE_ADD_TEST(suite, TestInputLogreadParseLineSystemd);

    return suite;
}

#include <stdio.h>
#include <stdlib.h>

#include "cu_test/cu_test.h"

CuSuite *CuStringGetSuite();
CuSuite *CuInputLogreadGetSuite();
CuSuite *CuUtilGetSuite();

void RunAllTests(void)
{
    CuString *output = CuStringNew();
    CuSuite *suites [] = {
        CuInputLogreadGetSuite(),
        CuUtilGetSuite()
    };

    CuSuite *suite = CuSuiteNew();

    for (size_t i = 0; i < sizeof(suites) / sizeof(void *); i++) {
        CuSuiteAddSuite(suite, suites[i]);
    }

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);

    for (size_t i = 0; i < sizeof(suites) / sizeof(void *); i++) {
        CuSuiteDelete(suites[i]);
    }
    free(suite);
    CuStringDelete(output);
}

int main(void)
{
    RunAllTests();
}

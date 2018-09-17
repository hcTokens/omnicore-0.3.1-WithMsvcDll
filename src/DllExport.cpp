
#include <stdio.h>
#include <stdio.h>
//#include "DllThread.h"
//#include "test.h"

extern "C" __declspec(dllexport)  int test(void* fn)
{
    //callback(fn);
    printf("Hello,This from Clang\n");
    return 0;
};
extern "C" __declspec(dllexport) void Test3()
{
    int i = 0;

    //CDataHandler<int>* pDataHandler = new CDllDataHandler;
    //Sleep(100);
    //pDataHandler->Put(i);
    //	g_DllThread.
}

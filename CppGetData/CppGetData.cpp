// CppGetData.cpp: 定义 DLL 应用程序的导出函数。
//

#include "stdafx.h"
#include "CppGetData.h"
#include "Meta.h"

const int MAXN = 1000;
using namespace std;

// 这是导出变量的一个示例
CPPGETDATA_API int nCppGetData = 0;

// 这是导出函数的一个示例。
CPPGETDATA_API int fnCppGetData(void)
{
	return 42;
}

// 这是已导出类的构造函数。
// 有关类定义的信息，请参阅 CppGetData.h


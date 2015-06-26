#include "stdafx.h"

#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <mutex>
#include <iostream>

#define LOG_STATIC_SIZE 128

/*! Set the log function pointer to default
*/
#ifdef _DEBUG
LogFunction g_debugLogFun = nullptr;
#endif
LogFunction g_logFun = SimpleLog;
LogFunction g_ErrorlogFun = SimpleLog;
LogFunction g_AlertlogFun = SimpleLog;

/*! Set the logger function
* \param [in] lFun a function pointer to a function that has the same definition as LogFunction -> int (*LogFunction)(const char *fmt, ...) \n
* printf can be passed, or a custom function can be written as well like CLILog
*/
LAMBGINE_API void SetDebugLogger(LogFunction lFun)
{
#ifdef _DEBUG
	g_debugLogFun = lFun;
#endif
}

LAMBGINE_API void SetLogger(LogFunction lFun)
{
	g_logFun = lFun;
}

LAMBGINE_API void SetErrorLogger(LogFunction lFun)
{
	g_ErrorlogFun = lFun;
}

LAMBGINE_API void SetAlertLogger(LogFunction lFun)
{
	g_AlertlogFun = lFun;
}

LAMBGINE_API int InternalLog(LogFunction logFunc, const char *format, ...)
{
	int stringLength = 0;
	char staticBuffer[LOG_STATIC_SIZE];
	char *dynamicBuffer = nullptr;
	char *usageBuffer = nullptr;
	va_list args;

	if (logFunc)
	{
		va_start(args, format);
		stringLength = (int)(_vscprintf(format, args) + 1);
		va_end(args);

		if (stringLength > LOG_STATIC_SIZE)
		{
			dynamicBuffer = new char[stringLength];
			usageBuffer = dynamicBuffer;
		}
		else
		{
			usageBuffer = staticBuffer;
		}

		if (usageBuffer)
		{
			va_start(args, format);

			vsprintf_s(usageBuffer, stringLength, format, args);

			//call the callback, check is already done above to see if logFunc is not nullptr
			logFunc(usageBuffer);

			va_end(args);

			if (dynamicBuffer)
				delete[] dynamicBuffer;
		}
		else
		{
			stringLength = 0;
		}
	}

	return stringLength;
}

LAMBGINE_API void __stdcall DoLog(const char *msg)
{
	if (g_logFun)
	{
		g_logFun(msg);
	}
}

LAMBGINE_API void __stdcall SimpleLog(const char *msg)
{
	printf(msg);
}

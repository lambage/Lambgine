#ifndef LOG_H
#define LOG_H

#include <Lambgine\Lambgine.h>
#include <functional>

/*! type definition for log function pointer
*/
typedef void(__stdcall *LogFunction)(const char *msg);

/*! Macro to wrap using the log function, works similarly to a printf function
*/
#ifdef _DEBUG
#define DebugLog(...) InternalLog(g_debugLogFun, __VA_ARGS__)
#else
#define DebugLog(...)
#endif
#define Log(...) InternalLog(g_logFun, __VA_ARGS__)
#define LogError(...) InternalLog(g_ErrorlogFun, __VA_ARGS__)
#define LogAlert(...) InternalLog(g_AlertlogFun, __VA_ARGS__)

#ifdef _DEBUG
LAMBGINE_API extern LogFunction g_debugLogFun;
#endif
LAMBGINE_API extern LogFunction g_logFun;
LAMBGINE_API extern LogFunction g_ErrorlogFun;
LAMBGINE_API extern LogFunction g_AlertlogFun;

LAMBGINE_API void SetDebugLogger(LogFunction lFun);
LAMBGINE_API void SetLogger(LogFunction lFun);
LAMBGINE_API void SetErrorLogger(LogFunction lFun);
LAMBGINE_API void SetAlertLogger(LogFunction lFun);

LAMBGINE_API int InternalLog(LogFunction lFun, const char *format, ...);

LAMBGINE_API void __stdcall DoLog(const char *format);

LAMBGINE_API void __stdcall SimpleLog(const char *msg);

#endif // LOG_H

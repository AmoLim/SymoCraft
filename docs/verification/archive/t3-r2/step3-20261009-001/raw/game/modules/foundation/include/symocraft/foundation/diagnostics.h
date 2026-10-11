#pragma once

namespace AmoBase {
// Log level
    enum class LogLevel
    {
        All = 0,
        Log = 1,
        Info = 2,
        Warning = 3,
        Error = 4,
        Assert = 5,
        None = 6,
    };


// Macro Definitions
#if !(defined(__GNUC__) || defined(__GNUG__))
#define AmoLogger_Log(var_format, ...) AmoBase::_AmoLogger_Log(__FILE__, __LINE__, var_format, __VA_ARGS__)
#define AmoLogger_Info(var_format, ...) AmoBase::_AmoLogger_Info(__FILE__, __LINE__, var_format, __VA_ARGS__)
#define AmoLogger_Warning(var_format, ...) AmoBase::_AmoLogger_Warning(__FILE__, __LINE__, var_format, __VA_ARGS__)
#define AmoLogger_Error(var_format, ...) AmoBase::_AmoLogger_Error(__FILE__, __LINE__, var_format, __VA_ARGS__)
#define AmoLogger_Assert(condition, var_format, ...) AmoBase::_AmoLogger_Assert(__FILE__, __LINE__, condition, var_format, __VA_ARGS__)
#else
#define AmoLogger_Log(var_format, ...) AmoBase::_AmoLogger_Log(__FILE__, __LINE__, var_format, ##__VA_ARGS__)
#define AmoLogger_Info(var_format, ...) AmoBase::_AmoLogger_Info(__FILE__, __LINE__, var_format, ##__VA_ARGS__)
#define AmoLogger_Warning(var_format, ...) AmoBase::_AmoLogger_Warning(__FILE__, __LINE__, var_format, ##__VA_ARGS__)
#define AmoLogger_Error(var_format, ...) AmoBase::_AmoLogger_Error(__FILE__, __LINE__, var_format, ##__VA_ARGS__)
#define AmoLogger_Assert(condition, var_format, ...) AmoBase::_AmoLogger_Assert(__FILE__, __LINE__, condition, var_format, ##__VA_ARGS__)
#endif

// Log
// Parameters: file name, code line, variables formats
    void _AmoLogger_Log( const char* filename, int line, const char* var_format, ...);

// Info
// Parameters: file name, code line, variables formats
    void _AmoLogger_Info( const char* filename, int line, const char* var_format, ...);

// Warning
// Parameters: file name, code line, variables formats
    void _AmoLogger_Warning( const char* filename, int line, const char* var_format, ...);

// Error
// Parameters: file name, code line, variables formats
    void _AmoLogger_Error( const char* filename, int line, const char* var_format, ...);

// Assertion failure
// Parameters: file name, code line, problem condition, variables formats
    void _AmoLogger_Assert( const char* filename, int line, int condition, const char* var_format, ...);

// LogLevel Setting
// Parameters: Log Level
    void AmoLogger_SetLevel(LogLevel level);

// LogLevel Return
    LogLevel AmoLogger_GetLevel();


}

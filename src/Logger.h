/**
@file Logger.h
@author Lime Microsystems
@brief API for logging library status messages.
*/

#ifndef LIMESUITE_LOGGER_H
#define LIMESUITE_LOGGER_H

#include "limesuite/config.h"
#include "limesuite/OpStatus.h"
#include <string>
#include <cstdarg>
#include <cerrno>
#include <stdexcept>
#include <cstdint>

#undef ERROR

namespace lime {

enum class LogLevel : uint8_t {
    CRITICAL, //!< A critical error. The application might not be able to continue running successfully.
    ERROR, //!< An error. An operation did not complete successfully, but the application as a whole is not affected.
    WARNING, //!< A warning. An operation completed with an unexpected result.
    INFO, //!< An informational message, usually denoting the successful completion of an operation.
    DEBUG, //!< A debugging message, only shown in Debug configuration.
};

// C-string versions
//! Log a critical error message with formatting
static inline void critical(const char* format, ...);

//! Log an error message with formatting
static inline int error(const char* format, ...);

//! Log a warning message with formatting
static inline void warning(const char* format, ...);

//! Log an information message with formatting
static inline void info(const char* format, ...);

//! Log a debug message with formatting
static inline void debug(const char* format, ...);

//! Log a message with formatting and specified logging level
static inline void log(const LogLevel level, const char* format, ...);

// C++ std::string versions
//! Log a critical error message
static inline void critical(const std::string& text);

//! Log an error message
static inline int error(const std::string& text);

//! Log a warning message
static inline void warning(const std::string& text);

//! Log an information message
static inline void info(const std::string& text);

//! Log a debug message
static inline void debug(const std::string& text);

//! Log a message with specified logging level
static inline void log(const LogLevel level, const std::string& text);

/*!
 * Send a message to the registered logger.
 * \param level a possible logging level
 * \param format a printf style format string
 * \param argList an argument list for the formatter
 */
LIME_API void log(const LogLevel level, const char* format, va_list argList);

/*!
 * Typedef for the registered log handler function.
 */
typedef void (*LogHandler)(const LogLevel level, const char* message);

/*!
 * Register a new system log handler.
 * Platforms should call this to replace the default stdio handler.
 */
LIME_API void registerLogHandler(const LogHandler handler);

//! Convert log level to a string name for printing
LIME_API const char* logLevelToName(const LogLevel level);

/*!
 * Get the error code to string + any optional message reported.
 */
LIME_API const char* GetLastErrorMessage(void);

/*!
 * Report a typical errno style error.
 * The resulting error message comes from strerror().
 * \param errnum a recognized error code
 * \return a non-zero status code to return
 */
LIME_API int ReportError(const int errnum);
LIME_API lime::OpStatus ReportError(const lime::OpStatus errnum);

/*!
 * Report an error as an integer code and a formatted message string.
 * \param errnum a recognized error code
 * \param format a format string followed by args
 * \return a non-zero status code to return
 */
inline int ReportError(const int errnum, const char* format, ...);
inline lime::OpStatus ReportError(const lime::OpStatus errnum, const char* format, ...);

/*!
 * Report an error as a formatted message string.
 * The reported errnum is 0 - no relevant error code.
 * \param format a format string followed by args
 * \return a non-zero status code to return
 */
inline int ReportError(const char* format, ...);

/*!
 * Report an error as an integer code and message format arguments
 * \param errnum a recognized error code
 * \param format a printf-style format string
 * \param argList the format string args as a va_list
 * \return a non-zero status code to return
 */
LIME_API int ReportError(const int errnum, const char* format, va_list argList);
LIME_API lime::OpStatus ReportError(const lime::OpStatus errnum, const char* format, va_list argList);

} // namespace lime

static inline void lime::log(const LogLevel level, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    lime::log(level, format, args);
    va_end(args);
}

static inline void lime::critical(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    lime::log(lime::LogLevel::CRITICAL, format, args);
    va_end(args);
}

static inline int lime::error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    lime::log(lime::LogLevel::ERROR, format, args);
    va_end(args);
    return -1;
}

static inline void lime::warning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    lime::log(lime::LogLevel::WARNING, format, args);
    va_end(args);
}

static inline void lime::info(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    lime::log(lime::LogLevel::INFO, format, args);
    va_end(args);
}

static inline void lime::debug(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    lime::log(lime::LogLevel::DEBUG, format, args);
    va_end(args);
}

static inline void lime::critical(const std::string& text)
{
    lime::log(lime::LogLevel::CRITICAL, text);
}

static inline int lime::error(const std::string& text)
{
    lime::log(lime::LogLevel::ERROR, text);
    return -1;
}

static inline void lime::warning(const std::string& text)
{
    lime::log(lime::LogLevel::WARNING, text);
}

static inline void lime::info(const std::string& text)
{
    lime::log(lime::LogLevel::INFO, text);
}

static inline void lime::debug(const std::string& text)
{
    lime::log(lime::LogLevel::DEBUG, text);
}

//! Log a message with formatting and specified logging level
static inline void lime::log(const LogLevel level, const std::string& text)
{
    lime::log(level, "%s", text.c_str());
}

inline int lime::ReportError(const int errnum, const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    int status = lime::ReportError(errnum, format, argList);
    va_end(argList);
    return status;
}

inline lime::OpStatus lime::ReportError(const lime::OpStatus errnum, const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    lime::OpStatus status = lime::ReportError(errnum, format, argList);
    va_end(argList);
    return status;
}

inline int lime::ReportError(const char* format, ...)
{
    va_list argList;
    va_start(argList, format);
    int status = lime::ReportError(-1, format, argList);
    va_end(argList);
    return status;
}

#endif //LIMESUITE_LOGGER_H

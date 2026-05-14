#ifndef INTERPRETER_LIBS_OS_H
#define INTERPRETER_LIBS_OS_H

#include "Interpreter/Base.h"
#include "Interpreter/Value.h"
#include "Interpreter/Core.h"

namespace LuaInterpreter {
namespace LuaLibs {

class Os {
public:
    static void include(Interpreter* interp);

    // Returns an approximation of the amount in seconds of CPU time used by the program, as a float.
    // os.clock ()
    static std::vector< std::shared_ptr<Value> > clock(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Returns a string or a table containing date and time, formatted according to the given string format.
    // If format starts with '!', formatted in UTC; otherwise local time. If format is "*t" or "!*t", returns a table.
    // Default format is "%c".
    // os.date ([format [, time]])
    static std::vector< std::shared_ptr<Value> > date(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Returns the difference, in seconds, from time t1 to time t2.
    // os.difftime (t2, t1)
    static std::vector< std::shared_ptr<Value> > difftime(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Passes command to be executed by an operating system shell.
    // Returns a boolean indicating success, plus a status string ("exit" / "signal") and the status code.
    // When called without a command, returns a boolean indicating whether a shell is available.
    // os.execute ([command])
    static std::vector< std::shared_ptr<Value> > execute(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Calls the ISO C function exit to terminate the host program.
    // os.exit ([code [, close]])
    static std::vector< std::shared_ptr<Value> > exit(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Returns the value of the process environment variable varname, or fail if the variable is not defined.
    // os.getenv (varname)
    static std::vector< std::shared_ptr<Value> > getenv(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Deletes the file (or empty directory, on POSIX systems) with the given name.
    // Returns true on success; otherwise returns fail plus a string describing the error and the error code.
    // os.remove (filename)
    static std::vector< std::shared_ptr<Value> > remove(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Renames the file or directory named oldname to newname.
    // Returns true on success; otherwise returns fail plus a string describing the error and the error code.
    // os.rename (oldname, newname)
    static std::vector< std::shared_ptr<Value> > rename(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Sets the current locale of the program. Returns the name of the new locale, or fail if it cannot be honored.
    // os.setlocale (locale [, category])
    static std::vector< std::shared_ptr<Value> > setlocale(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Returns the current time when called without arguments, or a time representing the local date and time
    // specified by the given table. The returned value is a number representing seconds since some given epoch.
    // os.time ([table])
    static std::vector< std::shared_ptr<Value> > time(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);

    // Returns a string with a file name that can be used as a temporary file.
    // os.tmpname ()
    static std::vector< std::shared_ptr<Value> > tmpname(Executioner* exec, std::vector< std::shared_ptr<Value> > &args);
};

}; // LuaLibs
}; // LuaInterpreter

#endif // INTERPRETER_LIBS_OS_H

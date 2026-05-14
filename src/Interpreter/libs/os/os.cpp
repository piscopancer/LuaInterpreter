#include "./os.h"
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <chrono>
#include <string>
#include <clocale>

#ifndef _WIN32
    #include <sys/wait.h>
    #include <unistd.h>
#endif

using namespace LuaInterpreter;
using namespace LuaLibs;
using namespace LuaValue;

void Os::include(Interpreter* interp) {
    std::shared_ptr<Table> table = std::make_shared<Table>();

    table->set("clock",     std::make_shared<Function>((cxx_func)&clock));
    interp->cxx_funcnames[(cxx_func) &clock] = "Os::clock";

    table->set("date",      std::make_shared<Function>((cxx_func)&date));
    interp->cxx_funcnames[(cxx_func) &date] = "Os::date";

    table->set("difftime",  std::make_shared<Function>((cxx_func)&difftime));
    interp->cxx_funcnames[(cxx_func) &difftime] = "Os::difftime";

    table->set("execute",   std::make_shared<Function>((cxx_func)&execute));
    interp->cxx_funcnames[(cxx_func) &execute] = "Os::execute";

    table->set("exit",      std::make_shared<Function>((cxx_func)&exit));
    interp->cxx_funcnames[(cxx_func) &exit] = "Os::exit";

    table->set("getenv",    std::make_shared<Function>((cxx_func)&getenv));
    interp->cxx_funcnames[(cxx_func) &getenv] = "Os::getenv";

    table->set("remove",    std::make_shared<Function>((cxx_func)&remove));
    interp->cxx_funcnames[(cxx_func) &remove] = "Os::remove";

    table->set("rename",    std::make_shared<Function>((cxx_func)&rename));
    interp->cxx_funcnames[(cxx_func) &rename] = "Os::rename";

    table->set("setlocale", std::make_shared<Function>((cxx_func)&setlocale));
    interp->cxx_funcnames[(cxx_func) &setlocale] = "Os::setlocale";

    table->set("time",      std::make_shared<Function>((cxx_func)&time));
    interp->cxx_funcnames[(cxx_func) &time] = "Os::time";

    table->set("tmpname",   std::make_shared<Function>((cxx_func)&tmpname));
    interp->cxx_funcnames[(cxx_func) &tmpname] = "Os::tmpname";

    interp->global.set("os", table);
}

// Pull integer field from a table or return a fallback. If missing and required, throws.
static std::int64_t table_get_int_field(
    const std::shared_ptr<Table>& t,
    const std::string& key,
    std::int64_t fallback,
    bool required,
    const char* fn
) {
    auto v = t->at(key);
    if (!v || v->type() == Value::Type::NIL) {
        if (required) throw std::runtime_error(std::string("Os::") + fn + " - field '" + key + "' missing");
        return fallback;
    }
    if (v->type() != Value::Type::NUMBER) {
        throw std::runtime_error(std::string("Os::") + fn + " - field '" + key + "' must be a number");
    }
    auto n = std::static_pointer_cast<Number>(v);
    return (n->kind == Number::Kind::INT) ? n->integer : (std::int64_t) n->floating;
}

static bool table_get_bool_field(
    const std::shared_ptr<Table>& t,
    const std::string& key
) {
    auto v = t->at(key);
    if (!v || v->type() != Value::Type::BOOLEAN) return false;
    return std::static_pointer_cast<Boolean>(v)->value;
}

// os.clock ()
std::vector<std::shared_ptr<Value>> Os::clock(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() != 0) throw std::runtime_error("Os::clock - Expected no arguments");
    std::float64_t seconds = (std::float64_t) std::clock() / (std::float64_t) CLOCKS_PER_SEC;
    return { std::make_shared<Number>(seconds) };
}

// os.date ([format [, time]])
std::vector<std::shared_ptr<Value>> Os::date(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    std::string format = "%c";
    std::time_t t = std::time(nullptr);

    if (args.size() >= 1 && args[0]->type() != Value::Type::NIL) {
        if (args[0]->type() != Value::Type::STRING) {
            throw std::runtime_error("Os::date - format must be a string");
        }
        format = std::static_pointer_cast<String>(args[0])->value;
    }
    if (args.size() >= 2 && args[1]->type() != Value::Type::NIL) {
        if (args[1]->type() != Value::Type::NUMBER) {
            throw std::runtime_error("Os::date - time must be a number");
        }
        auto n = std::static_pointer_cast<Number>(args[1]);
        t = (n->kind == Number::Kind::INT) ? (std::time_t) n->integer : (std::time_t) n->floating;
    }

    bool utc = false;
    if (!format.empty() && format[0] == '!') {
        utc = true;
        format.erase(0, 1);
    }

    std::tm tm_buf{};
#ifdef _WIN32
    if (utc) gmtime_s(&tm_buf, &t);
    else     localtime_s(&tm_buf, &t);
#else
    if (utc) gmtime_r(&t, &tm_buf);
    else     localtime_r(&t, &tm_buf);
#endif

    if (format == "*t") {
        auto out = std::make_shared<Table>();
        out->set("year",  std::make_shared<Number>((std::int64_t)(tm_buf.tm_year + 1900)));
        out->set("month", std::make_shared<Number>((std::int64_t)(tm_buf.tm_mon + 1)));
        out->set("day",   std::make_shared<Number>((std::int64_t) tm_buf.tm_mday));
        out->set("hour",  std::make_shared<Number>((std::int64_t) tm_buf.tm_hour));
        out->set("min",   std::make_shared<Number>((std::int64_t) tm_buf.tm_min));
        out->set("sec",   std::make_shared<Number>((std::int64_t) tm_buf.tm_sec));
        out->set("wday",  std::make_shared<Number>((std::int64_t)(tm_buf.tm_wday + 1)));
        out->set("yday",  std::make_shared<Number>((std::int64_t)(tm_buf.tm_yday + 1)));
        out->set("isdst", std::make_shared<Boolean>(tm_buf.tm_isdst > 0));
        return { out };
    }

    char buf[256];
    size_t n = std::strftime(buf, sizeof(buf), format.c_str(), &tm_buf);
    if (n == 0 && !format.empty()) {
        // Fallback: try a larger buffer once
        std::string big(4096, '\0');
        n = std::strftime(big.data(), big.size(), format.c_str(), &tm_buf);
        return { std::make_shared<String>(std::string(big.c_str(), n)) };
    }
    return { std::make_shared<String>(std::string(buf, n)) };
}

// os.difftime (t2, t1)
std::vector<std::shared_ptr<Value>> Os::difftime(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() != 2) throw std::runtime_error("Os::difftime - Expected exactly 2 arguments");
    if (args[0]->type() != Value::Type::NUMBER || args[1]->type() != Value::Type::NUMBER) {
        throw std::runtime_error("Os::difftime - Arguments must be numbers");
    }
    auto t2_num = std::static_pointer_cast<Number>(args[0]);
    auto t1_num = std::static_pointer_cast<Number>(args[1]);
    std::time_t t2 = (t2_num->kind == Number::Kind::INT) ? (std::time_t) t2_num->integer : (std::time_t) t2_num->floating;
    std::time_t t1 = (t1_num->kind == Number::Kind::INT) ? (std::time_t) t1_num->integer : (std::time_t) t1_num->floating;
    return { std::make_shared<Number>((std::float64_t) std::difftime(t2, t1)) };
}

// os.execute ([command])
std::vector<std::shared_ptr<Value>> Os::execute(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() == 0 || args[0]->type() == Value::Type::NIL) {
        return { std::make_shared<Boolean>(std::system(nullptr) != 0) };
    }
    if (args[0]->type() != Value::Type::STRING) {
        throw std::runtime_error("Os::execute - command must be a string");
    }
    const std::string& cmd = std::static_pointer_cast<String>(args[0])->value;

    int rc = std::system(cmd.c_str());

#ifdef _WIN32
    // On Windows std::system returns the command's exit code directly.
    return {
        std::make_shared<Boolean>(rc == 0),
        std::make_shared<String>("exit"),
        std::make_shared<Number>((std::int64_t) rc)
    };
#else
    if (WIFEXITED(rc)) {
        int code = WEXITSTATUS(rc);
        return {
            std::make_shared<Boolean>(code == 0),
            std::make_shared<String>("exit"),
            std::make_shared<Number>((std::int64_t) code)
        };
    }
    if (WIFSIGNALED(rc)) {
        int sig = WTERMSIG(rc);
        return {
            std::make_shared<Nil>(),
            std::make_shared<String>("signal"),
            std::make_shared<Number>((std::int64_t) sig)
        };
    }
    return {
        std::make_shared<Boolean>(rc == 0),
        std::make_shared<String>("exit"),
        std::make_shared<Number>((std::int64_t) rc)
    };
#endif
}

// os.exit ([code [, close]])
std::vector<std::shared_ptr<Value>> Os::exit(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    int code = EXIT_SUCCESS;
    if (args.size() >= 1 && args[0]->type() != Value::Type::NIL) {
        if (args[0]->type() == Value::Type::BOOLEAN) {
            code = std::static_pointer_cast<Boolean>(args[0])->value ? EXIT_SUCCESS : EXIT_FAILURE;
        } else if (args[0]->type() == Value::Type::NUMBER) {
            auto n = std::static_pointer_cast<Number>(args[0]);
            code = (int) ((n->kind == Number::Kind::INT) ? n->integer : (std::int64_t) n->floating);
        } else {
            throw std::runtime_error("Os::exit - code must be a number or boolean");
        }
    }
    // The optional 'close' argument is ignored: this interpreter has no Lua state finalizer to close.
    std::exit(code);
}

// os.getenv (varname)
std::vector<std::shared_ptr<Value>> Os::getenv(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() != 1) throw std::runtime_error("Os::getenv - Expected exactly 1 argument");
    if (args[0]->type() != Value::Type::STRING) {
        throw std::runtime_error("Os::getenv - varname must be a string");
    }
    const std::string& name = std::static_pointer_cast<String>(args[0])->value;
    const char* val = std::getenv(name.c_str());
    if (!val) return { std::make_shared<Nil>() };
    return { std::make_shared<String>(std::string(val)) };
}

// os.remove (filename)
std::vector<std::shared_ptr<Value>> Os::remove(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() != 1) throw std::runtime_error("Os::remove - Expected exactly 1 argument");
    if (args[0]->type() != Value::Type::STRING) {
        throw std::runtime_error("Os::remove - filename must be a string");
    }
    const std::string& name = std::static_pointer_cast<String>(args[0])->value;
    if (std::remove(name.c_str()) == 0) {
        return { std::make_shared<Boolean>(true) };
    }
    int err = errno;
    return {
        std::make_shared<Nil>(),
        std::make_shared<String>(std::string(name) + ": " + std::strerror(err)),
        std::make_shared<Number>((std::int64_t) err)
    };
}

// os.rename (oldname, newname)
std::vector<std::shared_ptr<Value>> Os::rename(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() != 2) throw std::runtime_error("Os::rename - Expected exactly 2 arguments");
    if (args[0]->type() != Value::Type::STRING || args[1]->type() != Value::Type::STRING) {
        throw std::runtime_error("Os::rename - arguments must be strings");
    }
    const std::string& oldn = std::static_pointer_cast<String>(args[0])->value;
    const std::string& newn = std::static_pointer_cast<String>(args[1])->value;
    if (std::rename(oldn.c_str(), newn.c_str()) == 0) {
        return { std::make_shared<Boolean>(true) };
    }
    int err = errno;
    return {
        std::make_shared<Nil>(),
        std::make_shared<String>(oldn + " -> " + newn + ": " + std::strerror(err)),
        std::make_shared<Number>((std::int64_t) err)
    };
}

// os.setlocale (locale [, category])
std::vector<std::shared_ptr<Value>> Os::setlocale(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    const char* locale = nullptr;
    if (args.size() >= 1 && args[0]->type() != Value::Type::NIL) {
        if (args[0]->type() != Value::Type::STRING) {
            throw std::runtime_error("Os::setlocale - locale must be a string");
        }
        locale = std::static_pointer_cast<String>(args[0])->value.c_str();
    }

    int category = LC_ALL;
    if (args.size() >= 2 && args[1]->type() != Value::Type::NIL) {
        if (args[1]->type() != Value::Type::STRING) {
            throw std::runtime_error("Os::setlocale - category must be a string");
        }
        const std::string& cat = std::static_pointer_cast<String>(args[1])->value;
        if      (cat == "all")      category = LC_ALL;
        else if (cat == "collate")  category = LC_COLLATE;
        else if (cat == "ctype")    category = LC_CTYPE;
        else if (cat == "monetary") category = LC_MONETARY;
        else if (cat == "numeric")  category = LC_NUMERIC;
        else if (cat == "time")     category = LC_TIME;
        else throw std::runtime_error("Os::setlocale - unknown category: " + cat);
    }

    const char* result = std::setlocale(category, locale);
    if (!result) return { std::make_shared<Nil>() };
    return { std::make_shared<String>(std::string(result)) };
}

// os.time ([table])
std::vector<std::shared_ptr<Value>> Os::time(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() == 0 || args[0]->type() == Value::Type::NIL) {
        return { std::make_shared<Number>((std::int64_t) std::time(nullptr)) };
    }
    if (args[0]->type() != Value::Type::TABLE) {
        throw std::runtime_error("Os::time - argument must be a table");
    }
    auto t = std::static_pointer_cast<Table>(args[0]);

    std::tm tm_buf{};
    tm_buf.tm_year = (int) (table_get_int_field(t, "year",  0,  true,  "time") - 1900);
    tm_buf.tm_mon  = (int) (table_get_int_field(t, "month", 0,  true,  "time") - 1);
    tm_buf.tm_mday = (int)  table_get_int_field(t, "day",   0,  true,  "time");
    tm_buf.tm_hour = (int)  table_get_int_field(t, "hour",  12, false, "time");
    tm_buf.tm_min  = (int)  table_get_int_field(t, "min",   0,  false, "time");
    tm_buf.tm_sec  = (int)  table_get_int_field(t, "sec",   0,  false, "time");
    tm_buf.tm_isdst = table_get_bool_field(t, "isdst") ? 1 : -1;

    std::time_t ts = std::mktime(&tm_buf);
    if (ts == (std::time_t) -1) {
        return { std::make_shared<Nil>() };
    }
    return { std::make_shared<Number>((std::int64_t) ts) };
}

// os.tmpname ()
std::vector<std::shared_ptr<Value>> Os::tmpname(Executioner* exec, std::vector<std::shared_ptr<Value>>& args) {
    if (args.size() != 0) throw std::runtime_error("Os::tmpname - Expected no arguments");
#ifdef _WIN32
    char buf[L_tmpnam];
    if (tmpnam_s(buf, sizeof(buf)) != 0) {
        throw std::runtime_error("Os::tmpname - unable to generate a unique filename");
    }
    return { std::make_shared<String>(std::string(buf)) };
#else
    char buf[] = "/tmp/lua_XXXXXX";
    int fd = mkstemp(buf);
    if (fd == -1) {
        throw std::runtime_error("Os::tmpname - unable to generate a unique filename");
    }
    ::close(fd);
    return { std::make_shared<String>(std::string(buf)) };
#endif
}

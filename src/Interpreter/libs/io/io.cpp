#include "io.h"

using namespace LuaInterpreter;
using namespace LuaLibs;

void IO::include(Interpreter* interp) {
    auto table = std::make_shared<LuaValue::Table>();

    interp->global.set("print"  , std::make_shared< LuaValue::Function >( (cxx_func) &print));
    interp->cxx_funcnames[(cxx_func) &print] = "IO::print";

    table->set("close"  , std::make_shared< LuaValue::Function >( (cxx_func) &close));
    interp->cxx_funcnames[(cxx_func) &close] = "IO::close";
    table->set("flush"  , std::make_shared< LuaValue::Function >( (cxx_func) &flush));
    interp->cxx_funcnames[(cxx_func) &flush] = "IO::flush";
    table->set("input"  , std::make_shared< LuaValue::Function >( (cxx_func) &input));
    interp->cxx_funcnames[(cxx_func) &input] = "IO::input";
    table->set("lines"  , std::make_shared< LuaValue::Function >( (cxx_func) &lines));
    interp->cxx_funcnames[(cxx_func) &lines] = "IO::lines";
    table->set("open"   , std::make_shared< LuaValue::Function >( (cxx_func) &open));
    interp->cxx_funcnames[(cxx_func) &open] = "IO::open";
    table->set("output" , std::make_shared< LuaValue::Function >( (cxx_func) &output));
    interp->cxx_funcnames[(cxx_func) &output] = "IO::output";
    table->set("read"   , std::make_shared< LuaValue::Function >( (cxx_func) &read));
    interp->cxx_funcnames[(cxx_func) &read] = "IO::read";
    table->set("type"   , std::make_shared< LuaValue::Function >( (cxx_func) &type));
    interp->cxx_funcnames[(cxx_func) &type] = "IO::type";
    table->set("write"  , std::make_shared< LuaValue::Function >( (cxx_func) &write));
    interp->cxx_funcnames[(cxx_func) &write] = "IO::write";

    interp->global.set("io", table);
}

// non-owning (cout, cin, cerr)
IO::File::File(std::istream &os): Userdata(typestr), buffer(os.rdbuf()) {
    prepare_metatable();
    this->data = buffer;
}
IO::File::File(std::ostream &os): Userdata(typestr), buffer(os.rdbuf()) {
    prepare_metatable();
    this->data = buffer;
}

// owning (files)
IO::File::File(const std::string& filepath, std::ios::openmode mode): Userdata(typestr) {
    file = std::make_shared<std::filebuf>();
    file->open(filepath, mode);
    buffer = file.get(); 
    prepare_metatable();
    this->data = buffer;
}

IO::File::File(std::shared_ptr< std::filebuf > file) {
    this->file = file;
    buffer = file.get();
    prepare_metatable();
}

IO::File::~File() {}


void IO::File::prepare_metatable() {
    meta.set("close",   std::make_shared<LuaValue::Function>( (cxx_func) & close_wrapper  ));
    meta.set("flush",   std::make_shared<LuaValue::Function>( (cxx_func) & flush_wrapper  ));
    meta.set("lines",   std::make_shared<LuaValue::Function>( (cxx_func) & lines_wrapper  ));
    meta.set("read",    std::make_shared<LuaValue::Function>( (cxx_func) & read_wrapper   ));
    meta.set("seek",    std::make_shared<LuaValue::Function>( (cxx_func) & seek_wrapper   ));
    meta.set("write",   std::make_shared<LuaValue::Function>( (cxx_func) & write_wrapper  ));
}

std::vector< std::shared_ptr<Value> > IO::File::close_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    auto self = std::static_pointer_cast<File>(args[0]);
    return self->close(exec, args);
}

std::vector< std::shared_ptr<Value> > IO::File::flush_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    auto self = std::static_pointer_cast<File>(args[0]);
    return self->flush(exec, args);
}

std::vector< std::shared_ptr<Value> > IO::File::lines_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    auto self = std::static_pointer_cast<File>(args[0]);
    return self->lines(exec, args);
}
std::vector< std::shared_ptr<Value> > IO::File::read_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    auto self = std::static_pointer_cast<File>(args[0]);
    return self->read(exec, args);
}
std::vector< std::shared_ptr<Value> > IO::File::seek_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    auto self = std::static_pointer_cast<File>(args[0]);
    return self->seek(exec, args);
}
std::vector< std::shared_ptr<Value> > IO::File::write_wrapper(
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    auto self = std::static_pointer_cast<File>(args[0]);
    args.erase(args.begin()); // remove self
    return self->write(exec, args);
}





std::vector< std::shared_ptr<Value> > IO::File::close (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    if (!file) throw std::runtime_error("Trying to close non-file buffer");
    file->close();
    return { };
}

std::vector< std::shared_ptr<Value> > IO::File::flush (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    buffer->pubsync();
    return { };
}

std::vector< std::shared_ptr<Value> > IO::File::lines (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    std::shared_ptr<LuaValue::Table> table = std::make_shared<LuaValue::Table>();
    std::string line;
    size_t idx = 0;
    
    std::istream is(buffer);
    while (std::getline(is, line)) {
        table->set(
            idx++,
            std::make_shared<LuaValue::String>(line)
        );
    }

    return {table};
}

std::vector< std::shared_ptr<Value> > IO::File::read (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    if (buffer == std::cin.rdbuf()) {
        std::string data;
        std::cin >> data;
        return {std::make_shared<LuaValue::String>(data)};
    }
    std::stringstream ss;
    ss << buffer;
    return {std::make_shared<LuaValue::String>(ss.str())};
    
}

std::vector< std::shared_ptr<Value> > IO::File::seek (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    // args[0] = this
    // args[1] = whence
    // args[2] = pos
    
    int pos = 0;
    std::ios::seekdir dir;

    if (args.size() < 2) {
        throw std::runtime_error("IO::File::seek - expected 'whence' argument");
    } 
    if (args[1]->type() != Value::Type::STRING) {
        throw std::runtime_error("IO::File::seek - 'whence' argument must be \"set\", \"cur\" or \"end\"");
    }
    auto arg1 = std::static_pointer_cast<LuaValue::String>(args[1]);

    if (arg1->value == "set") {
        dir = std::ios::beg;
    } else
    if (arg1->value == "cur") {
        dir = std::ios::cur;
    } else 
    if (arg1->value == "end") {
        dir = std::ios::end;
    } else {
        throw std::runtime_error("IO::File::seek - 'whence' argument must be \"set\", \"cur\" or \"end\"");
    }

    if (args.size() > 2) {
        if (args[2]->type() != Value::Type::NUMBER) {
            throw std::runtime_error("IO::File::seek - 'pos' argument must be an integer");
        }
        auto arg2 = std::static_pointer_cast<LuaValue::Number>(args[2]);
        if (arg2->kind != LuaValue::Number::Kind::INT) {
            throw std::runtime_error("IO::File::seek - 'pos' argument must be an integer");
        }
        pos = arg2->integer;
    }

    buffer->pubseekoff(pos, dir);

    return {};
}

std::vector< std::shared_ptr<Value> > IO::File::write (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    ensure_type(typestr);

    // no self reference in args

    int bytes = 0;
    size_t N = args.size();
    for (size_t i=0; i<N; i++) {
        auto &arg = args[i];

        std::stringstream data;
        auto type = arg->type();
        switch (type) {
            case Value::Type::NIL: { data << "nil"; } break;
            case Value::Type::BOOLEAN: {
                auto bl = std::static_pointer_cast<LuaValue::Boolean>(arg);
                if (bl->value) {
                    data << "true";
                } else {
                    data << "false";
                }
            } break;
            case Value::Type::NUMBER: {
                auto num = std::static_pointer_cast<LuaValue::Number>(arg);
                if (num->kind == LuaValue::Number::Kind::INT) {
                    data << std::to_string( num->integer );
                } else {
                    data << std::to_string( (double) num->floating );
                }
            } break;
            case Value::Type::STRING: {
                auto str = std::static_pointer_cast<LuaValue::String>(arg);
                data << str->value;
            } break;
            case Value::Type::FUNCTION: {
                auto func = std::static_pointer_cast<LuaValue::Function>(arg);
                data << "Function(" << func->key() << ")";
            } break;
            case Value::Type::THREAD: {} break;
            case Value::Type::USERDATA: {
                auto ud = std::static_pointer_cast<LuaValue::Userdata>(arg);
                auto typestr = exec->type_of(ud);
                data << typestr << "(" << ud.get() << ")";
            } break;
            case Value::Type::TABLE: {
                auto tb = std::static_pointer_cast<LuaValue::Table>(arg);
                data << "Table(" << tb.get() << ")";
            } break;
            default: {
                throw std::runtime_error("IO::File::write - cannot write object of type " + std::to_string((int)type));
            }
        }

        // if (i != N-1) {
        //     data += " ";
        // }

        auto fd = data.str();
        bytes += buffer->sputn(fd.data(), fd.size());
    }

    return { std::make_shared<LuaValue::Number> ((int64_t) bytes) };
}


std::shared_ptr< std::filebuf > IO::_input = nullptr;
std::shared_ptr< std::filebuf > IO::_output = nullptr;

std::vector< std::shared_ptr<Value> > IO::close (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() == 0) {
        if (_output) _output->close();
        else { /* ignore */ }
        return {};
    }
    if (args[0]->type() != Value::Type::USERDATA) {
        throw std::runtime_error("Cannot close non-file type");
    }
    auto file = std::static_pointer_cast< IO::File >( args[0] );
    
    file->close(exec, args);
    return {};
}

std::vector< std::shared_ptr<Value> > IO::flush (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() == 0) {
        if (_output) File(_output).flush(exec, args);
        else File(std::cout).flush(exec, args);
        return {};
    }
    throw std::runtime_error("IO::flush does not expect any arguments");
}

std::vector< std::shared_ptr<Value> > IO::input (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() == 0) {
        if (_input) return { std::make_shared<IO::File>(_input) };
        else return { std::make_shared<IO::File>(std::cin) };
    }
    
    if (args[0]->type() == Value::Type::STRING) {
        auto str = std::static_pointer_cast<LuaValue::String>(args[0]);
        auto file = std::make_shared<IO::File>(str->value, std::ios::in);

        _input = file->file;
        return { file };
    }

    if (args[0]->type() == Value::Type::USERDATA) {
        auto file = std::static_pointer_cast<IO::File>(args[0]);
        file->ensure_type(IO::File::typestr);

        _input = file->file;
        return { file };
    }

    throw std::runtime_error("IO::input - argument must be string or file");
}

std::vector< std::shared_ptr<Value> > IO::lines (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.empty()) {
        if (_input) {
            return File(_input).lines(exec, args);
        } else {
            return File(std::cin).lines(exec, args);
        }
    }

    if (args[0]->type() != Value::Type::STRING) {
        throw std::runtime_error("IO::lines - filename must be a string");        
    }
    const std::string& filename = std::static_pointer_cast<LuaValue::String>(args[0])->value;
    std::string mode = "r";
    if (args.size() > 1) {
        if (args[1]->type() != Value::Type::STRING) {
            throw std::runtime_error("IO::lines - mode must be a string");        
        }
        mode = std::static_pointer_cast<LuaValue::String>(args[1])->value;
    }
    if (mode.empty()) {
        throw std::runtime_error("IO::lines - mode must be a non-empty string");        
    }

    std::ios::openmode real_mode = (std::ios::openmode) 0;
    if (mode.contains('r')) real_mode |= std::ios::in;
    if (mode.contains('w')) real_mode |= std::ios::out;
    if (mode.contains('a')) real_mode |= std::ios::app;
    if (mode.contains('b')) real_mode |= std::ios::binary;

    if (mode.contains('+')) {
        real_mode |= std::ios::ate;
    } else {
        if (real_mode & std::ios::out) real_mode |= std::ios::trunc;
    }

    File file(filename, real_mode);
    return file.lines(exec, args);
}

std::vector< std::shared_ptr<Value> > IO::open (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.empty()) {
        throw std::runtime_error("IO::open - filepath is expected"); 
    }

    if (args[0]->type() != Value::Type::STRING) {
        throw std::runtime_error("IO::open - filepath must be a string");        
    }
    const std::string& filename = std::static_pointer_cast<LuaValue::String>(args[0])->value;
    std::string mode = "r";
    if (args.size() > 1) {
        if (args[1]->type() != Value::Type::STRING) {
            throw std::runtime_error("IO::open - mode must be a string");        
        }
        mode = std::static_pointer_cast<LuaValue::String>(args[1])->value;
    }
    if (mode.empty()) {
        throw std::runtime_error("IO::open - mode must be a non-empty string");        
    }

    std::ios::openmode real_mode = (std::ios::openmode) 0;
    if (mode.contains('r')) real_mode |= std::ios::in;
    if (mode.contains('w')) real_mode |= std::ios::out;
    if (mode.contains('a')) real_mode |= std::ios::app;
    if (mode.contains('b')) real_mode |= std::ios::binary;

    if (mode.contains('+')) {
        real_mode |= std::ios::ate;
    } else {
        if (real_mode & std::ios::out) real_mode |= std::ios::trunc;
    }

    auto file = std::make_shared<File>(filename, real_mode);

    if (real_mode & std::ios::in) _input = file->file;
    if (real_mode & std::ios::out) _output = file->file;

    return { file };
}

std::vector< std::shared_ptr<Value> > IO::output (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.size() == 0) {
        if (_output) return { std::make_shared<IO::File>(_output) };
        else return { std::make_shared<IO::File>(std::cout) };
    }
    
    if (args[0]->type() == Value::Type::STRING) {
        auto str = std::static_pointer_cast<LuaValue::String>(args[0]);
        auto file = std::make_shared<IO::File>(str->value, std::ios::out);

        _output = file->file;
        return { file };
    }

    if (args[0]->type() == Value::Type::USERDATA) {
        auto file = std::static_pointer_cast<IO::File>(args[0]);
        file->ensure_type(IO::File::typestr);

        _output = file->file;
        return { file };
    }

    throw std::runtime_error("IO::output - argument must be string or file");
}

std::vector< std::shared_ptr<Value> > IO::read (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (_input) return File(_input).read(exec, args);
    else return File(std::cin).read(exec, args);;
}

std::vector< std::shared_ptr<Value> > IO::type (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (args.empty()) throw std::runtime_error("IO::type - expected file");
    if (args[0]->type() != Value::Type::USERDATA) {
        return { std::make_shared<LuaValue::Nil>() };
    }
    if (!exec->is_type_of(args[0], File::typestr)) {
        return { std::make_shared<LuaValue::Nil>() };
    }
    auto file = std::static_pointer_cast< File >(args[0]);
    return { std::make_shared<LuaValue::String>("file") };
}

std::vector< std::shared_ptr<Value> > IO::write (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    if (_output) return File(_output).write(exec, args);
    else return File(std::cout).write(exec, args);
}

std::vector< std::shared_ptr<Value> > IO::print (
    Executioner* exec,
    std::vector< std::shared_ptr<Value> > &args
) {
    File file(std::cout);
    size_t N = args.size();
    for (size_t i=0; i<N; i++) {
        std::vector< std::shared_ptr<Value> > temp = { args[i] };
        file.write(exec, temp);
        if (i != N-1) {
            std::cout << " ";
        }
    }
    std::cout << "\n";
    return {};
}
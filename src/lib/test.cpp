#include "common.hpp"

namespace bacs {

int CTest::parse_id(cstr fn)
{
    string nm = name_from_filename(fn);
    string ids, dummy;
    parse_str(nm, '.', ids, dummy);
    return s2i(ids, INVALID_ID);
}

CTest::CTest(cstr _file_in)
{
    file_in = _file_in;
    file_out = file_in.substr(0, file_in.find_last_of('.')) + ".out";
    id = parse_id(file_in);
}

} // bacs

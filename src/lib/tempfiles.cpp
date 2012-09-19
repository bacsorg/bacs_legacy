#include "tempfiles.hpp"

namespace bacs {

char random_char()
{
    int x = random(36);
    if (x < 10) return '0' + x;
    else return 'a' + (x - 10);
}

string gen_unique_filename()
{
    static int k = 0;
    string s;
    while (1)
    {
        s = i2s(k++);
        s = s + random_char() + random_char();
        while (s.length() < 7) s = "0" + s;
        s = cfg("general.temp_dir") + "/_" + s + ".tmp";
        if (!file_exists(s)) break;
    }
    return s;
}

bool CTempFile::create(cstr source)
{
    string fn = gen_unique_filename();
    FILE *f;
    f = fopen(fn.c_str(), "wb");
    if (!f) return false;
    fwrite((const void *)source.c_str(), source.length(), 1, f);
    fclose(f);
    _name = fn;
    file_created = true;
    return true;
}

bool CTempFile::erase()
{
    if (file_created)
    {
        file_created = false;
        return delete_file(_name) || !file_exists(_name);
    }
    return true;
}

string CTempFile::read(int max_size)
{
    string res = "";
    if (!file_created)
        return res;
    ifstream file(_name.c_str(), ios::in | ios::binary | ios::ate);
    if (file.is_open())
    {
        ifstream::pos_type sz = file.tellg();
        int size = sz;
        if (max_size && (int)size > max_size)
        size = max_size;
        char *memblock = new char[size];
        file.seekg(0, ios::beg);
        file.read(memblock, size);
        res.assign(memblock, size);
        file.close();
        delete [] memblock;
    }
    return res;
}

} // bacs

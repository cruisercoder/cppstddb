#ifndef CPPSTDDB_ENDIAN_H
#define CPPSTDDB_ENDIAN_H

static int big4_to_native(const void *d) {
    // for 4 byte ints
    // read https://commandcenter.blogspot.fr/2012/04/byte-order-fallacy.html
    auto a = static_cast<const char *>(d);
    return (a[3]<<0) | (a[2]<<8) | (a[1]<<16) | (a[0]<<24);
}

#endif


#ifndef CPPSTDDB_DATE_PARSE
#define CPPSTDDB_DATE_PARSE

// modified utility sqlite date parsing code

#include <ctype.h>
#include <cassert>

namespace cppstddb {
    namespace impl {

        using u8 = uint8_t;
        using u16 = uint16_t;
        using sqlite3_int64 = int64_t;


        struct DateTime {
            sqlite3_int64 iJD;  /* The julian day number times 86400000 */
            int Y, M, D;        /* Year, month, and day */
            int h, m;           /* Hour and minutes */
            int tz;             /* Timezone offset in minutes */
            double s;           /* Seconds */
            char validJD;       /* True (1) if iJD is valid */
            char rawS;          /* Raw numeric value stored in s */
            char validYMD;      /* True (1) if Y,M,D are valid */
            char validHMS;      /* True (1) if h,m,s are valid */
            char validTZ;       /* True (1) if tz is valid */
            char tzSet;         /* Timezone was set explicitly */
            char isError;       /* An overflow has occurred */
        };

        static int parseYyyyMmDd(const char *zDate, DateTime *p);

        template<class S> date_t date_parse(const S& s) {
            DateTime dt;
            parseYyyyMmDd(s.c_str(), &dt);
            return date_t(dt.Y,dt.M,dt.D);
        }


        /*
         ** Convert zDate into one or more integers according to the conversion
         ** specifier zFormat.
         **
         ** zFormat[] contains 4 characters for each integer converted, except for
         ** the last integer which is specified by three characters.  The meaning
         ** of a four-character format specifiers ABCD is:
         **
         **    A:   number of digits to convert.  Always "2" or "4".
         **    B:   minimum value.  Always "0" or "1".
         **    C:   maximum value, decoded as:
         **           a:  12
         **           b:  14
         **           c:  24
         **           d:  31
         **           e:  59
         **           f:  9999
         **    D:   the separator character, or \000 to indicate this is the
         **         last number to convert.
         **
         ** Example:  To translate an ISO-8601 date YYYY-MM-DD, the format would
         ** be "40f-21a-20c".  The "40f-" indicates the 4-digit year followed by "-".
         ** The "21a-" indicates the 2-digit month followed by "-".  The "20c" indicates
         ** the 2-digit day which is the last integer in the set.
         **
         ** The function returns the number of successful conversions.
         */
        inline static int getDigits(const char *zDate, const char *zFormat, ...){
            /* The aMx[] array translates the 3rd character of each format
             ** spec into a max size:    a   b   c   d   e     f */
            static const u16 aMx[] = { 12, 14, 24, 31, 59, 9999 };
            va_list ap;
            int cnt = 0;
            char nextC;
            va_start(ap, zFormat);
            do{
                char N = zFormat[0] - '0';
                char min = zFormat[1] - '0';
                int val = 0;
                u16 max;

                assert( zFormat[2]>='a' && zFormat[2]<='f' );
                max = aMx[zFormat[2] - 'a'];
                nextC = zFormat[3];
                val = 0;
                while( N-- ){
                    if( !isdigit(*zDate) ){
                        goto end_getDigits;
                    }
                    val = val*10 + *zDate - '0';
                    zDate++;
                }
                if( val<(int)min || val>(int)max || (nextC!=0 && nextC!=*zDate) ){
                    goto end_getDigits;
                }
                *va_arg(ap,int*) = val;
                zDate++;
                cnt++;
                zFormat += 4;
            }while( nextC );
end_getDigits:
            va_end(ap);
            return cnt;
        }

        /*
         ** Parse a timezone extension on the end of a date-time.
         ** The extension is of the form:
         **
         **        (+/-)HH:MM
         **
         ** Or the "zulu" notation:
         **
         **        Z
         **
         ** If the parse is successful, write the number of minutes
         ** of change in p->tz and return 0.  If a parser error occurs,
         ** return non-zero.
         **
         ** A missing specifier is not considered an error.
         */
        inline static int parseTimezone(const char *zDate, DateTime *p){
            int sgn = 0;
            int nHr, nMn;
            int c;
            while( isspace(*zDate) ){ zDate++; }
            p->tz = 0;
            c = *zDate;
            if( c=='-' ){
                sgn = -1;
            }else if( c=='+' ){
                sgn = +1;
            }else if( c=='Z' || c=='z' ){
                zDate++;
                goto zulu_time;
            }else{
                return c!=0;
            }
            zDate++;
            if( getDigits(zDate, "20b:20e", &nHr, &nMn)!=2 ){
                return 1;
            }
            zDate += 5;
            p->tz = sgn*(nMn + nHr*60);
zulu_time:
            while( isspace(*zDate) ){ zDate++; }
            p->tzSet = 1;
            return *zDate!=0;
        }

        /*
         ** Parse times of the form HH:MM or HH:MM:SS or HH:MM:SS.FFFF.
         ** The HH, MM, and SS must each be exactly 2 digits.  The
         ** fractional seconds FFFF can be one or more digits.
         **
         ** Return 1 if there is a parsing error and 0 on success.
         */
        inline static int parseHhMmSs(const char *zDate, DateTime *p){
            int h, m, s;
            double ms = 0.0;
            if( getDigits(zDate, "20c:20e", &h, &m)!=2 ){
                return 1;
            }
            zDate += 5;
            if( *zDate==':' ){
                zDate++;
                if( getDigits(zDate, "20e", &s)!=1 ){
                    return 1;
                }
                zDate += 2;
                if( *zDate=='.' && isdigit(zDate[1]) ){
                    double rScale = 1.0;
                    zDate++;
                    while( isdigit(*zDate) ){
                        ms = ms*10.0 + *zDate - '0';
                        rScale *= 10.0;
                        zDate++;
                    }
                    ms /= rScale;
                }
            }else{
                s = 0;
            }
            p->validJD = 0;
            p->rawS = 0;
            p->validHMS = 1;
            p->h = h;
            p->m = m;
            p->s = s + ms;
            if( parseTimezone(zDate, p) ) return 1;
            p->validTZ = (p->tz!=0)?1:0;
            return 0;
        }

        /*
         ** Put the DateTime object into its error state.
         */
        static void datetimeError(DateTime *p){
            memset(p, 0, sizeof(*p));
            p->isError = 1;
        }

        /*
         ** Convert from YYYY-MM-DD HH:MM:SS to julian day.  We always assume
         ** that the YYYY-MM-DD is according to the Gregorian calendar.
         **
         ** Reference:  Meeus page 61
         */
        static void computeJD(DateTime *p){
            int Y, M, D, A, B, X1, X2;

            if( p->validJD ) return;
            if( p->validYMD ){
                Y = p->Y;
                M = p->M;
                D = p->D;
            }else{
                Y = 2000;  /* If no YMD specified, assume 2000-Jan-01 */
                M = 1;
                D = 1;
            }
            if( Y<-4713 || Y>9999 || p->rawS ){
                datetimeError(p);
                return;
            }
            if( M<=2 ){
                Y--;
                M += 12;
            }
            A = Y/100;
            B = 2 - A + (A/4);
            X1 = 36525*(Y+4716)/100;
            X2 = 306001*(M+1)/10000;
            p->iJD = (sqlite3_int64)((X1 + X2 + D + B - 1524.5 ) * 86400000);
            p->validJD = 1;
            if( p->validHMS ){
                p->iJD += p->h*3600000 + p->m*60000 + (sqlite3_int64)(p->s*1000);
                if( p->validTZ ){
                    p->iJD -= p->tz*60000;
                    p->validYMD = 0;
                    p->validHMS = 0;
                    p->validTZ = 0;
                }
            }
        }

        /*
         ** Parse dates of the form
         **
         **     YYYY-MM-DD HH:MM:SS.FFF
         **     YYYY-MM-DD HH:MM:SS
         **     YYYY-MM-DD HH:MM
         **     YYYY-MM-DD
         **
         ** Write the result into the DateTime structure and return 0
         ** on success and 1 if the input string is not a well-formed
         ** date.
         */
        static int parseYyyyMmDd(const char *zDate, DateTime *p){
            int Y, M, D, neg;

            if( zDate[0]=='-' ){
                zDate++;
                neg = 1;
            }else{
                neg = 0;
            }
            if( getDigits(zDate, "40f-21a-21d", &Y, &M, &D)!=3 ){
                return 1;
            }
            zDate += 10;
            while( isspace(*zDate) || 'T'==*(u8*)zDate ){ zDate++; }
            if( parseHhMmSs(zDate, p)==0 ){
                /* We got the time */
            }else if( *zDate==0 ){
                p->validHMS = 0;
            }else{
                return 1;
            }
            p->validJD = 0;
            p->validYMD = 1;
            p->Y = neg ? -Y : Y;
            p->M = M;
            p->D = D;
            if( p->validTZ ){
                computeJD(p);
            }
            return 0;
        }


    }
}

#endif



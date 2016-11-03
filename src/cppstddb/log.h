#ifndef H_CPPSTDDB_LOG
#define H_CPPSTDDB_LOG

#include <ostream>
#include <fstream>
#include <mutex>
#include <sstream>
#include <iostream>
#include <atomic>

/*
   Just a very simplistic log facility with specific features for this library
 */

namespace cppstddb {

    std::string environment_variable(const std::string &name);

    enum class log_level {
        none,
        error,
        warn,
        info,
        debug,
        trace,
    };

    struct log_level_info {
        log_level level;
        const char *name;
        static const log_level_info info[];

        static const log_level_info& get(log_level level) {
            return info[static_cast<int>(level)];
        }

        static const log_level_info& get(const std::string& name) {
            auto i = &info[0];
            while ((i)->name && (i)->name != name) i++;
            if ((i)->name) return *i;
            throw std::runtime_error("log level name not found: " + name);
        }
    };

    const log_level_info log_level_info::info[] = {
        {log_level::none,"NONE"},
        {log_level::error,"ERROR"},
        {log_level::warn,"WARN"},
        {log_level::info,"INFO"},
        {log_level::debug,"DEBUG"},
        {log_level::trace,"TRACE"},
        {},
    };

    inline std::ostream& operator<<(std::ostream &os, log_level level) {
        os << log_level_info::get(level).name;
        return os;
    }

    class log_impl {
        public:
            using string = std::string;
            using ostream = std::ostream;
            using sstream = std::stringstream;
            using guard_t = std::lock_guard<std::mutex>;

            log_impl();
            ~log_impl();

            bool is_level_enabled(log_level l) const {return level_ >= l;}
            log_level level() const {return level_;}
            void level(const string& level);

            void write(log_level level, const char *s, unsigned int n);
            void write_table(ostream &os, string &key) const;
            void clear();

            struct log_msg {
                log_level level;
                const char *data;
                unsigned int size;
            };

        private:
            //std::ofstream os_;
            std::ostream& os_;
            bool enabled_;
            std::atomic<log_level> level_;
            bool on_;
            string eoln_;

            mutable std::mutex mutex_;

            void write_internal(log_level level, const string &s);
            void write_internal(const log_msg& msg);
    };


    inline std::string environment_variable(const std::string &name) {
        std::string result;
#if defined(_WIN32) || defined(_WIN64)
        char buf[MAX_PATH];
        DWORD v = GetEnvironmentVariableA(name.c_str(), buf, sizeof(buf));
        if (v != nullptr) result = buf;
        return result;

#else
        char *v = getenv(name.c_str());
        if (v != nullptr) result = v;
        return result;
#endif
    }

    inline log_impl::log_impl():
        os_(std::cerr),
        enabled_(true),
        level_(log_level::warn),
        on_(true),
        eoln_("\n")
    {
        auto l = environment_variable("CPPSTDDB_LOG_LEVEL");
        if (!l.empty()) level(l);
    }

    inline log_impl::~log_impl() {
        if (!on_) return;
        os_.flush();
    }

    inline log_impl& log() {
        static log_impl log_;
        return log_;
    }

    void log_impl::level(const string& level) {
        auto l = log_level_info::get(level).level;
        sstream s;
        s << "setting log level from " << level_ << " to " << l;
        guard_t guard(mutex_);
        write_internal(log_level::info, s.str());
        level_ = l;
    }

    inline void log_impl::write(log_level level, const char *s, unsigned int n) {
        log_msg msg;
        msg.level = level;
        msg.data = s;
        msg.size = n;
        guard_t guard(mutex_);
        write_internal(msg);
    }

    // ====== private
    // note locking is generally done by public functions

    inline void log_impl::write_internal(log_level level, const string& s) {
        log_msg msg;
        msg.level = level;
        msg.data = s.data();
        msg.size = s.size();
        write_internal(msg);
    }

    inline void log_impl::write_internal(const log_msg& msg) {
        using namespace std;
        if (!on_) return;
        os_ << log_level_info::get(msg.level).name << ":";
        os_.write(msg.data,msg.size);
        os_ << eoln_ << flush;
    }

    class log_streambuf : public std::streambuf {
        public:
            using string = std::string;
            using streamsize = std::streamsize;
            using int_type = std::streambuf::int_type;

            log_streambuf(log_level level):level_(level) {} 
            ~log_streambuf() {log().write(level_, buf_.data(), buf_.size());}

        protected:
            virtual int_type overflow(int_type c);
            virtual streamsize xsputn(const char *s, streamsize n);

        private:
            log_level level_;
            string buf_;
    };


    inline log_streambuf::int_type log_streambuf::overflow(int_type c) {
        buf_.append(reinterpret_cast<const char*>(&c),1);
        return c;
    }

    inline log_streambuf::streamsize log_streambuf::xsputn(const char *s, streamsize n) {
        buf_.append(s,n);
        return n;
    };

    class log_stream : public std::ostream {
        protected:
            log_streambuf buf_;
        public:
            log_stream(log_level level):std::ostream(0),buf_(level) {rdbuf(&buf_);}
            std::ostream& stream() {return *this;}
    };

}

#define DB_LOG(L,X) if(L<=cppstddb::log().level()) {cppstddb::log_stream(L).stream() << X;}
#define DB_ERROR(X) if(cppstddb::log().is_level_enabled(log_level::error)) {cppstddb::log_stream(log_level::error).stream() << X;}
#define DB_WARN(X) if(cppstddb::log().is_level_enabled(log_level::warn)) {cppstddb::log_stream(log_level::warn).stream() << X;}
#define DB_INFO(X) if(cppstddb::log().is_level_enabled(log_level::info)) {cppstddb::log_stream(log_level::info).stream() << X;}
#define DB_DEBUG(X) if(cppstddb::log().is_level_enabled(log_level::debug)) {cppstddb::log_stream(log_level::debug).stream() << X;}
#define DB_TRACE(X) if(cppstddb::log().is_level_enabled(log_level::trace)) {cppstddb::log_stream(log_level::trace).stream() << X;}

#endif


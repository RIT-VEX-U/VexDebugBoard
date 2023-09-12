#pragma once

#include <variant>
#include <functional>

namespace websocketer
{
    static const char *TAG = "websocketer";

    namespace detail
    {
        using Callback = std::function(const char *, const char *);
        void no_callback(const char *, const char *) {}

        struct LoggerType
        {

            void write(std::string tag, std::string s)
            {
                write(tag.c_str(), s.c_str());
            }
            void write(const char *tag, const char *str)
            {
                _callback(tag, str);
            }
            void set_callback(Callback c)
            {
                _callback = c;
            }


            Callback _callback = no_callback;
        };
    }
    using connection_hdl = uint32_t;

    struct error_code
    {
        enum value
        {
            OK = 0,
            NOT_OK = 1,

        };
        operator bool()
        {
            return val != OK;
        }
        value val;
    };

    struct OpCode
    {
        enum value
        {
            TEXT,
            BINARY
        };
    };

    class Server
    {
    public:
        using endpoint_type = uint32_t;
        using message_ptr = void *;

        // will abort if failure
        void init_asio(error_code &ec)
        {

            ESP_LOGE(TAG, "Don't know how to init a websocket server yet");
            ec.val = NOT_OK;
        }

        detail::LoggerType &get_alog()
        {
            return alog;
        }
        detail::LoggerType &get_elog()
        {
            return elog;
        }

    private:
        detail::LoggerType alog, elog;
    };

    namespace log
    {
        enum level
        {
            devel,
            library,
            info,
            warn,
            rerror,
            fatal,
            app,
        };
    }

    namespace ip
    {

        struct ipv4_address
        {
            char nums[4];
            std::string to_string() const
            {
                char *cs = NULL;
                asprintf(&cs, "%d.%d.%d.%d", nums[0], nums[1], nums[2], nums[3]);
                std::string s(cs);
                free(cs);
                return s;
            }
        };
        struct ipv6_address
        {
            std::string to_string() const
            {
                return "ipv6_address.to_string() not implemented";
            }
        };
        struct address
        {
        public:
            bool is_v6() const
            {
                return std::holds_alternative<ipv6_address>(addr);
            }
            std::string to_string() const
            {
                if (is_v6())
                {
                    return std::get<ipv6_address>(addr).to_string();
                }
                return std::get<ipv4_address>(addr).to_string();
            }

        private:
            std::variant<ipv4_address, ipv6_address> addr;
        };
    }
}
#pragma once

#include <variant>
#include <functional>
#include <vector>

namespace websocketer
{
    // signal that something has gone wrong with the websocket or foxglove implementation
    // do so without killing the entire processor
    inline void abort_ws()
    {
        // for now just kill entire processor
        abort();
    }
    inline const char *TAG = "websocketer";

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
            address(std::variant<ipv4_address, ipv6_address> addr) : addr(addr) {}
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

    namespace detail
    {
        using Callback = std::function<void(const char *, const char *)>;
        void no_callback(const char *, const char *) {}

        struct LoggerType
        {
            Callback _callback = no_callback;

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
        };
    }

    enum status
    {
        UnknownStatus,
        going_away,
    };

    struct error_code
    {
    public:
        enum value
        {
            OK = 0,
            NOT_OK = 1,

        };
        error_code(): val(OK){}
        error_code(value v) : val(v) {}

        std::string message()
        {
            if (val == value::OK)
            {
                return "error_code OK";
            }
            return "error_code not ok";
        }
        operator bool()
        {
            return val != OK;
        }
        value val = NOT_OK;
    };

    class OpCode
    {
    public:
        enum value
        {
            TEXT,
            BINARY
        };
        OpCode(value val) : val(val) {}
        bool operator==(const value v) const
        {
            return val == v;
        }

    private:
        value val;
    };

    class Socket
    {
    };
    class Message
    {
    public:
        void set_compressed(bool c)
        {
            compressed = c;
        }
        void append_payload(const uint8_t *op, uint32_t size)
        {
            ESP_LOGE(TAG, "Connection::append_payload not written");
            abort_ws();
        }
        OpCode get_opcode()
        {
            ESP_LOGE(TAG, "Connection::get_opcode not written");
            abort_ws();
            return op;
        }
        std::string get_payload()
        {
            ESP_LOGE(TAG, "Connection::get_payload not written");
            abort_ws();
            return data;
        }

        void set_payload(const uint8_t *data, uint32_t size)
        {
            ESP_LOGE(TAG, "Connection::set_payload not written");
            abort_ws();
        }

    private:
        OpCode op;
        std::string data; // ?
        bool compressed;
    };

    class Connection
    {
    public:
        Socket get_raw_socket()
        {
            ESP_LOGE(TAG, "Connection::get_raw_socket not written");
            abort_ws();
            return Socket{};
        }

        uint32_t get_buffered_amount()
        {
            return buf_amt;
        }
        void set_option(int opt, error_code &ec)
        {
            ESP_LOGE(TAG, "Connection::set_option not written");
            abort_ws();
        }
        const std::vector<std::string> &get_requested_subprotocols()
        {
            ESP_LOGE(TAG, "Connection::get_requested_subprotocols not written");
            abort_ws();
            return subprotocols;
        }
        void select_subprotocol(std::string subprot)
        {
            selected_subprotocol = subprot;
        }
        void send(const Message *data)
        {
            ESP_LOGE(TAG, "Connection::send(message) not written");
            abort_ws();
            // send((uint8_t *)(data.c_str()), data.size());
        }

        void send(const std::string &data)
        {
            ESP_LOGE(TAG, "Connection::send not written");
            abort_ws();
            send((const uint8_t *)(data.c_str()), data.size());
        }
        void send(const uint8_t *data, size_t data_size)
        {
            ESP_LOGE(TAG, "Connection::send(2 args) not written");
            abort_ws();
        }
        void close(status s, std::string msg, error_code &ec)
        {
            ESP_LOGE(TAG, "Connection::close not written");
            abort_ws();
        }
        std::string get_resource()
        {
            ESP_LOGE(TAG, "Connection::get_resource not written");
            abort_ws();

            return "IDK what youre talking about";
        }
        Message *get_message(OpCode op, uint32_t payload_size)
        {
            ESP_LOGE(TAG, "Connection::get_message not written");
            abort_ws();
            return nullptr;
        }
        void terminate(error_code &ec)
        {
            ESP_LOGE(TAG, "Connection::terminate not written");
            abort_ws();
        }
        std::string get_remote_endpoint()
        {
            ESP_LOGE(TAG, "Connection::get_remote_endpoint not written");
            abort_ws();
            return "";
        }

    private:
        const std::vector<std::string> subprotocols = {}; // needs to have "foxglove.websocket.v1"
        std::string selected_subprotocol = "";
        uint32_t buf_amt = 0;
    };

    using connection_hdl = std::shared_ptr<Connection>;

    class Endpoint
    {
    public:
        ip::address address()
        {
            return ip::address(addr);
        }
        uint32_t port()
        {
            return prt;
        }

    private:
        ip::ipv4_address addr;
        uint32_t prt;
    };

    class Server
    {
    public:
        using endpoint_type = Endpoint;
        using message_ptr = Message *;

        void run()
        {
            ESP_LOGE(TAG, "Server::run unimplimented");
            abort_ws();
        }

        void listen(std::string host, std::string port, error_code &ec)
        {
            ESP_LOGE(TAG, "Server::listen unimplimented");
            abort_ws();
        }
        void start_accept(error_code &ec)
        {
            ESP_LOGE(TAG, "Server::poll_one start_accept not implemented");
            abort_ws();
        }

        void poll_one()
        {
            ESP_LOGE(TAG, "Server::poll_one unimplimented");
            abort_ws();
        }

        // will abort if failure
        void init_asio(error_code &ec)
        {

            ESP_LOGE(TAG, "Don't know how to init a websocket server yet");
            ec.val = error_code::value::NOT_OK;
        }

        Endpoint get_local_endpoint(error_code &ec)
        {
            ESP_LOGE(TAG, "Don't know how to init a websocket server yet");
            ec.val = error_code::value::NOT_OK;
            return {};
        }
        std::shared_ptr<Connection> get_con_from_hdl(connection_hdl h)
        {
            ESP_LOGE(TAG, "Server::get_con_from_hdl(1arg) not written");
            return nullptr;
        }

        std::shared_ptr<Connection> get_con_from_hdl(connection_hdl h, error_code &ec)
        {
            ESP_LOGE(TAG, "Server::get_con_from_hdl(2arg) not written");
            return nullptr;
        }

        void send(connection_hdl hdl, const std::string payload, OpCode op)
        {
            auto conn = get_con_from_hdl(hdl);
            conn->send(payload);
            ESP_LOGI(TAG, "Server::send(3 args) not implemented");
            abort_ws();
        }

        void send(connection_hdl hdl, const uint8_t *payload, size_t payload_size, OpCode op)
        {
            auto conn = get_con_from_hdl(hdl);
            conn->send((const uint8_t *)payload, payload_size);
            ESP_LOGI(TAG, "Server::send(4 args) not implemented");
            abort_ws();
        }

        void stop_perpetual()
        {
            ESP_LOGI(TAG, "Server::stop_perpetual not implemented");
            abort_ws();
        }
        bool is_listening()
        {
            ESP_LOGI(TAG, "Server::is_listening not implemented");
            abort_ws();
            return false;
        }
        void stop_listening(error_code &ec)
        {
            ESP_LOGI(TAG, "Server::stop_listening not implemented");
            abort_ws();
        }

        detail::LoggerType &get_alog()
        {
            return alog;
        }
        detail::LoggerType &get_elog()
        {
            return elog;
        }
        bool stopped()
        {
            return is_stopped;
        }
        void stop()
        {
            ESP_LOGI(TAG, "Server::stop not implemented");
            is_stopped = true;
        }

    private:
        bool is_stopped = true;
        detail::LoggerType alog, elog;
    };

}
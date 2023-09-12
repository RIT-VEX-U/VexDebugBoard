#pragma once

#include <functional>

#include "common.hpp"
#include <websocketer.hpp>

namespace foxglove
{

  using LogCallback = std::function<void(char const *, char const *)>;

  inline std::string IPAddressToString(const websocketer::ip::address &addr)
  {
    if (addr.is_v6())
    {
      return "[" + addr.to_string() + "]";
    }
    return addr.to_string();
  }

  inline void NoOpLogCallback(char const *, char const *) {}

  class CallbackLogger
  {
  public:
    CallbackLogger()
        : _callback(NoOpLogCallback) {}



    void set_callback(LogCallback callback)
    {
      _callback = callback;
    }
    /*
        void set_channels(websocketpp::log::level channels)
        {
          if (channels == 0)
          {
            clear_channels(0xffffffff);
            return;
          }

          _dynamicChannels |= (channels & _staticChannels);
        }

        void clear_channels(websocketpp::log::level channels)
        {
          _dynamicChannels &= ~channels;
        }
        */

    void write(websocketer::log::level channel, std::string const &msg)
    {
      write(channel, msg.c_str());
    }

    void write(websocketer::log::level channel, char const *msg)
    {
      // if (!this->dynamic_test(channel))
      // {
      // return;
      // }

      // if (_channelTypeHint == channel_type_hint::access)
      // // {
      // // _callback(WebSocketLogLevel::Info, msg);
      // // }
      // // else
      // {

      if (channel == websocketer::log::level::devel)
      {
        _callback("Debug", msg);
      }
      else if (channel == websocketer::log::level::library)
      {
        _callback("Debug", msg);
      }
      else if (channel == websocketer::log::level::info)
      {
        _callback("Info", msg);
      }
      else if (channel == websocketer::log::level::warn)
      {
        _callback("Warn", msg);
      }
      else if (channel == websocketer::log::level::rerror)
      {
        _callback("Error", msg);
      }
      else if (channel == websocketer::log::level::fatal)
      {
        _callback("Critical", msg);
      }
      // }
    }

    // constexpr bool static_test(websocketpp::log::level channel) const
    // {
      // return ((channel & _staticChannels) != 0);
    // }

    // bool dynamic_test(websocketpp::log::level channel)
    // {
      // return ((channel & _dynamicChannels) != 0);
    // }

  private:
    // websocketpp::log::level const _staticChannels;
    // websocketpp::log::level _dynamicChannels;
    // channel_type_hint::value _channelTypeHint;
    LogCallback _callback;
  };

} // namespace foxglove

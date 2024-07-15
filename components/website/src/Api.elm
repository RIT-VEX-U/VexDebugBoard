module Api exposing (..)

import Common exposing (Configuration, HeartbeatResponse, SysInfo)
import Http
import Json.Decode as D exposing (Decoder)


getToIP : String -> { path : String, expect : Http.Expect msg } -> Cmd msg
getToIP host data =
    Http.get { url = host ++ data.path, expect = data.expect }


getWithTimeout : Float -> String -> { path : String, expect : Http.Expect msg } -> Cmd msg
getWithTimeout timout host data =
    Http.request
        { method = "GET"
        , headers = []
        , url = host ++ data.path
        , body = Http.emptyBody
        , timeout = Just timout
        , expect = data.expect
        , tracker = Nothing
        }


heartbeatRequest : String -> (Result Http.Error HeartbeatResponse -> msg) -> Cmd msg
heartbeatRequest host onget =
    getWithTimeout 1000 host { path = "/api/heartbeat", expect = Http.expectJson onget parseHeartbeatResponse }


configRequest : String -> (Result Http.Error Configuration -> msg) -> Cmd msg
configRequest host onget =
    Http.get { url = host ++ "/api/config", expect = Http.expectJson onget parseConfiguration }


sysInfoRequest : String -> (Result Http.Error SysInfo -> msg) -> Cmd msg
sysInfoRequest host onget =
    -- special, as of now we don't know the ip to get to, have to go to the default before we get the IP
    Http.get { url = host ++ "/api/sysinfo", expect = Http.expectJson onget parseSysinfoResponse }


parseSysinfoResponse : Decoder SysInfo
parseSysinfoResponse =
    D.map6 SysInfo (D.field "esp_version" D.string) (D.field "sw_version" D.string) (D.field "model" D.string) (D.field "cores" D.int) (D.field "ip" D.string) (D.field "bootcount" D.int)


parseHeartbeatResponse : Decoder HeartbeatResponse
parseHeartbeatResponse =
    D.map HeartbeatResponse (D.field "uptimems" D.int)


parseConfiguration : Decoder Configuration
parseConfiguration =
    D.map3 Configuration (D.field "use_mdns" D.bool) (D.field "mdns_hostname" D.string) (D.field "trial_run" D.bool)

module Api exposing (..)

import Http
import Json.Decode as D exposing (Decoder, decodeString)


type BoardStatus
    = StatusOkay HeartbeatResponse
    | StatusErrored Http.Error


getToIP : String -> { path : String, expect : Http.Expect msg } -> Cmd msg
getToIP host data =
    Http.get { url = "http://" ++ host ++ data.path, expect = data.expect }


getToIpWithTimeout : Float -> String -> { path : String, expect : Http.Expect msg } -> Cmd msg
getToIpWithTimeout timout host data =
    Http.request
        { method = "GET"
        , headers = []
        , url = "http://" ++ host ++ data.path
        , body = Http.emptyBody
        , timeout = Just timout
        , expect = data.expect
        , tracker = Nothing
        }


type alias HeartbeatResponse =
    { uptimems : Int }


heartbeatRequest : String -> (Result Http.Error HeartbeatResponse -> msg) -> Cmd msg
heartbeatRequest host onget =
    getToIpWithTimeout 1000 host { path = "/api/heartbeat", expect = Http.expectJson onget parseHeartbeatResponse }


type alias SysInfo =
    { esp_version : String, sw_version : String, model : String, cores : Int, ip : String, bootcount : Int }


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

module Api exposing (..)

import Http
import Json.Decode as D exposing (Decoder, decodeString)


default_host =
    "http://debug.local"


type BoardStatus
    = StatusOkay HeartbeatResponse
    | StatusNotYetGot
    | StatusErrored Http.Error


getToHost : String -> { path : String, expect : Http.Expect msg } -> Cmd msg
getToHost host data =
    Http.get { url = host ++ data.path, expect = data.expect }


type alias HeartbeatResponse =
    { uptimems : Int }


heartbeatRequest : (Result Http.Error HeartbeatResponse -> msg) -> Cmd msg
heartbeatRequest onget =
    Http.get { url = default_host ++ "/api/heartbeat", expect = Http.expectJson onget parseHeartbeatResponse }


type alias SysInfo =
    { version : String, model : String, cores : Int, ip : String }


sysInfoRequest : (Result Http.Error SysInfo -> msg) -> Cmd msg
sysInfoRequest onget =
    -- special, as of now we don't know the ip to get to, have to go to the default before we get the IP
    Http.get { url = default_host ++ "/api/sysinfo", expect = Http.expectJson onget parseSysinfoResponse }


parseSysinfoResponse : Decoder SysInfo
parseSysinfoResponse =
    D.map4 SysInfo (D.field "version" D.string) (D.field "model" D.string) (D.field "cores" D.int) (D.field "ip" D.string)


parseHeartbeatResponse : Decoder HeartbeatResponse
parseHeartbeatResponse =
    D.map HeartbeatResponse (D.field "uptimems" D.int)

module Api exposing (..)

import Http
import Json.Decode as D exposing (Decoder, decodeString)
import Time


host =
    "http://debug.local"


type BoardStatus
    = StatusOkay HeartbeatResponse
    | StatusNotYetGot
    | StatusErrored Http.Error


heartbeatRequest : (Result Http.Error HeartbeatResponse -> msg) -> Cmd msg
heartbeatRequest onget =
    Http.get { url = host ++ "/api/heartbeat", expect = Http.expectJson onget parseHeartbeatResponse }


type alias HeartbeatResponse =
    { uptimems : Int }


parseHeartbeatResponse : Decoder HeartbeatResponse
parseHeartbeatResponse =
    D.map HeartbeatResponse (D.field "uptimems" D.int)

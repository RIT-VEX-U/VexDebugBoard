module Common exposing (..)

import Http
import Time


type alias StationConfig =
    { ssid : String, password : String }


type alias APConfig =
    {}


type alias WifiConfig =
    { use_ap : Bool
    , station_cfg : StationConfig
    , ap_cfg : APConfig
    }


type alias Configuration =
    { use_mdns : Bool, mdns_hostname : String, trial_run : Bool }


type Page
    = Dashboard
    | Config
    | Logs
    | Model3D
    | FileExplorer


type Msg
    = Goto Page
    | EditConfig Configuration
    | HeatbeatTick Time.Posix
    | HeartbeatReceived (Result Http.Error HeartbeatResponse)
    | ConfigReceived (Result Http.Error Configuration)


type alias Model =
    { page : Page
    , configs : Maybe ConfigPair
    , board_status : BoardStatus
    , sysinfo : SysInfo
    }


type alias SysInfo =
    { esp_version : String, sw_version : String, model : String, cores : Int, ip : String, bootcount : Int }


type alias HeartbeatResponse =
    { uptimems : Int }


type BoardStatus
    = StatusOkay HeartbeatResponse
    | StatusErrored Http.Error


type alias ConfigPair =
    { current : Configuration
    , initial : Configuration
    }

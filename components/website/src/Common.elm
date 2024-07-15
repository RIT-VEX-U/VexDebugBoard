module Common exposing (..)

import Api
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
    { use_mdns : Bool, mdns_hostname : String, trial_wifi : WifiConfig, last_known_good_wifi : WifiConfig }


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
    | HeartbeatReceived (Result Http.Error Api.HeartbeatResponse)


type alias Model =
    { page : Page
    , configs : Maybe ConfigPair
    , board_status : Api.BoardStatus
    , sysinfo : Api.SysInfo
    }


type alias ConfigPair =
    { current : Configuration
    , initial : Configuration
    }

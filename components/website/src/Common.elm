module Common exposing (..)

import Api
import Http
import Time


type alias WifiSetup =
    { mode : WifiMode, hostname : String }


type WifiMode
    = AP
    | STA


type alias Configuration =
    { wifi : WifiSetup, numToSave : Int }


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

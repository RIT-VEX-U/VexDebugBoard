module Main exposing (main)

import Api
import Browser exposing (Document)
import Configuration exposing (Configuration, viewWifi)
import Element exposing (Element, alignRight, centerX, centerY, column, el, fill, fillPortion, height, px, rgb255, row, shrink, text, width)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Element.Region as Region
import Html exposing (Html, button, div)
import Html.Events exposing (onClick)
import Http
import Time
import UiUtil exposing (pallete)


type alias Model =
    { page : Page
    , version : SWVersion
    , config : Configuration
    , initial_config : Configuration
    , board_status : Api.BoardStatus
    , sysinfo : Api.SysInfo
    }


type MModel
    = NotConnected String
    | Connected Model


def_cfg : { wifi : { mode : Configuration.WifiMode, hostname : String }, numToSave : number }
def_cfg =
    { wifi = { mode = Configuration.AP, hostname = "debug" }, numToSave = 1 }


initialModel : Api.SysInfo -> Model
initialModel info =
    { page = Dashboard
    , version = { major = 0, minor = 0, patch = 1, comment = Just "alpha" }
    , config = def_cfg
    , initial_config = def_cfg
    , board_status = Api.StatusOkay { uptimems = 0 }
    , sysinfo = info
    }


type Page
    = Dashboard
    | Config
    | Logs
    | Model3D
    | FileExplorer


type alias SWVersion =
    { major : Int, minor : Int, patch : Int, comment : Maybe String }


statusTextAndImage : String -> Element msg -> Maybe (Element Never) -> Element msg
statusTextAndImage status img tooltip =
    let
        tt =
            tooltip |> Maybe.map (\t -> [ UiUtil.tooltip Element.onLeft t ])

        attrs =
            List.append [ alignRight, Element.spacing 8 ] (tt |> Maybe.withDefault [])
    in
    row attrs [ el [ Font.size 18, centerY, Font.color pallete.darkgray ] (text status), img ]


viewGoodStatus : Api.HeartbeatResponse -> Element msg
viewGoodStatus status =
    let
        popup =
            text <| "Uptime: " ++ String.fromInt status.uptimems ++ " ms"
    in
    statusTextAndImage "Connected" UiUtil.connectedIcon (Just <| el [ Font.size 15, Background.color pallete.selectedPage, Border.color pallete.black, Border.rounded 4, Element.padding 5, Element.spacing 10 ] popup)



-- text ("Uptime: " ++ String.fromInt status.uptimems ++ "ms")


viewBadStatus : a -> Element msg
viewBadStatus e =
    statusTextAndImage "Disconnected" UiUtil.errorIcon Nothing


viewStatus : Api.BoardStatus -> Element msg
viewStatus status =
    case status of
        Api.StatusOkay s ->
            viewGoodStatus s

        Api.StatusErrored e ->
            viewBadStatus e


type Msg
    = Goto Page
    | UpdateConfig Configuration
    | HeatbeatTick Time.Posix
    | HeartbeatReceived (Result Http.Error Api.HeartbeatResponse)


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        Goto p ->
            ( { model | page = p }, Cmd.none )

        UpdateConfig cfg ->
            ( { model | config = cfg }, Cmd.none )

        HeatbeatTick _ ->
            ( model, Api.heartbeatRequest model.sysinfo.ip HeartbeatReceived )

        HeartbeatReceived res ->
            let
                newstatus =
                    case res of
                        Err e ->
                            Api.StatusErrored e

                        Ok hr ->
                            Api.StatusOkay hr
            in
            ( { model | board_status = newstatus }, Cmd.none )


h1size : number
h1size =
    30


pageTitle : String -> Element msg
pageTitle title =
    el [ Element.centerX, Font.size h1size, Element.paddingXY 0 20, Font.underline ] (Element.text title)


saveButton : Bool -> Bool -> Element msg
saveButton needsSave needsRestart =
    let
        text =
            if needsRestart then
                "Save and Restart"

            else
                "Save"
    in
    if needsSave then
        Input.button
            [ Background.color pallete.nonselectedPage, Element.padding 5 ]
            { label = Element.text text, onPress = Nothing }

    else
        Element.none


viewConfig : Configuration -> Configuration -> Element Msg
viewConfig config initial_config =
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Configuration"
        , viewWifi config initial_config |> Element.map (\wifi -> { config | wifi = wifi } |> UpdateConfig)
        , saveButton (config /= initial_config) False
        ]


viewSysInfo : Api.SysInfo -> Element msg
viewSysInfo info =
    column [ Element.spacingXY 0 10, Element.paddingXY 10 10 ]
        [ text ("IP Address: " ++ info.ip)
        , text ("Software version: " ++ info.sw_version)
        , text ("ESP version: " ++ info.esp_version)
        , text ("Chip Model: " ++ info.model)
        ]


viewDashboard : Model -> Element msg
viewDashboard mod =
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Dashboard"
        , text "System Info:" |> el [ Font.bold, Font.size 30 ]
        , viewSysInfo mod.sysinfo
        ]


menu_items : List ( String, Page )
menu_items =
    [ ( "Dashboard", Dashboard ), ( "Configuration", Config ), ( "Logs", Logs ), ( "3D Model", Model3D ), ( "File System", FileExplorer ) ]


viewPage : Model -> Element Msg
viewPage mod =
    case mod.page of
        Dashboard ->
            viewDashboard mod

        Config ->
            viewConfig mod.config mod.initial_config

        _ ->
            Element.el [ Element.centerX, Element.centerY ] (Element.text "Coming Soon")


headerButton : Page -> ( String, Page ) -> Element.Element Msg
headerButton current ( name, page_togo ) =
    Input.button
        [ Border.roundEach { bottomLeft = 0, bottomRight = 0, topLeft = UiUtil.headerTabRadius, topRight = UiUtil.headerTabRadius }
        , Element.paddingXY 6 4
        , Background.color <|
            if page_togo == current then
                pallete.selectedPage

            else
                pallete.nonselectedPage
        , Font.size 18
        , height fill
        , Element.alignBottom
        ]
        { label = Element.text name, onPress = Just (Goto page_togo) }


header : Model -> Element Msg
header mod =
    let
        tabs =
            List.map (headerButton mod.page) menu_items
    in
    row
        [ height (Element.shrink |> Element.minimum 45), Element.scrollbarX, width fill, Region.navigation, Element.spacing 4, Element.paddingEach { left = 5, right = 5, top = 5, bottom = 0 } ]
    <|
        List.append tabs [ viewStatus mod.board_status |> el [ alignRight ] ]


content : Model -> Element Msg
content mod =
    el [ width fill, height fill, Region.mainContent, Background.color pallete.selectedPage ] (viewPage mod)


page : Model -> Element Msg
page mod =
    column [ width fill, height fill, Background.color pallete.background, Font.color pallete.font ] [ header mod, content mod ]


focusStyle : { borderColor : Maybe a, backgroundColor : Maybe b, shadow : Maybe c }
focusStyle =
    { borderColor = Nothing
    , backgroundColor = Nothing
    , shadow = Nothing
    }


layout_options : { options : List Element.Option }
layout_options =
    { options = [ Element.focusStyle focusStyle ] }


view : Model -> Element Msg
view model =
    page model


default_host : String
default_host =
    -- controls where our requests go before we get an IP. when using elm reactor and hosting ui locally, this should be the mdns name or the ip of the board
    -- when "in production" (served from the esp32) leave this blank and it will request back to the board whereever it is
    "http://debug.local"


main : Program () MModel MMsg
main =
    Browser.document
        { init = \_ -> ( NotConnected "Connecting...", Api.sysInfoRequest default_host SysinfoReceived )
        , view = mview
        , update = mupdate
        , subscriptions = \_ -> Time.every 5000 (\t -> HeatbeatTick t |> AppMsg)
        }


mupdate : MMsg -> MModel -> ( MModel, Cmd MMsg )
mupdate rinfo mmodel =
    case rinfo of
        SysinfoReceived res ->
            case res of
                Err e ->
                    ( NotConnected "An unknown error connecting to the board occured. Reload?", Cmd.none )

                Ok info ->
                    ( Connected (initialModel info), Api.heartbeatRequest info.ip (\r -> HeartbeatReceived r |> AppMsg) )

        AppMsg msg ->
            case mmodel of
                NotConnected s ->
                    ( NotConnected ("Something bad happened: " ++ s), Api.sysInfoRequest default_host SysinfoReceived )

                Connected model ->
                    update msg model
                        |> Tuple.mapSecond (\cmd -> Cmd.map AppMsg cmd)
                        |> Tuple.mapFirst (\mod -> Connected mod)


mview : MModel -> Document MMsg
mview mmod =
    (case mmod of
        NotConnected s ->
            text s |> el [ Font.color pallete.font, centerX, centerY, width fill, height fill ]

        Connected mod ->
            view mod
    )
        |> (\el -> Element.layoutWith layout_options [ Font.family [ Font.monospace ], Background.color pallete.background ] (el |> Element.map AppMsg))
        |> (\html -> { title = "Vex Debug Board", body = [ html ] })


type MMsg
    = SysinfoReceived (Result Http.Error Api.SysInfo)
    | AppMsg Msg

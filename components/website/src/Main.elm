module Main exposing (main)

import Api
import Browser exposing (Document)
import Configuration exposing (Configuration, viewWifi)
import Element exposing (Element, alignRight, centerY, column, el, fill, fillPortion, height, px, rgb255, row, shrink, text, width)
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
    , sysinfo : Maybe Api.SysInfo
    }


def_cfg : { wifi : { mode : Configuration.WifiMode, hostname : String }, numToSave : number }
def_cfg =
    { wifi = { mode = Configuration.AP, hostname = "debug" }, numToSave = 1 }


initialModel : Model
initialModel =
    { page = Dashboard
    , version = { major = 0, minor = 0, patch = 1, comment = Just "alpha" }
    , config = def_cfg
    , initial_config = def_cfg
    , board_status = Api.StatusNotYetGot
    , sysinfo = Nothing
    }


type Page
    = Dashboard
    | Config
    | Logs
    | Model3D
    | FileExplorer


type alias SWVersion =
    { major : Int, minor : Int, patch : Int, comment : Maybe String }


viewSWVersion : SWVersion -> Element msg
viewSWVersion version =
    let
        nums =
            [ version.major, version.minor, version.patch ] |> List.map String.fromInt |> String.join "."

        extra =
            version.comment |> Maybe.map (\c -> "-" ++ c) |> Maybe.withDefault ""
    in
    Element.text (nums ++ extra)


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
        Api.StatusNotYetGot ->
            statusTextAndImage "Connecting..." UiUtil.loadingIcon Nothing

        Api.StatusOkay s ->
            viewGoodStatus s

        Api.StatusErrored e ->
            viewBadStatus e


type Msg
    = Goto Page
    | UpdateConfig Configuration
    | HeatbeatTick Time.Posix
    | HeartbeatReceived (Result Http.Error Api.HeartbeatResponse)
    | SysinfoReceived (Result Http.Error Api.SysInfo)


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        Goto p ->
            ( { model | page = p }, Cmd.none )

        UpdateConfig cfg ->
            ( { model | config = cfg }, Cmd.none )

        HeatbeatTick _ ->
            ( model, Api.heartbeatRequest HeartbeatReceived )

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

        SysinfoReceived res ->
            case res of
                Err _ ->
                    Debug.todo "Failed sysinfo"

                Ok info ->
                    ( { model | sysinfo = Just info }, Cmd.none )


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


viewSysInfo : Maybe Api.SysInfo -> Element msg
viewSysInfo maybe =
    case maybe of
        Nothing ->
            text "No System info yet!"

        Just info ->
            column []
                [ text ("IP Address: " ++ info.ip)
                , text ("Chip Model: " ++ info.model)
                ]


viewDashboard : Model -> Element msg
viewDashboard mod =
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Dashboard"
        , viewSWVersion mod.version
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


options : { options : List Element.Option }
options =
    { options = [ Element.focusStyle focusStyle ] }


view : Model -> Document Msg
view model =
    let
        body =
            Element.layoutWith options [ Font.family [ Font.monospace ] ] (page model)
    in
    { title = "Vex Debug Board", body = [ body ] }


main : Program () Model Msg
main =
    Browser.document
        { init = \_ -> ( initialModel, Api.sysInfoRequest SysinfoReceived )
        , view = view
        , update = update
        , subscriptions = \_ -> Time.every 5000 HeatbeatTick
        }

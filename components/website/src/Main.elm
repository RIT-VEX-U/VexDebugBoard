module Main exposing (main)

import Api
import Browser exposing (Document)
import Common exposing (Model, Msg(..), Page(..))
import Configuration
import Element exposing (Element, alignRight, centerX, centerY, column, el, fill, fillPortion, height, px, rgb255, row, shrink, spacing, text, width)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Element.Region as Region
import Http
import Pages.Dashboard as Dashboard
import Time
import UiUtil exposing (pageTitle, pallete)


type MModel
    = NotConnected (List String)
    | Connected Model


initialModel : Common.SysInfo -> Model
initialModel info =
    { page = Dashboard
    , configs = Nothing
    , board_status = Common.StatusOkay { uptimems = 0 }
    , sysinfo = info
    }


statusTextAndImage : String -> Element msg -> Maybe (Element Never) -> Element msg
statusTextAndImage status img tooltip =
    let
        tt =
            tooltip |> Maybe.map (\t -> [ UiUtil.tooltip Element.onLeft t ])

        attrs =
            List.append [ alignRight, Element.spacing 8 ] (tt |> Maybe.withDefault [])
    in
    row attrs [ el [ Font.size 18, centerY, Font.color pallete.darkgray ] (text status), img ]


viewGoodStatus : Common.HeartbeatResponse -> Element msg
viewGoodStatus status =
    let
        popup =
            text <| "Uptime: " ++ String.fromInt status.uptimems ++ " ms"
    in
    statusTextAndImage "Connected" UiUtil.connectedIcon (Just <| el [ Font.size 15, Background.color pallete.selectedPage, Border.color pallete.black, Border.rounded 4, Element.padding 5, Element.spacing 10 ] popup)



-- text ("Uptime: " ++ String.fromInt status.uptimems ++ "ms")


viewBadStatus : Http.Error -> Element msg
viewBadStatus e =
    let
        errtext =
            case e of
                Http.BadUrl _ ->
                    "Bad URL"

                Http.Timeout ->
                    "Timeout"

                Http.NetworkError ->
                    "NetworkError"

                Http.BadStatus _ ->
                    "BadStatus "

                Http.BadBody _ ->
                    "BadBody"
    in
    statusTextAndImage "Disconnected" UiUtil.errorIcon (Just <| text errtext)


viewStatus : Common.BoardStatus -> Element msg
viewStatus status =
    case status of
        Common.StatusOkay s ->
            viewGoodStatus s

        Common.StatusErrored e ->
            viewBadStatus e


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        Goto p ->
            ( { model | page = p }, Cmd.none )

        EditConfig cfg ->
            ( { model
                | configs = model.configs |> Maybe.map (\pair -> Configuration.updateCurrent pair cfg)
              }
            , Cmd.none
            )

        HeatbeatTick _ ->
            ( model, Api.heartbeatRequest default_host HeartbeatReceived )

        HeartbeatReceived res ->
            let
                newstatus =
                    case res of
                        Err e ->
                            Common.StatusErrored e

                        Ok hr ->
                            Common.StatusOkay hr
            in
            ( { model | board_status = newstatus }, Cmd.none )

        ConfigReceived res ->
            case res of
                Err e ->
                    Debug.todo "bad config response"

                Ok cfg ->
                    ( { model | configs = Just { initial = cfg, current = cfg } }, Cmd.none )


menu_items : List ( String, Page )
menu_items =
    [ ( "Dashboard", Dashboard ), ( "Configuration", Config ), ( "Logs", Logs ), ( "3D Model", Model3D ), ( "File System", FileExplorer ) ]


viewPage : Model -> Element Msg
viewPage mod =
    case mod.page of
        Dashboard ->
            Dashboard.view mod

        Config ->
            Configuration.view mod.configs

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
        { init = \_ -> ( NotConnected [ "Connecting..." ], Api.sysInfoRequest default_host SysinfoReceived )
        , view = mview
        , update = mupdate
        , subscriptions = subscriptions
        }


subscriptions : MModel -> Sub MMsg
subscriptions model =
    case model of
        NotConnected _ ->
            Sub.none

        Connected _ ->
            Time.every 3000 (\t -> HeatbeatTick t |> AppMsg)


describeHttpError : Http.Error -> String
describeHttpError e =
    case e of
        Http.NetworkError ->
            "Network Error. Is the board on? Are you connected to the same Wifi Network?"

        Http.BadUrl str ->
            "Bad URL: " ++ str

        Http.Timeout ->
            "Timed out: Is the board on? Are you connected to the same Wifi Network?"

        Http.BadStatus code ->
            "Received a bad status: " ++ String.fromInt code

        Http.BadBody str ->
            "Bad Body: " ++ str


mupdate : MMsg -> MModel -> ( MModel, Cmd MMsg )
mupdate rinfo mmodel =
    case rinfo of
        SysinfoReceived res ->
            case res of
                Err e ->
                    ( NotConnected
                        [ "An error occured while connecting to the board"
                        , describeHttpError e
                        , "When you've made sure, reload this page."
                        ]
                    , Cmd.none
                    )

                Ok info ->
                    ( Connected (initialModel info)
                    , Cmd.batch
                        [ Api.heartbeatRequest default_host (\r -> HeartbeatReceived r |> AppMsg)
                        , Api.configRequest default_host (\r -> ConfigReceived r |> AppMsg)
                        ]
                    )

        AppMsg msg ->
            case mmodel of
                NotConnected log ->
                    ( NotConnected (List.append log [ Debug.toString msg ]), Api.sysInfoRequest default_host SysinfoReceived )

                Connected model ->
                    update msg model
                        |> Tuple.mapSecond (\cmd -> Cmd.map AppMsg cmd)
                        |> Tuple.mapFirst (\mod -> Connected mod)


mview : MModel -> Document MMsg
mview mmod =
    (case mmod of
        NotConnected s ->
            List.map text s |> column [ spacing 5 ] |> el [ Font.color pallete.font, centerX, centerY, width fill, height fill ]

        Connected mod ->
            view mod
    )
        |> (\el -> Element.layoutWith layout_options [ Font.family [ Font.monospace ], Background.color pallete.background ] (el |> Element.map AppMsg))
        |> (\html -> { title = "Vex Debug Board", body = [ html ] })


type MMsg
    = SysinfoReceived (Result Http.Error Common.SysInfo)
    | AppMsg Msg

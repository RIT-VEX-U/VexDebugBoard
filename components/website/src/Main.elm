module Main exposing (main)

import Browser exposing (Document)
import Configuration exposing (Configuration, viewWifi)
import Element exposing (Element, column, el, fill, fillPortion, height, px, rgb255, row, shrink, text, width)
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
    , board_uptime : Maybe Int
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
    , board_uptime = Nothing
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


type Msg
    = Goto Page
    | UpdateConfig Configuration
    | HeatbeatTick Time.Posix
    | HeartbeatReceived (Result Http.Error ())


heartbeatRequest : Cmd Msg
heartbeatRequest =
    Http.get { url = "/api/heartbeat", expect = Http.expectWhatever HeartbeatReceived }


update : Msg -> Model -> ( Model, Cmd Msg )
update msg model =
    case msg of
        Goto p ->
            ( { model | page = p }, Cmd.none )

        UpdateConfig cfg ->
            ( { model | config = cfg }, Cmd.none )

        HeatbeatTick _ ->
            ( model, heartbeatRequest )

        HeartbeatReceived res ->
            ( model, Cmd.none )


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



{-
   Input.radio []
               { label = modifiedLabel "Wifi Mode" True |> Input.labelLeft []
               , onChange = \wm -> { cfg | wifi = wm }
               , options = []
               , selected = Nothing
               }
           ,
-}


viewConfig : Configuration -> Configuration -> Element Msg
viewConfig config initial_config =
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Configuration"
        , viewWifi config initial_config |> Element.map (\wifi -> { config | wifi = wifi } |> UpdateConfig)
        , saveButton (config /= initial_config) False
        ]


viewDashboard : { a | version : SWVersion } -> Element msg
viewDashboard mod =
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Dashboard"
        , viewSWVersion mod.version
        ]


menu_items : List ( String, Page )
menu_items =
    [ ( "Dashboard", Dashboard ), ( "Configuration", Config ), ( "Logs", Logs ), ( "3D Model", Model3D ), ( "File System", FileExplorer ) ]


viewPage : { a | page : Page, version : SWVersion, config : Configuration, initial_config : Configuration } -> Element Msg
viewPage mod =
    case mod.page of
        Dashboard ->
            viewDashboard mod

        Config ->
            viewConfig mod.config mod.initial_config

        _ ->
            Element.el [ Element.centerX, Element.centerY ] (Element.text "Coming Soon")


headerRadius : number
headerRadius =
    8


headerButton : Page -> ( String, Page ) -> Element.Element Msg
headerButton current ( name, page_togo ) =
    Input.button
        [ Border.roundEach { bottomLeft = 0, bottomRight = 0, topLeft = headerRadius, topRight = headerRadius }
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


header : { a | page : Page } -> Element Msg
header mod =
    row [ height (Element.shrink |> Element.minimum 45), Element.scrollbarX, width fill, Region.navigation, Element.spacing 4, Element.paddingEach { left = 5, right = 5, top = 5, bottom = 0 } ] <| List.map (headerButton mod.page) menu_items


content : { a | page : Page, version : SWVersion, config : Configuration, initial_config : Configuration } -> Element Msg
content mod =
    el [ width fill, height fill, Region.mainContent, Background.color pallete.selectedPage ] (viewPage mod)


page : { a | page : Page, version : SWVersion, config : Configuration, initial_config : Configuration } -> Element Msg
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
        { init = \_ -> ( initialModel, Cmd.none )
        , view = view
        , update = update
        , subscriptions = \_ -> Time.every 5000 HeatbeatTick
        }

module Configuration exposing (..)

import Common exposing (ConfigPair, Configuration, Msg(..))
import Element exposing (Element, centerX, centerY, column, el, fill, height, padding, paddingXY, px, row, text, width)
import Element.Background as Background
import Element.Border as Border
import Element.Font as Font
import Element.Input as Input
import Pages.Dashboard exposing (view)
import UiUtil exposing (pageTitle, pallete, textModifiedLabel, tooltip)

view : Maybe ConfigPair -> Element Msg
view mcp =
    case mcp of
        Nothing ->
            text "Config not yet loaded from device..." |> el [ Element.paddingXY 0 20, width fill, Font.center ]

        Just cfgs ->
            viewActual cfgs


inFrontRight : Element msg -> Element.Attribute msg
inFrontRight e =
    el [ Element.inFront e ] Element.none |> Element.onRight


labelWithQuestionTooltip : String -> Element Never -> Input.Label msg
labelWithQuestionTooltip label ttext =
    Input.labelLeft
        [ Element.paddingEach { left = 0, right = 20, top = 0, bottom = 0 } ]
    <|
        row [ Element.spacing 5 ]
            [ text label
            , text "(?)"
                |> el
                    [ Font.color pallete.darkgray
                    , tooltip inFrontRight
                        (ttext
                            |> el
                                [ Background.color pallete.black
                                , Element.padding 10
                                , Border.rounded 10
                                , Font.size 15
                                , Font.color pallete.font
                                ]
                        )
                    , centerY
                    ]
            ]


useMdnsLabel : Input.Label msg
useMdnsLabel =
    labelWithQuestionTooltip "Use mDNS" <|
        column [] [ text "Use Multicast DNS to give the Vex Debug Board a helpful name.", text "Without this, you will need to know the IP address of the board", text "(may not work on some computers)" ]


viewActual : ConfigPair -> Element Msg
viewActual cfgs =
    let
        initial_config =
            cfgs.initial

        config =
            cfgs.current
    in
    column [ width fill, height fill, Element.paddingXY 10 10, Element.spacingXY 0 20 ]
        [ pageTitle "Configuration"
        , Input.checkbox []
            { onChange = \s -> Common.EditConfig { config | use_mdns = s }
            , icon = Input.defaultCheckbox
            , checked = config.use_mdns
            , label = useMdnsLabel
            }
        , if config.use_mdns then
            hostnameEdit config

          else
            Element.none

        -- , viewWifi cfgs.current cfgs.initial |> Element.map (\wifi -> { config | wifi = wifi } |> EditConfig)
        , saveButton (isConfigValid cfgs)
        ]


hostnameEdit : Configuration -> Element Msg
hostnameEdit config =
    column []
        [ Input.text [ Font.color pallete.black ]
            { label = labelWithQuestionTooltip "Hostname" (column [] [ text "Where to go to see this on the next reboot" ])
            , text = config.mdns_hostname
            , placeholder = Nothing
            , onChange = \t -> { config | mdns_hostname = t } |> EditConfig
            }
        , if not <| String.endsWith ".local" config.mdns_hostname then
            text "Hostname must end with `.local`" |> el [ Font.color pallete.red, Element.padding 5 ]

          else
            Element.none
        ]


isConfigValid : ConfigPair -> { valid : Bool, needs_save : Bool }
isConfigValid { current, initial } =
    let
        needs_save =
            current /= initial

        mdns_valid =
            not current.use_mdns || (current.use_mdns && String.endsWith ".local" current.mdns_hostname)
    in
    { valid = mdns_valid, needs_save = needs_save }


saveButton : { valid : Bool, needs_save : Bool } -> Element msg
saveButton { valid, needs_save } =
    let
        button_style more_styles =
            Input.button <|
                List.append
                    [ Background.color pallete.nonselectedPage
                    , Element.padding 5
                    , centerX
                    , Element.alignBottom
                    , padding 15
                    , Font.size 25
                    , Border.rounded 10
                    ]
                    more_styles
    in
    if needs_save then
        if valid then
            button_style [] { label = Element.text "Save", onPress = Nothing }

        else
            button_style [ Font.color pallete.red ] { label = Element.text "Fix Errors Then Save", onPress = Nothing }

    else
        Element.none


updateCurrent : ConfigPair -> Configuration -> ConfigPair
updateCurrent pair new =
    { pair | current = new }



-- authType : WifiMode, ssid : String , password: String
-- https://man7.org/linux/man-pages/man7/hostname.7.html


isValidHostname : String -> Bool
isValidHostname hn =
    let
        badStart =
            String.startsWith "-" hn

        goodChars =
            String.toList hn |> List.map (\char -> Char.isAlphaNum char || char == '-')

        anyBadChars =
            List.any (\good -> not good) goodChars
    in
    badStart || anyBadChars |> not



-- viewWifi : Configuration -> Configuration -> Element WifiSetup
-- viewWifi cfg initial =
--     let
--         wifi =
--             cfg.wifi
--         iwifi =
--             initial.wifi
--     in
--     row []
--         [ textModifiedLabel "Hostname: " (wifi.hostname /= iwifi.hostname)
--         , Input.text [ Font.color pallete.black, Font.alignRight ]
--             { label = ".local" |> text |> Input.labelRight []
--             , onChange =
--                 \newname ->
--                     { wifi
--                         | hostname =
--                             if isValidHostname newname then
--                                 newname
--                             else
--                                 wifi.hostname
--                     }
--             , placeholder = Nothing
--             , text = wifi.hostname
--             }
--         ]

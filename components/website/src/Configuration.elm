module Configuration exposing (..)

import Common exposing (ConfigPair, Configuration, Msg(..))
import Element exposing (Element, centerX, centerY, column, el, fill, height, px, row, text, width)
import Element.Background as Background
import Element.Border
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
        [ tooltip inFrontRight
            (ttext
                |> el
                    [ Background.color pallete.black
                    , Element.padding 10
                    , Element.Border.rounded 10
                    , Font.size 15
                    ]
            )
        ]
    <|
        row [ Element.spacing 5 ] [ text label, text "(?)" ]


useMdnsLabel =
    labelWithQuestionTooltip "Use mDNS" <|
        column [] [ text "Use Multicast DNS to give the Vex Debug Board a helpful name.", text "Without this, you will need to know the IP address of the board", text "(may not work on some computers)" ]


viewActual : ConfigPair -> Element Msg
viewActual cfgs =
    let
        -- initial_config =
        --     cfgs.initial
        config =
            cfgs.current

        a =
            3
    in
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Configuration"
        , Input.checkbox []
            { onChange = \s -> Common.EditConfig { config | use_mdns = s }
            , icon = Input.defaultCheckbox
            , checked = config.use_mdns
            , label = useMdnsLabel
            }

        -- , viewWifi cfgs.current cfgs.initial |> Element.map (\wifi -> { config | wifi = wifi } |> EditConfig)
        -- , saveButton (config /= initial_config) False
        ]


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

module Configuration exposing (..)

import Element exposing (Element, centerX, centerY, height, px, row, text, width)
import Element.Font as Font
import Element.Input as Input
import Html exposing (col)
import UiUtil exposing (pallete, textModifiedLabel)


type WifiMode
    = AP
    | STA



-- authType : WifiMode, ssid : String , password: String


type alias WifiSetup =
    { mode : WifiMode, hostname : String }



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


type alias Configuration =
    { wifi : WifiSetup, numToSave : Int }


viewWifi : Configuration -> Configuration -> Element WifiSetup
viewWifi cfg initial =
    let
        wifi =
            cfg.wifi

        iwifi =
            initial.wifi
    in
    row []
        [ textModifiedLabel "Hostname: " (wifi.hostname /= iwifi.hostname)
        , Input.text [ Font.color pallete.black ]
            { label = ".local" |> text |> Input.labelRight []
            , onChange =
                \newname ->
                    { wifi
                        | hostname =
                            if isValidHostname newname then
                                newname

                            else
                                wifi.hostname
                    }
            , placeholder = Nothing
            , text = wifi.hostname
            }
        ]

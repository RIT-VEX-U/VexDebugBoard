module Pages.Dashboard exposing (..)

import Common exposing (Model)
import Element exposing (Element, column, el, fill, height, text, width)
import Element.Font as Font
import UiUtil exposing (pageTitle)


view : Model -> Element msg
view mod =
    column [ width fill, height fill, Element.paddingXY 10 10 ]
        [ pageTitle "Dashboard"
        , text "System Info:" |> el [ Font.bold, Font.size 30 ]
        , viewSysInfo mod.sysinfo
        ]


viewSysInfo : Common.SysInfo -> Element msg
viewSysInfo info =
    column [ Element.spacingXY 0 10, Element.paddingXY 10 10 ]
        [ text ("IP Address: " ++ info.ip)
        , text ("Boot count: " ++ String.fromInt info.bootcount)
        , text " "
        , text ("Software version: " ++ info.sw_version)
        , text ("ESP version: " ++ info.esp_version)
        , text ("Chip Model: " ++ info.model)
        ]
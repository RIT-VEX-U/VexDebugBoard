module UiUtil exposing (..)

import Element exposing (Attribute, Element, centerY, column, el, fill, height, htmlAttribute, inFront, mouseOver, rgb255, row, shrink, text, transparent, width)
import Element.Font as Font
import Html
import Html.Attributes


br : Element msg
br =
    Element.html <| Html.br [] []


h1size : number
h1size =
    30


pageTitle : String -> Element msg
pageTitle title =
    el [ Element.centerX, Font.size h1size, Element.paddingXY 0 20, Font.underline, Font.bold ] (Element.text title)


tooltip : (Element msg -> Attribute msg) -> Element Never -> Attribute msg
tooltip usher tooltip_ =
    inFront <|
        el
            [ width fill
            , height fill
            , transparent True
            , mouseOver [ transparent False ]
            , (usher << Element.map never) <|
                el [ htmlAttribute (Html.Attributes.style "pointerEvents" "none") ]
                    tooltip_
            ]
            Element.none


connectedIconB64 : String
connectedIconB64 =
    -- Source: https://glyphs.fyi/dir?q=check&i=checkCircle&v=bold&w
    "PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+CjxzdmcKICAgd2lkdGg9IjI1IgogICBoZWlnaHQ9IjI1IgogICB2aWV3Qm94PSIwIDAgMjUgMjUiCiAgIGZpbGw9Im5vbmUiCiAgIHZlcnNpb249IjEuMSIKICAgaWQ9InN2ZzEiCiAgIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIKICAgeG1sbnM6c3ZnPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+CiAgPGRlZnMKICAgICBpZD0iZGVmczEiIC8+CiAgPHBhdGgKICAgICBmaWxsLXJ1bGU9ImV2ZW5vZGQiCiAgICAgY2xpcC1ydWxlPSJldmVub2RkIgogICAgIGQ9Ik0gMTIuNSwyNSBDIDE5LjQwMzU3MiwyNSAyNSwxOS40MDM1NzIgMjUsMTIuNSAyNSw1LjU5NjQyOCAxOS40MDM1NzIsMCAxMi41LDAgNS41OTY0MjgsMCAwLDUuNTk2NDI4IDAsMTIuNSAwLDE5LjQwMzU3MiA1LjU5NjQyOCwyNSAxMi41LDI1IFogTSAyMC4wMzU5MzcsOS4wOTU0MDEgYyAwLjUyMDUzNywtMC41MjU0OSAwLjUyMDUzNywtMS4zNzc0NTUgMCwtMS45MDI5NDUgLTAuNTIwNTM2LC0wLjUyNTQ5MiAtMS4zNjQ1MDksLTAuNTI1NDkyIC0xLjg4NTA4OSwwIGwgLTcuODQ4NjYxLDcuOTIzMDggLTMuNDUzMDgsLTMuNDg1ODA0IGMgLTAuNTIwNTM1LC0wLjUyNTQ5MSAtMS4zNjQ1MDgsLTAuNTI1NDkxIC0xLjg4NTA0NCwwIC0wLjUyMDU4MSwwLjUyNTQ5MSAtMC41MjA1ODEsMS4zNzc0NTYgMCwxLjkwMjk0NiBsIDMuNzY3MjMxLDMuODAyOTQ3IGMgMC44Njc1OSwwLjg3NTg0OSAyLjI3NDE5NywwLjg3NTg0OSAzLjE0MTc4NywwIHoiCiAgICAgZmlsbD0iI0MyQ0NERSIKICAgICBpZD0icGF0aDEiCiAgICAgc3R5bGU9ImZpbGw6Izg0YmQwMDtmaWxsLW9wYWNpdHk6MTtzdHJva2Utd2lkdGg6MC40NDY0MjgiIC8+Cjwvc3ZnPgo="


errorIconB64 : String
errorIconB64 =
    -- Source: https://glyphs.fyi/dir?c=system&i=exclamationCircle&v=bold&w
    "PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+CjxzdmcKICAgd2lkdGg9IjI1IgogICBoZWlnaHQ9IjI1IgogICB2aWV3Qm94PSIwIDAgMjUgMjUiCiAgIGZpbGw9Im5vbmUiCiAgIHZlcnNpb249IjEuMSIKICAgaWQ9InN2ZzEiCiAgIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIKICAgeG1sbnM6c3ZnPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+CiAgPGRlZnMKICAgICBpZD0iZGVmczEiIC8+CiAgPHBhdGgKICAgICBmaWxsLXJ1bGU9ImV2ZW5vZGQiCiAgICAgY2xpcC1ydWxlPSJldmVub2RkIgogICAgIGQ9Ik0gMjUsMTIuNSBDIDI1LDE5LjQwMzU3MSAxOS40MDM1NzEsMjUgMTIuNSwyNSA1LjU5NjQyOSwyNSAwLDE5LjQwMzU3MSAwLDEyLjUgMCw1LjU5NjQyOSA1LjU5NjQyOSwwIDEyLjUsMCAxOS40MDM1NzEsMCAyNSw1LjU5NjQyOSAyNSwxMi41IFogTSAxMi40ODU2MjUsNC4yNDEwNzEgYyAwLjYxNjM4NCwwIDEuMTE2MDcxLDAuNDk5Njg4IDEuMTE2MDcxLDEuMTE2MDcyIHYgMTAgYyAwLDAuNjE2Mzg0IC0wLjQ5OTY4NywxLjExNjA3MSAtMS4xMTYwNzEsMS4xMTYwNzEgLTAuNjE2Mzg0LDAgLTEuMTE2MDcxLC0wLjQ5OTY4NyAtMS4xMTYwNzEsLTEuMTE2MDcxIHYgLTEwIGMgMCwtMC42MTYzODQgMC40OTk2ODcsLTEuMTE2MDcyIDEuMTE2MDcxLC0xLjExNjA3MiB6IG0gMS4xMTYwNzEsMTQuMzMwMzEzIGMgMCwtMC42MTYzODQgLTAuNDk5Njg3LC0xLjExNjA3MSAtMS4xMTYwNzEsLTEuMTE2MDcxIC0wLjYxNjM4NCwwIC0xLjExNjA3MSwwLjQ5OTY4NyAtMS4xMTYwNzEsMS4xMTYwNzEgdiAwLjcxNDI4NiBjIDAsMC42MTYzODQgMC40OTk2ODcsMS4xMTYwNzEgMS4xMTYwNzEsMS4xMTYwNzEgMC42MTYzODQsMCAxLjExNjA3MSwtMC40OTk2ODcgMS4xMTYwNzEsLTEuMTE2MDcxIHoiCiAgICAgZmlsbD0iI0MyQ0NERSIKICAgICBpZD0icGF0aDEiCiAgICAgc3R5bGU9ImZpbGw6I2RhMjkxYztmaWxsLW9wYWNpdHk6MTtzdHJva2Utd2lkdGg6MC40NDY0MjkiIC8+Cjwvc3ZnPgo="


loadingIconB64 : String
loadingIconB64 =
    -- Source: https://glyphs.fyi/dir?q=hou&i=hourglass100&v=bold&w
    "PD94bWwgdmVyc2lvbj0iMS4wIiBlbmNvZGluZz0iVVRGLTgiIHN0YW5kYWxvbmU9Im5vIj8+CjxzdmcKICAgd2lkdGg9IjI1IgogICBoZWlnaHQ9IjI1IgogICB2aWV3Qm94PSIwIDAgMjUgMjUiCiAgIGZpbGw9Im5vbmUiCiAgIHZlcnNpb249IjEuMSIKICAgaWQ9InN2ZzEiCiAgIHhtbG5zPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyIKICAgeG1sbnM6c3ZnPSJodHRwOi8vd3d3LnczLm9yZy8yMDAwL3N2ZyI+CiAgPGRlZnMKICAgICBpZD0iZGVmczEiIC8+CiAgPHBhdGgKICAgICBmaWxsLXJ1bGU9ImV2ZW5vZGQiCiAgICAgY2xpcC1ydWxlPSJldmVub2RkIgogICAgIGQ9Ik0gMS4yOTMxMDMsMCBDIDAuNTc4OTIyLDAgMCwwLjU0MTU3MjYgMCwxLjIwOTY3NyAwLDEuODc3NzgyIDAuNTc4OTIyLDIuNDE5MzU1IDEuMjkzMTAzLDIuNDE5MzU1IGggMi45OTQ1MjYgdiAzLjg0Nzk4NCBjIDAsMS45MDYxMjkgMC45MDEwNzgsMy43MTQ3MTcgMi40NjE0MjMsNC45NDA1NjQgbCAxLjMxNTc3NiwxLjAzMzY3IGMgMC4xMTQ4MjcsMC4wOTAyIDAuMjMyMDI1LDAuMTc2MzcxIDAuMzUxMzc5LDAuMjU4NTA4IC0wLjExOTM1NCwwLjA4MjE0IC0wLjIzNjU1MiwwLjE2ODMwNiAtMC4zNTEzNzksMC4yNTg1NDggbCAtMS4zMTU3NzYsMS4wMzM2NjkgYyAtMS41NjAzNDUsMS4yMjU4MDcgLTIuNDYxNDIzLDMuMDM0Mzk2IC0yLjQ2MTQyMyw0Ljk0MDU2NSB2IDMuODQ3NzgyIEggMS4yOTMxMDMgQyAwLjU3ODkyMiwyMi41ODA2NDUgMCwyMy4xMjIyMTggMCwyMy43OTAzMjMgMCwyNC40NTg0MjcgMC41Nzg5MjIsMjUgMS4yOTMxMDMsMjUgSCAyMy43MDY4OTcgQyAyNC40MjEwNzgsMjUgMjUsMjQuNDU4NDI3IDI1LDIzLjc5MDMyMyAyNSwyMy4xMjIyMTggMjQuNDIxMDc4LDIyLjU4MDY0NSAyMy43MDY4OTcsMjIuNTgwNjQ1IGggLTIuOTk0NjU2IHYgLTMuODQ3NzgyIGMgMCwtMS45MDYxNjkgLTAuOTAxMDM0LC0zLjcxNDc1OCAtMi40NjEzNzksLTQuOTQwNTY1IGwgLTEuMzE1Nzc2LC0xLjAzMzY2OSBjIC0wLjExNDgyNywtMC4wOTAyNCAtMC4yMzIwMjYsLTAuMTc2NDExIC0wLjM1MTM3OSwtMC4yNTg1NDggMC4xMTkzNTMsLTAuMDgyMTQgMC4yMzY1NTIsLTAuMTY4MzA3IDAuMzUxMzc5LC0wLjI1ODU0OSBsIDEuMzE1Nzc2LC0xLjAzMzYyOSBDIDE5LjgxMTIwNyw5Ljk4MjA1NiAyMC43MTIyNDEsOC4xNzM0NjggMjAuNzEyMjQxLDYuMjY3MzM5IFYgMi40MTkzNTUgaCAyLjk5NDY1NiBDIDI0LjQyMTA3OCwyLjQxOTM1NSAyNSwxLjg3Nzc4MiAyNSwxLjIwOTY3NyAyNSwwLjU0MTU3MjYgMjQuNDIxMDc4LDAgMjMuNzA2ODk3LDAgWiBtIDE2Ljk1NTQ3NSwyMC4yNDE5MzUgYyAwLDAuODkwNzY3IC0wLjc3MTg5NywxLjYxMjkwNCAtMS43MjQxMzgsMS42MTI5MDQgSCA4LjQ3NTQ3NCBjIC0wLjk1MjE5OCwwIC0xLjcyNDEzOCwtMC43MjIxMzcgLTEuNzI0MTM4LC0xLjYxMjkwNCB2IC0xLjE2ODk5MSBjIDAsLTEuNDIxNjk0IDAuNzE1NjQ3LC0yLjc2MDQwNCAxLjkzMTU1MiwtMy42MTMyNjcgbCAwLjkyMDk5MSwtMC42NDYwMDggYyAxLjcxNjI5MywtMS4yMDM4NzEgNC4wNzU4NjIsLTEuMjAzODcxIDUuNzkyMTU1LDAgbCAwLjkyMTAzNSwwLjY0NjAwOCBjIDEuMjE1OTA1LDAuODUyODYzIDEuOTMxNTA5LDIuMTkxNTczIDEuOTMxNTA5LDMuNjEzMjY3IHoiCiAgICAgZmlsbD0iI0MyQ0NERSIKICAgICBpZD0icGF0aDEiCiAgICAgc3R5bGU9ImZpbGw6I2Y2YmUwMDtmaWxsLW9wYWNpdHk6MTtzdHJva2Utd2lkdGg6MC40MTY4OTgiIC8+Cjwvc3ZnPgo="


makeB64SVG : String -> String
makeB64SVG b64 =
    "data:image/svg+xml;base64," ++ b64


connectedIcon : Element msg
connectedIcon =
    Element.image [ height shrink, width shrink, centerY ] { src = makeB64SVG connectedIconB64, description = "Connected" }


errorIcon : Element msg
errorIcon =
    Element.image [ height shrink, width shrink, centerY ] { src = makeB64SVG errorIconB64, description = "Error" }


loadingIcon : Element msg
loadingIcon =
    Element.image [ height shrink, width shrink, centerY ] { src = makeB64SVG loadingIconB64, description = "Loading" }


headerTabRadius : number
headerTabRadius =
    8


pallete =
    { background = rgb255 0 0 0
    , selectedPage = rgb255 41 41 41
    , nonselectedPage = rgb255 21 21 21
    , orange = rgb255 247 105 2
    , font = rgb255 255 255 255
    , black = rgb255 0 0 0
    , darkgray = rgb255 80 80 80
    }


colrows : List ( Element msg, Element msg ) -> Element msg
colrows l =
    let
        ( left, right ) =
            l |> List.unzip
    in
    row [ height fill, width fill ]
        [ column [ width fill, height fill ] left
        , column [ width fill, height fill ] right
        ]


textModifiedLabel : String -> Bool -> Element msg
textModifiedLabel str modified =
    let
        extra =
            if modified then
                "*"

            else
                " "
    in
    extra ++ str |> text

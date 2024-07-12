module General exposing (..)

import Element exposing (Element, column, fill, height, rgb255, row, text, width)


pallete : { background : Element.Color, selectedPage : Element.Color, nonselectedPage : Element.Color, orange : Element.Color, font : Element.Color, black : Element.Color }
pallete =
    { background = rgb255 0 0 0
    , selectedPage = rgb255 41 41 41
    , nonselectedPage = rgb255 21 21 21
    , orange = rgb255 247 105 2
    , font = rgb255 255 255 255
    , black = rgb255 0 0 0
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

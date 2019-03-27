/*
 * Command line Icecream status monitor
 * Copyright (C) 2019 The Qt Company Ltd. or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <initializer_list>
#include <tuple>

constexpr std::initializer_list<std::tuple<char, char, char>> all_colors = {
    std::tuple<char, char, char>{240, 248, 255}, // aliceblue
    std::tuple<char, char, char>{250, 235, 215}, // antiquewhite
    std::tuple<char, char, char>{0, 255, 255}, // aqua
    std::tuple<char, char, char>{127, 255, 212}, // aquamarine
    std::tuple<char, char, char>{240, 255, 255}, // azure
    std::tuple<char, char, char>{245, 245, 220}, // beige
    std::tuple<char, char, char>{255, 228, 196}, // bisque
    std::tuple<char, char, char>{255, 235, 205}, // blanchedalmond
    std::tuple<char, char, char>{0, 0, 255}, // blue
    std::tuple<char, char, char>{138, 43, 226}, // blueviolet
    std::tuple<char, char, char>{165, 42, 42}, // brown
    std::tuple<char, char, char>{222, 184, 135}, // burlywood
    std::tuple<char, char, char>{95, 158, 160}, // cadetblue
    std::tuple<char, char, char>{127, 255, 0}, // chartreuse
    std::tuple<char, char, char>{210, 105, 30}, // chocolate
    std::tuple<char, char, char>{255, 127, 80}, // coral
    std::tuple<char, char, char>{100, 149, 237}, // cornflowerblue
    std::tuple<char, char, char>{255, 248, 220}, // cornsilk
    std::tuple<char, char, char>{220, 20, 60}, // crimson
    std::tuple<char, char, char>{0, 255, 255}, // cyan
    std::tuple<char, char, char>{0, 0, 139}, // darkblue
    std::tuple<char, char, char>{0, 139, 139}, // darkcyan
    std::tuple<char, char, char>{184, 134, 11}, // darkgoldenrod
    std::tuple<char, char, char>{169, 169, 169}, // darkgray
    std::tuple<char, char, char>{0, 100, 0}, // darkgreen
    std::tuple<char, char, char>{169, 169, 169}, // darkgrey
    std::tuple<char, char, char>{189, 183, 107}, // darkkhaki
    std::tuple<char, char, char>{139, 0, 139}, // darkmagenta
    std::tuple<char, char, char>{85, 107, 47}, // darkolivegreen
    std::tuple<char, char, char>{255, 140, 0}, // darkorange
    std::tuple<char, char, char>{153, 50, 204}, // darkorchid
    std::tuple<char, char, char>{139, 0, 0}, // darkred
    std::tuple<char, char, char>{233, 150, 122}, // darksalmon
    std::tuple<char, char, char>{143, 188, 143}, // darkseagreen
    std::tuple<char, char, char>{72, 61, 139}, // darkslateblue
    std::tuple<char, char, char>{47, 79, 79}, // darkslategray
    std::tuple<char, char, char>{47, 79, 79}, // darkslategrey
    std::tuple<char, char, char>{0, 206, 209}, // darkturquoise
    std::tuple<char, char, char>{148, 0, 211}, // darkviolet
    std::tuple<char, char, char>{255, 20, 147}, // deeppink
    std::tuple<char, char, char>{0, 191, 255}, // deepskyblue
    std::tuple<char, char, char>{105, 105, 105}, // dimgray
    std::tuple<char, char, char>{105, 105, 105}, // dimgrey
    std::tuple<char, char, char>{30, 144, 255}, // dodgerblue
    std::tuple<char, char, char>{178, 34, 34}, // firebrick
    std::tuple<char, char, char>{255, 250, 240}, // floralwhite
    std::tuple<char, char, char>{34, 139, 34}, // forestgreen
    std::tuple<char, char, char>{255, 0, 255}, // fuchsia
    std::tuple<char, char, char>{220, 220, 220}, // gainsboro
    std::tuple<char, char, char>{248, 248, 255}, // ghostwhite
    std::tuple<char, char, char>{255, 215, 0}, // gold
    std::tuple<char, char, char>{218, 165, 32}, // goldenrod
    std::tuple<char, char, char>{128, 128, 128}, // gray
    std::tuple<char, char, char>{0, 128, 0}, // green
    std::tuple<char, char, char>{173, 255, 47}, // greenyellow
    std::tuple<char, char, char>{128, 128, 128}, // grey
    std::tuple<char, char, char>{240, 255, 240}, // honeydew
    std::tuple<char, char, char>{255, 105, 180}, // hotpink
    std::tuple<char, char, char>{205, 92, 92}, // indianred
    std::tuple<char, char, char>{75, 0, 130}, // indigo
    std::tuple<char, char, char>{255, 255, 240}, // ivory
    std::tuple<char, char, char>{240, 230, 140}, // khaki
    std::tuple<char, char, char>{230, 230, 250}, // lavender
    std::tuple<char, char, char>{255, 240, 245}, // lavenderblush
    std::tuple<char, char, char>{124, 252, 0}, // lawngreen
    std::tuple<char, char, char>{255, 250, 205}, // lemonchiffon
    std::tuple<char, char, char>{173, 216, 230}, // lightblue
    std::tuple<char, char, char>{240, 128, 128}, // lightcoral
    std::tuple<char, char, char>{224, 255, 255}, // lightcyan
    std::tuple<char, char, char>{250, 250, 210}, // lightgoldenrodyellow
    std::tuple<char, char, char>{211, 211, 211}, // lightgray
    std::tuple<char, char, char>{144, 238, 144}, // lightgreen
    std::tuple<char, char, char>{211, 211, 211}, // lightgrey
    std::tuple<char, char, char>{255, 182, 193}, // lightpink
    std::tuple<char, char, char>{255, 160, 122}, // lightsalmon
    std::tuple<char, char, char>{32, 178, 170}, // lightseagreen
    std::tuple<char, char, char>{135, 206, 250}, // lightskyblue
    std::tuple<char, char, char>{119, 136, 153}, // lightslategray
    std::tuple<char, char, char>{119, 136, 153}, // lightslategrey
    std::tuple<char, char, char>{176, 196, 222}, // lightsteelblue
    std::tuple<char, char, char>{255, 255, 224}, // lightyellow
    std::tuple<char, char, char>{0, 255, 0}, // lime
    std::tuple<char, char, char>{50, 205, 50}, // limegreen
    std::tuple<char, char, char>{250, 240, 230}, // linen
    std::tuple<char, char, char>{255, 0, 255}, // magenta
    std::tuple<char, char, char>{128, 0, 0}, // maroon
    std::tuple<char, char, char>{102, 205, 170}, // mediumaquamarine
    std::tuple<char, char, char>{0, 0, 205}, // mediumblue
    std::tuple<char, char, char>{186, 85, 211}, // mediumorchid
    std::tuple<char, char, char>{147, 112, 219}, // mediumpurple
    std::tuple<char, char, char>{60, 179, 113}, // mediumseagreen
    std::tuple<char, char, char>{123, 104, 238}, // mediumslateblue
    std::tuple<char, char, char>{0, 250, 154}, // mediumspringgreen
    std::tuple<char, char, char>{72, 209, 204}, // mediumturquoise
    std::tuple<char, char, char>{199, 21, 133}, // mediumvioletred
    std::tuple<char, char, char>{25, 25, 112}, // midnightblue
    std::tuple<char, char, char>{245, 255, 250}, // mintcream
    std::tuple<char, char, char>{255, 228, 225}, // mistyrose
    std::tuple<char, char, char>{255, 228, 181}, // moccasin
    std::tuple<char, char, char>{255, 222, 173}, // navajowhite
    std::tuple<char, char, char>{0, 0, 128}, // navy
    std::tuple<char, char, char>{253, 245, 230}, // oldlace
    std::tuple<char, char, char>{128, 128, 0}, // olive
    std::tuple<char, char, char>{107, 142, 35}, // olivedrab
    std::tuple<char, char, char>{255, 165, 0}, // orange
    std::tuple<char, char, char>{255, 69, 0}, // orangered
    std::tuple<char, char, char>{218, 112, 214}, // orchid
    std::tuple<char, char, char>{238, 232, 170}, // palegoldenrod
    std::tuple<char, char, char>{152, 251, 152}, // palegreen
    std::tuple<char, char, char>{175, 238, 238}, // paleturquoise
    std::tuple<char, char, char>{219, 112, 147}, // palevioletred
    std::tuple<char, char, char>{255, 239, 213}, // papayawhip
    std::tuple<char, char, char>{255, 218, 185}, // peachpuff
    std::tuple<char, char, char>{205, 133, 63}, // peru
    std::tuple<char, char, char>{255, 192, 203}, // pink
    std::tuple<char, char, char>{221, 160, 221}, // plum
    std::tuple<char, char, char>{176, 224, 230}, // powderblue
    std::tuple<char, char, char>{128, 0, 128}, // purple
    std::tuple<char, char, char>{255, 0, 0}, // red
    std::tuple<char, char, char>{188, 143, 143}, // rosybrown
    std::tuple<char, char, char>{65, 105, 225}, // royalblue
    std::tuple<char, char, char>{139, 69, 19}, // saddlebrown
    std::tuple<char, char, char>{250, 128, 114}, // salmon
    std::tuple<char, char, char>{244, 164, 96}, // sandybrown
    std::tuple<char, char, char>{46, 139, 87}, // seagreen
    std::tuple<char, char, char>{255, 245, 238}, // seashell
    std::tuple<char, char, char>{160, 82, 45}, // sienna
    std::tuple<char, char, char>{192, 192, 192}, // silver
    std::tuple<char, char, char>{135, 206, 235}, // skyblue
    std::tuple<char, char, char>{106, 90, 205}, // slateblue
    std::tuple<char, char, char>{112, 128, 144}, // slategray
    std::tuple<char, char, char>{112, 128, 144}, // slategrey
    std::tuple<char, char, char>{255, 250, 250}, // snow
    std::tuple<char, char, char>{0, 255, 127}, // springgreen
    std::tuple<char, char, char>{70, 130, 180}, // steelblue
    std::tuple<char, char, char>{210, 180, 140}, // tan
    std::tuple<char, char, char>{0, 128, 128}, // teal
    std::tuple<char, char, char>{216, 191, 216}, // thistle
    std::tuple<char, char, char>{255, 99, 71}, // tomato
    std::tuple<char, char, char>{64, 224, 208}, // turquoise
    std::tuple<char, char, char>{238, 130, 238}, // violet
    std::tuple<char, char, char>{245, 222, 179}, // wheat
    std::tuple<char, char, char>{255, 255, 255}, // white
    std::tuple<char, char, char>{245, 245, 245}, // whitesmoke
    std::tuple<char, char, char>{255, 255, 0}, // yellow
    std::tuple<char, char, char>{154, 205, 50}, // yellowgreen
};

Storage 
{
	{ Format 1.33 }
	{ GeneratedFrom TGD-version-2.20 }
	{ WrittenBy orion }
	{ WrittenOn "" }
}

Document 
{
	{ Type "Generic Diagram" }
	{ Name split-horizon-1.gd }
	{ Author orion }
	{ CreatedOn "" }
	{ Annotation "" }
	{ Hierarchy False }
}

Page 
{
	{ PageOrientation Portrait }
	{ PageSize A4 }
	{ ShowHeaders False }
	{ ShowFooters False }
	{ ShowNumbers False }
}

Scale 
{
	{ ScaleValue 1 }
}

# GRAPH NODES

GenericNode 1
{
	{ Name "A" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

GenericNode 2
{
	{ Name "C" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

GenericNode 3
{
	{ Name "B" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

GenericNode 16
{
	{ Name "D" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 24
{
	{ Name "1" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 25
{
	{ Name "1" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 26
{
	{ Name "1" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 27
{
	{ Name "1" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

# GRAPH EDGES

GenericEdge 4
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 1 }
	{ Subject2 2 }
}

GenericEdge 5
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 2 }
	{ Subject2 3 }
}

GenericEdge 18
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 16 }
	{ Subject2 2 }
}

GenericEdge 19
{
	{ Name "" }
	{ Annotation "" }
	{ Parent 0 }
	{ Subject1 1 }
	{ Subject2 3 }
}

# VIEWS AND GRAPHICAL SHAPES

View 6
{
	{ Index "0" }
	{ Parent 0 }
}

Circle 7
{
	{ View 6 }
	{ Subject 1 }
	{ Position 60 120 }
	{ Size 60 60 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--24*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Circle 8
{
	{ View 6 }
	{ Subject 2 }
	{ Position 210 180 }
	{ Size 60 60 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--24*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Circle 9
{
	{ View 6 }
	{ Subject 3 }
	{ Position 60 240 }
	{ Size 60 60 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--24*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 10
{
	{ View 6 }
	{ Subject 4 }
	{ FromShape 7 }
	{ ToShape 8 }
	{ Curved False }
	{ End1 Empty }
	{ End2 Empty }
	{ Points 2 }
	{ Point 88 131 }
	{ Point 182 169 }
	{ NamePosition 140 141 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--10*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 11
{
	{ View 6 }
	{ Subject 5 }
	{ FromShape 8 }
	{ ToShape 9 }
	{ Curved False }
	{ End1 Empty }
	{ End2 Empty }
	{ Points 2 }
	{ Point 182 191 }
	{ Point 88 229 }
	{ NamePosition 130 201 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--10*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Circle 20
{
	{ View 6 }
	{ Subject 16 }
	{ Position 390 180 }
	{ Size 60 60 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--24*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 21
{
	{ View 6 }
	{ Subject 18 }
	{ FromShape 20 }
	{ ToShape 8 }
	{ Curved False }
	{ End1 Empty }
	{ End2 Empty }
	{ Points 2 }
	{ Point 360 180 }
	{ Point 240 180 }
	{ NamePosition 300 170 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--10*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

Line 22
{
	{ View 6 }
	{ Subject 19 }
	{ FromShape 7 }
	{ ToShape 9 }
	{ Curved False }
	{ End1 Empty }
	{ End2 Empty }
	{ Points 2 }
	{ Point 60 150 }
	{ Point 60 210 }
	{ NamePosition 46 180 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--10*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

TextBox 28
{
	{ View 6 }
	{ Subject 24 }
	{ Position 300 160 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--14*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

TextBox 29
{
	{ View 6 }
	{ Subject 25 }
	{ Position 140 140 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--14*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

TextBox 30
{
	{ View 6 }
	{ Subject 26 }
	{ Position 140 230 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--14*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

TextBox 31
{
	{ View 6 }
	{ Subject 27 }
	{ Position 30 180 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--14*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}


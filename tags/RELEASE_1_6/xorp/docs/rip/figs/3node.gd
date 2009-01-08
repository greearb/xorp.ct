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
	{ Name 3node.gd }
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
	{ Name "B" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

GenericNode 3
{
	{ Name "C" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 12
{
	{ Name "1" }
	{ Annotation "" }
	{ Parent 0 }
	{ Index "" }
}

Comment 13
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
	{ Position 40 180 }
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
	{ Point 70 180 }
	{ Point 180 180 }
	{ NamePosition 125 170 }
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
	{ Point 240 180 }
	{ Point 360 180 }
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

TextBox 14
{
	{ View 6 }
	{ Subject 12 }
	{ Position 120 160 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--10*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}

TextBox 15
{
	{ View 6 }
	{ Subject 13 }
	{ Position 300 160 }
	{ Size 20 20 }
	{ Color "black" }
	{ LineWidth 1 }
	{ LineStyle Solid }
	{ FillStyle Unfilled }
	{ FillColor "white" }
	{ FixedName False }
	{ Font "-*-helvetica-medium-r-normal--10*" }
	{ TextAlignment Center }
	{ TextColor "black" }
	{ NameUnderlined False }
}


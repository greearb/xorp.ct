%
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% 
% This LaTeX class is an extension to the slides class which 
% allows much nicer headers and footers which are customized
% to the International Computer Science Institute.
% 
% This is an adaptation from a LaTeX class from the
% University of Rhode Island.
% See following URL for examples:
% http://www.ele.uri.edu/Info/latex/slides/
% 
%
% All options from the class slides are allowed as well as:
%   bw: make the header bar, footer bar, and logo black and
%     white, instead of blue and gold.
%
%   portrait: landscape is the default orientation, this chooses
%     specifies portrait.
%
%   nofoot: no footer will be at the bottom, and the default
%     text of the slide will be one inch longer.  The page
%     numbers will be in the upper right.
%
% The additional functions available in this class are:
%   \slidetitle{title}: this sets the title of the current slide,
%     and must occur somewhere before the \end{slide}.
%
%   \tinycaption{caption}: this prints the caption in the lower
%     lefthand side of the slide.  It will affect all following
%     slides.
%
% Functions to change the header and footer (these all must
% appear before the \begin{document}):
%   \sitename{Site Name}: this sets the text in the lowerbar.
%     To have no name use \sitename{}.
%   
%   \sitenamecolor{color}: this specifies the color to use for
%     the site name in the lower bar.
%
%   \barcolor{color}: this is the bar color.
%
%   \newlogo{logo}[width]: this specifies the logo to use in 
%     the upper lefthand corner of the slide.  If the optional
%     width is not specified, it will default to 0.75in.  To have
%     no logo, use \newlogo{}.
%
%
% Author: Tim Toolan, Department of Electrical and Computer Engineering,
%                     University of Rhode Island.
%

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% basic class header stuff

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{icsislides}[2002/05/01 v1.0 Extension of slides class]

% black and white headers and footers
\DeclareOption{bw}{\def\@wantbwslides{true}}

% don't want a footer to get more space
\DeclareOption{nofoot}{\def\@wantnofoot{true}}

% landscape is default, need to specify portrait
\DeclareOption{portrait}{\def\@wantportrait{true}}

% pass all the rest of the options to the real slides class
\DeclareOption*{\PassOptionsToClass{\CurrentOption}{slides}}

\ProcessOptions

\@ifundefined{@wantportrait}{
  \LoadClass[landscape]{slides}
}{
  \LoadClass{slides}
}

% need graphics for the logo and color for the two bars
\RequirePackage{graphics}
\RequirePackage{color}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% default values for International Computer Science Institute headers and footers

\def\@bwlogo{icsi-newlogo-bnw.eps} % black and white logo file
\def\@colorlogo{icsi-newlogo-col.eps} % color logo file
\def\@sitename{International Computer Science Institute} % site name for lower bar
\definecolor{@barcolor}{rgb}{0.0,0.0,0.4} % top and bottom bar color
\definecolor{@sitenamecolor}{rgb}{1.0,0.6,0.2} % lower bar text color

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% setup our default margin, header and footer dimensions

\setlength{\topmargin}{-0.6in}
\setlength{\headheight}{1.0in}
\setlength{\headsep}{0.25in}
\@ifundefined{@wantnofoot}{
  \setlength{\footskip}{0.75in}
}{
  \setlength{\footskip}{0.0in}
}

\setlength{\textheight}{\paperheight}
\@ifundefined{@wantnofoot}{
  \addtolength{\textheight}{-2.9in}
}{
  \addtolength{\textheight}{-1.9in}
}

\setlength{\oddsidemargin}{0.25in}
\setlength{\evensidemargin}{0.25in}

\setlength{\textwidth}{\paperwidth}
\addtolength{\textwidth}{-2.5in}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% new command definitions

%%%%%%%%%%%%%%%%
% lengths we need

\newlength{\@topbarlen}
\newlength{\@baroffset}
\newlength{\@logooffset}
\newlength{\@shortbarlen}
\newlength{\@tmplen}
\newlength{\@logowidth}
\newlength{\@caplen}

%%%%%%%%%%%%%%%%
% bar color functions

\@ifundefined{@wantbwslides}{
  \def\@barcolor{@barcolor}
  \def\@sitenamecolor{@sitenamecolor}
}{
  \def\@barcolor{black}
  \def\@sitenamecolor{black}
}

\def\barcolor#1{\global\def\@barcolor{#1}}
\def\sitenamecolor#1{\global\def\@sitenamecolor{#1}}

\def\@initbarcolors{
  \xdef\@exbarcolor{\@barcolor}
  \def\@usebarcolor{\color{\@exbarcolor}}
  \xdef\@exsitenamecolor{\@sitenamecolor}
  \def\@usesitenamecolor{\color{\@exsitenamecolor}}
} 

%%%%%%%%%%%%%%%%
% functions to set site name which will appear on the bottom bar,
% and determines segement lengths of bottom bar

\def\sitename#1{\def\@realsitename{#1}}

\sitename{\@sitename}

\def\@initsitename{
  \xdef\@exrealsitename{\@realsitename}
  \def\@sizedsitename{\tiny \@exrealsitename}

  \setlength{\@baroffset}{1in}
  \addtolength{\@baroffset}{\oddsidemargin}
  \addtolength{\@baroffset}{-0.5in}

  \setlength{\@topbarlen}{\paperwidth}
  \addtolength{\@topbarlen}{-1in}

  \def\@drawtopbar{{\@usebarcolor\hspace{-\@baroffset}%
        \rule{\@topbarlen}{0.04in}}%
      \hspace{-\@topbarlen}\hspace{\@baroffset}}

  \settowidth{\@tmplen}{\@exrealsitename}
  \ifnum\@tmplen=0
    % want no site name on bottom bar
    \def\@bottomline{\rule[0.25ex]{\@topbarlen}{0.04in}}
  \else
    % site name specified
    \def\@bottomline{\rule[0.25ex]{\@shortbarlen}{0.04in} %
      {\@usesitenamecolor \@sizedsitename} %
      {\@usebarcolor\rule[0.25ex]{1.0in}{0.04in}}}
  \fi
  
  \settowidth{\@tmplen}{\rule[0.25ex]{1.0in}{0.04in} %
    {\@usesitenamecolor \@sizedsitename} %
    {\@usebarcolor\rule[0.25ex]{1.0in}{0.04in}}}
  
  \setlength{\@shortbarlen}{\@topbarlen}
  \addtolength{\@shortbarlen}{-\@tmplen}
  \addtolength{\@shortbarlen}{1.0in}

  \def\@drawbottombar{\raisebox{0.4in}
    {{\@usebarcolor\hspace{-\@baroffset}\@bottomline}\hspace{-\@topbarlen}}%
    \hspace{\@baroffset}}
}

%%%%%%%%%%%%%%%%
% functions to deal with logo in header

% newlogo function which takes a filename and an optional width
% if filename is blank no logo is printed
\def\newlogo#1{%
  \@ifnextchar[{\@inewlogo{#1}}{\@iinewlogo{#1}}}
\def\@iinewlogo#1{\def\@newsitelogo{#1}}
\def\@inewlogo#1[#2]{\def\@newsitelogo{#1}%
  \def\@newlogowidth{#2}}

\def\@initnewlogo{
  \setlength{\@logooffset}{1in}
  \addtolength{\@logooffset}{\oddsidemargin}
  \addtolength{\@logooffset}{-1.0in}

  \@ifundefined{@newsitelogo}{
    % using a builtin logo
    \@ifundefined{@wantbwslides}{
      \xdef\@sitelogo{\@colorlogo}
    }{
      \xdef\@sitelogo{\@bwlogo}
    }
    \def\@drawlogox{\resizebox{!}{0.674in}{\includegraphics{\@sitelogo}}}
  }{
    \xdef\@sitelogo{\@newsitelogo}
    \setlength{\@logowidth}{\@newlogowidth}
    \settowidth{\@tmplen}{\@sitelogo}
    \ifnum\@tmplen=0
      \def\@drawlogox{\resizebox{\@logowidth}{!}{\hspace{0.75in}}}
    \else
      \def\@drawlogox{\resizebox{\@logowidth}{!}
        {\includegraphics{\@sitelogo}}}
    \fi
  }

  \def\@drawlogo{\hspace{-\@logooffset}%
    \raisebox{0.2in}{\@drawlogox}%
    \hspace{-\@logowidth}\hspace{\@logooffset}}
}

\setlength{\@logowidth}{0.75in}
\def\@newlogowidth{0.75in}

%%%%%%%%%%%%%%%%
% functions to implement small caption in footer

\newcommand\@puny{\@setfontsize\@puny{8}{10}}

\def\@drawtinycap{}

\def\tinycaption#1{\@settinycaption{#1}}

\def\@settinycaption#1{
  \global\def\@tinycaption{\@puny #1}

  \settowidth{\@caplen}{\@tinycaption}
  \ifnum\@caplen=0
    \def\@drawtinycap{}
  \else
    \def\@drawtinycap{\raisebox{0.3in}{\@tinycaption\hspace{-\@caplen}}}
  \fi
}

%%%%%%%%%%%%%%%%
% text for notes slides

\def\@drawnotescap{\makebox[\textwidth]{\small Notes}\hspace{-\textwidth}}

\if@clock
  \def\@drawclock{%
    \settowidth{\@tmplen}{\fbox{\large \@arabic\c@minutes\space min}}%
    \@drawnotescap\fbox%
    {\large \@arabic\c@minutes\space min}\hspace{-\@tmplen}}
  \def\@drawclockplain{\@drawclock}
  \def\@drawtopclock{%
    \settowidth{\@tmplen}{\fbox{\large \@arabic\c@minutes\space min}}%
    \raisebox{0.3in}{\fbox%
      {\large \@arabic\c@minutes\space min}}\hspace{-\@tmplen}}
\else
  \def\@drawclock{\@drawnotescap\@drawtinycap}
  \def\@drawclockplain{\@drawnotescap}
  \def\@drawtopclock{}
\fi

%%%%%%%%%%%%%%%%
% sets the title of the slide

\def\@slidetitle{}
\def\slidetitle#1{\global\def\@slidetitle{#1}}

\def\@drawtitle{\raisebox{0.3in}{\makebox[\textwidth]{\large \@slidetitle}}}

%%%%%%%%%%%%%%%%
% draws the page number

\def\@drawpagenum{\makebox[\textwidth][r]%
  {\@mainsize {\hfil\hb@xt@3em{\hspace{0.8in}\@thepagenum\hss}}}}%

\def\@drawtoppagenum{\raisebox{0.3in}{\@drawpagenum}}

%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% things that are determined at the begining of document and
% shouldn't be changed later

\AtBeginDocument{
  \@initbarcolors
  \@initsitename
  \@initnewlogo
  % for dvips when landscape letter paper
  \ifdim\paperheight=8.5in
    \ifdim\paperwidth=11in
      \special{papersize=11in,8.5in}
      \special{! TeXDict begin /landplus90{true}store end } 
    \fi
  \fi
}
  
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% things redefined from slides.cls

\@ifundefined{@wantnofoot}{
  \def\@commonhead{\@dodrawtopbar\@dodrawlogo\@drawtitle}
  \def\@commonfoot{\@dodrawbottombar\@dodrawtinycap\@drawpagenum}
}{
  \def\@commonhead{\@dodrawtopbar\@dodrawlogo\@drawtitle%
    \hspace{-\textwidth}\@drawtoppagenum}
  \def\@commonfoot{}
}

\def\ps@headings{%
  \def\ps@slide{%
    \def\@oddfoot{%
      \def\@dodrawbottombar{\@drawbottombar}%
      \def\@dodrawtinycap{\@drawtinycap}%
      \def\@thepagenum{\theslide}%
      \@commonfoot}%
    \def\@oddhead{%
      \def\@dodrawtopbar{\@drawtopbar}%
      \def\@dodrawlogo{\@drawlogo}%
      \def\@thepagenum{\theslide}%
      \@commonhead}%
    \def\@evenfoot{\@oddfoot}%
    \def\@evenhead{\@oddhead}}

  \def\ps@overlay{%
    \def\@oddfoot{%
      \def\@dodrawbottombar{\@drawbottombar}%
      \def\@dodrawtinycap{\@drawtinycap}%
      \def\@thepagenum{\theoverlay}%
      \@commonfoot}%
    \def\@oddhead{%
      \def\@dodrawtopbar{\@drawtopbar}%
      \def\@dodrawlogo{\@drawlogo}%
      \def\@thepagenum{\theoverlay}%
      \@commonhead}%
    \def\@evenfoot{\@oddfoot}%
    \def\@evenhead{\@oddhead}}

  \def\ps@note{%
    \def\@oddfoot{%
      \def\@dodrawbottombar{\@drawbottombar}%
      \def\@dodrawtinycap{\@drawclock}%
      \def\@thepagenum{\thenote}%
      \@commonfoot}%
    \def\@oddhead{%
      \def\@dodrawtopbar{\@drawtopbar}%
      \def\@dodrawlogo{\@ifundefined{@wantnofoot}{\@drawlogo}{\@drawtopclock}}%
      \def\@thepagenum{\thenote}%
      \@commonhead}%
    \def\@evenfoot{\@oddfoot}%
    \def\@evenhead{\@oddhead}}%
}

\def\ps@plain{%
  \def\ps@slide{%
    \def\@oddfoot{%
      \def\@dodrawbottombar{}%
      \def\@dodrawtinycap{}%
      \def\@thepagenum{\theslide}%
      \@commonfoot}%
    \def\@oddhead{%
      \def\@dodrawtopbar{}%
      \def\@dodrawlogo{}%
      \def\@thepagenum{\theslide}%
      \@commonhead}%
    \def\@evenfoot{\@oddfoot}%
    \def\@evenhead{\@oddhead}}

  \def\ps@overlay{%
    \def\@oddfoot{%
      \def\@dodrawbottombar{}%
      \def\@dodrawtinycap{}%
      \def\@thepagenum{\theoverlay}%
      \@commonfoot}%
    \def\@oddhead{%
      \def\@dodrawtopbar{}%
      \def\@dodrawlogo{}%
      \def\@thepagenum{\theoverlay}%
      \@commonhead}%
    \def\@evenfoot{\@oddfoot}%
    \def\@evenhead{\@oddhead}}

  \def\ps@note{%
    \def\@oddfoot{%
      \def\@dodrawbottombar{}%
      \def\@dodrawtinycap{}%
      \def\@thepagenum{\thenote}%
      \@commonfoot}%
    \def\@oddhead{%
      \def\@dodrawtopbar{}%
      \def\@dodrawlogo{\@drawtopclock}%
      \def\@thepagenum{\thenote}%
      \@commonhead}%
    \def\@evenfoot{\@oddfoot}%
    \def\@evenhead{\@oddhead}}%
}

\ps@headings

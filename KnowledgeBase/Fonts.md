# Fonts

### Font files:
A font describes how text should look on the screen.
"TTF" - Truetype
Usually described in the form of outlines. For example imagine adobe illustrator we would have
a set of vertices describing the letter T. We then think about a filed area within the vertices that 
will describe the character. These characters are also resolution independent, theres no pixels, we are just talking about an abstract shape.
The second thing a font file do is to describe a mapping from some character set encoding to those outline shapes.
To know what value should describe for example the character T we need some sort of encoding and thats why in the computer world we use character encodings such as ASCII or Unicode.
The font files also contain what we call Unicode codepoints.

### ASCII/ANSI
Basically a character set encoding that allows you to do most roman derived languages in a standard way where we just defined what all the characters are.
So for example to use a T we would use the defined value for T according to the ASCII encoding (84).
The choice of what character encoding to use is very important since it defined what characters you even can encode or which characters you have available.

### Unicode
Since ASCII is limited to 256 symbols and we have languages such as chinese which needs many more symbols than 256 we moved to a new approach which is called Unicode. Unicode is a way to encode drastically more characters. It goes up to 16 bits (2^16 = 65536) in the naive implementation like with windows called UTF-16. We then also have UTF-8 that allows for more since it's basically a variable length encoding. Its still 8 bit characters just like ASCII but you can double up the amount of characters by saying that theres for example a character after T that combines with T to produce a new character. This you can chain together to get for example 24 bit (16 million) glyphs.

### Unicode pros and cons:
- Good: every character has a unique identifier. All glyphs found their own home in unicode. It doesn't matter what
language you want to use, somebody somewhere hopefully has allocated some set of the unicode mapping and said
"Here's where those symbols go." What that means is that a single font could literally if it wanted to describe
all the symbols that all humans have ever used to write things down.
- Problem: You're stuck with that giant encoding. You cannot just use a table that is 65k entries long or more like
16 million entries long. That would be a crazy huge table to use for a game. The game simply are not going to
use all of those. So usually what you have to do with unicode is to treat is at as intermediary mapping because 
your lookups cannot be dense anymore. So what we have is our unicode that goes through some sort of mapping
into a table with lets say 500 glyphs with the glyphs that we are going to use. So to sum it up basically because 
unicode is this universal way to describe any glyph its not very good if we only want to use a small set of 
glyphs that you want to use. Entries into this unicode table is called "Codepoints".

### Basic typography:
In typography everything is about the solids and the empty spaces of the fonts and the shapes they make and how
they interrelate. Typographers spend a huge amount of time thinking all of this through. And they have a bunch of 
properties for fonts that they think about. 
For example the letters "gh" you would note that the g has a descender which is the part of the g that goes bellow
the baseline of the text and the h has an ascender which kind of goes a bit above the baseline. In a fontfile
what is usually described is "What is the baseline and how does the character glyph relate to it". Some other 
usefull information in the fontfiles is for example what is the maximum descender and ascender height. This allows
us to think of what the general span vertically and what is the region it typically uses. We also want to know
what the line height is, what the typographer envisioned the space be between lines. But most important of all is
something that is called "Kerning" which is information of how to adjust the positioning of letters horizontally
based on which pair of letters you're talking about. And to understand the difference between these we need to 
go through monospace fonts and proportional fonts.

### Monospace fonts
The oldschool kind that was on typewriters and terminals when they didn't level of sofistication necesary to 
advance the write position by a variable number of pixels or space. So what a font had to have was an equal amount
of space. So for example if we would have "gi" they both would have to take up the exact same amount of space width
wise which looks kind of dumb. Because there would be big gaps between small letters and it would confuse the reader
of what is a space and what is a part of the font.

### Proportional fonts
So when the spfistication needed arrived and we were available to add different spacing to our systems. So what
we can say using a proportional font is to say that whenever we draw a glyph when we draw the next glyph we will
just have some understanding of how far to go. Which is if you think about it quite a tricky problem because the 
actual distance you might want to go is not nessecairly constant depending on the letter. A naive proportional 
font implementation would say that for every letter there is a certain distance I will advance which is fine but
what it doesn't solve is the concept of which two letters you're putting back to back you might want to chose a
different amount of space. And thats where kerning information comes in as a way to modify the inter-character spacing
to account for pair-wise spacing. So what you have is another lookup table within the fontfile that would specify
the spacing for differnet character combination using a kerning value which typically is the delta between the 
normal spacing and the specific pair's spacing. So for example in the table it says that for every "g" we advance
10 units and the kerning value for "gj" would say that you were going to advance 10 units but now subtract 5 units
from it so you just advance 5 units.
The reason why the kerning values for pairs are specified in delta values is probably because if we were going to
have a big table for every glyph pair we would have a table of space O(N^2) which would get pretty large so instead
we have a default spacing value and only pairs in the table for when that default is supposed to be changed.

### Hinting
Fontfiles usually also include something that is called "Hinting". What hinting is if you're trying to make fonts
that appear very very tiny then theres some things that you don't want to do. For example if you talk about a 
font that would become rasterized on a very very small scale. And the rasterization becomes very bad and it 
for example uses half pixels in the rasterization grid and if that were to be rendered even with anti-aliasing
the letter we would render would become very thick because its like halfway in all over the place. What you actually
want is for the letter to snap to the grid and take up as little space possible. So hinting is a way to describe 
that you want that to happen for certain letters to try line them up with pixels. This is not that relevant anymore
on high resolution displays when the pixel count gets high enough you're always dealing with enough pixels for it 
to not matter.

### Ways to use fonts within our game:
Our game is not going to be a game that is very dependant on fonts so what we want is a simple way to render
the fonts so that it will be efficient and isn't going to be some concern where we think about how our fonts are
working.

1) Load TTF files and tesselate.
Load in the TTF files and turn the glyphs into triangles using a process called "Tesselation". This would be
difficult for characters like "s" that use bezier curves and we would have to pick which points that we would turn
into triangles.

2) Load TTF files and implicitly rasterize.
This is where we would load our TTF files and turn the glyphs into quads and use clip rectangles to decide for 
each pixel if its going to be filled by the quad if its included within all the edges of out clip rect.
Step through the pixels and say "Is this in the shape or not? And how do I fill it?".
Benefit: This works for any resolution.

- Implicit: f(x, y) - We have a function that takes x and y and it gives us a value for example if the pixel is included.
- Explicit: f(i) - Feed the function some parameter and get the x, y values back.

Those two options are probably overkill since we simply don't have that of an unpredictable, heavy reliance on font
shapes. Most games knows their target resolution and knows how big the fonts should appear on the screen. So we
would be wasting our time by performing this much work on our fonts.

3) Prerasterized fonts
Turns the fonts into something that our game can already handle. We can already draw bitmaps to the screen and we 
can also transform and scale our bitmaps. So this turns fonts into a problem that we've already solved.
So the way that this works is that we build bitmaps that capture the font at a certain resolution. For example
our target resolution is 1920x1080 and we want our fonts to be scaled for this so we would rasterize at a size
that would make sense for our resolution, for example 40 pixels high. Save the results of that rasterization into
a set of bitmaps or a single bitmap where they are packed together and then when we want to draw them we will simply
draw the bitmaps to the screen like we draw everything else. We also wouldnt have to worry about the cost of 
drawing those bitmaps to the screen since it goes through the same pass as our other drawing code.

### How to do prerasterized fonts
Start with a bitmap and then start packing glyphs into it. What do we store into the bitmaps? We do not need any color
information since they will probably be colored on the fly so they will just be white. So it will probably be a 
monochrome bitmap. An 8 bit value that says how much coverage there was.
Then we need a couple of tables, one for the codepoints which basically is a mapping between the encoding value
and our bitmap. For example: "Where is the glyph for the letter A?". Then we need some positioning information
for the letters "Where does the letter A go and whats the spacing?" how the letters should be positioned to the
baseline. And finally we need the kerning table. 
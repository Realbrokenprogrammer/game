# SIMD (Single instruction multiple data)

SIMD stands for "single instruction multiple data", what that means is that
for every instruction that the processor decodes it is actually going to
do what that instruction tells it to do on more than one piece of data at 
once.

**Currently this is just basic information about SIMD, it will be expanded
in the future**

For example if we have a piece of code that says something like 
`B = A + A;` which is how we would understand that in C and we
would be to translate it into some kind of instruction for the processor 
that might look like 
```
ADD B A, A
```
where we have an ADD instruction, the destination register of B and two 
source registers which is A and A. Important concept to understand here is 
that we have two registers (A and B) and we are telling the processor to
take the contents of the A register, add it to itself and put the result
in the B register.

So the C code in the two examples maps directly to the example code for the
processor, usually one or two instructions and very straight forward. 

### How we talk about code in SIMD

In SIMD we don't talk about code like the example above of `B = A + A;`, 
instead we talk about an operation that looks like this:
```
B0 = A0 + A0;
B1 = A1 + A1;
B2 = A2 + A2;
B3 = A3 + A3;
```

It is the same math but its operation is spread across multiple values at
the same time. We no longer talk about `B = A + A;` instead we talk about a
family of values called B and a family of values called A plus another family of values.

So the above example is what's called **"4-Wide"** because it is operating
on four things at once and they all have the same operation done to them. 
The family of values, in this case for example B and A is called 
**"lanes"** and the operation itself is a __SIMD instruction__. It's a 
single instruction, just an ADD but on multiple data. It is performed on 
four values at once. See the image bellow for visualized explanation.

![4-Wide SIMD](https://i.imgur.com/ZQfRSh0.png)

### Different kinds of SIMD

There is different kinds of SIMD instruction sets, there is **"SSE"** and 
**"Neon"**. Where __SSE is for x64__ and __Neon is for ARM__ processors and 
theese are both 4-Wide. A couple of years ago Intel introduced **AVX** which
is 8-Wide but exactly the same concept as previously explained. And 
announced we also have **AVX512** which is 16-Wide.

### How do we use it?

Because SIMD looks like this:
```
B0 = A0 + A0;
B1 = A1 + A1;
B2 = A2 + A2;
B3 = A3 + A3;
```
Everytime we issue an instruction it is going to do that same thing four 
times, what we want to do is to find groups of code or entire code paths
where we can do four things at once. 

For example in the game's renderer we are drawing pixels to the screen and
every pixel has to go through the same set of operations before it is being
displayed. Example of operations are converting from sRGB to "linear" 
brightness space, bilinear texture blending and clamping the color values to
a valid range.

So since all operations have to happen for every pixel what we could do 
instead of looping through every pixel and performing the operations instead
operate on four pixels at once.

So the simplest type of SIMD thing you can do is to take a loop like a for loop and instead of incrementing or going by one value at the time, you go
by four things at the time. Example code is as follows:
```
// Going by one
for (int X = MIN; X <= MAX; ++X) {
    // Some operations here
}

// Going by four
for (int X = MIN; X <= MAX; X+=4) {
    // Some operations here
}
```

### Our case of SIMD in our software renderer (CPU vs GPU)

In our case for this game engine we are currently using the CPU to perform
rendering instead of the GPU. But right now in our code our frame buffer is
stored so that pixels are contigous in X, basically we have a pointer to a
row of pixels and the pixels just go in order of R, G, B, A, R, G, B, A...

What is important to remember about that is that it's probably not the
best or most efficient way to organize the pixels that we're actually
operating on and its in fact not the way GPUs commonly organize those 
pixels. 

What happens when you operate on four pixels at the time the thing you have
to worry about is operating on a lot of pixels you actually didn't need to
operate on. Let's say we have a rectangle that is on our pixel grid, it is 
not 100% guaranteed that it would allow us to operate on them four at the 
time. Illustration follows:

![Pixels off the edge](https://i.imgur.com/8Wyw3BX.png)

Here you can see that the first four pixels are operated on as expected
but the next batch of four pixels we have three that goes outside of the
rectangle's range. We would be wasting those three pixels, we would 
calculate them and they would fail the inclusion test which means our loop
would still only operate on one pixel in this case.

So what GPUs do to prevent that is that they often rearrange the memory,
instead of storing things contigously like we do they actually store it
so that four contiguous pixels actually maps to a block 
(Imagine a rectangle of 4 pixels). And in fact when using blocks we wouldn't
be wasting pixels, the blocks fits our rectangle perfectly. The maximum 
wasted pixels would be one row around the edge, you can't waste a double 
row.

So what we're dealing with is basically a data organization thing. This wont
affect how the logic of pixels works at all but instead affect how we loop
over the pixels and how we pick the starting point.

### "SOA" vs "AOS"

Worth mentioning, specially when we are talking about data organization is
two things, **"SOA"** which stands for "Structure of arrays" and *"AOS"* 
which stands for Array of structures.

Array of structures is what C makes easy for us but for SIMD we actually 
want structure of arrays.

```
// This is what is C makes easy and what is expected in a C program.
struct color
{
    float R, G, B, A;
}

// Example of structure of arrays
struct colors
{
    float *R; // Array of red colors
    float *G; // Array of green colors
    float *B; // Array of blue colors
    float *A; // Array of alpha values
}
```

This is also what you see in most programs because C# and Java for example comes
from C in terms of their structural nature.

But typically we would want to perform the same operation on the same type of things
so what we actually want is four Rs together, four Gs togther, four Bs together and four 
As together.
```
RRRR GGGG BBBB AAAA
```

In our code we have for example:
```
color *Color; // Array of colors (Values we want to operate on)
```
For this array the problem in C is that they're typically packed as RGBA. But when
we do operations on them we want to do operations such as `R′ = A * R;` but the actual
values as they appear in memory isn't four Rs, four Gs etc so in order to get them into 
this format in the example, when we load them from memory they're not in the format a 
SIMD register expects (grouped together by four). And so on the load before performing the
SIMD instructions you end up with a bunch of instructions designed to grab all of the Rs 
and add them to one packed register.

How this actually works is just like the example of the translated processor instructions
in the first section. So inside the processor we might have some memory location and we 
want to move something into that register. Then we want to ADD some value to that register.
So the way SIMD works is exactly the same as that example, its just that the registers are
wider now, instead of 64-bits wide they're now 128-bits wide. 

So when we issue a move instruction to a SIMD register it's going to go and grab 128 bits
and stick it in there. And then there is SIMD instructions to operate on that, for example
an ADD but in addition to the ADD instruction you also specify how you're adding them 
together. For example in the normal processor instruction when we issue an ADD it just adds
the two registers together but in SIMD we can for example say that we want to ADD two 
registers and specify that it's four wide and floating point. Meaning that we'd want to 
treat the register as if its four floating point values in it and add them separately. We
could also do it differently saying that we want to add eight signed integers, meaning that
we would treat the register as eight values of signed integers.

So in addition to the operation we also basically specify what the register had or is 
going to have in it. So the difference when using SIMD is that we thing about how
do we fill up the 128 bits and how do we write them out later.

Reference for SIMD instructions:
[Intel intrinsics guide](https://software.intel.com/sites/landingpage/IntrinsicsGuide/)

# TODO
* Add example SIMD code.
* Add debugger screenshots of lanes are displayed etc.
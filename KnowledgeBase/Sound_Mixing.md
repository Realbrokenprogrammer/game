# Sound Mixing

Normally for sound effects, they are small and can be loaded in one go but if you have a longer sound for example a 2-4 minute piece of
music you do not want to keep the entire thing in memory and instead load chunks of it, for example 10 seconds at the time and once those
10 seconds have been played you load the next chunk.

This might not be needed for modern day machines since for example a 3 minute audio file would approximately take up 30 megabytes of
memory which is not that big of a deal.

### Sound and Music:
In the real world the way that you can hear sound in the first place is that the world is filled with molecules like for example air.
These molecules are usually moving around and when there is some type of force excerted on them for example they are pushed in a
direction they start to move coherently and you get an oscillation as they move a little and then return back a bit untill they come back
into a steady state and the vibration that comes as a result of the molecules hitting eachother and what not gets passed throughout the
world. 

For example when you have a small ripple in a pond and there is small objects nearby for example rocks, the original ripples would hit
the rock forming new ripples hitting the old ripples forming new patterns. That is the type of model you have to think about when you
think about sound. It's not the same as light that travels in a straight line, bouncing off things in a very specific way and then
travels in another straight line. Sound does not work that way, it doesn't travel from one point to another like we think of light,
instead we want to think of light as some sort of volume that can for example reach around corners.

### How we hear:
Inside our ears there is small hairs, what happens is that when the vibrations from the world enters your ear it goes through your
eardrum etc that will filter the sound and then these small hairs will move by those vibrations. They are pushed using the filtered
vibration that enter that inner ear area and based on how those hairs vibrate back and forth they generate signals to your brain that
tells us of what frequency a vibration is happening. So basically those hairs are telling us __how  fast (Speed)__ and 
__how much (Force)__ those vibrations are.

### Frequency:
The speed of the vibrations that enters our ears. For example when you drop a stone into water there will be small waves or ripples and
the frequency would be the waves length of the waves formed in the water. The way our brains understands frequency is that the higher the
frequency or the faster the vibrations are the higher pitch the sound will have. Something that is vibrating very quickly will have a
very high pitch sound while something that is vibrating really slowly will have a low and deep sound. There is on top of this a
threshhold of how slow and fast sounds we can hear but our body will still "feel" those sounds, and example for that is if you have a
really strong bass you will still actually feel your body vibrating but you just can't actually hear the sound. Summary: **Frequency = Pitch**
Something to also keep in mind is that the speed we can percieve visual information is drastically slower than audio information where VR
systems usually update each frame of video around 90 **Hz** while the most basic of modern audio output these days happens at 48000 
**Hz** per second. 

### Amplitude:
Amplitude is the way we percieve the volume of something. So if we go back to our example of dropping a rock into the water, the
amplitude would be how high the peaks and throughs are. So if you imagine dropping a really big stone into the water it would generate a
much bigger ripple which would have a bigger amplitude of its wave lengths than with a smaller rock. So amplitude is where we get our
volume from. So from our frequency we get our pitch of the sound and from the amplitude we get the volume of the sound and combined we
get all the charesterictics of sounds that we can hear hence the two things that we want to reproduce.

So how we reproduce it is that we have a speaker and what the speaker is designed to do is to reproduce those vibrations, they get a
small electrical signal and the cone within the speaker vibrates back and forth based on the signal that we send it. It vibrates further
back and forth based on the amplitude we send it and it vibrates faster based on the frequency signal. If we continously vary the
frequency and amplitude we are sending it over time we could basically reproduce any vibration that could be captured at a point. This is
often why you will use multi-speaker systems, since we humans have two ears and by using them both as inputs and some assumptions of how
sound travels through your head and what not you can roughly determine where a sound is coming from, we kind of can tell what we hear
from our left ear and what we hear from our right ear. When it comes to if the sound is coming from infront of us or behind us its really
difficult for us to tell and instead we have to rely on the charasteristics of the sound for example if its more muffled or louder.

### Sound and Software:
For a simpler game like the one we're developing we are not trying to emulate a realistic experience like for example VR games hence the
most important part of sound is for it to sound good and pleasing and that it does pleasing things based on whats happening within the
game.

So we have our speakers or our set of headphones or even a 5.1 suround system (5 speakers and also a bass vibration channel) but it all
comes down to that we have a number of channels. We need to output a number of channels and those channels are ways for us to telling the
speakers what vibrations to reproduce.

The way we tell them what vibrations to reproduce is encoding a wave form and we basically take snapshots (48000 of them per second) and
for each snapshot we say how we want that speaker cone to be vibrating. Imagine a curve with 48000 lines in it for each snapshot and for
each of them we want to store the position at that specific snapshot as a 16 bit signed integer and those positions becomes our values
that we want to output and we want to output that once per channel.

### Mixing sounds by adding them together:
So what does it mean that we want to output once per channel? It means that how we output sound boils down to one stream of numbers per
speaker that tells it how to vibrate. Since there is only one vibration going out to each speaker what that means is that no matter how 
many sounds we are playing, all of the sounds will be boiled down into one single set of vibrations per speaker.

From a single position in space there is no way for when for example molecules are vibrating back and forth for them to carry more 
information than the sound from that specific point. Going back to the stone in the water example there is no way to cause two ripples 
from the exact same point. 

So what actually happens when you want to play two sounds from the same location is that you need to create a new ripple or a new sound 
that is a combination of the two sounds that you want to play.

So what you do is simply adding the sounds together sample by sample to produce the two vibrations happening together.

So what our sound mixer will do is instead of copying sound into our sound buffer is instead adding it into our buffer. So we can instead 
think of a Sound Mixer as a __Sound Adder__.

Thinking about this the implementation will be pretty straight forward but optionaly there is some more complexity to keep in mind.

### Clipping:
So what happens when we add our sounds together in our 16 bit signed integer is that the sound might "overflow". We cannot prouce a sound 
that is any louder than what our 16 bit integer can store producing a sound that cannot be produced by the speaking resulting in a weird 
distorting sound called **Clipping**. 

So clipping is something you can address or chose not to, often times you just do not have to. But if you wanted to you can do something 
to your sound signals to protect your sound from this called compressions or __Soft knees__ or __Hard knees__ which are ways of mapping 
your sound range so that you can never reach "No mans land" when you're performing the adding of your sounds.

### Modulation and Interpolation:
A straight up "Adder" for a mixer is really simple. You're literally just taking two buffers and you're adding their contents together 
and writing the result. 

So if you disregard the clipping problem you're talking about a really simple routine. This would produce a resonable mixer but it would 
be somewhat limited. There might be some things that we would want to do with our sounds within the game that even if we're not audio 
specialists that perform fancy filtering but instead basic game designer controll over the sound there is some things that we probably 
want to do.

- Volume
- Pan (Left and right channel)
- Pitch

#### Volume and Pan:
These two things basically boils down to the same thing. For example if we have two speakers and we want to simmulate the sound being 
more from the left than the right we would want to play it louder on one side and lower on the other. So panning the sound means that 
we're modifying the volume between the channels of the speakers. So panning and volume is basically the same thing.

Volume is basically just modulation. The peak and troughs of the curve. So all you need to do to make a sound sound quieter you just need 
to multiply it by some value between 0-1 (You could choose a greater value if you want it to come out louder than when it started). The 
reason why this might be a problem is that you cannot change these values discountiniously. If all you want to do is to put a sound at a 
particular volume and pan and leave it there then it wouldn't be a problem but if you may want to do is to fade in sound. So the sound 
would start of kind of quiet and then become louder and louder.

#### Pitch:
There is two ways you can think about pitch changes.

- Length preserving
- Not length preserving

The frequency (Pitch) is the vertical axis of the curve. So to make it sound in a higher pitch would be to play the sound faster. So if 
the sound was sampled at a rate of 48000 what you could do is to play it at a rate of 2 * 48000 to produce a higher pitch sound.

The problem is that it would also speed the sound up, so if you would play the sound twice as fast it would last half as long. So if you 
would want to speed it up by a very small value this wouldn't be a problem but if you want to do something more advanced then what you 
want to look at is a length preserving pitch change which is __VERY__ complicated. So complicated that up untill recently(Not untill the 
last 10 years) noone knew how to do it properly. This is something that you would typically never see within a game because the cost is 
pretty big and the cases where a game would need to perform these pitch changes are pretty rare.

What you would normally do is just record all the sounds in the way you want them instead of pitch bending the sounds more than just a 
little bit.

So in our case we wouldn't want to change our pitch by a great magnitude, we would talk about values such as 1.02 or perhaps 1.1, very 
small variations. 

A cheap way to do this is to find the middle point of each snapshot we talked about earlier by pretending that there is a line between 
all the points and approximate where the curve would be. That would give our game some small pitch shifting for free since that is a very 
low cost operation(Linear interpolation).
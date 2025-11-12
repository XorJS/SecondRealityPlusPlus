# Second Reality++

C++ version of "Second Reality by Future Crew"

## Repository
- https://github.com/XorJS/SecondRealityPlusPlus

## Info
- Ported Second Reality from mixed C / C++ / Pascal / ASM to modern, cross-platform C++ targeting Windows (x64/x86), Linux, and WebAssembly
- Big thanks to Gargaj / Conspiracy for releasing a 32-bit Windows (Win32) port
- After seeing [this post on X](https://x.com/conspiracyhu/status/1951399870448775538), I forked and started this project (Second Reality++) and this one is based on the Win32 version
- More info about [this Port](https://www.jsr-productions.com/secondreality.html)

## Platforms
- Windows x64 / x86
- Linux (Ubuntu)
- Web browsers (e.g. Google Chrome)

## Videos
[Windows x64 - Fullscreen](http://www.youtube.com/watch?v=6vqV1JEFuog)
[![Windows x64 - Fullscreen](http://img.youtube.com/vi/6vqV1JEFuog/0.jpg)](http://www.youtube.com/watch?v=6vqV1JEFuog)

[Windows x86 - windows mode](http://www.youtube.com/watch?v=dqctyPvdK64)
[![Windows x86 - windows mode](http://img.youtube.com/vi/dqctyPvdK64/0.jpg)](http://www.youtube.com/watch?v=dqctyPvdK64)

[Ubuntu - windows mode](http://www.youtube.com/watch?v=2fx-b-zLOcc)
[![Ubuntu - windows mode](http://img.youtube.com/vi/2fx-b-zLOcc/0.jpg)](http://www.youtube.com/watch?v=2fx-b-zLOcc)

[Web - Google Chrome](http://www.youtube.com/watch?v=MDkMdTUCUKE)
[![Web - Google Chrome](http://img.youtube.com/vi/MDkMdTUCUKE/0.jpg)](http://www.youtube.com/watch?v=MDkMdTUCUKE)

## Web
- [Launch in your Web Browser](https://www.jsr-productions.com/secondreality/index.html)

- [Start at the Logo scene](https://www.jsr-productions.com/secondreality/logo.html)

* Don't forget to tap or click on screen to enable the audio (Web browsers requirement)

## How to Build

### Windows
- Open SecondReality++.sln with Visual Studio and build the solution

### Linux
- Launch ./buildLinux.sh script

### Web
- Setup [emscripten](https://emscripten.org/docs/getting_started/downloads.html)
- Call BuildWeb.bat script
- Don't forget to launch once WebServer.bat script if you run it locally

## TODO ****************************************************************
- Tested
- todo/to-improve

## References
- Original Windows 32bits Port:  https://github.com/ConspiracyHu/SecondRealityW32
- Original Source Code: https://github.com/mtuomi/SecondReality

	The repositories above are included as Git submodules under the References/ folder

	If you don't see the repos:
	```
		git submodule init
		git submodule update --init --recursive
	```

## Thanks
- Huge thanks to Gargaj / Conspiracy, whose Win32 port inspired this C++ port

## Acknowledgements
- Original code by
  - Sami "PSI" Tammilehto
  - Mika "Trug" Tuomi
  - Arto "Wildfire" Vuori
- [st3play v1.01](https://pastebin.com/raw/AwRXZAw7) by Olav "8bitbubsy" Sørensen (https://16-bits.org)

## Links
- [Future Crew](http://en.wikipedia.org/wiki/Future_Crew)
- [Second Reality](http://en.wikipedia.org/wiki/Second_Reality)
- [More Info about this Port](https://www.jsr-productions.com/secondreality.html)

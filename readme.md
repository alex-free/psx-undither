# PSX Undither

_By Alex Free_.

Patch PlayStation 1 games to turn off the [dithering effect](#what-is-dithering) by either modifying the CD image directly, or generating GameShark codes from a DuckStation RAM dump.

| [Homepage](https://alex-free.github.io/psx-undither) | [Github](https://github.com/alex-free/psx-undither) |

## Table Of Contents

*   [Downloads](#downloads)
*   [What Is Dithering?](#what-is-dithering)
*   [How Does This Work?](#how-does-this-work)
*   [Downloads](#downloads)
*   [Usage](#usage)
*   [Generating GameShark Codes](#generating-gameshark-codes)
*   [License](#license)
*   [Credits](#credits)
*   [Building](build.md)

## Downloads

### Version 1.0 (7/23/2024)

Changes:

*    Initial release.

----------------------------------------------------

*	[psx-undither-v1.0-windows-i686-static.zip](https://github.com/alex-free/psx-undither/releases/download/v1.0/psx-undither-v1.0-windows-i686-static.zip) _Portable Release For Windows 95 OSR 2.5 and above, Pentium CPU minimum (32 bit)_

*	[psx-undither-v1.0-windows-x86\_64-static.zip](https://github.com/alex-free/psx-undither/releases/download/v1.0/psx-undither-v1.0-windows-x86_64-static.zip) _Portable Release For x86_64 Windows (64 bit)_

*	[psx-undither-v1.0-linux-i386-static.zip](https://github.com/alex-free/psx-undither/releases/download/v1.0/psx-undither-v1.0-linux-i386-static.zip) _Portable Release For Linux 3.2.0 and above, 386 CPU minimum (32 bit)_

*	[psx-undither-v1.0-linux-i386-static.deb](https://github.com/alex-free/psx-undither/releases/download/v1.0/psx-undither-v1.0-linux-i386-static.deb) _Deb package file For Linux 3.2.0 and above, 386 CPU minimum (32 bit)_

*	[psx-undither-v1.0-linux-x86\_64-static.zip](https://github.com/alex-free/psx-undither/releases/download/v1.0/psx-undither-v1.0-linux-x86_64-static.zip) _Portable Release For x86\_64 Linux 3.2.0 and above (64 bit)_

*	[psx-undither-v1.0-linux-x86\_64-static.deb](https://github.com/alex-free/psx-undither/releases/download/v1.0/psx-undither-v1.0-linux-x86_64-static.deb) _Deb package file for x86_64 Linux 3.2.0 and above (64 bit)_

---------------------------------------

## What Is Dithering?

Dithering is a graphical effect that exploit the blur of a CRT set in combination with a composite signal in order to create transparency or texture related detail/color illusion effects. Most PS1 games were explicitly designed with the expected setup of a CRT TV using a composite signal (be that S-Video or standard AV RCA cables) in mind. Some games even heavily depend on dithering (i.e. Silent Hill) to hide color banding.

PS1 Console models with the original [dual-ported VRAM GPU](https://problemkaputt.de/psx-spx.htm#gpuversions) (SCPH-1000, early SCPH-1001, early SCPH-1002, and early SCPH-3000) have a less advanced dithering capability then ones with the newer SGRAM GPU. This may make disabling of dithering cause less of a visual change on these older console models with the original GPU design, according to no Martin Korth of No $ fame.

Without a CRT TV dithering can make some games appear less sharp and nosier (an opinion shared by many, but not a fact), causing a non-intentional checkerboard pattern on textures and or a noticeable drop in clarity on TV's using Plasma, LCD, OLED, or similar non-CRT-based screen technologies. Dithering is usually lost in translation when using emulators (varies from emulator to emulator), however one way to see it is to use [DuckStation](https://www.duckstation.org/) with the software renderer:

## Dither ON (DuckStation Software Renderer):
![tekken 3 dither on](images/tekken-3-dither-on.png)

([Full Screen Image](https://raw.githubusercontent.com/alex-free/psx-undither/master/images/tekken-3-dither-on.png))

## Dither OFF (DuckStation Software Renderer):
![tekken 3 dither off](images/tekken-3-dither-off.png)

([Full Screen Image](https://raw.githubusercontent.com/alex-free/psx-undither/master/images/tekken-3-dither-off.png))

Note: the floor, the background, the character models.

For more info on PS1 dithering, please see [ConsoleMods Wiki: PS1 De-dithering Patch](https://consolemods.org/wiki/PS1:De-dithering_Patch) and [PlayStation 1 Dithering Removal - by Chris Covell](https://www.chrismcovell.com/psxdither.html). The latter contains **great side by side comparison images** and further technical information. There are also comparisons on YouTube.

## How Does This Work?

For games that do not bake dithering into the texture themselves, a method to turn off the dither flag in GPU related code was first [documented](https://www.chrismcovell.com/psxdither.html) by [Chris Corvell](https://www.chrismcovell.com/). The user  user [cr4zymanz0r](https://shmups.system11.org/memberlist.php?mode=viewprofile&u=12851&sid=d2afebf2bcbd60fef0365818dcf66fa5) then posted "[Disabling PS1 hardware dithering (better looking 2D)
](https://shmups.system11.org/viewtopic.php?t=61746)", which led to him developing a CD image patcher named [PS1 De-Dither](https://github.com/cr4zymanz0r/PS1_De-Dither) to disable dithering using Chris Covell's methods. 

The cr4zymanz0r patcher is a [batch script](https://github.com/cr4zymanz0r/PS1_De-Dither/blob/master/Patch_PS1_dither.bat) calling sed and [ECCscan](https://github.com/Terraonion-dev/ECCScan) Windows binaries. So when I found it I became excited immediately to write **my own patcher, with additional features and improvements such as:**


*   Support for patching dither code that spans more then 1 sector. The dither code could potentially start at the very end of a sector, and then mid-way through get cut off by EDC, ECC and the header of the next sector. That would mean the dither code ends at the user data portion of the next sector. That isn't handled by the cr4zymanz0r PS1 De-Dither patcher (or hex editor find and replace method first mentioned by Chris Covell for that matter).

*   EDC/ECC correction after any sectors are modified using the same C functions that [CDRDAO](https://github.com/cdrdao/cdrdao) uses.

*   Portable C source code with an [incredible portable-centric build system](https://gitub.com/ezre) targeting everything from Linux v3.2.0 i386 and Windows 95 OSR 2.5 to their modern 64 bit equivalent systems. Portable releases for Windows and Linux are created (including deb packages for Linux). Porting to other OS'es and CPU architectures should be trivial if so desired.

*   Patching of CD image input files directly. The cr4zymanz0r patcher creates an entirely new file named [PatchedISO.bin](https://github.com/cr4zymanz0r/PS1_De-Dither/blob/8a63a13449eb98449966317d724c14132dae76c4/Patch_PS1_dither.bat#L12) that additionally must be renamed to the original's name and replaced in the CD disc image directory by the user after the patcher is ran.

*   Automatic switch into CD image patch mode after detecting a valid PlayStation 1 disc image is given as the input file (uses my [Lib-PS-CD-ID](https://github.com/alex-free/lib-ps-cd-id) library).    

*   Automatic switch into GameShark code generation mode when a valid DuckStation generated RAM dump file is given as the input file.

## Usage

`psxund <input file>`

`<input file>      Can be either the data track bin file of a PlayStation 1 disc image (the sole .bin file or the .bin file named something like track 01), or a DuckStation RAM dump file.`

There are 2 ways you can interact with the patcher:

### Drag n' Drop

On Windows and most Linux distributions, you can simply drag the input file on top of the `psxund.exe` (Windows releases) or `psxund` (linux releases) executable file found in the portable releases.

### Traditional CLI

On Windows execute `psxund.exe` with the input file as an argument, i.e. `psxund.exe <input file>` using `cmd.exe`, `command.com`  or similar shell. On Linux execute `./psxund <input file>` found in the portable release using the Terminal application. Alternatively if you have install the `.deb` package file `psxund`  is available as a global user command to the system, so just `psxund <input file>` works fine.

### Disc Image Patching Examples

![ridge racer 1](images/ridge-racer-1.png)

![ridge racer 2](images/ridge-racer-2.png)

![medievil 1](images/medievil-1.png)

![medievil 2](images/medievil-2.png)

## Generating GameShark Codes

1) Download [DuckStation](https://www.duckstation.org/), and make sure the `Debug` menu is enabled in the settings. Start the target game.

![ridge racer gs 1](images/ridge-racer-gs-1.png)

2) Use the `Debug` menu to `Dump RAM`.

![ridge racer gs 2](images/ridge-racer-gs-2.png)

![ridge racer gs 3](images/ridge-racer-gs-3.png)

3) Use the RAM dump as the input file for PSX Undither:

![ridge racer gs 4](images/ridge-racer-gs-4.png)

![ridge racer gs 5](images/ridge-racer-gs-5.png)

One way to apply these codes is [Tonyhax International's GameShark feature](https://github.com/alex-free/tonyhax/blob/master/gameshark-code-support.md).

## License

PSX Undither is released as open source software under the GNU GPLv2 license (required by the use of [CDRDAO](https://github.com/cdrdao/cdrdao) code for the EDC/ECC regeneration functions). Please see the file `license.md` in each release for more info. In the future, I'd like to dual license this 3-BSD (my code), and use a CDDL or MIT license to cover the EDC/ECC code.

## Credits

*   [CDRDAO](https://github.com/cdrdao/cdrdao) for EDC/ECC regeneration code.

*	[cr4zymanz0r](https://github.com/cr4zymanz0r) for the original [PS1 De-Dither](https://github.com/cr4zymanz0r/PS1_De-Dither) patcher.

*   [Chris Covell](https://www.chrismcovell.com) for his [PSX dither documentation](https://www.chrismcovell.com/psxdither.html) on patching CD images as well as creating GameShark codes.
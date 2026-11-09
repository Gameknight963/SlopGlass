![header](assets/banner.png)

# Experience the native Aero Glass interface on Windows 10+

SlopGlass restores the full glass effect to window frames, with control over blur, reflections, colorization, caption rendering, and theme integration.

This fork is focused on the Legacy compositor. It includes:
 - frame highlights adapted from [arceuss/OpenGlass](https://github.com/arceuss/OpenGlass).
 - Changes to Override Accent. See [this issue](https://github.com/ALTaleX531/OpenGlass/issues/354) on OpenGlass for more information.

MILComp is not a maintenance target for this fork, since I don't use Windows 11.

## AI disclosure

If it wasn't obvious by the name, this repo is basically just AI slop. I told it to not touch any of the DWM internal stuff with a 5-foot pole so hopefully it's stable lol.

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/ALTaleX531/OpenGlass)
[![Upstream CI](https://github.com/ALTaleX531/OpenGlass/actions/workflows/build.yml/badge.svg?branch=main)](https://github.com/ALTaleX531/OpenGlass/actions/workflows/build.yml)

## Supported Windows versions

| Windows version (OS build) | Status |
| --- | --- |
| Windows 10 1809–22H2 (17763–19045) | Stable |
| Windows 11 21H2–25H2 (22000–26200) | Stable |
| Windows 11 26H2 (26300) | Stable |
| Windows Server 2022 (20348) | Supported |

MILComp and is outside this fork's maintenance scope. Insider builds are not supported. See [Compatibility and DWM architectures](https://github.com/ALTaleX531/OpenGlass/wiki/Compatibility-and-DWM-architectures) for details.


## Downloads

[Download nightly](https://nightly.link/Gameknight963/SlopGlass/workflows/build/main/v3.1.0.4266-unsigned.zip)
<br> 
Binary releases are not planned, so you must "live on the edge" in order to use SlopGlass. You're already doing this if you trust a vibecoded project to inject into DWM, so go ahead and download it.

The installer is unsigned, so Windows might block it since it does dangerous-looking stuff like injecting into DWM

## Quick start


1. Download the installer from the section above and run it.
2. Open the configuration window from the Start menu shortcut **Configure OpenGlass** (or check **Launch OpenGlass GUI** on the installation finish page). It requests administrator rights.
3. Adjust the appearance. Settings are written immediately. **Save** accepts the changes, while **Revert** or closing the GUI undoes pending changes.


### Accent compatability feature

By default, "Limit accent override to supported shell windows" leaves ordinary context menus and drop-downs with their native Accent effects. To let "Override accent" also affect menus customized by tools such as TranslucentFlyouts, uncheck that option under "Compatibility mitigations."

> [!TIP]
> **Emergency Exit:** Hold <kbd>Ctrl</kbd>+<kbd>Win</kbd>+<kbd>Shift</kbd>+<kbd>Q</kbd> to terminate DWM if the system becomes unresponsive.

OpenGlass is a specialized implementation focused on Aero Glass, using aggressive optimizations for that purpose and primarily targeting advanced users. Readers who simply want a translucent desktop appearance should consider [DWMBlurGlass](https://github.com/Maplespe/DWMBlurGlass).

## Windows theme integration for developers

The theme extension connects Windows theme application and the classic color panel to OpenGlass. It is intended for developers who build and customize it from source. The installer does not include or manage it. See [Windows theme integration](https://github.com/ALTaleX531/OpenGlass/wiki/Windows-theme-integration) for building, manual installation and behavior.

## Reporting issues

Report bugs specific to this fork in [GitHub Issues](https://github.com/Gameknight963/SlopGlass/issues/new), with your exact Windows build, SlopGlass version and reproduction steps. For crashes or opaque glass, start with the configuration window's **Diagnostics** tab and the [troubleshooting guide](https://github.com/ALTaleX531/OpenGlass/wiki/Troubleshooting-and-crash-dumps).

There's a decent chance it's a problem with the upstream I don't know how to fix, in which case I'll tell you to go recreate your issue there.

## Building

For build prerequisites, dependencies, and CI caching details, see [Building OpenGlass](https://github.com/ALTaleX531/OpenGlass/wiki/Building-OpenGlass), [Python setup](tools/README.md#dependencies), and [Windows dependency reuse](.github/actions/prepare-build/README.md).

```powershell
python -m pip install -r requirements.txt
msbuild OpenGlass.slnx /m /restore /p:RestorePackagesConfig=true /p:Configuration=Release /p:Platform=x64
```

Install the Python requirements with the same interpreter used by MSBuild. If needed, select it explicitly with `/p:ProjectionPython="C:\path\to\python.exe"`.

## Credits

- [@ALTaleX531](https://github.com/ALTaleX531) created OpenGlass. See [CONTRIBUTORS.md](CONTRIBUTORS.md) for the wider project credits.
- Frame highlights, artwork integration, and Windows 7/Vista/theme style options are adapted from [@arceuss's OpenGlass fork](https://github.com/arceuss/OpenGlass/commit/d8a826e943dba8df498bfaa00b8cb239532bcf6c).
- GUI icon: The glass-window artwork is from [AeroShotCRE](https://github.com/Cvolton/AeroShotCRE) and is credited to RUvlad1, and the registry artwork is from Microsoft's Windows 11 Registry Editor. See the [assets guide](OpenGlassGUI/Assets/README.md) for source and license details.
- [Banner for OpenGlass](https://github.com/ALTaleX531/OpenGlass/discussions/11) by [@aubymori](https://github.com/aubymori), using [metalheart jawn #2](https://www.deviantart.com/kfh83/art/metalheart-jawn-2-1068250045) by [@kfh83](https://github.com/kfh83)
- [[MS-RDPCR2]: Remote Desktop Protocol: Composited Remoting V2](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpcr2)
- [KNSoft.SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours)
- [VC-LTL](https://github.com/Chuyu-Team/VC-LTL5)
- [Windows Implementation Libraries](https://github.com/Microsoft/wil)
- [libvalinet](https://github.com/valinet/libvalinet), whose symbol download work inspired OpenGlass
- [TranslucentTB](https://github.com/TranslucentTB/TranslucentTB), whose C++ project structure inspired OpenGlass

<br>

If you find SlopGlass valuable, please consider [supporting ALTaleX531 on Ko-fi](https://ko-fi.com/altalex531). This project wouldn't exist without his extensive DWM reverse-engineering, all I did was make some slop changes lol.
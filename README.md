# GGXrdChangeBorderlessWindowPos

## What is the problem

Usually, when you have multiple monitors, you can just un-maximize the window off the wrong monitor, drag it to the right one and maximimze it on that one. That usually works with most apps.

With Guilty Gear Xrd Rev 2 this does not work, because:

1) It has no maximize button;
2) The built-in settings have drawbacks that prevent this:
    1) Fullscreen mode - always goes fullscreen only on the first, or main, monitor;
    2) Windowed mode - cannot be resized or maximized;
    3) Fullscreen windowed mode - cannot be moved, resized, and always goes on the first, or main, monitor.

If you drag the game's window while it's in Windowed mode to another mmonitor, then switch to either Fullscreen or Windowed Fullscreen, it will go back to the first monitor.

## Description

In multi-monitor setups, allows to change which monitor Guilty Gear Xrd Rev 2 appears on. For both fullscreen and windowed fullscreen modes.

Work for version 2211 of the game (displayed in the bottom right corner right after starting the game).

## How to use

1) Go to Releases section <https://www.github.com/kkots/GGXrdChangeBorderlessWindowPos/releases/latest>;
2) Download the .EXE file from the Releases;
3) Close the game;
4) Launch the downloaded .EXE;
5) Select which monitor the game should appear on by pressing "Move Xrd Here";
6) You may close the mod. Launch the game and it should appear on that monitor.

You may patch the same EXE repeatedly without swapping to a backup each time.

Additionally, and you could've done this without this mod, you can go to the game's folder, edit the `GUILTY GEAR Xrd -REVELATOR-\REDGame\Config\REDSystemSettings.ini` file and change ResX and ResY in it. This will allow you to set the resolution to whatever you like.

If you start having weird glitches like the fills of menu buttons going outside of their edges, set `MaxMultiSamples=1`.

If you unplug one of your monitors, the game may no longer work if it's patched. In that case, patch it again or restore from a backup copy. Backup copies are saved automatically in the game's Binaries/Win32 folder every time the mod performs a patch.

## How to run on Linux

There are reports this does not work on Nix and weird Fedora setups.

If you're running the game under Steam Proton on Ubuntu, this may work for you.

You can use the `launch_GGXrdChangeBorderlessWindowPos_linux.sh` script to launch the Windows EXE under Wine in the same environment as GuiltyGearXrd.exe. In order to detect the right environment, Xrd must be running. So run it, then launch the script, then you will need to close the game in order to do the patching.

```bash
chmod u+x launch_GGXrdChangeBorderlessWindowPos_linux.sh
./launch_GGXrdChangeBorderlessWindowPos_linux.sh
```

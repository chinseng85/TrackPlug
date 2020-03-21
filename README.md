![Screenshot](https://i.imgur.com/O1G0EUq.png)
# What is this?
A plugin that keeps track of how long you played your games, and an app to show you the values.

# How is this "Better"?
- As it's rewritten as a kernel plugin, the files that record the playtime won't ever get corrupted.
- Complete support for Adrenaline and Adrenaline Bubbles.
- The app works at 60FPS instead of 30FPS.

# How do I install this?
Just put BetterTrackPlug.skprx under *KERNEL in config.txt
```
*KERNEL
ur0:tai/BetterTrackPlug.skprx
```
and install the vpk.

# Features To Be Added
- Blacklist support so you don't have to see how much time you've spent on VitaShell.

# Notes
![Bubbles](https://i.imgur.com/qZwPMXU.png)
I would advise you to set up your bubbles in a way that their title ID's will be the same as the corresponding PSP game's title ID instead of the default PSPEMUXXX, this way, they will use the same file that stores the playtime and you won't see the game twice on the list if you launch it both from bubble or directly from adrenaline.

As it's currently not possible for me to extract icons from inside Adrenaline, games launched directly from Adrenaline won't have any icons if there is no corresponding bubble for the said game. I also don't think it would be efficient even if I knew how to do it.

Please let me know if you see any bugs, it would be extremely helpful.
# Credits
- Special thanks to **teakhanirons**, [dots-tb](https://github.com/dots-tb), [cuevavirus](https://github.com/cuevavirus/) and Team CBPS for helping me make this plugin and not losing their minds because of my questions. Without their help I wouldn't even know where to start.
- [Rinnegatamante](https://github.com/Rinnegatamante) for [LPP-Vita](https://github.com/Rinnegatamante/lpp-vita) and making the idea of TrackPlug in the first place.
- [Electry](https://github.com/Electry/) for his code chunk responsible for getting titles inside Adrenaline.
- **ecamci** for making the assets.
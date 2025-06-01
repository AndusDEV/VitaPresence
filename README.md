# VitaPresence
> [!NOTE]
> The only change in this fork is that it uses TITLEID's of Vita games and gets the icon from Discord Dev Portal instead of icon0.png from Vita directly

Change your Discord rich presence to your currently playing PS Vita game!

Inspired by [SwitchPresence](https://github.com/Sun-Research-University/SwitchPresence-Rewritten)
<br>

![vitapresence](https://github.com/user-attachments/assets/03117968-b8a8-4141-a34b-33635efa2243)

Works with PSVita & Adrenaline (including custom bubbles) games/apps

## Disclaimer
The client app (on Windows PC) must be running in the background, and the PC must be on the same network as your Vita.

It would be nice to have rich presence working with only the Vita itself, but this isn't currently possible due to Discord's RPC API restrictions.

## Setup
- Install the .skprx plugin within the `*KERNEL` section of your taiHEN config.txt
- Create an application at the [Discord Developer Portal](https://discordapp.com/developers/applications/), call your application `PS Vita` or whatever you would like.
- Add a Rich Presence Assets and name it `playstation_app_icon`, you can use [this image](https://upload.wikimedia.org/wikipedia/commons/9/91/PlayStation_App_Icon.jpg) or anything you'd like. It will be used if VitaPresence fails to fetch icon0.png from the Vita for one reason or another.
- Forward port 51966 of your Vita, how you can do that varies from one router to another so I'll let you do some research.
- Enter your client ID and Vita's IP or MAC address into the VitaPresence client and you're done!
<br>

## Credits
- [Electry](https://github.com/Electry) for the original VitaPresence

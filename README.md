# Sockfish

Sockfish is a chess program written in C.

Overview:
- sdl
  - board
  - ui
  - engine (bridge-to-sockfish)
- uci
- sockfish (engine-itself)

## Build

Build and run:
```
git clone https://github.com/DrShahinstein/sockfish.git
cd sockfish/
bash build.sh

# run sdl interface
bash run.sh

# run uci protocol
bash run.sh uci
```

Dependencies: SDL3
Installation below...

### Linux

#### pacman
```
sudo pacman -S sdl3 sdl3_ttf sdl3_image
```

#### dnf
```
sudo dnf install SDL3 SDL3_image SDL3_ttf
```

#### apt
```
sudo apt install libsdl3-dev libsdl3-image-dev libsdl3-ttf-dev
```

Just in case: [manual sdl installation](manual-sdl-installation.md).

### Macos

```bash
brew install sdl3 sdl3_image sdl3_ttf
```

### Windows (MSYS2)

The file system in Windows is case-insensitive, which means files like q.png and Q.png are treated as the same file.

So, you would first need to fix this jerky Windows issue. Create an empty folder "Sockfish" by yourself and make it case-sensitive:

```
# supervisor windows termnial
fsutil.exe file setCaseSensitiveInfo C:\YourPath\Sockfish enable
```

Clone the repo into that directory. Keep going with msys2 --**mingw**-- terminal.

```
cd Sockfish 

pacman -S                   \
git                         \
mingw-w64-x86_64-gcc        \
mingw-w64-x86_64-sdl3       \
mingw-w64-x86_64-sdl3-image \
mingw-w64-x86_64-sdl3-ttf   \
mingw-w64-x86_64-meson      \
mingw-w64-x86_64-python     \
mingw-w64-x86_64-ninja      \

git clone https://github.com/DrShahinstein/sockfish.git
cd sockfish
bash build.sh
bash run.sh
```

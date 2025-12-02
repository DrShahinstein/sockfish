If any fellow stumbles upon the repo, it's still very much in development. Honestly it's something very wholesome and fun to work on.

quick refs...
* https://wiki.libsdl.org/SDL3/README-linux
* https://wiki.libsdl.org/SDL3/CategoryAPI

## Build

Build and run:
```
git clone https://github.com/DrShahinstein/sockfish.git
cd sockfish/
bash build.sh
bash run.sh
```

Dependencies below...

### Linux

#### pacman
```
sudo pacman -S sdl3 sdl3_ttf
sudo yay -S sdl3_image
```

#### dnf
```
sudo dnf install SDL3 SDL3_image SDL3_ttf
```

#### apt

In apt packages, there is no official `sdl3`, `sdl3_image` and `sdl3_tff` packages released yet.

First, see [https://wiki.libsdl.org/SDL3/README-linux](https://wiki.libsdl.org/SDL3/README-linux).

Then,

```
mkdir -p ~/sdl3_build
cd ~/sdl3_build

git clone https://github.com/libsdl-org/SDL.git SDL3
git clone https://github.com/libsdl-org/SDL_image.git SDL3_image
git clone https://github.com/libsdl-org/SDL_ttf.git SDL3_ttf

cd SDL3
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install

cd ../../SDL3_image
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install

cd ../../SDL3_ttf
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install

sudo ldconfig
```

To verify:
```
pkg-config --modversion sdl3        # 3.3.0 (my version)
pkg-config --modversion sdl3-image  # 3.3.0 (my version)
pkg-config --modversion sdl3-ttf    # 3.3.0 (my version)
```

### Macos

```bash
brew install sdl3 sdl3_image sdl3_ttf meson gcc
```

### Windows (MSYS2)

The file system in Windows is case-insensitive, which means files like q.png and Q.png are treated as the same file.

So, you would first need to fix this jerky Windows issue. Create an empty folder "Sockfish" by yourself and make it case-sensitive:

```
# supervisor windows termnial
fsutil.exe file setCaseSensitiveInfo C:\YourPath\Sockfish enable
```

You will clone the repo into that directory. Keep going with msys2 **mingw** terminal.

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

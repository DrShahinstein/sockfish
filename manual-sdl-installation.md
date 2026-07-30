First, see [https://wiki.libsdl.org/SDL3/README-linux](https://wiki.libsdl.org/SDL3/README-linux) and install the required packages.

Then:
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
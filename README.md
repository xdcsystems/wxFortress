# wxFortress
It's a small gaming app, based on sharing OpenGL, OpenAL, wxWidgets, and FFMPEG libraries.</br>
It is specifically designed and optimized for running on slow computers. Only the OpenGL 2.0+ driver support is required to work.</br>
It's a fully cross-platform app. Windows and Linux builds are currently being tested.</br>
Thanks to used libraries, application can be easily ported to mobile platforms too.</br>
CMake build system is used to build the project.</br>
Automated googletests unit tests are connected to the build process.</br>
You can see work of an already assembled application, taking files from the .bin folder</br>
</br>
</br>
</br>
## Build
First of all, need clone repo with submodules:

    ```git clone --recursive https://github.com/xdcsystems/wxFortress.git```

__For build on linux need:__
1. Install gtk 3.0: sudo apt-get install libgtk-3-dev
2. Extract ffmpeg from archive: 
     2.1. cd to "external/ffmpeg/linux/"
     2.2. tar -xvzf ./ffmpeg_4.3.tar.gz
3. Create build folder at root of project and go to it
4. Configure project. FFMPEG will build at this stage.
     ```cmake -DCMAKE_BUILD_TYPE=Release ../```
5. Build project
     ```cmake --build . -j5```

__For build on Windows need:__
1. This project is guaranteed to build using the Visual Studio 2019 IDE. Build on other IDE is not guaranteed.
2. CMake build system used for initially configure project
3. Run build.bat , it will create VS 2019 Project (Debug by default - you can change this in .bat file)
    Configured project will be placed at x86 folder
4. Open solution from x86/wxFortress.sln and build it



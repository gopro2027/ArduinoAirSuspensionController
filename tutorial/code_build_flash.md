

### ESP32 Flashing Steps
# Cloning the project code
1. Use git to clone the project normally. It is recommended you clone it and not download it, so that you can get future updates easily.
2. Here is what I recommend if you would like to easily clone the project, and what I will reference in the rest of this documentation:
    1. Dowload and install [github desktop](https://desktop.github.com/download/)
    2. Sign in to your github account
    3. Click clone project and clone `https://github.com/gopro2027/ArduinoAirSuspensionController.git`
    4. You can now choose branches, make your own local branches, pull updates, ect, all from within the GUI of github desktop
# Building and flashing
1. Download and install [visual studio code](https://code.visualstudio.com/)
2. In visual studio code go to extensions (4 cubes icon on lefthand bar) and install "PlatformIO IDE"
3. Open PlatformIO by clicking the alien icon on the lefthand bar and making sure it fully installs
4. Open the project:
    1. Double click the file `OASMan_ESP32.code-workspace` in `ArduinoAirSuspensionController/OASMan_ESP32/` and wait for all of the project dependencies to automatically download and install via platformio
    2. Tip: In github desktop at the top you can click `Repository` -> `Show in explorer` and then navigate into the folder `OASMan_ESP32` and you will see the file `OASMan_ESP32.code-workspace`
5. Build the project and test for errors:
    1. look in the top right corner and you should see a checkmark that says 'Build' when you hover over it. Click that and verify for no errors.
6. Install the project onto your esp32 by clicking the dropdown arrow next to the build button and choosing "Upload"
# Editing the code/config
1. In github desktop, create a new branch and call it `my_config` or whatever you want
2. Go into the file `user_defines.h` and make necessary edits.
3. Build and test your changes to make sure it is to your liking
4. Once you are finished with your edits, go back to github desktop and commit them to your `my_config` branch.
# Updating to the latest updates
1. Open github desktop
2. Select your `my_config` branch as your current branch or `main` if you have no custom configuration
3. Click `Fetch` in the top right
4. At the top click `Branch` -> `Update from main` and your code will be updated
5. If there is any merge conflicts you will need to fix them at this point. Feel free to ask the discord if you need help!
6. Now you can flash to your esp32 with the new update without loosing any custom configurations.

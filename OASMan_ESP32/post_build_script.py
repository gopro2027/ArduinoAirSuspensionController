Import("env")
import shutil, os

# print("Current CLI targets", COMMAND_LINE_TARGETS)
# print("Current Build targets", BUILD_TARGETS)

def isInBuildFlags(env, f):
    build_flags = env.GetProjectOption("build_flags")
    for v in build_flags:
        if f in v:
            return True
    return False

def post_program_action(source, target, env):
    #print("Program has been built!")
    #print("Build flags: ", env['upload_port'])
    #print(env.Dump())
    prod = isInBuildFlags(env, "OFFICIAL_RELEASE")
    acc_functionality = isInBuildFlags(env, "ACCESSORY_WIRE_FUNCTIONALITY")
    # print("is prod: ",prod)
    # print("acc wire: ",acc)
    
    # program_path = target[0].get_abspath()
    # print("Program path", program_path)
    # Use case: sign a firmware, do any manipulations with ELF, etc
    # env.Execute(f"sign --elf {program_path}")

    if prod:
        print("\n\nPost execution prod script running:")
        path = '../../gopro2027.github.io/oasman/firmware/manifold'
        #if acc_functionality == False:
            #path = '../../gopro2027.github.io/oasman/firmware/manifold_no_acc'
        if os.path.exists(path):
            print("new path: ",path)
            print("location of bins: ",target[0].get_dir())
            fw = str(target[0].get_dir())+"/firmware.bin"
            partitions = str(target[0].get_dir())+"/partitions.bin"
            bootloader = str(target[0].get_dir())+"/bootloader.bin"
            shutil.copy2(fw, path)
            #shutil.copy2(partitions, path)
            #shutil.copy2(bootloader, path)
        else:
            print("Github.io path does not exists, not copying files")
        

env.AddPostAction("$BUILD_DIR/firmware.bin", post_program_action)

#
# Upload actions
#

# def before_upload(source, target, env):
#     print("before_upload")
#     # do some actions

#     # call Node.JS or other script
#     env.Execute("node --version")


# def after_upload(source, target, env):
#     print("after_upload")
#     # do some actions

# env.AddPreAction("upload", before_upload)
# env.AddPostAction("upload", after_upload)

#
# Custom actions when building program/firmware
#

# env.AddPreAction("buildprog", callback...)
# env.AddPostAction("buildprog", callback...)

#
# Custom actions for specific files/objects
#

# env.AddPreAction("$PROGPATH", callback...)
# env.AddPreAction("$BUILD_DIR/${PROGNAME}.elf", [callback1, callback2,...])
# env.AddPostAction("$BUILD_DIR/${PROGNAME}.hex", callback...)

# custom action before building SPIFFS image. For example, compress HTML, etc.
# env.AddPreAction("$BUILD_DIR/spiffs.bin", callback...)

# custom action for project's main.cpp
# env.AddPostAction("$BUILD_DIR/src/main.cpp.o", callback...)

# Custom HEX from ELF
# env.AddPostAction(
#     "$BUILD_DIR/${PROGNAME}.elf",
#     env.VerboseAction(" ".join([
#         "$OBJCOPY", "-O", "ihex", "-R", ".eeprom",
#         "$BUILD_DIR/${PROGNAME}.elf", "$BUILD_DIR/${PROGNAME}.hex"
#     ]), "Building $BUILD_DIR/${PROGNAME}.hex")
# )
MACOSX_ARCHITECURES = -arch i386 -arch x86_64
MACOSX_SYS_ROOT = -isysroot  /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
MACOSX_MIN_VERSION = -mmacosx-version-min=10.6
MACOSX_COMPILER_SETTINGS = -I/usr/local/include
MACOSX_LINKER_SETTINGS = -L/usr/local/lib
MACOSX_CC = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/cc
MACOSX_CXX = /Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/c++

# compile for MacOS
#CC = $(MACOSX_CC)
#CFLAGS = $(MACOSX_SYS_ROOT) $(MACOSX_MIN_VERSION) $(MACOSX_COMPILER_SETTINGS) -I../include -O2
#LFLAGS = $(MACOSX_LINKER_SETTINGS)
#PLATFORM_LIBRARIES = -lallegro_main-static -framework IOKit -framework AudioToolbox -framework OpenGL -framework OpenAL -framework Cocoa
#PLATFORM_OBJECTS = ../src/macos/coremidi.o

# compile for Linux
CC = gcc
CFLAGS = -I../include -O2
LFLAGS =
PLATFORM_LIBRARIES = -lasound
PLATFORM_OBJECTS = ../src/unix/alsa.o

EX_PLAY_MIDI_OFILES = ex_play_midi.o rtk/io_allegro.o rtk/io.o rtk/midi.o ../src/midia5.o $(PLATFORM_OBJECTS)
EX_PLAY_MIDI_LIBRARIES = `pkg-config --libs --static allegro-static-5` $(PLATFORM_LIBRARIES)

ex_play_midi: $(EX_PLAY_MIDI_OFILES)
	$(CC) $(EX_PLAY_MIDI_OFILES) $(MACOSX_LINKER_SETTINGS) $(EX_PLAY_MIDI_LIBRARIES) -o ex_play_midi

clean:
	rm $(EX_PLAY_MIDI_OFILES)

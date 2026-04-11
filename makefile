#Mouse Injector Makefile
#Use with TDM-GCC 4.9.2-tdm-3 or MinGW on Windows
#On Linux: just run 'make'

#Source directories
SRCDIR = ./
MANYMOUSEDIR = $(SRCDIR)manymouse/
GAMESDIR = $(SRCDIR)games/
OBJDIR = $(SRCDIR)obj/

#Detect platform
ifeq ($(OS),Windows_NT)
    #Compiler directories
    #MINGWDIR = C:/Dev/Dev-Cpp/MinGW64/bin/
    MINGWDIR = C:/msys64/mingw64/bin/
    CC = $(MINGWDIR)gcc
    WINDRES = $(MINGWDIR)windres
    EXENAME = "$(SRCDIR)Mouse Injector.exe"
    RESFLAGS = --target=pe-x86-64 --input-format=rc -O coff
    PLATFORM_OBJS = $(OBJDIR)windows_wminput.o $(OBJDIR)icon.res $(OBJDIR)export.o
    LIBS = -static-libgcc -lpsapi -lwinmm
    PLATFORM_LFLAGS = -m64 -s
else
    CC = gcc
    EXENAME = $(SRCDIR)mouseinjector
    PLATFORM_OBJS = $(OBJDIR)linux_evdev.o $(OBJDIR)export.o
    LIBS = -lX11 -lm
    PLATFORM_LFLAGS = -s
endif

#Compiler flags
CFLAGS = -O2 -std=c99 -D_DEFAULT_SOURCE -Wall
WFLAGS = -Wextra -pedantic -Wno-parentheses

#Linker flags
OBJS = $(OBJDIR)main.o $(OBJDIR)memory.o $(OBJDIR)mouse.o $(OBJDIR)manymouse.o $(PLATFORM_OBJS)
GAMEOBJS = $(patsubst $(GAMESDIR)%.c, $(OBJDIR)%.o, $(wildcard $(GAMESDIR)*.c))
LFLAGS = $(OBJS) $(GAMEOBJS) -o $(EXENAME) $(LIBS) $(PLATFORM_LFLAGS)

#Main recipes
mouseinjector: $(OBJS) $(GAMEOBJS)
	$(CC) $(LFLAGS)

all: clean mouseinjector

#Individual recipes
$(OBJDIR)main.o: $(SRCDIR)main.c $(SRCDIR)main.h $(SRCDIR)memory.h $(SRCDIR)mouse.h $(GAMESDIR)game.h
	$(CC) -c $(SRCDIR)main.c -o $(OBJDIR)main.o $(CFLAGS) $(WFLAGS)
	
$(OBJDIR)export.o: $(SRCDIR)export.c $(SRCDIR)export.h
	$(CC) -c $(SRCDIR)export.c -o $(OBJDIR)export.o $(CFLAGS) $(WFLAGS)
	
$(OBJDIR)memory.o: $(SRCDIR)memory.c $(SRCDIR)memory.h $(SRCDIR)main.h $(SRCDIR)export.h
	$(CC) -c $(SRCDIR)memory.c -o $(OBJDIR)memory.o $(CFLAGS) $(WFLAGS)

$(OBJDIR)mouse.o: $(SRCDIR)mouse.c $(SRCDIR)mouse.h $(MANYMOUSEDIR)manymouse.h
	$(CC) -c $(SRCDIR)mouse.c -o $(OBJDIR)mouse.o $(CFLAGS) $(WFLAGS)

$(OBJDIR)manymouse.o: $(MANYMOUSEDIR)manymouse.c $(MANYMOUSEDIR)manymouse.h
	$(CC) -c $(MANYMOUSEDIR)manymouse.c -o $(OBJDIR)manymouse.o $(CFLAGS) $(WFLAGS)

ifeq ($(OS),Windows_NT)
$(OBJDIR)windows_wminput.o: $(MANYMOUSEDIR)windows_wminput.c $(MANYMOUSEDIR)manymouse.h
	$(CC) -c $(MANYMOUSEDIR)windows_wminput.c -o $(OBJDIR)windows_wminput.o $(CFLAGS)

$(OBJDIR)icon.res: $(SRCDIR)icon.rc $(SRCDIR)icon.ico
	$(WINDRES) -i $(SRCDIR)icon.rc -o $(OBJDIR)icon.res $(RESFLAGS)
else
$(OBJDIR)linux_evdev.o: $(MANYMOUSEDIR)linux_evdev.c $(MANYMOUSEDIR)manymouse.h
	$(CC) -c $(MANYMOUSEDIR)linux_evdev.c -o $(OBJDIR)linux_evdev.o $(CFLAGS) $(WFLAGS)
endif

#Game drivers recipe
$(OBJDIR)%.o: $(GAMESDIR)%.c $(SRCDIR)main.h $(SRCDIR)memory.h $(SRCDIR)mouse.h $(GAMESDIR)game.h
	$(CC) -c $< -o $@ $(CFLAGS) $(WFLAGS)

clean:
ifeq ($(OS),Windows_NT)
	rm -f $(SRCDIR)*.exe $(OBJDIR)*.o $(OBJDIR)*.res $(SRCDIR)*.ini
else
	rm -f $(EXENAME) $(OBJDIR)*.o
endif

ifneq ($(OS),Windows_NT)
setup-perms:
	@echo "Setting up permissions to run without sudo..."
	@echo 'KERNEL=="uinput", MODE="0660", GROUP="input"' | sudo tee /etc/udev/rules.d/99-uinput.rules
	sudo udevadm control --reload-rules && sudo udevadm trigger
	sudo usermod -aG input $(USER)
	@echo ""
	@echo "Done. Log out and back in for group changes to take effect."
	@echo "After that, run 'make setcap' each time you rebuild."

setcap: $(EXENAME)
	sudo setcap cap_sys_ptrace=ep $(EXENAME)
	@echo "Capability set. Run with: ./mouseinjector 2>debug.log"
endif
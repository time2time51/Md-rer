# --- Minimal SGDK makefile ---
# ROM name (output will be out/rom.bin)
PROJECT_NAME := rom

# Sources & resources folders
SRC := src
RES := res

# Include SGDK rules from the Docker image
include /sgdk/makefile.gen

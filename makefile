# --- Projet SGDK minimal ---
# nom de la ROM (sans extension)
TARGET := rom

# dossiers du projet
SRCDIR := src
INCDIR := inc
RESDIR := res
BINDIR := out
LIBDIR :=

# Active la compression LZ4W des ressources (optionnel)
RESCOMPFLAGS := -z

# Inclut le makefile générique de SGDK dans le conteneur
SGDK := /sgdk
include $(SGDK)/makefile.gen

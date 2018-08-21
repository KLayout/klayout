.PHONY: help build deploy test dropbox-deploy

GITCOMMIT := $(shell git rev-parse --short HEAD)

.ONESHELL:

default: help

help:
	@echo "For Mac OS only"
	@echo "make build PYTHON_VERSION=B37"
	@echo "make deploy PYTHON_VERSION=B37"
	@echo "make test MACOS_VERSION=HighSierra"
	@echo "Valid Mac OS Versions: [Yosemite, ElCapitan, Sierra, HighSierra]"
	@echo "Valid Python Version: [nil, Sys, B37]"

build:
	@echo "Building for Mac $(GITCOMMIT)"
	./build4mac.py -p $(PYTHON_VERSION) -q Qt5Brew -c; \
	./build4mac.py -p $(PYTHON_VERSION) -q Qt5Brew

deploy: build
	@echo "Deploying 4 Mac $(GITCOMMIT)"
	./build4mac.py -p $(PYTHON_VERSION) -q Qt5Brew -y
	
test: deploy
	@echo "Testing 4 Mac $(GITCOMMIT)"
	qt5.pkg.macos-$(MACOS_VERSION)-release/klayout.app/Contents/MacOS/klayout -b -r test-pylib-script.py; \
	cd qt5.build.macos-$(MACOS_VERSION)-release; \
	ln -s klayout.app/Contents/MacOS/klayout klayout; \
	export TESTTMP=testtmp; \
	export TESTSRC=..; \
	./ut_runner -h; \
#	./ut_runner || true; \
	cd ..;

dropbox-deploy: test
	@echo "Preparing for dropbox deployment $(MACOS_VERSION) $(GITCOMMIT)"
	mkdir deploy; \
	pwd; \
	ls -lah; \
	touch build.txt; \
	cp build.txt deploy; \
	tar czf "deploy/qt5.pkg.macos-$(MACOS_VERSION)-release-$(GITCOMMIT).tar.gz" qt5.pkg.macos-$(MACOS_VERSION)-release

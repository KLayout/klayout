.PHONY: help build deploy test dropbox-deploy

GITCOMMIT := $(shell git rev-parse --short HEAD)
KLAYOUT_VERSION := $(shell source version.sh && echo $$KLAYOUT_VERSION)
ifndef PYTHON_VERSION
    PYTHON_VERSION := HB38
endif
ifndef MACOS_VERSION
	MACOS_VERSION := Catalina
endif

.ONESHELL:

default: help

help:
	@echo "For Mac OS only"
	@echo "make build PYTHON_VERSION=HB38"
	@echo "make deploy PYTHON_VERSION=HB38"
	@echo "make test MACOS_VERSION=HighSierra"
	@echo "Valid Mac OS Versions: [Yosemite, ElCapitan, Sierra, HighSierra, Mojave, Catalina]"
	@echo "Valid Python Version: [nil, Sys, HB38]"

build:
	@echo "Building for Mac $(GITCOMMIT)"
	./build4mac.py -p $(PYTHON_VERSION) -q Qt5Brew -c; \
	./build4mac.py -p $(PYTHON_VERSION) -q Qt5Brew

deploy:
	@echo "Deploying 4 Mac $(GITCOMMIT)"
	./build4mac.py -p $(PYTHON_VERSION) -q Qt5Brew -y

test:
	@echo "Testing 4 Mac $(GITCOMMIT)"
	PIP_REQUIRE_VIRTUALENV="false" HW-qt5Brew.pkg.macos-$(MACOS_VERSION)-release-RsysPhb38/klayout.app/Contents/MacOS/klayout -b -r test-pylib-script.py; \
	cd qt5Brew.build.macos-$(MACOS_VERSION)-release-RsysPhb38; \
	ln -s klayout.app/Contents/MacOS/klayout klayout; \
	export TESTTMP=testtmp; \
	export TESTSRC=..; \
	export DYLD_LIBRARY_PATH=.:db_plugins/:lay_plugins/; \
	./ut_runner -h || true; \
	cd ..

dmg-template:
	mkdir -p testtemplate/klayout.app
	./makeDMG4mac.py -p testtemplate -m -z -t klayoutDMGTemplate.dmg
	cp -a klayoutDMGTemplate.dmg* macbuild/Resources/
	rm -Rf testtemplate

dropbox-deploy:
	@echo "Preparing for dropbox deployment $(MACOS_VERSION) $(GITCOMMIT)"
	mkdir -p deploy/$(MACOS_VERSION)/$(PYTHON_VERSION)/$(KLAYOUT_VERSION)
	pwd
	ls -lah
	touch build.txt
	cp build.txt deploy/$(MACOS_VERSION)/$(PYTHON_VERSION)/$(KLAYOUT_VERSION)/qt5.pkg.macos-$(MACOS_VERSION)-$(PYTHON_VERSION)-release-$(KLAYOUT_VERSION)-$(GITCOMMIT).log.txt
	hdiutil convert macbuild/Resources/klayoutDMGTemplate.dmg -ov -format UDRW -o work-KLayout.dmg
	hdiutil resize -size 500m work-KLayout.dmg
	hdiutil attach -readwrite -noverify -quiet -mountpoint tempKLayout -noautoopen work-KLayout.dmg
	cp -a HW-qt5Brew.pkg.macos-$(MACOS_VERSION)-release-RsysPhb38/ tempKLayout/
	hdiutil detach tempKLayout
	hdiutil convert work-KLayout.dmg -ov -format UDZO -imagekey zlib-level=9 -o deploy/$(MACOS_VERSION)/$(PYTHON_VERSION)/$(KLAYOUT_VERSION)/qt5.pkg.macos-$(MACOS_VERSION)-$(PYTHON_VERSION)-release-$(KLAYOUT_VERSION)-$(GITCOMMIT).dmg
	md5 -q deploy/$(MACOS_VERSION)/$(PYTHON_VERSION)/$(KLAYOUT_VERSION)/qt5.pkg.macos-$(MACOS_VERSION)-$(PYTHON_VERSION)-release-$(KLAYOUT_VERSION)-$(GITCOMMIT).dmg > deploy/$(MACOS_VERSION)/$(PYTHON_VERSION)/$(KLAYOUT_VERSION)/qt5.pkg.macos-$(MACOS_VERSION)-$(PYTHON_VERSION)-release-$(KLAYOUT_VERSION)-$(GITCOMMIT).dmg.md5
	rm work-KLayout.dmg

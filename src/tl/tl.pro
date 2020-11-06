
TEMPLATE = subdirs
SUBDIRS = tl unit_tests
*bsd* {
    LIBS += -lexecinfo
}

unit_tests.depends += tl


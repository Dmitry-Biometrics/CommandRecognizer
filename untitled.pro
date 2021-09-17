#-------------------------------------------------
#
# Project created by QtCreator 2016-05-23T12:34:34
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

QT       += multimedia

#CONFIG += console

QMAKE_CXXFLAGS += -Wall -std=c++11

include(audio/audio.pri)

TARGET = untitled
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    citis/VoiceSplitter.cpp \
    citis/AudioFormat.cpp \
    citis/VoiceRecognizer.cpp \
    lbnt/CSpeechRecog.cpp

HEADERS  += mainwindow.h \
    citis/VoiceSplitter.h \
    citis/AudioFormat.h \
    citis/VoiceRecognizer.h \
    lbnt/CSpeechRecog.h

FORMS    += mainwindow.ui

INCLUDEPATH += C:\QtProjects\CMUSphinx\sphinxbase-5prealpha\sphinxbase-5prealpha\include\
INCLUDEPATH += C:\QtProjects\CMUSphinx\pocketsphinx-5prealpha\pocketsphinx-5prealpha\include\

LIBS += C:\QtProjects\CMUSphinx\pocketsphinx-5prealpha\pocketsphinx-5prealpha\bin\Release\Win32\pocketsphinx.lib
LIBS += C:\QtProjects\CMUSphinx\sphinxbase-5prealpha\sphinxbase-5prealpha\bin\Release\Win32\sphinxbase.lib

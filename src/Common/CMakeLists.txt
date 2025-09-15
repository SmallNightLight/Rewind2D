cmake_minimum_required(VERSION 3.27)
project(Common LANGUAGES CXX)

add_library(Common INTERFACE
    Log.h
    Action/Action.h
    Action/ActionManager.h
    Action/ActionCollection.h
)
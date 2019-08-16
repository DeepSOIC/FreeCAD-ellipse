
SET(Show_SRCS
    __init__.py
    Containers.py
    DepGraphTools.py
    FrozenClass.py
    SceneDetail.py
    TempoVis.py
    TVObserver.py
    TVStack.py
    Utils.py
    SceneDetails/__init__.py
    SceneDetails/Camera.py
    SceneDetails/ClipPlane.py
    SceneDetails/Pickability.py
    SceneDetails/VProperty.py
)

SOURCE_GROUP("" FILES ${Show_SRCS})

ADD_CUSTOM_TARGET(Show ALL
    SOURCES ${Show_SRCS}
)

fc_copy_sources(Show "${CMAKE_BINARY_DIR}/Mod/Show" ${Show_SRCS})

INSTALL(
    FILES
        ${Show_SRCS}
    DESTINATION Mod/Show
)

cmake_minimum_required(VERSION 3.3)

project(XenoMax VERSION 1.2)

set(TARGETEX_LOCATION 3rd_party/XenoLib/3rd_party/PreCore/cmake)
include(${TARGETEX_LOCATION}/3dsmax.cmake)

set (XenoLibLibraryPath ../XenoLib_${CMAKE_GENERATOR_PLATFORM}_${CHAR_TYPE})

add_subdirectory(3rd_party/XenoLib ${XenoLibLibraryPath})

build_target(
	TYPE SHARED
	SOURCES
		src/DllEntry.cpp
		src/XenoImp.cpp
		src/XenoImport.cpp
	    src/XenoImp.rc
		src/XenoMax.def
		${MAX_EX_DIR}/win/About.rc
	LINKS
		gdiplus bmm core Morpher XenoLib flt mesh maxutil maxscrpt paramblk2 geom
	DEFINITIONS
		${MaxDefinitions}
	INCLUDES
		${MaxSDK}/include
		3rd_party/XenoLib/include
		3rd_party/XenoLib/3rd_party/PreCore
	LINK_DIRS
		${MaxSDKLibrariesPath}
	AUTHOR "Lukas Cone"
	DESCR "Xenoblade Engine 3DS Max Plugin"
	NAME "XenoMax"
	START_YEAR 2017
	PROPERTIES
		SUFFIX .dlu
		${MaxProperties}
)

build_morpher()
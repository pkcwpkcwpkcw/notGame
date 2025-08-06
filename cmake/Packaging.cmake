# Packaging.cmake - CPack 패키징 설정

message(STATUS "Configuring packaging...")

# CPack 기본 설정
set(CPACK_PACKAGE_NAME "NotGame")
set(CPACK_PACKAGE_VENDOR "NotGame Team")
set(CPACK_PACKAGE_DESCRIPTION "High-performance NOT gate logic circuit sandbox game")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Build complex logic circuits using only NOT gates")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/pkcwpkcwpkcw/notGame")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "NotGame")

# 리소스 파일 경로
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# 플랫폼별 패키지 생성기 설정
if(WIN32)
    # Windows 패키징
    set(CPACK_GENERATOR "ZIP;NSIS")
    
    # NSIS 설정
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_DISPLAY_NAME "NotGame")
    set(CPACK_NSIS_PACKAGE_NAME "NotGame - NOT Gate Sandbox")
    set(CPACK_NSIS_MUI_ICON "${CMAKE_SOURCE_DIR}/assets/icons/notgame.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_SOURCE_DIR}/assets/icons/notgame.ico")
    set(CPACK_NSIS_HELP_LINK "https://github.com/pkcwpkcwpkcw/notGame")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/pkcwpkcwpkcw/notGame")
    set(CPACK_NSIS_CONTACT "support@notgame.example.com")
    set(CPACK_NSIS_MODIFY_PATH ON)
    
    # 시작 메뉴 바로가기
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\NotGame.lnk' '$INSTDIR\\\\bin\\\\notgame.exe'"
    )
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\NotGame.lnk'"
    )
    
    # Windows 실행에 필요한 DLL 포함
    if(SDL2_FOUND AND DEFINED SDL2_LIBRARY)
        # SDL2.dll 복사
        if(TARGET SDL2::SDL2)
            # Modern CMake 타겟 사용
            install(FILES $<TARGET_FILE:SDL2::SDL2> DESTINATION bin OPTIONAL)
        elseif(EXISTS "${SDL2_LIBRARY}")
            get_filename_component(SDL2_DLL_DIR "${SDL2_LIBRARY}" DIRECTORY)
            file(GLOB SDL2_DLLS "${SDL2_DLL_DIR}/../bin/*.dll")
            if(SDL2_DLLS)
                install(FILES ${SDL2_DLLS} DESTINATION bin)
            endif()
        endif()
    endif()
    
    # Visual C++ 재배포 가능 패키지
    include(InstallRequiredSystemLibraries)
    
elseif(APPLE)
    # macOS 패키징
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    
    # DMG 설정
    set(CPACK_DMG_VOLUME_NAME "NotGame")
    set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/DMGSetup.scpt")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/assets/dmg_background.png")
    
    # 번들 설정
    set(CPACK_BUNDLE_NAME "NotGame")
    set(CPACK_BUNDLE_PLIST "${CMAKE_SOURCE_DIR}/Info.plist")
    set(CPACK_BUNDLE_ICON "${CMAKE_SOURCE_DIR}/assets/icons/notgame.icns")
    
elseif(UNIX)
    # Linux 패키징
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    
    # DEB 패키지 설정
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "NotGame Team")
    set(CPACK_DEBIAN_PACKAGE_SECTION "games")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    
    # 의존성 설정
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17)")
    if(SDL2_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libsdl2-2.0-0")
    endif()
    if(OpenGL_FOUND)
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "${CPACK_DEBIAN_PACKAGE_DEPENDS}, libgl1")
    endif()
    
    # RPM 패키지 설정
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Amusements/Games")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17")
    if(SDL2_FOUND)
        set(CPACK_RPM_PACKAGE_REQUIRES "${CPACK_RPM_PACKAGE_REQUIRES}, SDL2")
    endif()
    
    # .desktop 파일 설치
    configure_file(
        "${CMAKE_SOURCE_DIR}/assets/notgame.desktop.in"
        "${CMAKE_BINARY_DIR}/notgame.desktop"
        @ONLY
    )
    install(FILES "${CMAKE_BINARY_DIR}/notgame.desktop"
        DESTINATION share/applications
    )
    
    # 아이콘 설치
    install(FILES "${CMAKE_SOURCE_DIR}/assets/icons/notgame.png"
        DESTINATION share/icons/hicolor/256x256/apps
    )
endif()

# 포함할 파일 설정
set(CPACK_PACKAGE_FILES
    notgame
    assets/
    README.md
    LICENSE
)

# 제외할 파일 패턴
set(CPACK_SOURCE_IGNORE_FILES
    /\\.git/
    /\\.github/
    /\\.vscode/
    /\\.idea/
    /build/
    /cmake-build-.*/
    /CMakeCache\\.txt
    /CMakeFiles/
    /Makefile
    \\.swp$
    \\.DS_Store
    Thumbs\\.db
    *~
)

# 소스 패키지 설정
set(CPACK_SOURCE_GENERATOR "TGZ;ZIP")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}-src")

# 설치 디렉토리 구조
install(TARGETS notgame
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# 리소스 파일 설치
install(DIRECTORY ${CMAKE_SOURCE_DIR}/assets/
    DESTINATION assets
    PATTERN "*.in" EXCLUDE
    PATTERN ".gitkeep" EXCLUDE
)

# 문서 파일 설치
install(FILES 
    ${CMAKE_SOURCE_DIR}/README.md
    ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION .
)

# 개발 헤더 설치 (선택적)
option(INSTALL_DEV_FILES "Install development files" OFF)
if(INSTALL_DEV_FILES)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/src/
        DESTINATION include/notgame
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
    )
endif()

# 컴포넌트 설정
set(CPACK_COMPONENTS_ALL Runtime Assets Documentation)
set(CPACK_COMPONENT_RUNTIME_DISPLAY_NAME "NotGame Executable")
set(CPACK_COMPONENT_RUNTIME_DESCRIPTION "The main NotGame executable")
set(CPACK_COMPONENT_ASSETS_DISPLAY_NAME "Game Assets")
set(CPACK_COMPONENT_ASSETS_DESCRIPTION "Sprites, levels, and other game data")
set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "README and license files")

# CPack 포함
include(CPack)

# 패키징 타겟 추가
add_custom_target(package_source
    COMMAND ${CMAKE_CPACK_COMMAND} --config CPackSourceConfig.cmake
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Creating source package..."
)

# 플랫폼별 패키지 생성 타겟
if(WIN32)
    add_custom_target(package_windows
        COMMAND ${CMAKE_CPACK_COMMAND} -G NSIS
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating Windows installer..."
    )
    
    add_custom_target(package_portable
        COMMAND ${CMAKE_CPACK_COMMAND} -G ZIP
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating portable ZIP package..."
    )
elseif(APPLE)
    add_custom_target(package_dmg
        COMMAND ${CMAKE_CPACK_COMMAND} -G DragNDrop
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating macOS DMG..."
    )
elseif(UNIX)
    add_custom_target(package_deb
        COMMAND ${CMAKE_CPACK_COMMAND} -G DEB
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating Debian package..."
    )
    
    add_custom_target(package_rpm
        COMMAND ${CMAKE_CPACK_COMMAND} -G RPM
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Creating RPM package..."
    )
endif()

message(STATUS "Packaging configured successfully")
message(STATUS "Available generators: ${CPACK_GENERATOR}")
message(STATUS "To create packages, run: cpack or cmake --build . --target package")
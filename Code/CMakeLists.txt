
# Currently we are in the Code folder: ${CMAKE_CURRENT_LIST_DIR}
# Get the platform specific folder ${pal_dir} for the current folder: ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME}
# Note: o3de_pal_dir will take care of the details for us, as this may be a restricted platform
#       in which case it will see if that platform is present here or in the restricted folder.
#       i.e. It could here in our gem : Gems/VehicleDynamics/Code/Platform/<platorm_name>  or
#            <restricted_folder>/<platform_name>/Gems/VehicleDynamics/Code
o3de_pal_dir(pal_dir ${CMAKE_CURRENT_LIST_DIR}/Platform/${PAL_PLATFORM_NAME} "${gem_restricted_path}" "${gem_path}" "${gem_parent_relative_path}")

# Now that we have the platform abstraction layer (PAL) folder for this folder, thats where we will find the
# traits for this platform. Traits for a platform are defines for things like whether or not something in this gem
# is supported by this platform.
include(${pal_dir}/PAL_${PAL_PLATFORM_NAME_LOWERCASE}.cmake)

# Check to see if building the Gem Modules are supported for the current platform
if(NOT PAL_TRAIT_VEHICLEDYNAMICS_SUPPORTED)
    return()
endif()

# The ${gem_name}.Static target
ly_add_target(
    NAME ${gem_name}.Static STATIC
    NAMESPACE Gem
    FILES_CMAKE
        vehicledynamics_private_files.cmake
        ${pal_dir}/vehicledynamics_private_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Check if ROS2 gem is available and add conditional dependencies
set(ROS2_GEM_AVAILABLE FALSE)
if(TARGET Gem::ROS2.API)
    set(ROS2_GEM_AVAILABLE TRUE)
    message(STATUS "ROS2 gem detected, enabling ROS2 vehicle control support")
else()
    message(STATUS "ROS2 gem not found, ROS2 vehicle control will be disabled")
endif()

# Add conditional ROS2 dependencies to the Static target
if(ROS2_GEM_AVAILABLE)
    target_link_libraries(${gem_name}.Static 
        PUBLIC 
            Gem::ROS2.API
    )
    target_compile_definitions(${gem_name}.Static PUBLIC ROS2_GEM_ENABLED)
    
    # Find ROS2 packages needed by ROS2 gem
    find_package(rclcpp QUIET)
    find_package(std_msgs QUIET)
    find_package(geometry_msgs QUIET)
    find_package(sensor_msgs QUIET)
    find_package(tf2_ros QUIET)
    
    if(rclcpp_FOUND AND std_msgs_FOUND)
        target_link_libraries(${gem_name}.Static 
            PUBLIC 
                rclcpp::rclcpp
                ${std_msgs_TARGETS}
        )
        
        # Add additional ROS2 packages if found
        if(geometry_msgs_FOUND)
            target_link_libraries(${gem_name}.Static PUBLIC ${geometry_msgs_TARGETS})
        endif()
        if(sensor_msgs_FOUND)
            target_link_libraries(${gem_name}.Static PUBLIC ${sensor_msgs_TARGETS})
        endif()
        if(tf2_ros_FOUND)
            target_link_libraries(${gem_name}.Static PUBLIC ${tf2_ros_TARGETS})
        endif()
        
        message(STATUS "ROS2 packages found and linked for VehicleDynamics gem")
    else()
        message(WARNING "ROS2 packages not found, ROS2 functionality will be limited")
    endif()
endif()

# The ${gem_name}.API target declares the common interface that users of this gem should depend on in their targets
ly_add_target(
    NAME ${gem_name}.API INTERFACE
    NAMESPACE Gem
    FILES_CMAKE
        vehicledynamics_api_files.cmake
        ${pal_dir}/vehicledynamics_api_files.cmake
    INCLUDE_DIRECTORIES
        INTERFACE
            Include
    BUILD_DEPENDENCIES
        INTERFACE
           AZ::AzCore
)

# The ${gem_name}.Private.Object target is an internal target
# It should not be used outside of this Gems CMakeLists.txt
ly_add_target(
    NAME ${gem_name}.Private.Object STATIC
    NAMESPACE Gem
    FILES_CMAKE
        vehicledynamics_private_files.cmake
        ${pal_dir}/vehicledynamics_private_files.cmake
    TARGET_PROPERTIES
        O3DE_PRIVATE_TARGET TRUE
    INCLUDE_DIRECTORIES
        PRIVATE
            Include
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            AZ::AzCore
            AZ::AzFramework
)

# Apply ROS2 dependencies to Private.Object target as well
if(ROS2_GEM_AVAILABLE)
    target_link_libraries(${gem_name}.Private.Object 
        PUBLIC 
            Gem::ROS2.API
    )
    target_compile_definitions(${gem_name}.Private.Object PUBLIC ROS2_GEM_ENABLED)
    
    if(rclcpp_FOUND AND std_msgs_FOUND)
        target_link_libraries(${gem_name}.Private.Object 
            PUBLIC 
                rclcpp::rclcpp
                ${std_msgs_TARGETS}
        )
        
        # Add additional ROS2 packages if found
        if(geometry_msgs_FOUND)
            target_link_libraries(${gem_name}.Private.Object PUBLIC ${geometry_msgs_TARGETS})
        endif()
        if(sensor_msgs_FOUND)
            target_link_libraries(${gem_name}.Private.Object PUBLIC ${sensor_msgs_TARGETS})
        endif()
        if(tf2_ros_FOUND)
            target_link_libraries(${gem_name}.Private.Object PUBLIC ${tf2_ros_TARGETS})
        endif()
    endif()
endif()

# Here add ${gem_name} target, it depends on the Private Object library and Public API interface
ly_add_target(
    NAME ${gem_name} ${PAL_TRAIT_MONOLITHIC_DRIVEN_MODULE_TYPE}
    NAMESPACE Gem
    FILES_CMAKE
        vehicledynamics_shared_files.cmake
        ${pal_dir}/vehicledynamics_shared_files.cmake
    INCLUDE_DIRECTORIES
        PUBLIC
            Include
        PRIVATE
            Source
    BUILD_DEPENDENCIES
        PUBLIC
            Gem::${gem_name}.API
        PRIVATE
            Gem::${gem_name}.Private.Object
            AZ::AzFramework
)

# Apply ROS2 dependencies to main gem target as well
if(ROS2_GEM_AVAILABLE)
    target_link_libraries(${gem_name} 
        PUBLIC 
            Gem::ROS2.API
    )
    target_compile_definitions(${gem_name} PUBLIC ROS2_GEM_ENABLED)
    
    if(rclcpp_FOUND AND std_msgs_FOUND)
        target_link_libraries(${gem_name} 
            PUBLIC 
                rclcpp::rclcpp
                ${std_msgs_TARGETS}
        )
        
        # Add additional ROS2 packages if found
        if(geometry_msgs_FOUND)
            target_link_libraries(${gem_name} PUBLIC ${geometry_msgs_TARGETS})
        endif()
        if(sensor_msgs_FOUND)
            target_link_libraries(${gem_name} PUBLIC ${sensor_msgs_TARGETS})
        endif()
        if(tf2_ros_FOUND)
            target_link_libraries(${gem_name} PUBLIC ${tf2_ros_TARGETS})
        endif()
    endif()
endif()

# By default, we will specify that the above target ${gem_name} would be used by
# Client and Server type targets when this gem is enabled.  If you don't want it
# active in Clients or Servers by default, delete one of both of the following lines:
ly_create_alias(NAME ${gem_name}.Clients NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Servers NAMESPACE Gem TARGETS Gem::${gem_name})
ly_create_alias(NAME ${gem_name}.Unified NAMESPACE Gem TARGETS Gem::${gem_name})

# For the Client and Server variants of ${gem_name} Gem, an alias to the ${gem_name}.API target will be made
ly_create_alias(NAME ${gem_name}.Clients.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Servers.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)
ly_create_alias(NAME ${gem_name}.Unified.API NAMESPACE Gem TARGETS Gem::${gem_name}.API)

# Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
# for the Clients, Servers, Unified gem variants
o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Clients Servers Unified)

# If we are on a host platform, we want to add the host tools targets like the ${gem_name}.Editor MODULE target
if(PAL_TRAIT_BUILD_HOST_TOOLS)
    # The ${gem_name}.Editor.API target can be used by other gems that want to interact with the ${gem_name}.Editor module
    ly_add_target(
        NAME ${gem_name}.Editor.API INTERFACE
        NAMESPACE Gem
        FILES_CMAKE
            vehicledynamics_editor_api_files.cmake
            ${pal_dir}/vehicledynamics_editor_api_files.cmake
        INCLUDE_DIRECTORIES
            INTERFACE
                Include
        BUILD_DEPENDENCIES
            INTERFACE
                AZ::AzToolsFramework
    )

    # The ${gem_name}.Editor.Private.Object target is an internal target
    # which is only to be used by this gems CMakeLists.txt and any subdirectories
    # Other gems should not use this target
    ly_add_target(
        NAME ${gem_name}.Editor.Private.Object STATIC
        NAMESPACE Gem
        FILES_CMAKE
            vehicledynamics_editor_private_files.cmake
        TARGET_PROPERTIES
            O3DE_PRIVATE_TARGET TRUE
        INCLUDE_DIRECTORIES
            PRIVATE
                Include
                Source
        BUILD_DEPENDENCIES
            PUBLIC
                AZ::AzToolsFramework
                Gem::${gem_name}.Private.Object
    )

    ly_add_target(
        NAME ${gem_name}.Editor GEM_MODULE
        NAMESPACE Gem
        AUTOMOC
        FILES_CMAKE
            vehicledynamics_editor_shared_files.cmake
        INCLUDE_DIRECTORIES
            PRIVATE
                Source
            PUBLIC
                Include
        BUILD_DEPENDENCIES
            PUBLIC
                Gem::${gem_name}.Editor.API
            PRIVATE
                Gem::${gem_name}.Editor.Private.Object
    )

    # By default, we will specify that the above target ${gem_name} would be used by
    # Tool and Builder type targets when this gem is enabled.  If you don't want it
    # active in Tools or Builders by default, delete one of both of the following lines:
    ly_create_alias(NAME ${gem_name}.Tools    NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)
    ly_create_alias(NAME ${gem_name}.Builders NAMESPACE Gem TARGETS Gem::${gem_name}.Editor)

    # For the Tools and Builders variants of ${gem_name} Gem, an alias to the ${gem_name}.Editor API target will be made
    ly_create_alias(NAME ${gem_name}.Tools.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)
    ly_create_alias(NAME ${gem_name}.Builders.API NAMESPACE Gem TARGETS Gem::${gem_name}.Editor.API)

    # Add in CMake dependencies for each gem dependency listed in this gem's gem.json file
    # for the Tools and Builders gem variants
    o3de_add_variant_dependencies_for_gem_dependencies(GEM_NAME ${gem_name} VARIANTS Tools Builders)
endif()

################################################################################
# Tests
################################################################################
# See if globally, tests are supported
if(PAL_TRAIT_BUILD_TESTS_SUPPORTED)
    # We globally support tests, see if we support tests on this platform for ${gem_name}.Tests
    if(PAL_TRAIT_VEHICLEDYNAMICS_TEST_SUPPORTED)
        # We support ${gem_name}.Tests on this platform, add dependency on the Private Object target
        ly_add_target(
            NAME ${gem_name}.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
            NAMESPACE Gem
            FILES_CMAKE
                vehicledynamics_tests_files.cmake
            INCLUDE_DIRECTORIES
                PRIVATE
                    Tests
                    Source
                    Include
            BUILD_DEPENDENCIES
                PRIVATE
                    AZ::AzTest
                    AZ::AzFramework
                    Gem::${gem_name}.Private.Object
        )

        # Add ${gem_name}.Tests to googletest
        ly_add_googletest(
            NAME Gem::${gem_name}.Tests
        )
    endif()

    # If we are a host platform we want to add tools test like editor tests here
    if(PAL_TRAIT_BUILD_HOST_TOOLS)
        # We are a host platform, see if Editor tests are supported on this platform
        if(PAL_TRAIT_VEHICLEDYNAMICS_EDITOR_TEST_SUPPORTED)
            # We support ${gem_name}.Editor.Tests on this platform, add ${gem_name}.Editor.Tests target which depends on
            # private ${gem_name}.Editor.Private.Object target
            ly_add_target(
                NAME ${gem_name}.Editor.Tests ${PAL_TRAIT_TEST_TARGET_TYPE}
                NAMESPACE Gem
                FILES_CMAKE
                    vehicledynamics_editor_tests_files.cmake
                INCLUDE_DIRECTORIES
                    PRIVATE
                        Tests
                        Source
                        Include
                BUILD_DEPENDENCIES
                    PRIVATE
                        AZ::AzTest
                        Gem::${gem_name}.Editor.Private.Object
            )

            # Add ${gem_name}.Editor.Tests to googletest
            ly_add_googletest(
                NAME Gem::${gem_name}.Editor.Tests
            )
        endif()
    endif()
endif()

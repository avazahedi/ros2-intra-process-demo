cmake_minimum_required(VERSION 3.8)
project(zero_copy_data_transport)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS imgcodecs)

set(dependencies
  rclcpp
  rclcpp_components
  sensor_msgs
)

add_library(${PROJECT_NAME} SHARED
  src/intra_proc_image_publisher.cpp
  src/intra_proc_image_subscriber.cpp
)

ament_target_dependencies(${PROJECT_NAME} ${dependencies})
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

rclcpp_components_register_nodes(${PROJECT_NAME}
  "intraproc::ImagePublisher"
  "intraproc::ImageSubscriber"
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(TARGETS ${PROJECT_NAME}
  EXPORT export_${PROJECT_NAME}
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

ament_export_targets(export_${PROJECT_NAME} HAS_LIBRARY_TARGET)
ament_export_dependencies(${dependencies})

ament_package()

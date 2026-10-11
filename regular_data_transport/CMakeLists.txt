cmake_minimum_required(VERSION 3.8)
project(regular_data_transport)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS imgcodecs)

add_executable(image_publisher
  src/image_publisher.cpp
)

target_link_libraries(image_publisher
  ${OpenCV_LIBS}
)

ament_target_dependencies(image_publisher
  rclcpp
  sensor_msgs
)

add_executable(image_subscriber
  src/image_subscriber.cpp
)

target_link_libraries(image_subscriber)

ament_target_dependencies(image_subscriber
  rclcpp
  sensor_msgs
)

install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME}
)

install(TARGETS
  image_publisher
  image_subscriber
  DESTINATION lib/${PROJECT_NAME})

ament_package()

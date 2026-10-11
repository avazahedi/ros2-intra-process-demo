#include <chrono>
#include <memory>

#include <opencv2/imgcodecs.hpp>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

class ImagePublisher : public rclcpp::Node
{
public:
  ImagePublisher() : Node("image_publisher")
  {
    const auto path = declare_parameter<std::string>("image_path", "1920x1080.png");
    log_to_console_ = declare_parameter<bool>("log_to_console", false);

    cv::Mat img = cv::imread(path, cv::IMREAD_COLOR);
    if (img.empty()) {
      throw std::runtime_error("Could not load image: " + path);
    }

    image_msg_.header.frame_id = "camera";
    image_msg_.height = img.rows;
    image_msg_.width = img.cols;
    image_msg_.encoding = "bgr8";
    image_msg_.step = img.cols * img.elemSize();
    image_msg_.data.assign(img.data, img.data + img.total() * img.elemSize());

    pub_ = create_publisher<sensor_msgs::msg::Image>("/image", 10);
    timer_ = create_wall_timer(std::chrono::milliseconds(100), [this]() {
      if (log_to_console_)
      {
        RCLCPP_INFO(get_logger(), "Published data address: %p, size: %zu bytes, "
                                  "image dimensions: %ux%u",
                    static_cast<void *>(image_msg_.data.data()), image_msg_.data.size(),
                    image_msg_.width, image_msg_.height);
      }
      image_msg_.header.stamp = now();
      pub_->publish(image_msg_);
    });
  }

private:
  bool log_to_console_ { false };
  sensor_msgs::msg::Image image_msg_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ImagePublisher>());
  rclcpp::shutdown();
  return 0;
}

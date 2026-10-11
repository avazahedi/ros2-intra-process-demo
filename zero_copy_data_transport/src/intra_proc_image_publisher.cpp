#include <chrono>
#include <memory>
#include <string>

#include <opencv2/imgcodecs.hpp>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

namespace intraproc
{
class ImagePublisher : public rclcpp::Node
{
public:
  // Note that intra-process comms is enabled.
  explicit ImagePublisher(const rclcpp::NodeOptions & options)
  : Node("image_publisher", rclcpp::NodeOptions(options).use_intra_process_comms(true))
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
      // Publish a unique_ptr so ownership moves to the intra-process manager and the
      // subscriber can receive this same allocation. The one copy from the cached
      // image happens here, on the publisher side.
      auto msg = std::make_unique<sensor_msgs::msg::Image>(image_msg_);
      msg->header.stamp = now();

      if (log_to_console_) {
        RCLCPP_INFO(get_logger(), "Published data address: %p, size: %zu bytes, "
                                  "image dimensions: %ux%u",
                    static_cast<void *>(msg->data.data()), msg->data.size(),
                    msg->width, msg->height);
      }
      pub_->publish(std::move(msg));
    });
  }

private:
  bool log_to_console_ { false };
  sensor_msgs::msg::Image image_msg_;
  rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_;
  rclcpp::TimerBase::SharedPtr timer_;
};
} // namespace intraproc

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(intraproc::ImagePublisher)

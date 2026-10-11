#include <fstream>
#include <iomanip>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

namespace intraproc
{
class ImageSubscriber : public rclcpp::Node
{
public:
  // Intra-process comms must be enabled on BOTH the publisher and the subscriber nodes.
  explicit ImageSubscriber(const rclcpp::NodeOptions & options)
  : Node("image_subscriber", rclcpp::NodeOptions(options).use_intra_process_comms(true))
  {
    log_to_console_ = declare_parameter<bool>("log_to_console", false);
    log_to_csv_ = declare_parameter<bool>("log_to_csv", false);
    const auto csv_path =
      declare_parameter<std::string>("csv_path", "intra_proc_data_transport.csv");

    if (log_to_csv_) {
      csv_.open(csv_path);
      csv_ << "stamp_seconds,latency_ms,size_bytes" << std::endl;
    }

    sub_ = create_subscription<sensor_msgs::msg::Image>(
      "/image", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));
  }

private:
  void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr & image_msg)
  {
    if (log_to_csv_) {
      const rclcpp::Time stamp(image_msg->header.stamp);
      const double latency_ms = (now() - stamp).seconds() * 1e3;
      csv_ << image_msg->header.stamp.sec << "."
           << std::setw(9) << std::setfill('0') << image_msg->header.stamp.nanosec
           << "," << latency_ms << "," << image_msg->data.size() << std::endl;
    }

    if (log_to_console_) {
      RCLCPP_INFO(get_logger(), "Received data address: %p, size: %zu bytes, "
                                "image dimensions: %ux%u",
                  static_cast<const void *>(image_msg->data.data()), image_msg->data.size(),
                  image_msg->width, image_msg->height);
    }
  }

  bool log_to_console_ { false };
  bool log_to_csv_ { false };
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
  std::ofstream csv_;
};
} // namespace intraproc

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(intraproc::ImageSubscriber)

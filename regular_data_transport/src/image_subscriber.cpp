#include <fstream>
#include <iomanip>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/image.hpp"

class ImageSubscriber : public rclcpp::Node
{
public:
  ImageSubscriber() : Node("image_subscriber")
  {
    log_to_console_ = declare_parameter<bool>("log_to_console", false);
    log_to_csv_ = declare_parameter<bool>("log_to_csv", false);

    sub_ = this->create_subscription<sensor_msgs::msg::Image>(
        "/image", 10, std::bind(&ImageSubscriber::imageCallback, this, std::placeholders::_1));

    if (log_to_csv_)
    {
      csv_ << "stamp_seconds,latency_ms,size_bytes" << std::endl;
    }
  }

private:
  void imageCallback(const sensor_msgs::msg::Image::ConstSharedPtr& image_msg)
  {
    if (log_to_csv_)
    {
      const rclcpp::Time stamp(image_msg->header.stamp);
      const double latency_ms = (now() - stamp).seconds() * 1e3;
      // header: "stamp_seconds,latency_ms,size_bytes"
      csv_ << image_msg->header.stamp.sec << "."
          << std::setw(9) << std::setfill('0') << image_msg->header.stamp.nanosec
          << "," << latency_ms << "," << image_msg->data.size() << std::endl;
    }

    if (log_to_console_)
    {
      RCLCPP_INFO(get_logger(), "Received data address: %p, size: %zu bytes, "
                                "image dimensions: %ux%u",
                  static_cast<const void *>(image_msg->data.data()), image_msg->data.size(),
                  image_msg->width, image_msg->height);
    }
  }

  bool log_to_console_ { false };
  bool log_to_csv_ { false };
  rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_;
  std::ofstream csv_{"regular_data_transport.csv"};
};

int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<ImageSubscriber>());
  rclcpp::shutdown();
  return 0;
}

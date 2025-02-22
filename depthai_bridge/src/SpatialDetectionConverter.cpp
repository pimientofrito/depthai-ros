#include <depthai_bridge/SpatialDetectionConverter.hpp>

namespace dai {
namespace ros {

SpatialDetectionConverter::SpatialDetectionConverter(std::string frameName, int width, int height, bool normalized)
    : _frameName(frameName), _width(width), _height(height), _normalized(normalized) {}

void SpatialDetectionConverter::toRosMsg(std::shared_ptr<dai::SpatialImgDetections> inNetData, SpatialMessages::SpatialDetectionArray& opDetectionMsg) {
    // setting the header
    auto tstamp = inNetData->getTimestamp();

#ifndef IS_ROS2
    auto rosNow = ::ros::Time::now();
    auto steadyTime = std::chrono::steady_clock::now();
    auto diffTime = steadyTime - tstamp;
    uint64_t nsec = rosNow.toNSec() - diffTime.count();
    auto rosStamp = rosNow.fromNSec(nsec);
    opDetectionMsg.header.stamp = rosStamp;
    opDetectionMsg.header.seq = inNetData->getSequenceNum();
#else
    auto rclNow = rclcpp::Clock().now();
    auto steadyTime = std::chrono::steady_clock::now();
    auto diffTime = steadyTime - tstamp;
    auto rclStamp = rclNow - diffTime;
    opDetectionMsg.header.stamp = rclStamp;
#endif

    opDetectionMsg.header.frame_id = _frameName;
    opDetectionMsg.detections.resize(inNetData->detections.size());

    // TODO(Sachin): check if this works fine for normalized detection
    // publishing
    for(int i = 0; i < inNetData->detections.size(); ++i) {
        int xMin, yMin, xMax, yMax;
        if(_normalized) {
            xMin = inNetData->detections[i].xmin;
            yMin = inNetData->detections[i].ymin;
            xMax = inNetData->detections[i].xmax;
            yMax = inNetData->detections[i].ymax;
        } else {
            xMin = inNetData->detections[i].xmin * _width;
            yMin = inNetData->detections[i].ymin * _height;
            xMax = inNetData->detections[i].xmax * _width;
            yMax = inNetData->detections[i].ymax * _height;
        }

        float xSize = xMax - xMin;
        float ySize = yMax - yMin;
        float xCenter = xMin + xSize / 2;
        float yCenter = yMin + ySize / 2;

        opDetectionMsg.detections[i].results.resize(1);

#ifdef IS_GALACTIC
        opDetectionMsg.detections[i].results[0].class_id = std::to_string(inNetData->detections[i].label);
#elif IS_ROS2
        opDetectionMsg.detections[i].results[0].id = std::to_string(inNetData->detections[i].label);
#else
        opDetectionMsg.detections[i].results[0].id = inNetData->detections[i].label;
#endif

        opDetectionMsg.detections[i].results[0].score = inNetData->detections[i].confidence;

        opDetectionMsg.detections[i].bbox.center.x = xCenter;
        opDetectionMsg.detections[i].bbox.center.y = yCenter;
        opDetectionMsg.detections[i].bbox.size_x = xSize;
        opDetectionMsg.detections[i].bbox.size_y = ySize;
        // opDetectionMsg.detections[i].is_tracking = _isTracking;

        // converting mm to meters since per ros rep-103 lenght should always be in meters
        opDetectionMsg.detections[i].position.x = inNetData->detections[i].spatialCoordinates.x / 1000;
        opDetectionMsg.detections[i].position.y = inNetData->detections[i].spatialCoordinates.y / 1000;
        opDetectionMsg.detections[i].position.z = inNetData->detections[i].spatialCoordinates.z / 1000;
    }
}

SpatialDetectionArrayPtr SpatialDetectionConverter::toRosMsgPtr(std::shared_ptr<dai::SpatialImgDetections> inNetData) {
#ifdef IS_ROS2
    SpatialDetectionArrayPtr ptr = std::make_shared<SpatialMessages::SpatialDetectionArray>();
#else
    SpatialDetectionArrayPtr ptr = boost::make_shared<SpatialMessages::SpatialDetectionArray>();
#endif
    toRosMsg(inNetData, *ptr);
    return ptr;
}

}  // namespace ros
}  // namespace dai
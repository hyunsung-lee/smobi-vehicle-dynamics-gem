/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <VehicleDynamics/VehicleDynamicsBus.h>
#include <VehicleDynamics/VehicleDynamicsTypeIds.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/containers/vector.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>

#ifdef ROS2_GEM_ENABLED
#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/float32.hpp>
#include <std_msgs/msg/float32_multi_array.hpp>
#include <std_msgs/msg/bool.hpp>
#include <geometry_msgs/msg/pose_stamped.hpp>
#include <ROS2/ROS2Bus.h>
#include <ROS2/Utilities/ROS2Conversions.h>
#endif

namespace VehicleDynamics
{
    //! Configuration structure for ROS2 topic subscriptions and publications
    struct ROS2VehicleControlConfiguration
    {
        AZ_TYPE_INFO(ROS2VehicleControlConfiguration, "{F1234567-89AB-CDEF-0123-456789ABCDEF}");
        
        // Control input topics
        bool m_enableSteeringControl = true;
        AZStd::string m_steeringTopicName = "/vehicle/steering_angle";
        
        bool m_enableAccelerationControl = true;
        AZStd::string m_accelerationTopicName = "/vehicle/acceleration";
        
        // Wheel speed publishing
        bool m_enableWheelSpeedPublishing = true;
        AZStd::string m_wheelSpeedTopicName = "/vehicle/wheel_speeds";
        float m_wheelSpeedPublishRate = 10.0f; // Hz
        
        // AD (Autonomous Driving) system
        bool m_enableADSystem = true;
        AZStd::string m_adEnableTopicName = "/vehicle/ad_enable";
        AZStd::string m_adStatusTopicName = "/vehicle/ad_status";
        float m_adStatusPublishRate = 5.0f; // Hz
        float m_manualInputTimeoutSeconds = 2.0f; // Time after manual input before AD can be re-engaged
        
        // Pose publishing
        bool m_enablePosePublishing = true;
        AZStd::string m_poseTopicName = "/vehicle/pose";
        float m_posePublishRate = 10.0f; // Hz
        AZ::EntityId m_poseReferenceEntity; // Entity whose coordinate will become the reference point
        
        static void Reflect(AZ::ReflectContext* context);
    };

    //! Component that subscribes to ROS2 Float32 topics for vehicle control and publishes wheel speeds
    class ROS2VehicleControlComponent
        : public AZ::Component
        , private AZ::TickBus::Handler
        , public AzFramework::InputChannelEventListener
    {
    public:
        AZ_COMPONENT(ROS2VehicleControlComponent, ROS2VehicleControlComponentTypeId);

        ROS2VehicleControlComponent() = default;
        ~ROS2VehicleControlComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        // Removed GetRequiredServices - this component works independently

        //////////////////////////////////////////////////////////////////////////
        // AZ::Component overrides
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AZ::TickBus::Handler overrides
        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // InputChannelEventListener overrides
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
        //////////////////////////////////////////////////////////////////////////

    private:
        void InitializeROS2Subscriptions();
        void ShutdownROS2Subscriptions();
        void PublishWheelSpeeds();
        AZStd::vector<float> GetWheelSpeedsKmh();
        
        // AD system methods
        void PublishADStatus();
        void UpdateADState();
        void HandleManualInput();
        void HandleKeyboardEvent(const AzFramework::InputChannel& inputChannel);
        bool IsManualInputActive() const;
        
        // Pose publishing methods
        void PublishPose();
        
#ifdef ROS2_GEM_ENABLED
        void OnSteeringMessage(const std_msgs::msg::Float32::SharedPtr msg);
        void OnAccelerationMessage(const std_msgs::msg::Float32::SharedPtr msg);
        void OnADEnableMessage(const std_msgs::msg::Bool::SharedPtr msg);
        
        rclcpp::Node::SharedPtr m_rosNode;
        rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr m_steeringSubscription;
        rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr m_accelerationSubscription;
        rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr m_adEnableSubscription;
        rclcpp::Publisher<std_msgs::msg::Float32MultiArray>::SharedPtr m_wheelSpeedPublisher;
        rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr m_adStatusPublisher;
        rclcpp::Publisher<geometry_msgs::msg::PoseStamped>::SharedPtr m_posePublisher;
#endif

        ROS2VehicleControlConfiguration m_configuration;
        float m_lastWheelSpeedPublishTime = 0.0f;
        
        // AD system state variables
        bool m_adEnabled = false;           // External AD enable state (from /ad_enable topic)
        bool m_adStatus = false;            // Current AD active status (published to /ad_status topic)
        bool m_manualInputDetected = false; // Flag for manual input detection
        float m_lastManualInputTime = 0.0f; // Time since last manual input
        float m_lastADStatusPublishTime = 0.0f; // Time tracking for AD status publishing
        float m_currentTime = 0.0f;         // Current simulation time
        
        // Pose publishing state variables
        float m_lastPosePublishTime = 0.0f; // Time tracking for pose publishing
    };

} // namespace VehicleDynamics

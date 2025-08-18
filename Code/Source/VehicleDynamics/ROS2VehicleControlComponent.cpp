/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ROS2VehicleControlComponent.h"
#include "VehicleController.h"
#include "WheelController.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>

#ifdef ROS2_GEM_ENABLED
#include <ROS2/ROS2Bus.h>
#endif

namespace VehicleDynamics
{
    //////////////////////////////////////////////////////////////////////////
    // ROS2VehicleControlConfiguration
    void ROS2VehicleControlConfiguration::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ROS2VehicleControlConfiguration>()
                ->Version(1)
                ->Field("EnableSteeringControl", &ROS2VehicleControlConfiguration::m_enableSteeringControl)
                ->Field("SteeringTopicName", &ROS2VehicleControlConfiguration::m_steeringTopicName)
                ->Field("EnableAccelerationControl", &ROS2VehicleControlConfiguration::m_enableAccelerationControl)
                ->Field("AccelerationTopicName", &ROS2VehicleControlConfiguration::m_accelerationTopicName)
                ->Field("EnableWheelSpeedPublishing", &ROS2VehicleControlConfiguration::m_enableWheelSpeedPublishing)
                ->Field("WheelSpeedTopicName", &ROS2VehicleControlConfiguration::m_wheelSpeedTopicName)
                ->Field("WheelSpeedPublishRate", &ROS2VehicleControlConfiguration::m_wheelSpeedPublishRate)
                ->Field("EnableADSystem", &ROS2VehicleControlConfiguration::m_enableADSystem)
                ->Field("ADEnableTopicName", &ROS2VehicleControlConfiguration::m_adEnableTopicName)
                ->Field("ADStatusTopicName", &ROS2VehicleControlConfiguration::m_adStatusTopicName)
                ->Field("ADStatusPublishRate", &ROS2VehicleControlConfiguration::m_adStatusPublishRate)
                ->Field("ManualInputTimeoutSeconds", &ROS2VehicleControlConfiguration::m_manualInputTimeoutSeconds);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ROS2VehicleControlConfiguration>("ROS2 Vehicle Control Configuration", "Configuration for ROS2 topic subscriptions and publications")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Control Input Topics")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, 
                        &ROS2VehicleControlConfiguration::m_enableSteeringControl,
                        "Enable Steering Control", 
                        "Subscribe to ROS2 steering angle topic")
                    ->DataElement(AZ::Edit::UIHandlers::LineEdit, 
                        &ROS2VehicleControlConfiguration::m_steeringTopicName,
                        "Steering Topic Name", 
                        "ROS2 topic name for steering angle (Float32)")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, 
                        &ROS2VehicleControlConfiguration::m_enableAccelerationControl,
                        "Enable Acceleration Control", 
                        "Subscribe to ROS2 acceleration topic")
                    ->DataElement(AZ::Edit::UIHandlers::LineEdit, 
                        &ROS2VehicleControlConfiguration::m_accelerationTopicName,
                        "Acceleration Topic Name", 
                        "ROS2 topic name for acceleration (Float32)")
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Wheel Speed Publishing")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, 
                        &ROS2VehicleControlConfiguration::m_enableWheelSpeedPublishing,
                        "Enable Wheel Speed Publishing", 
                        "Publish wheel speeds to ROS2 topic")
                    ->DataElement(AZ::Edit::UIHandlers::LineEdit, 
                        &ROS2VehicleControlConfiguration::m_wheelSpeedTopicName,
                        "Wheel Speed Topic Name", 
                        "ROS2 topic name for wheel speeds (Float32MultiArray)")
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, 
                        &ROS2VehicleControlConfiguration::m_wheelSpeedPublishRate,
                        "Publish Rate (Hz)", 
                        "Rate at which to publish wheel speeds")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->ClassElement(AZ::Edit::ClassElements::Group, "Autonomous Driving System")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, 
                        &ROS2VehicleControlConfiguration::m_enableADSystem,
                        "Enable AD System", 
                        "Enable autonomous driving system with manual override")
                    ->DataElement(AZ::Edit::UIHandlers::LineEdit, 
                        &ROS2VehicleControlConfiguration::m_adEnableTopicName,
                        "AD Enable Topic Name", 
                        "ROS2 topic name for AD enable/disable commands (Bool)")
                    ->DataElement(AZ::Edit::UIHandlers::LineEdit, 
                        &ROS2VehicleControlConfiguration::m_adStatusTopicName,
                        "AD Status Topic Name", 
                        "ROS2 topic name for AD status publishing (Bool)")
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, 
                        &ROS2VehicleControlConfiguration::m_adStatusPublishRate,
                        "AD Status Publish Rate (Hz)", 
                        "Rate at which to publish AD status")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.1f)
                        ->Attribute(AZ::Edit::Attributes::Max, 50.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::SpinBox, 
                        &ROS2VehicleControlConfiguration::m_manualInputTimeoutSeconds,
                        "Manual Input Timeout (s)", 
                        "Time after manual input before AD can be re-engaged")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.5f)
                        ->Attribute(AZ::Edit::Attributes::Max, 10.0f)
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // ROS2VehicleControlComponent
    void ROS2VehicleControlComponent::Reflect(AZ::ReflectContext* context)
    {
        ROS2VehicleControlConfiguration::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ROS2VehicleControlComponent, AZ::Component>()
                ->Version(1)
                ->Field("Configuration", &ROS2VehicleControlComponent::m_configuration);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<ROS2VehicleControlComponent>("ROS2 Vehicle Control", "Subscribes to ROS2 Float32 topics for vehicle control and publishes wheel speeds")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
                        ->Attribute(AZ::Edit::Attributes::Category, "SMOBI Vehicle Dynamics")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Component_Placeholder.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Component_Placeholder.svg")
                    ->DataElement(AZ::Edit::UIHandlers::Default, 
                        &ROS2VehicleControlComponent::m_configuration, 
                        "ROS2 Configuration", 
                        "ROS2 topic subscription and publishing configuration");
            }
        }
    }

    void ROS2VehicleControlComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ROS2VehicleControlService"));
    }

    void ROS2VehicleControlComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ROS2VehicleControlService"));
    }

    // Removed GetRequiredServices - this component works independently

    void ROS2VehicleControlComponent::Activate()
    {
        AZ_Printf("ROS2VehicleControl", "ROS2VehicleControlComponent::Activate() called");
        
#ifdef ROS2_GEM_ENABLED
        AZ_Printf("ROS2VehicleControl", "ROS2_GEM_ENABLED flag is set - ROS2 support available");
#else
        AZ_Printf("ROS2VehicleControl", "ROS2_GEM_ENABLED flag is NOT set - ROS2 support disabled");
#endif
        
        InitializeROS2Subscriptions();
        AZ::TickBus::Handler::BusConnect();
        InputChannelEventListener::Connect();
        AZ_Printf("ROS2VehicleControl", "ROS2VehicleControlComponent activation complete");
    }

    void ROS2VehicleControlComponent::Deactivate()
    {
        InputChannelEventListener::Disconnect();
        AZ::TickBus::Handler::BusDisconnect();
        ShutdownROS2Subscriptions();
    }

    void ROS2VehicleControlComponent::OnTick(float deltaTime, AZ::ScriptTimePoint time)
    {
        // Update current time
        m_currentTime += deltaTime;
        
        // Handle AD system updates
        if (m_configuration.m_enableADSystem)
        {
            HandleManualInput();
            UpdateADState();
            
            // Publish AD status
            m_lastADStatusPublishTime += deltaTime;
            const float adStatusPublishInterval = 1.0f / m_configuration.m_adStatusPublishRate;
            if (m_lastADStatusPublishTime >= adStatusPublishInterval)
            {
                PublishADStatus();
                m_lastADStatusPublishTime = 0.0f;
            }
        }
        
        // Publish wheel speeds
        if (m_configuration.m_enableWheelSpeedPublishing)
        {
            m_lastWheelSpeedPublishTime += deltaTime;
            const float publishInterval = 1.0f / m_configuration.m_wheelSpeedPublishRate;
            
            if (m_lastWheelSpeedPublishTime >= publishInterval)
            {
                PublishWheelSpeeds();
                m_lastWheelSpeedPublishTime = 0.0f;
            }
        }
    }

    void ROS2VehicleControlComponent::InitializeROS2Subscriptions()
    {
#ifdef ROS2_GEM_ENABLED
        AZ_Printf("ROS2VehicleControl", "Initializing ROS2 subscriptions...");
        
        // Get the ROS2 node from the ROS2 system
        auto ros2Interface = ROS2::ROS2Interface::Get();
        if (!ros2Interface)
        {
            AZ_Warning("ROS2VehicleControl", false, "ROS2 interface not available. Make sure ROS2 gem is enabled and active.");
            return;
        }

        AZ_Printf("ROS2VehicleControl", "ROS2 interface found, getting node...");
        m_rosNode = ros2Interface->GetNode();
        if (!m_rosNode)
        {
            AZ_Warning("ROS2VehicleControl", false, "Failed to get ROS2 node.");
            return;
        }
        
        AZ_Printf("ROS2VehicleControl", "ROS2 node obtained successfully");

        // Create steering subscription
        if (m_configuration.m_enableSteeringControl && !m_configuration.m_steeringTopicName.empty())
        {
            m_steeringSubscription = m_rosNode->create_subscription<std_msgs::msg::Float32>(
                m_configuration.m_steeringTopicName.c_str(),
                10,
                [this](const std_msgs::msg::Float32::SharedPtr msg) { OnSteeringMessage(msg); });
                
            AZ_Printf("ROS2VehicleControl", "Subscribed to steering topic: %s", m_configuration.m_steeringTopicName.c_str());
        }

        // Create acceleration subscription
        if (m_configuration.m_enableAccelerationControl && !m_configuration.m_accelerationTopicName.empty())
        {
            m_accelerationSubscription = m_rosNode->create_subscription<std_msgs::msg::Float32>(
                m_configuration.m_accelerationTopicName.c_str(),
                10,
                [this](const std_msgs::msg::Float32::SharedPtr msg) { OnAccelerationMessage(msg); });
                
            AZ_Printf("ROS2VehicleControl", "Subscribed to acceleration topic: %s", m_configuration.m_accelerationTopicName.c_str());
        }

        // Create wheel speed publisher
        if (m_configuration.m_enableWheelSpeedPublishing && !m_configuration.m_wheelSpeedTopicName.empty())
        {
            m_wheelSpeedPublisher = m_rosNode->create_publisher<std_msgs::msg::Float32MultiArray>(
                m_configuration.m_wheelSpeedTopicName.c_str(),
                10);
                
            AZ_Printf("ROS2VehicleControl", "Created wheel speed publisher on topic: %s", m_configuration.m_wheelSpeedTopicName.c_str());
        }

        // Create AD system subscriptions and publishers
        if (m_configuration.m_enableADSystem)
        {
            // AD enable subscription
            if (!m_configuration.m_adEnableTopicName.empty())
            {
                m_adEnableSubscription = m_rosNode->create_subscription<std_msgs::msg::Bool>(
                    m_configuration.m_adEnableTopicName.c_str(),
                    10,
                    [this](const std_msgs::msg::Bool::SharedPtr msg) { OnADEnableMessage(msg); });
                    
                AZ_Printf("ROS2VehicleControl", "Subscribed to AD enable topic: %s", m_configuration.m_adEnableTopicName.c_str());
            }

            // AD status publisher
            if (!m_configuration.m_adStatusTopicName.empty())
            {
                m_adStatusPublisher = m_rosNode->create_publisher<std_msgs::msg::Bool>(
                    m_configuration.m_adStatusTopicName.c_str(),
                    10);
                    
                AZ_Printf("ROS2VehicleControl", "Created AD status publisher on topic: %s", m_configuration.m_adStatusTopicName.c_str());
            }
        }
#else
        AZ_Warning("ROS2VehicleControl", false, "ROS2 gem is not enabled. ROS2 vehicle control will not function.");
#endif
    }
    
    void ROS2VehicleControlComponent::ShutdownROS2Subscriptions()
    {
#ifdef ROS2_GEM_ENABLED
        AZ_Printf("ROS2VehicleControl", "Shutting down ROS2 subscriptions...");
        m_steeringSubscription.reset();
        m_accelerationSubscription.reset();
        m_adEnableSubscription.reset();
        m_wheelSpeedPublisher.reset();
        m_adStatusPublisher.reset();
        m_rosNode.reset();
#endif
    }

    void ROS2VehicleControlComponent::PublishWheelSpeeds()
    {
#ifdef ROS2_GEM_ENABLED
        if (!m_wheelSpeedPublisher)
        {
            return;
        }

        AZStd::vector<float> wheelSpeeds = GetWheelSpeedsKmh();
        
        if (!wheelSpeeds.empty())
        {
            std_msgs::msg::Float32MultiArray msg;
            msg.data.reserve(wheelSpeeds.size());
            
            for (float speed : wheelSpeeds)
            {
                msg.data.push_back(speed);
            }
            
            // Add layout information for clarity
            msg.layout.dim.resize(1);
            msg.layout.dim[0].label = "wheel_speeds_kmh";
            msg.layout.dim[0].size = wheelSpeeds.size();
            msg.layout.dim[0].stride = wheelSpeeds.size();
            msg.layout.data_offset = 0;
            
            m_wheelSpeedPublisher->publish(msg);
        }
#endif
    }

    AZStd::vector<float> ROS2VehicleControlComponent::GetWheelSpeedsKmh()
    {
        AZStd::vector<float> wheelSpeeds;
        
        // Get the vehicle controller component
        VehicleController* vehicleController = GetEntity()->FindComponent<VehicleController>();
        if (!vehicleController)
        {
            AZ_Warning("ROS2VehicleControl", false, "No VehicleController component found on entity");
            return wheelSpeeds;
        }

        // Get wheel entity IDs from vehicle configuration (now public)
        const VehicleConfiguration& config = vehicleController->m_configuration;
        
        // Process front axle wheels
        for (const AZ::EntityId& wheelEntityId : config.m_frontAxleWheelIds)
        {
            if (wheelEntityId.IsValid())
            {
                AZ::Entity* wheelEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    wheelEntity, &AZ::ComponentApplicationRequests::FindEntity, wheelEntityId);
                
                if (wheelEntity)
                {
                    WheelController* wheelController = wheelEntity->FindComponent<WheelController>();
                    if (wheelController)
                    {
                        // Get wheel speed from the velocity at hit local (m/s) and convert to km/h
                        float wheelSpeedMs = wheelController->GetVelocityAtHitLocal().GetX();
                        float wheelSpeedKmh = wheelSpeedMs * 3.6f; // Convert m/s to km/h
                        wheelSpeeds.push_back(wheelSpeedKmh);
                    }
                }
            }
        }
        
        // Process rear axle wheels
        for (const AZ::EntityId& wheelEntityId : config.m_rearAxleWheelIds)
        {
            if (wheelEntityId.IsValid())
            {
                AZ::Entity* wheelEntity = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(
                    wheelEntity, &AZ::ComponentApplicationRequests::FindEntity, wheelEntityId);
                
                if (wheelEntity)
                {
                    WheelController* wheelController = wheelEntity->FindComponent<WheelController>();
                    if (wheelController)
                    {
                        // Get wheel speed from the velocity at hit local (m/s) and convert to km/h
                        float wheelSpeedMs = wheelController->GetVelocityAtHitLocal().GetX();
                        float wheelSpeedKmh = wheelSpeedMs * 3.6f; // Convert m/s to km/h
                        wheelSpeeds.push_back(wheelSpeedKmh);
                    }
                }
            }
        }
        
        return wheelSpeeds;
    }

#ifdef ROS2_GEM_ENABLED
    void ROS2VehicleControlComponent::OnSteeringMessage(const std_msgs::msg::Float32::SharedPtr msg)
    {
        // Only process ROS2 commands when AD is active or AD system is disabled
        if (!m_configuration.m_enableADSystem || m_adStatus)
        {
            VehicleDynamicsRequestBus::Event(
                GetEntityId(),
                &VehicleDynamicsRequests::SetSteeringAngle,
                msg->data);
        }
    }

    void ROS2VehicleControlComponent::OnAccelerationMessage(const std_msgs::msg::Float32::SharedPtr msg)
    {
        // Only process ROS2 commands when AD is active or AD system is disabled
        if (!m_configuration.m_enableADSystem || m_adStatus)
        {
            VehicleDynamicsRequestBus::Event(
                GetEntityId(),
                &VehicleDynamicsRequests::SetAcceleration,
                msg->data);
        }
    }

    void ROS2VehicleControlComponent::OnADEnableMessage(const std_msgs::msg::Bool::SharedPtr msg)
    {
        m_adEnabled = msg->data;
        AZ_Printf("ROS2VehicleControl", "AD Enable received: %s", m_adEnabled ? "true" : "false");
        
        // If AD is disabled, immediately turn off AD status
        if (!m_adEnabled)
        {
            m_adStatus = false;
            AZ_Printf("ROS2VehicleControl", "AD Status set to false due to AD disable");
        }
    }
#endif

    void ROS2VehicleControlComponent::PublishADStatus()
    {
#ifdef ROS2_GEM_ENABLED
        if (m_adStatusPublisher)
        {
            std_msgs::msg::Bool msg;
            msg.data = m_adStatus;
            m_adStatusPublisher->publish(msg);
        }
#endif
    }

    void ROS2VehicleControlComponent::UpdateADState()
    {
        // AD status can only be true if AD is enabled
        if (!m_adEnabled)
        {
            if (m_adStatus)
            {
                m_adStatus = false;
                AZ_Printf("ROS2VehicleControl", "AD Status disabled because AD Enable is false");
            }
            return;
        }

        // If manual input is detected, disable AD status
        if (m_manualInputDetected)
        {
            if (m_adStatus)
            {
                m_adStatus = false;
                AZ_Printf("ROS2VehicleControl", "AD Status disabled due to manual input override");
            }
        }
    }

    bool ROS2VehicleControlComponent::OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel)
    {
        const AzFramework::InputDeviceId& deviceId = inputChannel.GetInputDevice().GetInputDeviceId();

        if (AzFramework::InputDeviceKeyboard::IsKeyboardDevice(deviceId))
        {
            HandleKeyboardEvent(inputChannel);
        }

        return false;
    }

    void ROS2VehicleControlComponent::HandleKeyboardEvent(const AzFramework::InputChannel& inputChannel)
    {
        const AzFramework::InputChannelId& channelId = inputChannel.GetInputChannelId();

        if (inputChannel.IsStateBegan())
        {
            // Handle AD engage key (A) - only handle this key, let VehicleController handle arrow keys
            if (channelId == AzFramework::InputDeviceKeyboard::Key::AlphanumericA)
            {
                // Can only engage AD if:
                // 1. AD system is enabled
                // 2. AD is enabled externally
                // 3. No manual input is currently detected
                // 4. AD is not already active
                if (m_configuration.m_enableADSystem && 
                    m_adEnabled && 
                    !m_manualInputDetected && 
                    !m_adStatus)
                {
                    m_adStatus = true;
                    AZ_Printf("ROS2VehicleControl", "AD Status engaged via A key");
                }
                else
                {
                    AZ_Printf("ROS2VehicleControl", "AD engagement failed - Enable: %s, Manual: %s, Status: %s", 
                        m_adEnabled ? "true" : "false",
                        m_manualInputDetected ? "true" : "false", 
                        m_adStatus ? "true" : "false");
                }
            }
            
            // Detect manual override keys (Arrow keys) but don't interfere with VehicleController
            if (channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowUp ||
                channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowDown ||
                channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowLeft ||
                channelId == AzFramework::InputDeviceKeyboard::Key::NavigationArrowRight)
            {
                m_manualInputDetected = true;
                m_lastManualInputTime = m_currentTime;
                AZ_Printf("ROS2VehicleControl", "Manual input detected, disabling AD");
            }
        }
    }

    void ROS2VehicleControlComponent::HandleManualInput()
    {
        // Check if enough time has passed since last manual input
        if (m_manualInputDetected && 
            (m_currentTime - m_lastManualInputTime) >= m_configuration.m_manualInputTimeoutSeconds)
        {
            m_manualInputDetected = false;
            AZ_Printf("ROS2VehicleControl", "Manual input timeout expired, AD can be re-engaged");
        }
    }

    bool ROS2VehicleControlComponent::IsManualInputActive() const
    {
        // This method is now used only for timeout checking
        // The actual input detection is handled in HandleKeyboardEvent
        return m_manualInputDetected;
    }

} // namespace VehicleDynamics